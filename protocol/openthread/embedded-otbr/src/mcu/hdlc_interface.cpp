/*
 *  Copyright (c) 2018, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 *   This file includes the implementation for the HDLC interface to radio (RCP).
 */

#include "hdlc_interface.hpp"

#include "platform-posix.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#if OPENTHREAD_POSIX_CONFIG_RCP_PTY_ENABLE
#if defined(__APPLE__) || defined(__NetBSD__)
#include <util.h>
#elif defined(__FreeBSD__)
#include <libutil.h>
#else
#include <pty.h>
#endif
#endif
#include <stdarg.h>
#include <stdlib.h>

#include <openthread/logging.h>

#include "common/code_utils.hpp"
#include "lib/spinel/spinel.h"
#include "utils/uart.h"

#if OPENTHREAD_POSIX_CONFIG_SPINEL_HDLC_INTERFACE_ENABLE
#include "si91x/platform-si91x.h"

void otPlatUartReceived(const uint8_t *aBuf, uint16_t aBufLength)
{
    platformHDLCRecv(aBuf, aBufLength);
}

namespace ot {
namespace Posix {

const char HdlcInterface::kLogModuleName[] = "HdlcIntface";

HdlcInterface::HdlcInterface(const Url::Url &aRadioUrl)
    : mReceiveFrameCallback(nullptr)
    , mReceiveFrameContext(nullptr)
    , mReceiveFrameBuffer(nullptr)
    , mBaudRate(0)
    , mHdlcDecoder()
    , mRadioUrl(aRadioUrl)
{
    memset(&mInterfaceMetrics, 0, sizeof(mInterfaceMetrics));
    mInterfaceMetrics.mRcpInterfaceType = kSpinelInterfaceTypeHdlc;
}

otError HdlcInterface::Init(ReceiveFrameCallback aCallback, void *aCallbackContext, RxFrameBuffer &aFrameBuffer)
{
    otError error = OT_ERROR_NONE;
    platformHDLCInit();
    mHdlcDecoder.Init(aFrameBuffer, HandleHdlcFrame, this);
    mReceiveFrameCallback = aCallback;
    mReceiveFrameContext  = aCallbackContext;
    mReceiveFrameBuffer   = &aFrameBuffer;

    // exit:
    return error;
}

HdlcInterface::~HdlcInterface(void)
{
    Deinit();
}

void HdlcInterface::Deinit(void)
{
    if (otPlatUartDisable() != OT_ERROR_NONE)
    {
        LogCrit("Failed to disable UART connection.");
    }

    mReceiveFrameCallback = nullptr;
    mReceiveFrameContext  = nullptr;
    mReceiveFrameBuffer   = nullptr;
}

void HdlcInterface::Read(void)
{
    uint8_t buffer[512];
    int     rval;
    memset(buffer, 0, sizeof(buffer));

    do
    {
        rval = platformHDLCRead(buffer);

        if (rval > 0)
        {
            Decode(buffer, static_cast<uint16_t>(rval));
        }
    } while ((rval > 0));
}

void HdlcInterface::Decode(const uint8_t *aBuffer, uint16_t aLength)
{
    mHdlcDecoder.Decode(aBuffer, aLength);
}

otError HdlcInterface::SendFrame(const uint8_t *aFrame, uint16_t aLength)
{
    otError                            error = OT_ERROR_NONE;
    Spinel::FrameBuffer<kMaxFrameSize> encoderBuffer;
    Hdlc::Encoder                      hdlcEncoder(encoderBuffer);

    SuccessOrExit(error = hdlcEncoder.BeginFrame());
    SuccessOrExit(error = hdlcEncoder.Encode(aFrame, aLength));
    SuccessOrExit(error = hdlcEncoder.EndFrame());

    error = Write(encoderBuffer.GetFrame(), encoderBuffer.GetLength());

exit:
    if ((error == OT_ERROR_NONE) && IsSpinelResetCommand(aFrame, aLength))
    {
        mHdlcDecoder.Reset();
        error = ResetConnection();
    }

    return error;
}

otError HdlcInterface::Write(const uint8_t *aFrame, uint16_t aLength)
{
    otError error = OT_ERROR_NONE;
    // otLogPlat(OT_LOG_LEVEL_INFO, kLogModuleName, "Write Frame: %d", aLength);
    // for (uint8_t idx = 0; idx < aLength; idx++)
    // {
    //     otLogPlat(OT_LOG_LEVEL_INFO, kLogModuleName, "[%d]: 0x%02X", idx, aFrame[idx]);
    // }
    if (platformHDLCWrite(aFrame, aLength) != OT_ERROR_NONE)
    {
        error = OT_ERROR_FAILED;
    }

    mInterfaceMetrics.mTransferredFrameCount++;
    if (error == OT_ERROR_NONE)
    {
        mInterfaceMetrics.mTxFrameCount++;
        mInterfaceMetrics.mTxFrameByteCount += aLength;
        mInterfaceMetrics.mTransferredValidFrameCount++;
    }
    else
    {
        mInterfaceMetrics.mTransferredGarbageFrameCount++;
    }

    return error;
}

otError HdlcInterface::WaitForFrame(uint64_t aTimeoutUs)
{
    otError error = OT_ERROR_NONE;
    if (platformHDLCIsBufferOccupied(aTimeoutUs / US_PER_MS))
    {
        Read();
    }
    return error;
}

void HdlcInterface::UpdateFdSet(void *aMainloopContext)
{
    (void)aMainloopContext;
}

void HdlcInterface::Process(const void *aMainloopContext)
{
    (void)aMainloopContext;
    Read();
}

otError HdlcInterface::WaitForWritable(void)
{
    return OT_ERROR_NONE;
}

void HdlcInterface::HandleHdlcFrame(void *aContext, otError aError)
{
    static_cast<HdlcInterface *>(aContext)->HandleHdlcFrame(aError);
}

void HdlcInterface::HandleHdlcFrame(otError aError)
{
    VerifyOrExit((mReceiveFrameCallback != nullptr) && (mReceiveFrameBuffer != nullptr));

    mInterfaceMetrics.mTransferredFrameCount++;

    if (aError == OT_ERROR_NONE)
    {
        mInterfaceMetrics.mRxFrameCount++;
        mInterfaceMetrics.mRxFrameByteCount += mReceiveFrameBuffer->GetLength();
        mInterfaceMetrics.mTransferredValidFrameCount++;
        mReceiveFrameCallback(mReceiveFrameContext);
    }
    else
    {
        mInterfaceMetrics.mTransferredGarbageFrameCount++;
        mReceiveFrameBuffer->DiscardFrame();
        LogWarn("Error decoding hdlc frame: %s", otThreadErrorToString(aError));
    }

exit:
    return;
}

otError HdlcInterface::ResetConnection(void)
{
    otError  error = OT_ERROR_NONE;
    uint64_t end;

    if (mRadioUrl.HasParam("uart-reset"))
    {
        // TODO: OS Delay
        // usleep(static_cast<useconds_t>(kRemoveRcpDelay) * US_PER_MS);
        otPlatUartDisable();

        end = otPlatTimeGet() + kResetTimeout * US_PER_MS;
        do
        {
            error = otPlatUartEnable(); // TODO: disable before enabling??
            if (error == OT_ERROR_NONE)
            {
                ExitNow();
            }
            // TODO: OS Delay
            // usleep(static_cast<useconds_t>(kOpenFileDelay) * US_PER_MS);
        } while (end > otPlatTimeGet());

        LogCrit("Failed to reopen UART connection after resetting the RCP device.");
        error = OT_ERROR_FAILED;
    }

exit:
    return error;
}

} // namespace Posix
} // namespace ot
#endif // OPENTHREAD_POSIX_CONFIG_SPINEL_HDLC_INTERFACE_ENABLE

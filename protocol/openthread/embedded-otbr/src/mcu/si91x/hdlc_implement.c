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

#include "cmsis_os2.h"
#include "platform-si91x.h"
#include "ringbufs.h"
#include <openthread/logging.h>
#include <openthread/platform/time.h>
#include "common/code_utils.hpp"
#include "utils/uart.h"

#ifdef __cplusplus
extern "C" {
#endif

static uint8_t  sUartRxBuf[256];
static uint16_t sMaxFrameSize = 1;
static rbufs_t  sRxBuf;

void platformHDLCInit(void)
{
    memset(sUartRxBuf, 0, sizeof(sUartRxBuf));
    rbufsInit(&sRxBuf, sUartRxBuf, sizeof(sUartRxBuf));
}

size_t platformHDLCRead(const uint8_t *aBuf)
{
    if (sRxBuf.buff != NULL)
    {
        return rbufsRead(&sRxBuf, (void *)aBuf, sMaxFrameSize);
    }
    return 0;
}
void platformHDLCRecv(const uint8_t *aBuf, uint16_t aBufLength)
{
    rbufsWrite(&sRxBuf, aBuf, aBufLength);
    // otLogPlat(OT_LOG_LEVEL_INFO, "hdlc", "Recv: %d", aBufLength);
}
otError platformHDLCWrite(const uint8_t *aBuf, size_t length)
{
    return otPlatUartSend(aBuf, length);
}
bool platformHDLCIsBufferOccupied(size_t aTimeoutMs)
{
    uint64_t now = otPlatTimeGet();
    uint64_t end = now + aTimeoutMs * 1000;
    while (true)
    {
        now = otPlatTimeGet();
        if (rbufsGetByteCount(&sRxBuf) != 0)
        {
            ExitNow();
        }
        // osDelay(1);

        if (end < now)
        {
            break;
        }
    }
    return false;
exit:
    return true;
}

#ifdef __cplusplus
}
#endif

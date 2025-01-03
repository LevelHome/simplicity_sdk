/*
 *  Copyright (c) 2024, The OpenThread Authors.
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
 *   This file implements the OpenThread platform abstraction for UART communication.
 *
 */

#include "utils/uart.h"
#include "board_config.h"
#include "ringbufs.h"
#include "sl_ot_rtos_adaptation.h"
#include "sl_si91x_usart.h"
#include "sl_status.h"
#include "sl_uart.h"
#include <openthread-core-config.h>
#include <openthread-system.h>
#include <stddef.h>
#include <string.h>
#include <openthread/platform/time.h>
#include "utils/code_utils.h"

#define SL_UART_RX_BUFFER_SIZE 256
#define SL_UART_TX_BUFFER_SIZE 256

static uint8_t uartRxBuf[SL_UART_RX_BUFFER_SIZE];
static uint8_t uartTxBuf[SL_UART_TX_BUFFER_SIZE];

static rbufs_t rxBuf;
static rbufs_t txBuf;

slUartContext_t uartPlatform = {.periph       = SL_PLATFORM_UART_PERIPHERAL,
                                .initialize   = 0,
                                .txInprogress = 0,
                                .irqPrio      = 8};

static void uartPlatformIrqCallback(uint32_t event);

static void uartPlatformIrqCallback(uint32_t event)
{
    slUartContext_t *ctx = &uartPlatform;

    if (event == SL_USART_EVENT_RECEIVE_COMPLETE)
    {
        ctx->rxLastTime = otPlatTimeGet();
        rbufsWrite(ctx->rxBuf, ctx->rxTmp, 1);
        sl_si91x_usart_receive_data(ctx->handle, ctx->rxTmp, 1);
        sl_ot_rtos_set_pending_event(SL_OT_RTOS_EVENT_UART);
        //        bcnt = sl_si91x_usart_get_rx_data_count(ctx->handle);
    }
    else if (event == SL_USART_EVENT_SEND_COMPLETE)
    {
        uint32_t bcnt = sl_si91x_usart_get_tx_data_count(ctx->handle);

        // ctx->txLastTime = otPlatTimeGet();
        rbufsSkip(ctx->txBuf, bcnt);
        ctx->txCurrentLen = 0;
        ctx->txInprogress = 0;
        slUartStartTransfer(ctx);
    }
}

otError otPlatUartEnable(void)
{
    sl_status_t status;

    rbufsInit(&rxBuf, uartRxBuf, sizeof(uartRxBuf));
    rbufsInit(&txBuf, uartTxBuf, sizeof(uartTxBuf));

    uartPlatform.txBuf       = &txBuf;
    uartPlatform.rxBuf       = &rxBuf;
    uartPlatform.irqCallback = uartPlatformIrqCallback;

    status = slUartInit(&uartPlatform);

    return status == SL_STATUS_OK ? OT_ERROR_NONE : OT_ERROR_FAILED;
}

otError otPlatUartDisable(void)
{
    return slUartDeinit(&uartPlatform) == SL_STATUS_OK ? OT_ERROR_NONE : OT_ERROR_FAILED;
}

otError otPlatUartSend(const uint8_t *aBuf, uint16_t aBufLength)
{
    // sl_ot_rtos_set_pending_event(SL_OT_RTOS_EVENT_UART);
    if (slUartSend(&uartPlatform, aBuf, aBufLength) != SL_STATUS_OK)
    {
        return OT_ERROR_FAILED;
    }
    return OT_ERROR_NONE;
}

otError otPlatUartFlush(void)
{
    return OT_ERROR_NOT_IMPLEMENTED;
}

static void processReceive(void)
{
    slUartContext_t *ctx = &uartPlatform;
    uint8_t          tmp[1024];
    size_t           nBytes;

    // if (ctx->rxLastTime != 0 && otPlatTimeGet() - ctx->rxLastTime > ctx->tcharUs)
    {
        do
        {
            nBytes = rbufsRead(ctx->rxBuf, tmp, sizeof(tmp));
            otPlatUartReceived(tmp, nBytes);
        } while (rbufsGetByteCount(ctx->rxBuf) != 0);
        otSysEventSignalPending();
        ctx->rxLastTime = 0;
    }
}

// static void processTransmit(void)
// {
//     slUartContext_t *ctx = &uartPlatform;

//     if (ctx->txLastTime != 0 && otPlatTimeGet() - ctx->txLastTime > ctx->tcharUs)
//     {
//         if (rbufsGetByteCount(ctx->txBuf) == 0)
//         {
//             otPlatUartSendDone();
//         }

//         ctx->txLastTime = 0;
//     }
// }

void si91xUartProcess()
{
    processReceive();
    // processTransmit();
}

bool si91xUartPoll(void)
{
    slUartContext_t *ctx = &uartPlatform;
    return (rbufsGetByteCount(ctx->txBuf) != 0);
}

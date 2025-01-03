/*
 *  Copyright (c) 2017, The OpenThread Authors.
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
#include <openthread-core-config.h>
#include <stdarg.h>
#include <stdio.h>
#include <openthread/config.h>

#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/debug_uart.h>
#include <openthread/platform/time.h>
#include <openthread/platform/toolchain.h>

#include "board_config.h"
#include "sl_ot_rtos_adaptation.h"
#include "sl_uart.h"

#define SL_LOG_UART_TX_BUFFER_SIZE 256
#define SL_LOG_UART_RX_BUFFER_SIZE 256

slUartContext_t uartDebug = {
    .periph       = SL_LOG_UART_PERIPHERAL,
    .initialize   = 0,
    .txInprogress = 0,
    .irqPrio      = 8,
};

static uint8_t uartLogTxBuf[SL_LOG_UART_TX_BUFFER_SIZE];
static uint8_t uartLogRxBuf[SL_LOG_UART_RX_BUFFER_SIZE];
static rbufs_t txBuf;
static rbufs_t rxBuf;

static void uartLogIrqCallback(uint32_t event)
{
    slUartContext_t *ctx = &uartDebug;

    if (event == SL_USART_EVENT_RECEIVE_COMPLETE)
    {
        // ctx->rxLastTime = otPlatTimeGet();
        rbufsWrite(ctx->rxBuf, ctx->rxTmp, 1);
        sl_si91x_usart_receive_data(ctx->handle, ctx->rxTmp, 1);
        sl_ot_rtos_set_pending_event(SL_OT_RTOS_EVENT_UART_CLI);
    }
    else if (event == SL_USART_EVENT_SEND_COMPLETE)
    {
        uint32_t cnt = sl_si91x_usart_get_tx_data_count(ctx->handle);

        // ctx->txLastTime = otPlatTimeGet();
        rbufsSkip(ctx->txBuf, cnt);
        ctx->txInprogress = 0;
        ctx->txCurrentLen = 0;
        slUartStartTransfer(ctx);
    }
}

void si91xLogInit(void)
{
    rbufsInit(&txBuf, uartLogTxBuf, sizeof(uartLogTxBuf));
    rbufsInit(&rxBuf, uartLogRxBuf, sizeof(uartLogRxBuf));

    uartDebug.txBuf       = &txBuf;
    uartDebug.rxBuf       = &rxBuf;
    uartDebug.irqCallback = uartLogIrqCallback;

    slUartInit(&uartDebug);
}

void si91xLogDeinit(void)
{
    slUartDeinit(&uartDebug);
}

void otPlatDebugUart_printf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    otPlatDebugUart_vprintf(fmt, ap);
    va_end(ap);
}

void otPlatDebugUart_vprintf(const char *fmt, va_list ap)
{
    char buf[256];
    /* by standard ...
     * vsnprintf() always null terminates
     */
    vsnprintf(buf, sizeof(buf), fmt, ap);
    /* however ... some platforms have bugs */
    buf[sizeof(buf) - 1] = 0;
    otPlatDebugUart_puts_no_nl(buf);
}

void otPlatDebugUart_write_bytes(const uint8_t *pBytes, int nBytes)
{
    slUartSend(&uartDebug, pBytes, nBytes);
}

void otPlatDebugUart_puts_no_nl(const char *s)
{
    otPlatDebugUart_write_bytes((const uint8_t *)s, strlen(s));
}

void otPlatDebugUart_puts(const char *s)
{
    otPlatDebugUart_puts_no_nl(s);
    otPlatDebugUart_putchar('\n');
}

void otPlatDebugUart_putchar(int c)
{
    /* map lf to crlf as needed */
    if (c == '\n')
    {
        otPlatDebugUart_putchar_raw('\r');
    }

    otPlatDebugUart_putchar_raw(c);
}

void otPlatDebugUart_putchar_raw(int c)
{
    uint8_t ch = c;

    otPlatDebugUart_write_bytes(&ch, 1);
}

int otPlatDebugUart_kbhit(void)
{
    return 0; /* nothing */
}

int otPlatDebugUart_getc(void)
{
    uint8_t ch;

    if (rbufsRead(uartDebug.rxBuf, &ch, 1) == 1)
    {
        return ch;
    }

    return -1; /* nothing */
}

otError otPlatDebugUart_logfile(const char *filename)
{
    OT_UNUSED_VARIABLE(filename);

    return OT_ERROR_FAILED;
}

#if (OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_DEBUG_UART)

void otPlatLog(otLogLevel aLogLevel, otLogRegion aLogRegion, const char *aFormat, ...)
{
    OT_UNUSED_VARIABLE(aLogLevel);
    OT_UNUSED_VARIABLE(aLogRegion);

    va_list  ap;
    uint32_t now;
    if (aLogLevel <= OPENTHREAD_CONFIG_LOG_LEVEL)
    {
        now = otPlatAlarmMilliGetNow();
        otPlatDebugUart_printf("%3d.%03d | ", (int)(now / 1000), (int)(now % 1000));
        va_start(ap, aFormat);
        otPlatDebugUart_vprintf(aFormat, ap);
        va_end(ap);

        otPlatDebugUart_putchar('\n');
    }
}
#endif

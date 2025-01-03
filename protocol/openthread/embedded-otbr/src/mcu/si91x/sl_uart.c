/*******************************************************************************
 * @file
 * @brief SL UART
 *******************************************************************************
 * # License
 * <b>Copyright 2024 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 ******************************************************************************/

#include "sl_uart.h"

int32_t slUartInit(slUartContext_t *uartCtx)
{
    sl_status_t                     status;
    sl_si91x_usart_control_config_t config;
    sl_si91x_usart_control_config_t get_config;

    // Initialize the UART
    do
    {
        status = sl_si91x_usart_init(uartCtx->periph, &uartCtx->handle);
        if (status != SL_STATUS_OK)
        {
            break;
        }

        // Register user callback function
        sl_si91x_usart_multiple_instance_register_event_callback(uartCtx->periph, uartCtx->irqCallback);

        // Configure the USART configurations
        status = sl_si91x_usart_set_configuration(uartCtx->handle, &config);
        if (status != SL_STATUS_OK)
        {
            break;
        }

        if (uartCtx->rxBuf != NULL)
        {
            // Start receiver data
            status = sl_si91x_usart_receive_data(uartCtx->handle, uartCtx->rxTmp, 1);
            if (status != SL_STATUS_OK)
            {
                break;
            }
        }
        sl_si91x_usart_get_configurations(uartCtx->periph, &get_config);
    } while (false);

    uartCtx->tcharUs = 2 * 1000000 / get_config.baudrate * 8;

    uartCtx->initialize = 1;
    return status;
}

int32_t slUartDeinit(slUartContext_t *uartCtx)
{
    sl_status_t status;

    status              = sl_si91x_usart_deinit(uartCtx->handle);
    uartCtx->initialize = 0;

    return status;
}

int32_t slUartStartTransfer(slUartContext_t *uartCtx)
{
    sl_status_t status;
    // uint32_t    primask = __get_PRIMASK();

    if (!uartCtx->initialize)
    {
        return SL_STATUS_FAIL;
    }

    if (uartCtx->txInprogress)
    {
        return SL_STATUS_BUSY;
    }

    // __disable_irq();
    if (uartCtx->txCurrentLen == 0)
    {
        uartCtx->txCurrentLen = rbufsGetBlockReadLength(uartCtx->txBuf);
        if (uartCtx->txCurrentLen > 0)
        {
#if 1
            // FIXME: Need to check again
            uartCtx->txCurrentLen = 1;
#endif
            uartCtx->txInprogress = 1;
            uartCtx->txCurrentCnt = sl_si91x_usart_get_tx_data_count(uartCtx->handle);
            void *txData          = rbufsGetBlockReadAddress(uartCtx->txBuf);
            status                = sl_si91x_usart_send_data(uartCtx->handle, txData, uartCtx->txCurrentLen);
            if (status == SL_STATUS_OK)
            {
            }
        }
    }
    // __set_PRIMASK(primask);
    // __enable_irq();

    return SL_STATUS_OK;
}

int32_t slUartSend(slUartContext_t *uartCtx, const uint8_t *aBuf, uint16_t aBufLength)
{
    sl_status_t status;

    rbufsWrite(uartCtx->txBuf, aBuf, aBufLength);
    status = slUartStartTransfer(uartCtx);

    return status;
}

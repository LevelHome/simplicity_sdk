/***************************************************************************//**
 * @file
 * @brief CPC EUSART driver configuration file.
 *******************************************************************************
 * # License
 * <b>Copyright 2023 Silicon Laboratories Inc. www.silabs.com</b>
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

// <<< Use Configuration Wizard in Context Menu >>>

#ifndef SL_CPC_DRV_UART_EUSART_INSTANCE_CONFIG_H
#define SL_CPC_DRV_UART_EUSART_INSTANCE_CONFIG_H

// <h> CPC - EUSART Driver Configuration

// <o SL_CPC_DRV_UART_INSTANCE_RX_QUEUE_SIZE> Number of frame that can be queued in the driver receive queue
// <i> Default: 10
#define SL_CPC_DRV_UART_INSTANCE_RX_QUEUE_SIZE            10

// <o SL_CPC_DRV_UART_INSTANCE_TX_QUEUE_SIZE> Number of frame that can be queued in the driver transmit queue
// <i> Default: 10
#define SL_CPC_DRV_UART_INSTANCE_TX_QUEUE_SIZE            10

// <o SL_CPC_DRV_UART_INSTANCE_BAUDRATE> EUSART Baudrate
// <i> Default: 115200
#define SL_CPC_DRV_UART_INSTANCE_BAUDRATE              115200

// <o SL_CPC_DRV_UART_INSTANCE_FLOW_CONTROL_TYPE> Flow control
// <eusartHwFlowControlNone=> None
// <eusartHwFlowControlCtsAndRts=> CTS/RTS
// <i> Default: eusartHwFlowControlCtsAndRts
#define SL_CPC_DRV_UART_INSTANCE_FLOW_CONTROL_TYPE eusartHwFlowControlCtsAndRts
// </h>

// <<< end of configuration section >>>

// <<< sl:start pin_tool >>>
// <eusart signal=TX,RX,(CTS),(RTS)> SL_CPC_DRV_UART_INSTANCE
// $[EUSART_SL_CPC_DRV_UART_INSTANCE]
#warning "CPC EUSART peripheral not configured"
// #define SL_CPC_DRV_UART_INSTANCE_PERIPHERAL                 EUSART0
// #define SL_CPC_DRV_UART_INSTANCE_PERIPHERAL_NO              0
//
// #define SL_CPC_DRV_UART_INSTANCE_TX_PORT                    gpioPortD
// #define SL_CPC_DRV_UART_INSTANCE_TX_PIN                     13
//
// #define SL_CPC_DRV_UART_INSTANCE_RX_PORT                    gpioPortD
// #define SL_CPC_DRV_UART_INSTANCE_RX_PIN                     14
//
// #define SL_CPC_DRV_UART_INSTANCE_CTS_PORT                   gpioPortD
// #define SL_CPC_DRV_UART_INSTANCE_CTS_PIN                    13
//
// #define SL_CPC_DRV_UART_INSTANCE_RTS_PORT                   gpioPortD
// #define SL_CPC_DRV_UART_INSTANCE_RTS_PIN                    14
// [EUSART_SL_CPC_DRV_UART_INSTANCE]$
// <<< sl:end pin_tool >>>

#endif /* SL_CPC_DRV_UART_INSTANCE_CONFIG_H */

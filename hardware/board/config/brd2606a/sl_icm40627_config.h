/*****************************CS**********************************************//**
 * @file
 * @brief ICM40627 Config
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

#ifndef SL_ICM40627_CONFIG_H
#define SL_ICM40627_CONFIG_H

// <<< sl:start pin_tool >>>
// <eusart signal=TX,RX,SCLK,CS> SL_ICM40627_SPI_EUSART
// $[EUSART_SL_ICM40627_SPI_EUSART]
#define SL_ICM40627_SPI_EUSART_PERIPHERAL        EUSART1
#define SL_ICM40627_SPI_EUSART_PERIPHERAL_NO     1

// EUSART1 TX on PB03
#define SL_ICM40627_SPI_EUSART_TX_PORT           gpioPortB
#define SL_ICM40627_SPI_EUSART_TX_PIN            3

// EUSART1 RX on PB02
#define SL_ICM40627_SPI_EUSART_RX_PORT           gpioPortB
#define SL_ICM40627_SPI_EUSART_RX_PIN            2

// EUSART1 SCLK on PB01
#define SL_ICM40627_SPI_EUSART_SCLK_PORT         gpioPortB
#define SL_ICM40627_SPI_EUSART_SCLK_PIN          1

// EUSART1 CS on PB00
#define SL_ICM40627_SPI_EUSART_CS_PORT           gpioPortB
#define SL_ICM40627_SPI_EUSART_CS_PIN            0

// [EUSART_SL_ICM40627_SPI_EUSART]$

// <gpio optional=true> SL_ICM40627_INT
// $[GPIO_SL_ICM40627_INT]
#define SL_ICM40627_INT_PORT                     gpioPortD
#define SL_ICM40627_INT_PIN                      5

// [GPIO_SL_ICM40627_INT]$
// <<< sl:end pin_tool >>>

#endif // SL_ICM40627_CONFIG_H

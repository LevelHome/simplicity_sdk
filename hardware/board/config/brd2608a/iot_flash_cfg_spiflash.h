/***************************************************************************//**
 * @file    iot_flash_cfg_inst.h
 * @brief   Common I/O flash instance configurations.
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
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

/*******************************************************************************
 *                              SAFE GUARD
 ******************************************************************************/

#ifndef _IOT_FLASH_CFG_SPIFLASH_H_
#define _IOT_FLASH_CFG_SPIFLASH_H_

/*******************************************************************************
 *                         Flash Default Configs
 ******************************************************************************/

// <<< Use Configuration Wizard in Context Menu >>>

// <h> Flash General Options

// <o IOT_FLASH_CFG_SPIFLASH_INST_NUM> Instance number
// <i> Instance number used when iot_flash_open() is called.
// <i> Default: 0
#define IOT_FLASH_CFG_SPIFLASH_INST_NUM              0

// <o IOT_FLASH_CFG_SPIFLASH_EXTERNAL_FLASH_BASE> External Flash Base Address <f.h>
// <i> Hex value of the base address of the external flash used.
// <i> Default: 0x0
#define IOT_FLASH_CFG_SPIFLASH_EXTERNAL_FLASH_BASE   0x0

// <o IOT_FLASH_CFG_SPIFLASH_INST_TYPE> Instance type
// <0=> Internal Flash (MSC)
// <1=> External Flash (SPI)
// <i> Specify whether this instance is for internal flash (MSC)
//     or an external SPI flash. If external, then you need to setup
//     SPI configs below.
// <i> Default: 0
#define IOT_FLASH_CFG_SPIFLASH_INST_TYPE             1

// </h>

// <h> SPI Configuration

// <o  IOT_FLASH_CFG_SPIFLASH_SPI_BITRATE> Default SPI bitrate
// <i> Default: 1000000
#define IOT_FLASH_CFG_SPIFLASH_SPI_BITRATE           1000000

// <o  IOT_FLASH_CFG_SPIFLASH_SPI_FRAME_LENGTH> Default SPI frame length <4-16>
// <i> Default: 8
#define IOT_FLASH_CFG_SPIFLASH_SPI_FRAME_LENGTH      8

// <o  IOT_FLASH_CFG_SPIFLASH_SPI_TYPE> Default SPI master/slave mode
// <spidrvMaster=> Master
// <spidrvSlave=> Slave
#define IOT_FLASH_CFG_SPIFLASH_SPI_TYPE              spidrvMaster

// <o  IOT_FLASH_CFG_SPIFLASH_SPI_BIT_ORDER> Default SPI bit order
// <spidrvBitOrderLsbFirst=> LSB transmitted first
// <spidrvBitOrderMsbFirst=> MSB transmitted first
#define IOT_FLASH_CFG_SPIFLASH_SPI_BIT_ORDER         spidrvBitOrderMsbFirst

// <o  IOT_FLASH_CFG_SPIFLASH_SPI_CLOCK_MODE> Default SPI clock mode
// <spidrvClockMode0=> SPI mode 0: CLKPOL=0, CLKPHA=0
// <spidrvClockMode1=> SPI mode 1: CLKPOL=0, CLKPHA=1
// <spidrvClockMode2=> SPI mode 2: CLKPOL=1, CLKPHA=0
// <spidrvClockMode3=> SPI mode 3: CLKPOL=1, CLKPHA=1
#define IOT_FLASH_CFG_SPIFLASH_SPI_CLOCK_MODE        spidrvClockMode0

// <o  IOT_FLASH_CFG_SPIFLASH_SPI_CS_CONTROL> Default SPI CS control scheme
// <spidrvCsControlAuto=> CS controlled by the SPI driver
// <spidrvCsControlApplication=> CS controlled by the application
#define IOT_FLASH_CFG_SPIFLASH_SPI_CS_CONTROL        spidrvCsControlApplication

// <o  IOT_FLASH_CFG_SPIFLASH_SPI_SLAVE_START_MODE> Default SPI transfer scheme
// <spidrvSlaveStartImmediate=> Transfer starts immediately
// <spidrvSlaveStartDelayed=> Transfer starts when the bus is idle
#define IOT_FLASH_CFG_SPIFLASH_SPI_SLAVE_START_MODE  spidrvSlaveStartImmediate

// </h>

// <<< end of configuration section >>>

/*******************************************************************************
 *                   EXTERNAL FLASH: H/W PERIPHERAL CONFIG
 ******************************************************************************/

// <<< sl:start pin_tool >>>
// <usart signal=TX,RX,CLK,CS> IOT_FLASH_CFG_SPIFLASH_SPI
// $[USART_IOT_FLASH_CFG_SPIFLASH_SPI]
#define IOT_FLASH_CFG_SPIFLASH_SPI_PERIPHERAL    USART0
#define IOT_FLASH_CFG_SPIFLASH_SPI_PERIPHERAL_NO 0

// USART0 TX on PC03
#define IOT_FLASH_CFG_SPIFLASH_SPI_TX_PORT       gpioPortC
#define IOT_FLASH_CFG_SPIFLASH_SPI_TX_PIN        3

// USART0 RX on PC02
#define IOT_FLASH_CFG_SPIFLASH_SPI_RX_PORT       gpioPortC
#define IOT_FLASH_CFG_SPIFLASH_SPI_RX_PIN        2

// USART0 CLK on PC01
#define IOT_FLASH_CFG_SPIFLASH_SPI_CLK_PORT      gpioPortC
#define IOT_FLASH_CFG_SPIFLASH_SPI_CLK_PIN       1

// USART0 CS on PA07
#define IOT_FLASH_CFG_SPIFLASH_SPI_CS_PORT       gpioPortA
#define IOT_FLASH_CFG_SPIFLASH_SPI_CS_PIN        7

// [USART_IOT_FLASH_CFG_SPIFLASH_SPI]$
// <<< sl:end pin_tool >>>

/*******************************************************************************
 *                              SAFE GUARD
 ******************************************************************************/

#endif /* _IOT_FLASH_CFG_SPIFLASH_H_ */

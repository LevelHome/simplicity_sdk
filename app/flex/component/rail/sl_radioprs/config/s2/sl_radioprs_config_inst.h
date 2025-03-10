/***************************************************************************//**
 * @file
 * @brief
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

#ifndef SL_RADIOPRS_CONFIG_INSTANCE_H
#define SL_RADIOPRS_CONFIG_INSTANCE_H

// <<< Use Configuration Wizard in Context Menu >>>
// <h> Radio PRS Configuration

// <o RADIOPRS_INSTANCE_SOURCESEL> Radio PRS signal source
// <PRS_RACL_RX=> RX active
// <PRS_RACL_TX=> TX active
// <PRS_RACL_ACTIVE=> Radio enabled
// <PRS_RACL_LNAEN=> LNA enabled for RX
// <PRS_RACL_PAEN=> PA enabled for TX
// <PRS_MODEML_ANT0=> Antenna 0
// <PRS_MODEML_ANT1=> Antenna 1
// <PRS_MODEML_DCLK=> Modem clock out
// <PRS_MODEML_DOUT=> Modem data out
// <PRS_MODEML_FRAMEDET=> Sync detected
// <PRS_MODEM_FRAMESENT=> Frame sent
// <PRS_MODEM_PREDET=> Preamble detected
// <PRS_MODEMH_PRESENT=> Preamble sent
// <PRS_MODEMH_RSSIJUMP=> change in RSSI
// <PRS_MODEMH_SYNCSENT=> Sync word sent
// <PRS_MODEMH_EOF=> end of frame
// <PRS_FRC_DCLK=> FRC clock out
// <PRS_FRC_DOUT=> FRC data out
// <i> Default: PRS_MODEML_DOUT
#define RADIOPRS_INSTANCE_SOURCESEL PRS_MODEML_DOUT

// </h>

// <<< end of configuration section >>>

// Useful Pins that work on most boards
// Pin,  Ch, Board gen  (conflict)  WSTK_P?     EXP_HEADER?
// PC00   6  xG21       (DISP_SPI)  WSTK_P01    EXP_HEADER4
// PA06   0  xG22       (I2C)       WSTK_P11    EXP_HEADER14
// PC05   6  xG23       (I2C)       WSTK_P12    EXP_HEADER15
// PC07   6  xG24       (I2C)       WSTK_P13    EXP_HEADER16

// <<< sl:start pin_tool >>>
// <prs gpio=true > RADIOPRS_INSTANCE
// $[PRS_RADIOPRS_INSTANCE]
#define RADIOPRS_INSTANCE_CHANNEL    6
#define RADIOPRS_INSTANCE_PORT       gpioPortC
#define RADIOPRS_INSTANCE_PIN        5
// [PRS_RADIOPRS_INSTANCE]$
// <<< sl:end pin_tool >>>
#endif // SL_RADIOPRS_CONFIG_INSTANCE_H

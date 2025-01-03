/***************************************************************************//**
 * @file
 * @brief CS Reflector configuration
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

#ifndef CS_REFLECTOR_CONFIG_H
#define CS_REFLECTOR_CONFIG_H

// <o CS_REFLECTOR_MAX_TX_POWER_DBM> Maximum transmit power of the reflector radio [dBm] <-127..20>
// <i> Default: 20
// <i> Maximum transmit power of the reflector radio
#define CS_REFLECTOR_MAX_TX_POWER_DBM      (20)

// <o CS_REFLECTOR_CS_SYNC_ANTENNA> Select antenna for CS sync packets
// <CS_SYNC_ANTENNA_1=> Antenna 1
// <CS_SYNC_ANTENNA_2=> Antenna 2
// <CS_SYNC_SWITCHING=> Switching between all antennas
// <i> Default: CS_SYNC_SWITCHING
#define CS_REFLECTOR_CS_SYNC_ANTENNA      CS_SYNC_SWITCHING

#define REFLECTOR_CONFIG_DEFAULT                          \
  {                                                       \
    .max_tx_power_dbm =    CS_REFLECTOR_MAX_TX_POWER_DBM, \
    .cs_sync_antenna =     CS_REFLECTOR_CS_SYNC_ANTENNA   \
  }

#endif // CS_REFLECTOR_CONFIG_H

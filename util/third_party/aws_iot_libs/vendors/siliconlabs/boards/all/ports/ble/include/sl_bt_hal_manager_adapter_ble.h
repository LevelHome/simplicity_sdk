/***************************************************************************//**
 * @file
 * @brief Silicon Labs implementation of FreeRTOS Bluetooth Low Energy library.
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

#ifndef SL_BT_HAL_MANAGER_ADAPTER_BLE_H
#define SL_BT_HAL_MANAGER_ADAPTER_BLE_H

/* Standard library includes */
#include <stddef.h>

/* FreeRTOS BLE HAL includes */
#include "bt_hal_manager_adapter_ble.h"

/* Silicon Labs includes */
#include "sl_bt_hal_config.h"
#include "sl_bt_api.h"

/**
 * @brief Retrieves the HAL LE interface.
 *
 * This function implements pxGetLeAdapter function in @ref BTInterface_t.
 */
const void * prvGetLeAdapter( );

/**
 * @brief Get pointer of BTBdaddr_t of the connection handle.
 *
 * @param[in] ucConnectionHandle The connection handle
 *
 * @return Pointer of BTBdaddr_t structure which stores the remote client address
 */
BTBdaddr_t * prvGetConnectionAddress( uint8_t ucConnectionHandle );

/**
 * @brief Read the address of a bonded device, if any, from non-volatile memory
 *
 * @param[in] ucBondingHandle The bonding handle
 * @param[out] pxBdAddr The Bluetooth address in the original connection, if found
 *
 * @return eBTStatusSuccess on success, otherwise an error code
 */
BTStatus_t prvLoadBondedDeviceAddress( uint8_t ucBondingHandle,
                                       BTBdaddr_t * pxBdAddr );

/**
 * @brief Get the connection handle by a Bluetooth device address.
 *
 * @param[in] pxRemoteAddress Pointer of BTBdaddr_t of the connection
 * @param[out] pucConnectionHandle The connection handle if the connection exists
 *
 * @return eBTStatusSuccess if the connection exists.
 */
BTStatus_t prvGetConnectionHandle( const BTBdaddr_t * pxRemoteAddress, uint8_t *pucConnectionHandle );

/**
 * @brief Map a BLE Adapter API power level index to SL BT API power value
 *
 * @param[in] ucTxPower BLE Adapter API power level index
 * @param[out] psSlBtPower Set to the corresponding SL BT API power value
 *
 * @return eBTStatusSuccess on success, otherwise an error code
 */
BTStatus_t prvTxPowerIndexToSlBtPower( uint8_t ucTxPower,
                                       int16_t * psSlBtPower );

/**
 * @brief Invoked on BTInterface_t's pxDisable() to cleanup adapter state.
 */
void prvBleAdapterOnPxDisable();

/**
 * @brief Invoked on Bluetooth event to handle events the BLE adapter needs.
 *
 * @param[in] evt The Bluetooth event
 */
void prvBleAdapterOnSlBtEvent(sl_bt_msg_t* evt);

#endif /* SL_BT_HAL_MANAGER_ADAPTER_BLE_H */

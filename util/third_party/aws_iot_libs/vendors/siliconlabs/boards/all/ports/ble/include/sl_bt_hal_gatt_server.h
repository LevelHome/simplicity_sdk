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

#ifndef SL_BT_HAL_GATT_SERVER_H
#define SL_BT_HAL_GATT_SERVER_H

/* Standard library includes */
#include <stddef.h>

/* FreeRTOS BLE HAL includes */
#include "bt_hal_manager_adapter_ble.h"

/* Silicon Labs includes */
#include "sl_bt_hal_config.h"
#include "sl_bt_api.h"

/**
 * @brief Get the GATT server interface.
 *
 * This function implements ppvGetGattServerInterface function in @ref BTBleAdapter_t.
 */
const void * prvGetGattServerInterface( );

/**
 * @brief Invoked on Bluetooth event to handle events the GATT Server needs
 *
 * @param[in] evt The Bluetooth event
 */
void prvGattServerOnSlBtEvent(sl_bt_msg_t* evt);

/**
 * @brief Invoked by Adapter BLE to handle opened connection event by GATT Server
 *
 * @param[in] ucConnectionHandle Connection handle
 *
 * @param[in] address Pointer to BTBdaddr_t structure
 */
void prvGattServerOpenConnectionEventCb( uint8_t ucConnectionHandle, BTBdaddr_t * pBTAddress );

/**
 * @brief Invoked by Adapter BLE to handle closed connection event by GATT Server
 *
 * @param[in] ucConnectionHandle Connection handle
 *
 * @param[in] address Pointer to BTBdaddr_t structure
 */
void prvGattServerCloseConnectionEventCb( uint8_t ucConnectionHandle, BTBdaddr_t * pBTAddress );

#endif /* SL_BT_HAL_GATT_SERVER_H */

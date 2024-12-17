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

#ifndef SL_BT_HAL_MANAGER_H
#define SL_BT_HAL_MANAGER_H

/* Standard library includes */
#include <stddef.h>

/* FreeRTOS BLE HAL includes */
#include "bt_hal_manager.h"

/* Silicon Labs includes */
#include "sl_bt_hal_config.h"
#include "sl_status.h"

/**
 * Enumeration listing the states of the Bluetooth stack
 */
typedef enum
{
  eSlBtHalBluetoothStopped       = 0, /**< Bluetooth stack is stopped */
  eSlBtHalBluetoothStartingSync  = 1, /**< Bluetooth stack has been requested to start
                                           and prvEnable will handle it synchronously */
  eSlBtHalBluetoothStartingAsync = 2, /**< Bluetooth stack has been requested to start
                                           and it will finish asynchronously */
  eSlBtHalBluetoothStarted       = 3, /**< Bluetooth stack has been successfully started  */
} SlBtHalState_t;

/**
 * @brief Structure to collect the properties of the device
 */
typedef struct
{
  uint8_t *      pucDeviceName;      /**< Device name. The memory is a heap allocation owned by this
                                          context and must be freed using sl_free() when not needed
                                          anymore. */
  size_t         xDeviceNameLen;     /**< Length of the device name */
  uint16_t       usMaxMtu;           /**< Max local ATT MTU configured to the Bluetooth stack */
  uint8_t        ucBondable;         /**< Bondable mode configured to the Bluetooth stack */
  uint8_t        ucSmConfigureFlags; /**< Flags configured in a call to sl_bt_sm_configure() */
  uint8_t        ucIoCapabilities;   /**< I/O capabilities configured to the Bluetooth stack */
  SlBtHalState_t eBtState;           /**< State of the Bluetooth stack */
  bool           bKernelStarted;     /**< Set to true when the kernel has been started */
} SlBtHalManager_t;

extern SlBtHalManager_t xSlBtHalManager;

/**
 * @brief Handle a new pairing event.
 *
 * @param[in] ucBondingHandle The bonding handle in Bluetooth stack
 * @param[in] pxAddress Pointer to the remote device address
 * @param[in] xSecurityLevel The security level of the pairing
 */
void prvBtHalManagerOnNewPairingEvent( uint8_t ucBondingHandle,
                                       BTBdaddr_t * pxAddress,
                                       BTSecurityLevel_t xSecurityLevel );

#endif /* SL_BT_HAL_MANAGER_H */

/***************************************************************************//**
 * @file
 * @brief Bluetooth event handler registered to SL Bluetooth stack.
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

#ifndef SL_BT_HAL_EVENT_HANDLER_H
#define SL_BT_HAL_EVENT_HANDLER_H

/* Silicon Labs includes */
#include "sl_bt_api.h"

/**
 * @brief Function invoked by SL Bluetooth stack on a Bluetooth event
 *
 * @param[in] evt The Bluetooth event
 */
void sl_bt_hal_on_event(sl_bt_msg_t* evt);

/**
 * @brief Function called by platform init when the RTOS kernel is started
 */
void sl_bt_hal_on_kernel_start(void);

#endif /* SL_BT_HAL_EVENT_HANDLER_H */

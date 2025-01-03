/***************************************************************************//**
 * @file
 * @brief sl_rail_sdk_sleep.c
 *******************************************************************************
 * # License
 * <b>Copyright 2018 Silicon Laboratories Inc. www.silabs.com</b>
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

// -----------------------------------------------------------------------------
//                                   Includes
// -----------------------------------------------------------------------------
#include "sl_rail_sdk_sleep.h"
#include "sl_clock_manager.h"
#include "sl_rail_util_init.h"
#include "rail.h"
// -----------------------------------------------------------------------------
//                              Macros and Typedefs
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//                          Static Function Declarations
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//                                Global Variables
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//                          Public Function Definitions
// -----------------------------------------------------------------------------
/*******************************************************************************
 * API prepares the RAIL and Power Manager to work together.
 * Enables RAIL timer syncronization after sleep.
 ******************************************************************************/
RAIL_Status_t sl_rail_sdk_sleep_init(void)
{
  RAIL_Status_t status = RAIL_STATUS_NO_ERROR;
  RAIL_Handle_t rail_handle = NULL;

  sl_clock_manager_enable_bus_clock(SL_BUS_CLOCK_PRS);
  status = RAIL_InitPowerManager();

  if (RAIL_STATUS_NO_ERROR == status) {
    rail_handle = sl_rail_util_get_handle(SL_RAIL_UTIL_HANDLE_INST0);
    if (NULL == rail_handle) {
      status = RAIL_STATUS_INVALID_PARAMETER;
    }
  }

  if (RAIL_STATUS_NO_ERROR == status) {
    status = RAIL_ConfigSleep(rail_handle, RAIL_SLEEP_CONFIG_TIMERSYNC_ENABLED);
  }

  return status;
}

// -----------------------------------------------------------------------------
//                          Static Function Definitions
// -----------------------------------------------------------------------------

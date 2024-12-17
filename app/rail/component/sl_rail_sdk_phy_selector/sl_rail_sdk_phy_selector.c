/***************************************************************************//**
 * @file sl_rail_sdk_phy_selector.c
 * @brief RAIL SDK - RAIL PHY Selector Component
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

// -----------------------------------------------------------------------------
//                                   Includes
// -----------------------------------------------------------------------------
#include "sl_component_catalog.h"
#ifdef SL_CATALOG_APP_LOG_PRESENT
#include "app_log.h"
#endif
#ifdef SL_CATALOG_RAIL_SDK_CHANNEL_SELECTOR_PRESENT
#include "sl_rail_sdk_channel_selector.h"
#endif
#include "sl_rail_sdk_phy_selector.h"
#include "sl_rail_util_init.h"
#ifdef SL_CATALOG_RAIL_PACKET_ASSISTANT_PRESENT
#include "sl_rail_sdk_packet_assistant.h"
#endif

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
//                                Static Variables
// -----------------------------------------------------------------------------
static volatile uint16_t selected_phy = 0;

// -----------------------------------------------------------------------------
//                          Public Function Definitions
// -----------------------------------------------------------------------------
/******************************************************************************
 * Set which channel the app will communicate on
 *****************************************************************************/
uint8_t set_selected_phy(uint16_t new_phy)
{
  uint16_t new_channel = 0;
  uint8_t status = 0;
  if (channelConfigs[new_phy] != NULL) {
    selected_phy = new_phy;
    RAIL_Idle(sl_rail_util_get_handle(SL_RAIL_UTIL_HANDLE_INST0), RAIL_IDLE_ABORT, true);
    new_channel = RAIL_ConfigChannels(sl_rail_util_get_handle(SL_RAIL_UTIL_HANDLE_INST0), channelConfigs[selected_phy], NULL);
#ifdef SL_CATALOG_RAIL_SDK_CHANNEL_SELECTOR_PRESENT
    set_selected_channel(new_channel);
#endif
#ifdef SL_CATALOG_RAIL_PACKET_ASSISTANT_PRESENT
    update_assistant_pointers(selected_phy);
#endif
  } else {
#ifdef SL_CATALOG_APP_LOG_PRESENT
    app_log_warning("No possible phy\n");
#endif
    status = 1;
  }
  return status;
}

/******************************************************************************
 * Get which channel the app will communicate on
 *****************************************************************************/
uint16_t get_selected_phy(void)
{
  return selected_phy;
}

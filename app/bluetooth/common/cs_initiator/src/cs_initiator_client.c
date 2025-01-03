/***************************************************************************//**
 * @file
 * @brief CS Initiator Client
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

// -----------------------------------------------------------------------------
// Includes

#include <stdint.h>
#include <string.h>
#include "cs_initiator_client.h"

// -----------------------------------------------------------------------------
// Public function definitions

/******************************************************************************
 * Set channel map based on the preset.
 *****************************************************************************/
void cs_initiator_apply_channel_map_preset(cs_channel_map_preset_t preset, uint8_t *channel_map)
{
  switch (preset) {
    case CS_CHANNEL_MAP_PRESET_LOW:
    {
      uint8_t channel_map_low[10] = { 0x00, 0x00, 0x00, 0xC0, 0xFF, 0xFF, 0x03, 0x00, 0x00, 0x00 };
      memcpy(channel_map, channel_map_low, sizeof(channel_map_low));
    }
    break;
    case CS_CHANNEL_MAP_PRESET_MEDIUM:
    {
      uint8_t channel_map_medium[10] = { 0x54, 0x55, 0x55, 0x54, 0x55, 0x55, 0x55, 0x55, 0x55, 0x15 };
      memcpy(channel_map, channel_map_medium, sizeof(channel_map_medium));
    }
    break;
    case CS_CHANNEL_MAP_PRESET_HIGH:
    {
      uint8_t channel_map_high[10] = { 0xFC, 0xFF, 0x7F, 0xFC, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x1F };
      memcpy(channel_map, channel_map_high, sizeof(channel_map_high));
    }
    break;
    case CS_CHANNEL_MAP_PRESET_CUSTOM:
    {
      uint8_t channel_map_custom[10] = CS_CUSTOM_CHANNEL_MAP;
      memcpy(channel_map, channel_map_custom, sizeof(channel_map_custom));
    }
    break;
    default:
      // No other values allowed
      break;
  }
}

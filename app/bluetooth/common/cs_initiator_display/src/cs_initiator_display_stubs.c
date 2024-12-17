/***************************************************************************//**
 * @file
 * @brief CS Initiator display stubs
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
#include "cs_initiator_display_core.h"
#include "cs_initiator_display.h"

// -----------------------------------------------------------------------------
// Public function definitions

/******************************************************************************
 * CS Initiator display init stub function.
 *****************************************************************************/
sl_status_t cs_initiator_display_init(void)
{
  return SL_STATUS_OK;
}

/******************************************************************************
 * Set distance value to display
 *****************************************************************************/
void cs_initiator_display_set_distance(float distance)
{
  (void)distance;
}

/******************************************************************************
 * Set distance progress percentage to display
 *****************************************************************************/
void cs_initiator_display_set_distance_progress(float progress_percentage)
{
  (void)progress_percentage;
}

/******************************************************************************
 * Set RSSI based distance value to display
 *****************************************************************************/
void cs_initiator_display_set_rssi_distance(float distance)
{
  (void)distance;
}

/******************************************************************************
 * Set the likeliness parameter to display
 *****************************************************************************/
void cs_initiator_display_set_likeliness(float likeliness)
{
  (void)likeliness;
}

/******************************************************************************
 * Set the Bit Error Rate (BER) value to display
 *****************************************************************************/
void cs_initiator_display_set_bit_error_rate(float ber)
{
  (void)ber;
}

/******************************************************************************
 * Set the measurement mode and object tracking mode and show on LCD
 *****************************************************************************/
void cs_initiator_display_set_measurement_mode(sl_bt_cs_mode_t mode,
                                               uint8_t algo_mode)
{
  (void)mode;
  (void)algo_mode;
}

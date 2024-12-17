/*******************************************************************************
 * @file
 * @brief Alarm timers configuration file.
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

#ifndef _SL_ALARM_TIMERS_H
#define _SL_ALARM_TIMERS_H
#include "sl_si91x_ulp_timer.h"

#define DOWN_COUNTER 0
#define UP_COUNTER 1
#define CLK_TYPE_STATIC 1
#define CLK_TYPE_DYNAMIC 0

#define SL_OT_ULP_TIMER ULP_TIMER_0
#define SL_OT_ULP_TIMER_MODE ULP_TIMER_MODE_ONESHOT
#define SL_OT_ULP_TIMER_TYP ULP_TIMER_TYP_1US
#define SL_OT_TIMER_MATCH_VALUE_DEFAULT 0xFFFFFFFF
#define SL_OT_ULP_TIMER_DIRECTION UP_COUNTER
#define SL_OT_ULP_TIMER_CLK_TYPE CLK_TYPE_STATIC
#define SL_OT_ULP_TIMER_SYNC_TO_ULPSS_PCLK 0
#define SL_OT_ULP_TIMER_CLK_INPUT_SOURCE ULP_TIMER_32MHZ_RC_CLK_SRC
#define SL_OT_ULP_TIMER_SKIP_SWITCH_TIME 0

#endif // _SL_ALARM_TIMERS_H

/*
 *  Copyright (c) 2024, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 *   This file implements the OpenThread platform abstraction for the alarm.
 *
 */

#include <assert.h>
#include <openthread-core-config.h>
#include <openthread-system.h>
#include <stdbool.h>
#include <stdint.h>
#include <openthread/platform/alarm-micro.h>
#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/diag.h>
#include "common/debug.hpp"
#include "common/logging.hpp"

#include "alarm.h"
#include "platform-si91x.h"
#include "utils/code_utils.h"
// TODO To verify to remove this include
//#include "em_core.h"
#include "sl_alarm_timers.h"
#include "sl_core.h"
#include "sl_sleeptimer.h"

#ifndef TESTING
#define STATIC static
#else
#define STATIC
#endif

// timer data for handling wrapping
typedef struct wrap_timer_data wrap_timer_data_t;
struct wrap_timer_data
{
    uint16_t overflow_counter;
    uint16_t overflow_max;
};

// millisecond timer (sleeptimer)
static sl_sleeptimer_timer_handle_t sl_handle;
static uint64_t                     sMsAlarm         = 0;
static bool                         sIsMsRunning     = false;
static wrap_timer_data_t            milli_timer_data = {0};

#if OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE
// microsecond timer
static uint64_t          sUsAlarm         = 0;
static bool              sIsUsRunning     = false;
static wrap_timer_data_t micro_timer_data = {0};

static ulp_timer_config_t ulp_timer_handle = {
    .timer_num         = SL_OT_ULP_TIMER,
    .timer_mode        = SL_OT_ULP_TIMER_MODE,
    .timer_type        = SL_OT_ULP_TIMER_TYP,
    .timer_match_value = SL_OT_TIMER_MATCH_VALUE_DEFAULT,
    .timer_direction   = SL_OT_ULP_TIMER_DIRECTION,
};

static ulp_timer_clk_src_config_t ulp_timer_clk_handle = {
    .ulp_timer_clk_type           = SL_OT_ULP_TIMER_CLK_TYPE,
    .ulp_timer_sync_to_ulpss_pclk = SL_OT_ULP_TIMER_SYNC_TO_ULPSS_PCLK,
    .ulp_timer_clk_input_src      = SL_OT_ULP_TIMER_CLK_INPUT_SOURCE,
    .ulp_timer_skip_switch_time   = SL_OT_ULP_TIMER_SKIP_SWITCH_TIME,
};
#endif

// millisecond-alarm callback
SL_CODE_CLASSIFY(SL_CODE_COMPONENT_OT_PLATFORM_ABSTRACTION, SL_CODE_CLASS_TIME_CRITICAL)
STATIC void AlarmCallback(sl_sleeptimer_timer_handle_t *aHandle, void *aData)
{
    if (aData == NULL)
    {
        OT_UNUSED_VARIABLE(aHandle);
        otSysEventSignalPending();
    }
    else
    {
        sl_status_t        status;
        wrap_timer_data_t *timer_data = (wrap_timer_data_t *)aData;

        if (timer_data->overflow_counter < timer_data->overflow_max)
        {
            status = sl_sleeptimer_start_timer_ms(aHandle,
                                                  sl_sleeptimer_get_max_ms32_conversion(),
                                                  AlarmCallback,
                                                  (void *)timer_data,
                                                  0,
                                                  SL_SLEEPTIMER_NO_HIGH_PRECISION_HF_CLOCKS_REQUIRED_FLAG);
            OT_ASSERT(status == SL_STATUS_OK);
            timer_data->overflow_counter++;
        }
        else
        {
            if (timer_data->overflow_max != 0)
            {
                sIsMsRunning = false;
                sl_sleeptimer_stop_timer(aHandle);
            }
        }
    }
}

// microsecond-alarm callback
SL_CODE_CLASSIFY(SL_CODE_COMPONENT_OT_PLATFORM_ABSTRACTION, SL_CODE_CLASS_TIME_CRITICAL)
#if OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE
STATIC void ulpTimerExpired(void)
{
    sl_status_t status;

    if (micro_timer_data.overflow_counter < micro_timer_data.overflow_max)
    {
        status = sl_si91x_ulp_timer_stop(SL_OT_ULP_TIMER);
        OT_ASSERT(status == SL_STATUS_OK);

        status = sl_si91x_ulp_timer_set_count(SL_OT_ULP_TIMER, UINT32_MAX);
        OT_ASSERT(status == SL_STATUS_OK);

        status = sl_si91x_ulp_timer_start(SL_OT_ULP_TIMER);
        OT_ASSERT(status == SL_STATUS_OK);

        micro_timer_data.overflow_counter++;
    }
    else
    {
        if (micro_timer_data.overflow_max != 0)
        {
            sIsUsRunning = false;
            status       = sl_si91x_ulp_timer_stop(SL_OT_ULP_TIMER);
            OT_ASSERT(status == SL_STATUS_OK);
        }
    }
}
#endif

void otPlatAlarmInit(void)
{
    memset(&sl_handle, 0, sizeof sl_handle);
    sMsAlarm     = 0;
    sUsAlarm     = 0;
    sIsMsRunning = false;
    sIsUsRunning = false;
    memset(&milli_timer_data, 0, sizeof milli_timer_data);
#if OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE
    memset(&micro_timer_data, 0, sizeof micro_timer_data);

    sl_status_t status = sl_si91x_ulp_timer_init(&ulp_timer_clk_handle);
    OT_ASSERT(status == SL_STATUS_OK);

    status = sl_si91x_ulp_timer_set_configuration(&ulp_timer_handle);
    OT_ASSERT(status == SL_STATUS_OK);

    status = sl_si91x_ulp_timer_register_timeout_callback(SL_OT_ULP_TIMER, &ulpTimerExpired);
    OT_ASSERT(status == SL_STATUS_OK);

    status = sl_si91x_ulp_timer_start(SL_OT_ULP_TIMER);
    OT_ASSERT(status == SL_STATUS_OK);
#endif
}

void otPlatAlarmProcess(otInstance *aInstance)
{
    int64_t remaining;
    bool    alarmMilliFired = false;
#if OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE
    bool alarmMicroFired = false;
#endif

    CORE_DECLARE_IRQ_STATE;
    CORE_ENTER_ATOMIC();

    if (sIsMsRunning)
    {
        remaining = (int64_t)sMsAlarm - (int64_t)otPlatAlarmMilliGetNow();
        if (remaining <= 0)
        {
            otPlatAlarmMilliStop(aInstance);
            alarmMilliFired = true;
        }
    }

#if OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE
    if (sIsUsRunning)
    {
        remaining = (int64_t)sUsAlarm - (int64_t)otPlatAlarmMicroGetNow();
        if (remaining <= 0)
        {
            otPlatAlarmMicroStop(aInstance);
            alarmMicroFired = true;
        }
    }
#endif

    CORE_EXIT_ATOMIC();

    if (alarmMilliFired)
    {
#if OPENTHREAD_CONFIG_DIAG_ENABLE
        if (otPlatDiagModeGet())
        {
            otPlatDiagAlarmFired(aInstance);
        }
        else
#endif
        {
            otPlatAlarmMilliFired(aInstance);
        }
    }

#if OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE
    if (alarmMicroFired)
    {
        otPlatAlarmMicroFired(aInstance);
    }
#endif
}

uint64_t otPlatAlarmPendingTime(void)
{
    uint64_t remaining = 0;
    uint32_t now       = otPlatAlarmMilliGetNow();
    if (sIsMsRunning && (sMsAlarm > now))
    {
        remaining = sMsAlarm - (uint64_t)now;
    }
    return remaining;
}

bool otPlatAlarmIsRunning(otInstance *aInstance)
{
    return (otInstanceIsInitialized(aInstance) ? sIsMsRunning : false);
}

uint32_t otPlatAlarmMilliGetNow(void)
{
    uint64_t    ticks;
    uint64_t    now;
    sl_status_t status;

    ticks  = sl_sleeptimer_get_tick_count64();
    status = sl_sleeptimer_tick64_to_ms(ticks, &now);
    OT_ASSERT(status == SL_STATUS_OK);
    return (uint32_t)now;
}

uint32_t otPlatTimeGetXtalAccuracy(void)
{
#if defined(SL_CATALOG_POWER_MANAGER_PRESENT)
    // For sleepies, we need to account for the low-frequency crystal
    // accuracy when they go to sleep.  Accounting for that as well,
    // for the worst case.
    if (si91xAllowSleepCallback())
    {
        return SL_OPENTHREAD_HFXO_ACCURACY + SL_OPENTHREAD_LFXO_ACCURACY;
    }
#endif
    return SL_OPENTHREAD_HFXO_ACCURACY;
}

void otPlatAlarmMilliStartAt(otInstance *aInstance, uint32_t aT0, uint32_t aDt)
{
    OT_UNUSED_VARIABLE(aInstance);

    sl_status_t status;
    int64_t     remaining;
    uint32_t    initial_wrap_time;

    otEXPECT(sl_ot_rtos_task_can_access_pal());

    sl_sleeptimer_stop_timer(&sl_handle);

    sMsAlarm     = (uint64_t)aT0 + (uint64_t)aDt;
    remaining    = (int64_t)sMsAlarm - (int64_t)otPlatAlarmMilliGetNow();
    sIsMsRunning = true;

    if (remaining <= 0)
    {
        otSysEventSignalPending();
    }
    else
    {
        // The maximum value accepted by sleep timer ms32 APIs can be retrieved
        // using sl_sleeptimer_get_max_ms32_conversion().
        //
        // (See platform/service/sleeptimer/inc/sl_sleeptimer.h)
        //
        if (remaining > sl_sleeptimer_get_max_ms32_conversion())
        {
            initial_wrap_time                 = (uint32_t)(remaining % sl_sleeptimer_get_max_ms32_conversion());
            milli_timer_data.overflow_max     = (uint16_t)(remaining / sl_sleeptimer_get_max_ms32_conversion());
            milli_timer_data.overflow_counter = 0;

            // Start a timer with the initial time
            status = sl_sleeptimer_start_timer_ms(&sl_handle,
                                                  initial_wrap_time,
                                                  AlarmCallback,
                                                  (void *)&milli_timer_data,
                                                  0,
                                                  SL_SLEEPTIMER_NO_HIGH_PRECISION_HF_CLOCKS_REQUIRED_FLAG);
            OT_ASSERT(status == SL_STATUS_OK);
        }
        else
        {
            status = sl_sleeptimer_start_timer_ms(&sl_handle,
                                                  (uint32_t)remaining,
                                                  AlarmCallback,
                                                  NULL,
                                                  0,
                                                  SL_SLEEPTIMER_NO_HIGH_PRECISION_HF_CLOCKS_REQUIRED_FLAG);
            OT_ASSERT(status == SL_STATUS_OK);
        }
    }

exit:
    return;
}

void otPlatAlarmMilliStop(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    otEXPECT(sl_ot_rtos_task_can_access_pal());

    sl_sleeptimer_stop_timer(&sl_handle);
    sIsMsRunning = false;

exit:
    return;
}

#if OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE
uint32_t otPlatAlarmMicroGetNow(void)
{
    uint32_t    count  = 0;
    sl_status_t status = sl_si91x_ulp_timer_get_count(SL_OT_ULP_TIMER, &count);
    OT_ASSERT(status == SL_STATUS_OK);
    return count;
}
#endif

// Note: This function should be called at least once per wrap
// period for the wrap-around logic to work below
uint64_t otPlatTimeGet(void)
{
    static uint32_t timerWraps   = 0U;
    static uint32_t prev32TimeUs = 0U;
    uint32_t        now32TimeUs;
    uint64_t        now64TimeUs;
    CORE_DECLARE_IRQ_STATE;
    CORE_ENTER_CRITICAL();
    sl_status_t status = sl_si91x_ulp_timer_get_count(SL_OT_ULP_TIMER, &now32TimeUs);
    OT_ASSERT(status == SL_STATUS_OK);
    if (now32TimeUs < prev32TimeUs)
    {
        timerWraps += 1U;
    }
    prev32TimeUs = now32TimeUs;
    now64TimeUs  = ((uint64_t)timerWraps << 32) + now32TimeUs;
    CORE_EXIT_CRITICAL();
    return now64TimeUs;
}

#if OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE
void otPlatAlarmMicroStartAt(otInstance *aInstance, uint32_t aT0, uint32_t aDt)
{
    OT_UNUSED_VARIABLE(aInstance);
    sl_status_t status;
    int64_t     remaining;
    uint32_t    initial_wrap_time;

    otEXPECT(sl_ot_rtos_task_can_access_pal());

    status = sl_si91x_ulp_timer_stop(SL_OT_ULP_TIMER);
    OT_ASSERT(status == SL_STATUS_OK);

    sUsAlarm     = (uint64_t)aT0 + (uint64_t)aDt;
    remaining    = (int64_t)sUsAlarm - (int64_t)otPlatAlarmMicroGetNow();
    sIsUsRunning = true;

    if (remaining <= 0)
    {
        otSysEventSignalPending();
    }
    else
    {
        if (remaining > UINT32_MAX)
        {
            initial_wrap_time                 = (uint32_t)(remaining % UINT32_MAX);
            micro_timer_data.overflow_max     = (uint16_t)(remaining / UINT32_MAX);
            micro_timer_data.overflow_counter = 0;

            // Start a timer with the initial time
            status = sl_si91x_ulp_timer_set_count(SL_OT_ULP_TIMER, initial_wrap_time);
            OT_ASSERT(status == SL_STATUS_OK);
        }
        else
        {
            status = sl_si91x_ulp_timer_set_count(SL_OT_ULP_TIMER, remaining);
            OT_ASSERT(status == SL_STATUS_OK);
        }

        status = sl_si91x_ulp_timer_start(SL_OT_ULP_TIMER);
        OT_ASSERT(status == SL_STATUS_OK);
    }

exit:
    return;
}

void otPlatAlarmMicroStop(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    otEXPECT(sl_ot_rtos_task_can_access_pal());

    sl_status_t status = sl_si91x_ulp_timer_stop(SL_OT_ULP_TIMER);
    OT_ASSERT(status == SL_STATUS_OK);
    sIsUsRunning = false;

exit:
    return;
}

OT_TOOL_WEAK void otPlatAlarmMicroFired(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
}
#endif

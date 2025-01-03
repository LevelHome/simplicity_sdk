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
 *   This file includes all compile-time configuration constants used by
 *   si91x applications for OpenThread.
 */

#ifndef OPENTHREAD_CORE_SI91X_CONFIG_H_
#define OPENTHREAD_CORE_SI91X_CONFIG_H_

#ifdef SL_COMPONENT_CATALOG_PRESENT
#include "sl_component_catalog.h"
#endif

// Use (user defined) application config file to define OpenThread configurations
#ifdef SL_OPENTHREAD_APPLICATION_CONFIG_FILE
#include SL_OPENTHREAD_APPLICATION_CONFIG_FILE
#endif

// Use (pre-defined) stack features config file available for applications built
// with Simplicity Studio
#ifdef SL_OPENTHREAD_STACK_FEATURES_CONFIG_FILE
#include SL_OPENTHREAD_STACK_FEATURES_CONFIG_FILE
#endif

#include "board_config.h"
#include "openthread-si91x-config.h"

/**
 * @def OPENTHREAD_CONFIG_PLATFORM_BOOTLOADER_MODE_ENABLE
 *
 * Allow triggering a platform reset to bootloader mode, if supported.
 *
 */
#ifndef OPENTHREAD_CONFIG_PLATFORM_BOOTLOADER_MODE_ENABLE
#if defined(SL_CATALOG_GECKO_BOOTLOADER_INTERFACE_PRESENT)
#define OPENTHREAD_CONFIG_PLATFORM_BOOTLOADER_MODE_ENABLE 1
#else
#define OPENTHREAD_CONFIG_PLATFORM_BOOTLOADER_MODE_ENABLE 0
#endif
#endif

/**
 * @def OPENTHREAD_CONFIG_PLATFORM_INFO
 *
 * The platform-specific string to insert into the OpenThread version string.
 *
 */
#ifndef OPENTHREAD_CONFIG_PLATFORM_INFO
#define OPENTHREAD_CONFIG_PLATFORM_INFO "SI91X"
#endif

/**
 * @def OPENTHREAD_CONFIG_UPTIME_ENABLE
 *
 * (For FTDs only) Define to 1 to enable tracking the uptime of OpenThread instance.
 *
 */
#ifndef OPENTHREAD_CONFIG_UPTIME_ENABLE
#define OPENTHREAD_CONFIG_UPTIME_ENABLE OPENTHREAD_FTD
#endif

/**
 * @def OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE
 *
 * Define to 1 if you want to support microsecond timer in platform.
 *
 */
#ifndef OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE
#define OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
#endif

/**
 * @def OPENTHREAD_CONFIG_PLATFORM_FLASH_API_ENABLE
 *
 * Define to 1 to enable otPlatFlash* APIs to support non-volatile storage.
 *
 * When defined to 1, the platform MUST implement the otPlatFlash* APIs instead of the otPlatSettings* APIs.
 *
 */
#ifndef OPENTHREAD_CONFIG_PLATFORM_FLASH_API_ENABLE
#define OPENTHREAD_CONFIG_PLATFORM_FLASH_API_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_PSA_ITS_NVM_OFFSET
 *
 * This is the offset in ITS where the persistent keys are stored.
 * For Silabs OT applications, this needs to be in the range of
 * 0x20000 to 0x2ffff.
 *
 */
#define OPENTHREAD_CONFIG_PSA_ITS_NVM_OFFSET 0x20000

/**
 * @def SL_OPENTHREAD_HFXO_ACCURACY
 *
 * Worst case XTAL accuracy in units of ± ppm. Also used for calculations during CSL operations.
 *
 * @note Platforms may optimize this value based on operational conditions (i.e.: temperature).
 *
 */
#ifndef SL_OPENTHREAD_HFXO_ACCURACY
#ifdef SL_CATALOG_CLOCK_MANAGER_PRESENT
#define SL_OPENTHREAD_HFXO_ACCURACY 50
#else
#define SL_OPENTHREAD_HFXO_ACCURACY 50
#endif
#endif

#ifndef OPENTHREAD_CONFIG_NUM_MESSAGE_BUFFERS
#define OPENTHREAD_CONFIG_NUM_MESSAGE_BUFFERS 256
#endif

#ifndef OPENTHREAD_CONFIG_DIAG_OUTPUT_BUFFER_SIZE
#define OPENTHREAD_CONFIG_DIAG_OUTPUT_BUFFER_SIZE 500
#endif

#ifndef OPENTHREAD_CONFIG_LOG_PLATFORM
#define OPENTHREAD_CONFIG_LOG_PLATFORM 1
#endif

#ifndef OPENTHREAD_CONFIG_LOG_OUTPUT
#define OPENTHREAD_CONFIG_LOG_OUTPUT OPENTHREAD_CONFIG_LOG_OUTPUT_PLATFORM_DEFINED
#endif

#ifndef OPENTHREAD_CONFIG_LOG_LEVEL
#define OPENTHREAD_CONFIG_LOG_LEVEL OT_LOG_LEVEL_INFO
#endif

#ifndef OPENTHREAD_CONFIG_LOG_LEVEL_INIT
#define OPENTHREAD_CONFIG_LOG_LEVEL_INIT OT_LOG_LEVEL_CRIT
#endif

#ifndef OPENTHREAD_CONFIG_LOG_LEVEL_DYNAMIC_ENABLE
#define OPENTHREAD_CONFIG_LOG_LEVEL_DYNAMIC_ENABLE 1
#endif

#ifndef OPENTHREAD_CONFIG_IP6_SLAAC_ENABLE
#define OPENTHREAD_CONFIG_IP6_SLAAC_ENABLE 1
#endif

#ifndef OPENTHREAD_CONFIG_NCP_HDLC_ENABLE
#define OPENTHREAD_CONFIG_NCP_HDLC_ENABLE 1
#endif

#ifndef OPENTHREAD_CONFIG_PLATFORM_RADIO_COEX_ENABLE
#define OPENTHREAD_CONFIG_PLATFORM_RADIO_COEX_ENABLE 0
#endif

#ifndef OPENTHREAD_CONFIG_LOG_MAX_SIZE
#define OPENTHREAD_CONFIG_LOG_MAX_SIZE 1024
#endif

#ifndef OPENTHREAD_CONFIG_COMMISSIONER_MAX_JOINER_ENTRIES
#define OPENTHREAD_CONFIG_COMMISSIONER_MAX_JOINER_ENTRIES 4
#endif

#ifndef OPENTHREAD_CONFIG_MLE_MAX_CHILDREN
#define OPENTHREAD_CONFIG_MLE_MAX_CHILDREN 64
#endif

#ifndef OPENTHREAD_CONFIG_MLE_IP_ADDRS_PER_CHILD
#define OPENTHREAD_CONFIG_MLE_IP_ADDRS_PER_CHILD 16
#endif

#ifndef OPENTHREAD_CONFIG_IP6_MAX_EXT_UCAST_ADDRS
#define OPENTHREAD_CONFIG_IP6_MAX_EXT_UCAST_ADDRS 8
#endif

#ifndef OPENTHREAD_CONFIG_IP6_MAX_EXT_MCAST_ADDRS
#define OPENTHREAD_CONFIG_IP6_MAX_EXT_MCAST_ADDRS 8
#endif

#ifndef OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE
#define OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE 1
#endif

#ifndef OPENTHREAD_CONFIG_HEAP_INTERNAL_SIZE
#define OPENTHREAD_CONFIG_HEAP_INTERNAL_SIZE (63 * 1024)
#endif

#ifndef OPENTHREAD_CONFIG_HEAP_INTERNAL_SIZE_NO_DTLS
#define OPENTHREAD_CONFIG_HEAP_INTERNAL_SIZE_NO_DTLS (63 * 1024)
#endif

#ifndef OPENTHREAD_CONFIG_CLI_MAX_LINE_LENGTH
#define OPENTHREAD_CONFIG_CLI_MAX_LINE_LENGTH 640
#endif

#ifndef OPENTHREAD_CONFIG_CLI_UART_RX_BUFFER_SIZE
#define OPENTHREAD_CONFIG_CLI_UART_RX_BUFFER_SIZE 640
#endif

#ifndef OPENTHREAD_CONFIG_UPTIME_ENABLE
#define OPENTHREAD_CONFIG_UPTIME_ENABLE 1
#endif

#ifndef OPENTHREAD_CONFIG_LOG_PREPEND_UPTIME
#define OPENTHREAD_CONFIG_LOG_PREPEND_UPTIME 1
#endif

#ifndef OPENTHREAD_CONFIG_SRP_CLIENT_BUFFERS_MAX_SERVICES
#define OPENTHREAD_CONFIG_SRP_CLIENT_BUFFERS_MAX_SERVICES 20
#endif

#ifndef OPENTHREAD_CONFIG_ASSERT_CHECK_API_POINTER_PARAM_FOR_NULL
#define OPENTHREAD_CONFIG_ASSERT_CHECK_API_POINTER_PARAM_FOR_NULL 1
#endif

#ifndef OPENTHREAD_CONFIG_PLATFORM_POWER_CALIBRATION_ENABLE
#define OPENTHREAD_CONFIG_PLATFORM_POWER_CALIBRATION_ENABLE 1
#endif

#endif // OPENTHREAD_CORE_SI91X_CONFIG_H_

/*******************************************************************************
 * @file
 * @brief CLI application code.
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

#include "openthread-si91x-config.h"

#include <openthread/config.h>

#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <libgen.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <syslog.h>
#include <unistd.h>

#ifndef HAVE_LIBEDIT
#define HAVE_LIBEDIT 0
#endif

#ifndef HAVE_LIBREADLINE
#define HAVE_LIBREADLINE 0
#endif

#include <openthread/cli.h>
#include <openthread/diag.h>
#include <openthread/logging.h>
#include <openthread/tasklet.h>
#include <openthread/thread.h>
#include <openthread/platform/radio.h>
#if !OPENTHREAD_POSIX_CONFIG_DAEMON_ENABLE
#include <openthread/cli.h>
#include "cli/cli_config.h"
#endif
#include "sl_system_init.h"
#include <common/code_utils.hpp>
#include <lib/platform/exit_code.h>
#include <lib/platform/reset_util.h>
#include <lib/spinel/coprocessor_type.h>
#include <openthread/openthread-system.h>
#include <openthread/platform/misc.h>

#include "cmsis_os2.h"

#include "sl_net.h"
#include "sl_status.h"
#include "sl_utility.h"
#include "sl_wifi.h"
#include "sl_wifi_callback_framework.h"

#include "sl_board_configuration.h"
#include "sl_net_wifi_types.h"
#include "sl_si91x_driver.h"

/**
 * Initializes NCP app.
 *
 * @param[in]  aInstance    A pointer to the OpenThread instance.
 *
 */
void otAppNcpInit(otInstance *aInstance);

/**
 * Deinitializes NCP app.
 *
 */
void otAppNcpUpdate(otSysMainloopContext *aContext);

/**
 * Updates the file descriptor sets with file descriptors used by console.
 *
 * @param[in,out]   aMainloop   A pointer to the mainloop context.
 *
 */
void otAppNcpProcess(const otSysMainloopContext *aContext);

/**
 * Initializes CLI app.
 *
 * @param[in]  aInstance    A pointer to the OpenThread instance.
 *
 */
void otAppCliInit(otInstance *aInstance);

/**
 * Deinitializes CLI app.
 *
 */
void otAppCliDeinit(void);

/**
 * Updates the file descriptor sets with file descriptors used by console.
 *
 * @param[in,out]   aMainloop   A pointer to the mainloop context.
 *
 */
void otAppCliUpdate(otSysMainloopContext *aMainloop);

/**
 * Performs console driver processing.
 *
 * @param[in]    aMainloop      A pointer to the mainloop context.
 *
 */
void        otAppCliProcess(const otSysMainloopContext *aMainloop);
extern void otSysProcessDrivers(otInstance *aInstance);

static otInstance *sInstance = NULL;

static otInstance *InitInstance(otPlatformConfig *aConfig)
{
    otLogPlat(OT_LOG_LEVEL_INFO, "app", "Running %s", otGetVersionString());
    otLogPlat(OT_LOG_LEVEL_INFO, "app", "Thread version: %hu", otThreadGetVersion());
#if OPENTHREAD_CONFIG_LOG_LEVEL_DYNAMIC_ENABLE
    IgnoreError(otLoggingSetLevel(aConfig->mLogLevel));
#endif
    sInstance = otSysInit(aConfig);
    VerifyOrDie(sInstance != NULL, OT_EXIT_FAILURE);
    // syslog(LOG_INFO, "Thread interface: %s", otSysGetThreadNetifName());

    if (aConfig->mCoprocessorType != OT_COPROCESSOR_RCP)
    {
        otLogPlat(OT_LOG_LEVEL_CRIT, "app", "Only RCP is supported by this app now!\n");
        exit(OT_EXIT_FAILURE);
    }

    otLogPlat(OT_LOG_LEVEL_CRIT, "app", "RCP version: %s", otPlatRadioGetVersionString(sInstance));

    if (aConfig->mDryRun)
    {
        exit(OT_EXIT_SUCCESS);
    }

    return sInstance;
}

// void otTaskletsSignalPending(otInstance *aInstance) { OT_UNUSED_VARIABLE(aInstance); }

void otPlatReset(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    // gPlatResetReason = OT_PLAT_RESET_REASON_SOFTWARE;

    otSysDeinit();
    // longjmp(gResetJump, 1);
    assert(false);
}

otInstance *otGetInstance(void)
{
    return sInstance;
}

void sl_ot_create_instance(void)
{
    otPlatformConfig platformConfig = {
        /* Backbone is unused */
        .mBackboneInterfaceName = NULL,
        /* Netif is unused */
        .mInterfaceName   = NULL,
        .mCoprocessorUrls = {.mUrls = {OPENTHREAD_CONFIG_MCU_SYSTEM_RADIO_URL}, .mNum = 1},
        .mRealTimeSignal  = 0,
        /* This factor will scale up the speed of the time in the platform */
        .mSpeedUpFactor = 1,
        /* True if no posix platform deamon should be initialized */
        .mDryRun = false};
    sInstance = InitInstance(&platformConfig);
    assert(sInstance);
}

void sl_ot_cli_init(void)
{
    otAppCliInit(sInstance);
}

/******************************************************************************
 * Application Init.
 *****************************************************************************/

void app_init(void)
{
    OT_SETUP_RESET_JUMP(argv);
}

/******************************************************************************
 * Application Process Action.
 *****************************************************************************/
void app_process_action(void)
{
    otSysMainloopContext mainloop;
    otTaskletsProcess(sInstance);
    otSysMainloopUpdate(sInstance, &mainloop);
    otSysMainloopProcess(sInstance, &mainloop);
    otSysProcessDrivers(sInstance);
}

void app_cli_process_action(void)
{
    if (sInstance != NULL)
    {
        otAppCliUpdate(NULL);
        otAppCliProcess(NULL);
    }
}

/******************************************************************************
 * Application Exit.
 *****************************************************************************/
void app_exit(void)
{
    otAppCliDeinit();
    otSysDeinit();
}

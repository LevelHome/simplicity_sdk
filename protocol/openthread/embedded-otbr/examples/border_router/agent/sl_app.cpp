/*******************************************************************************
 * @file
 * @brief Application interface provided to main().
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

#include <vector>

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
#include <openthread/openthread-system.h>
#include <openthread/tasklet.h>
#include <openthread/thread.h>
#include <openthread/platform/radio.h>
#if !OPENTHREAD_POSIX_CONFIG_DAEMON_ENABLE
#include <openthread/cli.h>
#include "cli/cli_config.h"
#endif
#include "sl_system_init.h"
#include <lib/platform/reset_util.h>
#include <lib/spinel/coprocessor_type.h>
#include <openthread/openthread-system.h>
#include <openthread/platform/misc.h>

#include "agent/application.hpp"
#include "border_agent/border_agent.hpp"
#include "common/logging.hpp"
#include "common/mainloop.hpp"
#include "common/otbr_code_utils.hpp"
#include "common/types.hpp"
#include "mdns/mdns.hpp"
#include "rcp/thread_host.hpp"

#include "cmsis_os2.h"
#include "sl_cmsis_os2_common.h"
#include "sl_component_catalog.h"

#include "lwip/timeouts.h"

#include "platform-si91x.h"

#ifdef __cplusplus
extern "C" {
#endif

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

static void sOTBRAppTask(void *context);

/* OT main stack task specific settings */
__ALIGNED(8) static uint8_t sOTBRAppTaskMem[4608];
__ALIGNED(4) static uint8_t sOTBRAppTaskCb[osThreadCbSize];
static const osThreadAttr_t sOTBRAppTaskAttr =
    {"OTBR App", 0u, sOTBRAppTaskCb, osThreadCbSize, sOTBRAppTaskMem, 4608, (osPriority_t)24, 0, 0};
static const osSemaphoreAttr_t sOTBRAppSemaphoreAtt = {"OT Stack Semaphore", 0, 0, 0};
static osSemaphoreId_t         sOTBRAppSemaphoreId;
static osThreadId_t            sOTBRAppTaskId;

static otInstance        *sInstance               = NULL;
static otbr::Application *gApp                    = nullptr;
static const char         kDefaultInterfaceName[] = "wpan0";

// Port number used by Rest server.
static const uint32_t kPortNumber = 8081;

void otPlatReset(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    VerifyOrDie(gApp != nullptr, "gApp is null");
    gApp->Deinit();
    gApp = nullptr;
    otSysDeinit();
    assert(false);
}

otInstance *otGetInstance(void)
{
    return sInstance;
}

void sl_ot_create_instance(void)
{
}

void sl_ot_cli_init(void)
{
    // otAppCliInit(sInstance);
}

/**************************************************************************/ /**
                                                                              * Application Init.
                                                                              *****************************************************************************/

void app_init(void)
{
    sOTBRAppSemaphoreId = osSemaphoreNew(1, 0, &sOTBRAppSemaphoreAtt);
    sOTBRAppTaskId      = osThreadNew(sOTBRAppTask, NULL, &sOTBRAppTaskAttr);
}

/**************************************************************************/ /**
                                                                              * Application Process Action.
                                                                              *****************************************************************************/
void app_process_action(void)
{
    // gApp->Run();
}

static void sOTBRAppTask(void *context)
{
    OT_UNUSED_VARIABLE(context);
    platformNetifInitialize();
    platformNetifStart();
    while (!platformNetifHasIPv6())
    {
        osDelay(100);
    }
    const char               *interfaceName     = kDefaultInterfaceName;
    bool                      enableAutoAttach  = true;
    const char               *restListenAddress = "";
    int                       restListenPort    = kPortNumber;
    std::vector<const char *> backboneInterfaceNames;
    std::vector<const char *> radioUrls;

    radioUrls.push_back(OPENTHREAD_CONFIG_MCU_SYSTEM_RADIO_URL);
    otbr::Application app(interfaceName,
                          backboneInterfaceNames,
                          radioUrls,
                          enableAutoAttach,
                          restListenAddress,
                          restListenPort);
    gApp      = &app;
    sInstance = app.Init();
    assert(sInstance);
    otAppCliInit(sInstance);
    while (true)
    {
        otPlatTimeGet();
        // Process callbacks and tasklets
        // TODO otSysProcessDrivers(instance);
        // TODO otTaskletsProcess(instance);
        app.Run();

        if (!otTaskletsArePending(sInstance))
        {
            osSemaphoreAcquire(sOTBRAppSemaphoreId, osWaitForever);
        }
    }
    app.Deinit();
    // If Reset was requested, finalize OT and terminate the thread..
    otInstanceFinalize(sInstance);
    osThreadTerminate(sOTBRAppTaskId);
}

void otTaskletsSignalPending(otInstance *aInstance)
{
    (void)aInstance;
    osSemaphoreRelease(sOTBRAppSemaphoreId);
}

void otSysEventSignalPending(void)
{
    osSemaphoreRelease(sOTBRAppSemaphoreId);
}

void app_cli_process_action(void)
{
    if (sInstance != NULL)
    {
        otAppCliUpdate(NULL);
        otAppCliProcess(NULL);
    }
}

/**************************************************************************/ /**
                                                                              * Application Exit.
                                                                              *****************************************************************************/
void app_exit(void)
{
    otAppCliDeinit();
    otSysDeinit();
}

#ifdef __cplusplus
}
#endif

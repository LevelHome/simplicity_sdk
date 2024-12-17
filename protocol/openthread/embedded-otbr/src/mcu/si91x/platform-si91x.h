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
 *   This file includes the platform-specific initializers.
 *
 */

#ifndef PLATFORM_SI91X_H_
#define PLATFORM_SI91X_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <openthread/instance.h>

#include "em_device.h"
// TODO: remove #include "em_system.h"

// TODO: remove #include "rail.h"

// Global OpenThread instance structure
// extern otInstance *sInstance;

#ifdef SL_COMPONENT_CATALOG_PRESENT
#include "sl_component_catalog.h"
#endif // SL_COMPONENT_CATALOG_PRESENT

#ifndef SL_CATALOG_KERNEL_PRESENT
#define sl_ot_rtos_task_can_access_pal() (true)
#elif defined(MATTER_INTEGRATION) && MATTER_INTEGRATION
// TODO: Temporary for matter integration. This will be fixed later.
#define sl_ot_rtos_task_can_access_pal() (true)
#else
#include "sl_ot_rtos_adaptation.h"
#endif
// TODO: remove RAIL
//  Global reference to rail handle
#ifndef SL_CATALOG_RAIL_MULTIPLEXER_PRESENT
#define gRailHandle emPhyRailHandle // use gRailHandle in the OpenThread PAL.
#endif

// TODO: remove extern RAIL_Handle_t gRailHandle; // coex needs the emPhyRailHandle symbol.

/**
 * This function performs all platform-specific initialization of
 * OpenThread's drivers.
 *
 */
void sl_ot_sys_init(void);

/**
 * This function performs UART driver processing.
 *
 */
void si91xUartProcess(void);

/**
 * This function performs CPC driver processing.
 *
 */
void si91xCpcProcess(void);

/**
 * This function performs SPI driver processing.
 *
 */
void si91xSpiProcess(void);

/**
 * Initialization of Misc module.
 *
 */
void si91xMiscInit(void);

/**
 * Initialization of ADC module for random number generator.
 *
 */
void si91xRandomInit(void);

/**
 * Initialization of Logger driver.
 *
 */
void si91xLogInit(void);

/**
 * Deinitialization of Logger driver.
 *
 */
void si91xLogDeinit(void);

/**
 * Print reset info.
 *
 */
void si91xPrintResetInfo(void);

/**
 * Set 802.15.4 CCA mode
 *
 * A call to this function should be made after RAIL has been
 * initialized and a valid handle is available. On platforms that
 * don't support different CCA modes, a call to this function with
 * non-Default CCA mode (i.e. with any value except
 * RAIL_IEEE802154_CCA_MODE_RSSI) will return a failure.
 *
 * @param[in] aMode Mode of CCA operation.
 * @return RAIL Status code indicating success of the function call.
 */
// TODO: remove RAIL_Status_t si91xRadioSetCcaMode(uint8_t aMode);

/**
 * This callback is used to check if is safe to put the si91x into a
 * low energy sleep mode.
 *
 * The callback should return true if it is ok to enter sleep mode.
 * Note that the callback must add an EM1 requirement if it intends
 * to idle (EM1) instead of entering a deep sleep (EM2) mode.
 */

bool si91xAllowSleepCallback(void);

/**
 * Load the channel configurations.
 *
 * @param[in]  aChannel   The radio channel.
 * @param[in]  aTxPower   The radio transmit power in dBm.
 *
 * @retval OT_ERROR_NONE         Successfully enabled/disabled .
 * @retval OT_ERROR_INVALID_ARGS Invalid channel.
 *
 */
otError si91xRadioLoadChannelConfig(uint8_t aChannel, int8_t aTxPower);

void    platformHDLCInit(void);
size_t  platformHDLCRead(const uint8_t *aBuf);
void    platformHDLCRecv(const uint8_t *aBuf, uint16_t aBufLength);
otError platformHDLCWrite(const uint8_t *aBuf, size_t lengh);
bool    platformHDLCIsBufferOccupied(size_t aTimeoutMs);
void    platformNetifInitialize(void);
void    platformNetifStart(void);
bool    platformNetifHasIPv6(void);

// TODO: remove? otError railStatusToOtError(RAIL_Status_t status);

#ifdef __cplusplus
}
#endif

#endif // PLATFORM_SI91X_H_

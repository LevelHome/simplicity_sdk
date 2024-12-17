/***************************************************************************//**
 * @file
 * @brief CS Initiator configuration
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

#ifndef CS_INITIATOR_CONFIG_H
#define CS_INITIATOR_CONFIG_H

#include "sl_bt_api.h"
#include "sl_rtl_clib_api.h"

/***********************************************************************************************//**
 * @addtogroup cs_initiator
 * @{
 **************************************************************************************************/

// <<< Use Configuration Wizard in Context Menu >>>

// <h> CS Initiator configuration

// <o CS_INITIATOR_MAX_CONNECTIONS> Specify maximum initiator connections <1..4>
// <i> Default: 1
#ifndef CS_INITIATOR_MAX_CONNECTIONS
#define CS_INITIATOR_MAX_CONNECTIONS                  (1)
#endif

// <o CS_INITIATOR_MAX_SUBEVENT_PER_PROC> Specify maximum subevents per procedure <1..6>
// <i> Default: 6
#ifndef CS_INITIATOR_MAX_SUBEVENT_PER_PROC
#define CS_INITIATOR_MAX_SUBEVENT_PER_PROC            (6)
#endif

// <o CS_INITIATOR_MAX_STEP_DATA_LEN> Specify maximum step data length <32..2048>
// <i> Default: 2048
#ifndef CS_INITIATOR_MAX_STEP_DATA_LEN
#define CS_INITIATOR_MAX_STEP_DATA_LEN                (2048)
#endif

// <e CS_INITIATOR_LOG> Enable initiator log
// <i> Default: 1
// <i> Enable Initiator component logger
#ifndef CS_INITIATOR_LOG
#define CS_INITIATOR_LOG                              (1)
#endif

// <s CS_INITIATOR_LOG_PREFIX> Log prefix
// <i> Default: "[Initiator]"
#ifndef CS_INITIATOR_LOG_PREFIX
#define CS_INITIATOR_LOG_PREFIX                       "[Initiator]"
#endif

// </e>

// <q CS_INITIATOR_RTL_LOG> Enable RTL log
// <i> Default: 1
#ifndef CS_INITIATOR_RTL_LOG
#define CS_INITIATOR_RTL_LOG                          (1)
#endif

// <o CS_INITIATOR_CS_EVENT_BUF_SIZE> Size of the Channel Sounding event buffer [elements] <1..255>
// <i> Default: 16
// <i> Size of the Channel Sounding event buffer
#ifndef CS_INITIATOR_CS_EVENT_BUF_SIZE
#define CS_INITIATOR_CS_EVENT_BUF_SIZE                (16)
#endif

// <o CS_INITIATOR_DEFAULT_MIN_PROCEDURE_INTERVAL> Minimum delay between CS measurements [connection events] <1..255>
// <i> Default: 30
// <i> Minimum duration in number of connection events between consecutive CS measurement procedures
#ifndef CS_INITIATOR_DEFAULT_MIN_PROCEDURE_INTERVAL
#define CS_INITIATOR_DEFAULT_MIN_PROCEDURE_INTERVAL                     (30)
#endif

// <o CS_INITIATOR_DEFAULT_MAX_PROCEDURE_INTERVAL> Maximum delay between CS measurements [connection events] <1..255>
// <i> Default: 30
// <i> Maximum duration in number of connection events between consecutive CS measurement procedures
#ifndef CS_INITIATOR_DEFAULT_MAX_PROCEDURE_INTERVAL
#define CS_INITIATOR_DEFAULT_MAX_PROCEDURE_INTERVAL                     (30)
#endif

// <o CS_INITIATOR_ANTENNA_OFFSET> Specify antenna offset
// <0=> Wireless antenna offset
// <1=> Wired antenna offset
// <i> Default: 0
#ifndef CS_INITIATOR_ANTENNA_OFFSET
#define CS_INITIATOR_ANTENNA_OFFSET                   0
#endif

// <o CS_INITIATOR_ERROR_TIMEOUT_MS> Error timeout [msec] <100..5000>
// <i> Timeout value in order to avoid stuck in error state indefinitely.
// <i> Once the time elapses the initiator instance's error callback executes to
// <i> inform the user about the issue.
// <i> Default: 3000
#define CS_INITIATOR_ERROR_TIMEOUT_MS                3000

// <o CS_INITIATOR_PROCEDURE_TIMEOUT_MS> CS procedure timeout [msec] <100..5000>
// <i> Timeout value for procedures - in order to avoid getting stuck in a procedure indefinitely.
// <i> Once the time elapses the initiator instance's error callback executes to
// <i> inform the user about the issue.
// <i> Default: 3000
#define CS_INITIATOR_PROCEDURE_TIMEOUT_MS            3000

// <a.10 CS_CUSTOM_CHANNEL_MAP> Custom channel map <0..255> <f.h>
// <i> Default: { 0xFC, 0xFF, 0x7F, 0xFC, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x1F }
#define CS_CUSTOM_CHANNEL_MAP                        { 0xFC, 0xFF, 0x7F, 0xFC, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x1F }

// <o CS_INITIATOR_DEFAULT_CS_MODE> Main mode
// <1=> RTT
// <2=> PBR
#define CS_INITIATOR_DEFAULT_CS_MODE                 2

// <o CS_INITIATOR_DEFAULT_MIN_CONNECTION_INTERVAL> Minimum connection interval (in 1.25 ms steps) <6..3200>
// <i> Default: 6
#define CS_INITIATOR_DEFAULT_MIN_CONNECTION_INTERVAL         6

// <o CS_INITIATOR_DEFAULT_MIN_CONNECTION_INTERVAL> Maximum connection interval (in 1.25 ms steps) <6..3200>
// <i> Default: 6
#define CS_INITIATOR_DEFAULT_MAX_CONNECTION_INTERVAL         6

// <o CS_INITIATOR_DEFAULT_MAX_PROCEDURE_COUNT> Procedure execution number
// <0=> Free running
// <1=> Start new procedure after one finished
// <i> Default: 1
#define CS_INITIATOR_DEFAULT_MAX_PROCEDURE_COUNT         1

// <o CS_INITIATOR_DEFAULT_CONN_PHY> Connection PHY
// <sl_bt_gap_phy_1m=> 1M
// <sl_bt_gap_phy_2m=> 2M
// <i> Default: 2M
#define CS_INITIATOR_DEFAULT_CONN_PHY         sl_bt_gap_phy_2m

// <o CS_INITIATOR_DEFAULT_CS_SYNC_PHY> CS sync PHY
// <sl_bt_gap_phy_1m=> 1M
// <sl_bt_gap_phy_2m=> 2M
// <i> Default: sl_bt_gap_phy_1m
#define CS_INITIATOR_DEFAULT_CS_SYNC_PHY       sl_bt_gap_phy_1m

// <o CS_INITIATOR_DEFAULT_MODE0_STEPS> Mode 0 steps
// <i> Number of Mode 0 steps to be included at the beginning of the test CS subevent
// <i> Default: 3
#define CS_INITIATOR_DEFAULT_MODE0_STEPS      3

// <o CS_INITIATOR_DEFAULT_CS_TONE_ANTENNA_CONFIG_IDX_REQ> CS tone antenna configuration requested
// <i> Use all the available antennas that can be supported
// <i> Value: 0. 1 antenna path, [1:1] antenna [initiator:reflector]
// <i> Value: 1. 2 antenna path, [2:1] antenna
// <i> Value: 2. 3 antenna path, [3:1] antenna (not supported)
// <i> Value: 3. 4 antenna path, [4:1] antenna (not supported)
// <i> Value: 4. 2 antenna path, [1:2] antenna
// <i> Value: 5. 3 antenna path, [1:3] antenna (not supported)
// <i> Value: 6. 4 antenna path, [1:4] antenna (not supported)
// <i> Value: 7. 4 antenna path, [2:2] antenna
// <CS_ANTENNA_CONFIG_INDEX_SINGLE_ONLY=> 1 antenna path, [1:1] antenna
// <CS_ANTENNA_CONFIG_INDEX_DUAL_I_SINGLE_R=> 2 antenna path, [2:1] antenna
// <CS_ANTENNA_CONFIG_INDEX_SINGLE_I_DUAL_R=> 2 antenna path, [1:2] antenna
// <CS_ANTENNA_CONFIG_INDEX_DUAL_ONLY=> 4 antenna path, [2:2] antenna
// <i> Default: CS_ANTENNA_CONFIG_INDEX_DUAL_ONLY
#define CS_INITIATOR_DEFAULT_CS_TONE_ANTENNA_CONFIG_IDX_REQ   CS_ANTENNA_CONFIG_INDEX_DUAL_ONLY

// <o CS_INITIATOR_DEFAULT_CS_SYNC_ANTENNA_REQ> CS sync antenna configuration requested
// <i> Use all the available antennas that can be supported
// <CS_SYNC_ANTENNA_1=> Antenna 1
// <CS_SYNC_ANTENNA_2=> Antenna 2
// <CS_SYNC_SWITCHING=> Switching
// <i> Default: CS_SYNC_SWITCHING
#define CS_INITIATOR_DEFAULT_CS_SYNC_ANTENNA_REQ  CS_SYNC_SWITCHING

// <o CS_INITIATOR_DEFAULT_PREFERRED_PEER_ANTENNA> Preferred peer antenna
// <i> The preferred peer-ordered antenna elements to be used by the remote device
// <i> for the antenna configuration denoted by the tone antenna config selection.
// <i> Default: 1
#define CS_INITIATOR_DEFAULT_PREFERRED_PEER_ANTENNA  1

// <o CS_INITIATOR_DEFAULT_MAX_TX_POWER> Maximum transmit power level to be used in all CS transmissions <-127..20>
// <i> Default: 20
#define CS_INITIATOR_DEFAULT_MAX_TX_POWER  20

// <o CS_INITIATOR_DEFAULT_CREATE_CONTEXT> Create context
// <i> Write CS configuration in the local controller only
// <i> or both local and remote controller using a configuration procedure
// <0=> Local controller only
// <1=> Both local and remote
// <i> Default: 1
#define CS_INITIATOR_DEFAULT_CREATE_CONTEXT  1

// <o CS_INITIATOR_DEFAULT_RSSI_REF_TX_POWER> RSSI reference TX power <-110..30>
// <i> Reference RSSI value of the remote Reflector device at 1.0 m distance in dBm
// <i> Default: -40.0F
#define CS_INITIATOR_DEFAULT_RSSI_REF_TX_POWER  -40.0F

// <o CS_INITIATOR_DEFAULT_CONNECTION_PERIPHERAL_LATENCY> Connection peripheral latency
// <i> Peripheral latency, which defines how many connection
// <i> intervals the peripheral can skip if it has no data to send
// <i> Default: 0
#define CS_INITIATOR_DEFAULT_CONNECTION_PERIPHERAL_LATENCY  0

// <o CS_INITIATOR_DEFAULT_TIMEOUT> Supervision timeout [msec]
// <i> Default: 200
#define CS_INITIATOR_DEFAULT_TIMEOUT 200

// <o CS_INITIATOR_DEFAULT_MIN_CE_LENGTH> Minimum length of the connection event <1..65535>
// <i> Value in units of 0.625 ms
// <i> Default: 0
#define CS_INITIATOR_DEFAULT_MIN_CE_LENGTH 0

// <o CS_INITIATOR_DEFAULT_MAX_CE_LENGTH> Maximum length of the connection event <1..65535>
// <i> Value in units of 0.625 ms
// <i> Default: 65535
#define CS_INITIATOR_DEFAULT_MAX_CE_LENGTH        65535

// <o CS_INITIATOR_DEFAULT_CHANNEL_SELECTION_TYPE> Channel selection type
// <sl_bt_cs_channel_selection_algorithm_3b=> Algorithm 3b
// <sl_bt_cs_channel_selection_algorithm_3c=> Algorithm 3c
// <sl_bt_cs_channel_selection_algorithm_user_shape_interleaved=> Algorithm user shape interleaved
// <i> Default: sl_bt_cs_channel_selection_algorithm_3b
#define CS_INITIATOR_DEFAULT_CHANNEL_SELECTION_TYPE          sl_bt_cs_channel_selection_algorithm_3b

// <o CS_INITIATOR_DEFAULT_CH3C_SHAPE> Ch3c shape
// <sl_bt_cs_ch3c_shape_hat=> Ch3c shape hat
// <sl_bt_cs_chc3_shape_interleaved=> Ch3c shape interleaved
// <i> Default: sl_bt_cs_ch3c_shape_hat
#define CS_INITIATOR_DEFAULT_CH3C_SHAPE             sl_bt_cs_ch3c_shape_hat

// <o CS_INITIATOR_DEFAULT_ALGO_MODE> Object tracking mode
// <SL_RTL_CS_ALGO_MODE_REAL_TIME_BASIC=> Real time basic
// <SL_RTL_CS_ALGO_MODE_STATIC_HIGH_ACCURACY=> Static high accuracy
// <i> Default: SL_RTL_CS_ALGO_MODE_REAL_TIME_BASIC
#define CS_INITIATOR_DEFAULT_ALGO_MODE           SL_RTL_CS_ALGO_MODE_REAL_TIME_BASIC

// <o CS_INITIATOR_DEFAULT_CHANNEL_MAP_PRESET> Channel map preset
// <CS_CHANNEL_MAP_PRESET_LOW=> Low
// <CS_CHANNEL_MAP_PRESET_MEDIUM=> Medium
// <CS_CHANNEL_MAP_PRESET_HIGH=> High
// <CS_CHANNEL_MAP_PRESET_CUSTOM=> Custom
// <i> Default: CS_CHANNEL_MAP_PRESET_HIGH
#define CS_INITIATOR_DEFAULT_CHANNEL_MAP_PRESET           CS_CHANNEL_MAP_PRESET_HIGH

// </h>

// <<< end of configuration section >>>

// Ch3c jump <2..8>
// Number of channels skipped in each rising and falling sequence.
// Default: 2
#define CS_INITIATOR_DEFAULT_CH3C_JUMP   2

// Max procedure duration <1..65535>
// Maximum duration for each measurement procedure.
// Value in units of 0.625 ms
// Default: 65535
#define CS_INITIATOR_DEFAULT_MAX_PROCEDURE_DURATION   65535

// Transmit power delta, in signed dB.
// Default: 0
#define CS_INITIATOR_DEFAULT_TX_PWR_DELTA   0

// Config ID
// Only one channel is supported
// Default: 1
#define CS_INITIATOR_DEFAULT_CONFIG_ID      1

// Minimum subevent length. <1250..3999999>
// Minimum suggested duration for each CS subevent in microseconds.
// Default: 1250
#define CS_INITIATOR_DEFAULT_MIN_SUBEVENT_LEN          1250

// Maximum subevent length. <1250..3999999>
// Maximum suggested duration for each CS subevent in microseconds.
// Default: 3999999
#define CS_INITIATOR_DEFAULT_MAX_SUBEVENT_LEN          3999999

/** @} (end addtogroup cs_initiator) */
#endif // CS_INITIATOR_CONFIG_H

/***************************************************************************//**
 * @file
 * @brief CS NCP host
 *
 * Reference implementation of a CS host with initiator and reflector support.
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
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include "app.h"
#include "ncp_host.h"
#include "app_log.h"
#include "app_log_cli.h"
#include "app_assert.h"
#include "sl_bt_api.h"
#include "host_comm.h"
#include "gatt_db.h"
#include "ble_peer_manager_common.h"
#include "ble_peer_manager_connections.h"
#include "ble_peer_manager_central.h"
#include "ble_peer_manager_peripheral.h"
#include "ble_peer_manager_filter.h"
#include "cs_reflector.h"
#include "cs_reflector_config.h"
#include "cs_acp.h"
#include "cs_initiator_client.h"
#include "cs_antenna.h"
#include "extended_result.h"

// MAX_CONNECTIONS should be less or equal to the SL_BT_CONFIG_MAX_CONNECTIONS in the CS NCP example.
// Sum of reflector and initiator instances should not exceed MAX_CONNECTIONS.
#define MAX_CONNECTIONS                             4u
// MAX_INITIATOR_INSTANCES should be less or equal to the CS_INITIATOR_MAX_CONNECTIONS in the CS NCP example.
#define MAX_INITIATOR_INSTANCES                     MAX_CONNECTIONS
// MAX_REFLECTOR_INSTANCES should be less or equal to the CS_REFLECTOR_MAX_CONNECTIONS in the CS NCP example.
#define MAX_REFLECTOR_INSTANCES                     MAX_CONNECTIONS

// Extended result format, used by tooling.
#ifndef EXTENDED_RESULT
#define EXTENDED_RESULT 0
#endif

// Prefixes
#define APP_PREFIX                  "[APP] "
#define INSTANCE_PREFIX             "[%u] "
#define APP_INSTANCE_PREFIX         APP_PREFIX INSTANCE_PREFIX

// Connection TX power settings
#define CS_INITIATOR_MIN_TX_POWER_DBM               -3
#define CS_INITIATOR_MAX_TX_POWER_DBM               20

#define CREATE_REFLECTOR_MSG_LEN (sizeof(cs_acp_cmd_id_t) + sizeof(cs_acp_create_reflector_cmd_data_t))
#define REFLECTOR_ACTION_MSG_LEN (sizeof(cs_acp_cmd_id_t) + sizeof(cs_acp_reflector_action_cmd_data_t))

#define CREATE_INITATOR_MSG_LEN (sizeof(cs_acp_cmd_id_t) + sizeof(cs_acp_create_initiator_cmd_data_t))
#define INITATOR_ACTION_MSG_LEN (sizeof(cs_acp_cmd_id_t) + sizeof(cs_acp_initiator_action_cmd_data_t))

static uuid_128 ras_service_uuid = {
  .data = { 0xf3, 0x20, 0x18, 0xc7, 0x32, 0x2d, 0xc7, 0xab, 0xcf, 0x46, 0xf7, 0xff, 0x70, 0x9e, 0xb9, 0xbb }
};

// Optstring argument for getopt
#define OPTSTRING NCP_HOST_OPTSTRING APP_LOG_OPTSTRING "m:R:I:F:wo:p:a:q:TPh"

// Usage info
#define USAGE APP_LOG_NL "%s " NCP_HOST_USAGE APP_LOG_USAGE                                                      \
  "\n[-m <cs_mode>] [-R <max_reflector_instances>] [-I <max_initiator_instances>] [-F <reflector_ble_address>] " \
  "[-w] [-o] [-p <channel_map_preset>] [-a <cs_tone_antenna_config_idx_req>] [-q <cs_sync_antenna_req>] [-T] [-P] [-h]" APP_LOG_NL

// Detailed argument list
#define CS_HOST_OPTIONS                                                      \
  "    -m  CS mode.\n"                                                       \
  "        <mode>         Integer representing CS mode, default: 2, PBR.\n"  \
  "        1 : RTT\n"                                                        \
  "        2 : PBR\n"                                                        \
  "    -R  Maximum number of reflector instances, default: 1\n"              \
  "        <max_reflector_instances>\n"                                      \
  "    -I  Maximum number of initiator instances, default: 1\n"              \
  "        <max_initiator_instances>\n"                                      \
  "    -F  Enable reflector BLE address filtering in format:\n"              \
  "        AA:BB:CC:DD:EE:FF or aabbccddeeff\n"                              \
  "        <reflector_ble_address>\n"                                        \
  "    -w  Use wired antenna offset\n"                                       \
  "    -o  Object tracking mode, default: 0\n"                               \
  "        0 : moving object tracking (up to 5 km/h) (REAL_TIME_BASIC)\n"    \
  "        1 : stationary object tracking (STATIC_HIGH_ACCURACY)\n"          \
  "    -p  Pre-set parameters for channel map selection, default: 2\n"       \
  "        0 : low (channel spacing: 1, number of channels: 20)\n"           \
  "        1 : medium (channel spacing: 2, number of channels: 38)\n"        \
  "        2 : high (channel spacing: 1, number of channels: 72)\n"          \
  "        3 : load custom from configuration macro CS_CUSTOM_CHANNEL_MAP\n" \
  "    -a  Antenna configuration index for antenna switching, default: 7\n"  \
  "        0 : Single antennas on both sides\n"                              \
  "        1 : Dual antenna initiator & single antenna reflector\n"          \
  "        4 : Single antenna initiator & dual antenna reflector\n"          \
  "        7 : Dual antennas on both sides\n"                                \
  "        Note: considered only with CS mode: PBR!\n"                       \
  "    -q  Antenna usage for CS SYNC packets, default: 0xFE\n"               \
  "        1 : use antenna ID1 only\n"                                       \
  "        2 : use antenna ID2 only\n"                                       \
  "        0xFE : Switching between antennas for each channel\n"             \
  "        Note: considered only with CS mode: RTT!\n"                       \
  "    -T  Enable RTT trace including BGAPI messages and RTL log.\n"         \
  "        Note that the RTT blocks the target if no client is connected.\n" \
  "    -P  Use 1M connection PHY\n"                                          \
  "        Note: Default is 2M"

// Options info
#define OPTIONS    \
  "\nOPTIONS\n"    \
  NCP_HOST_OPTIONS \
  APP_LOG_OPTIONS  \
  CS_HOST_OPTIONS  \
  "    -h  Print this help message.\n"

#define BT_ADDR_LEN 6u
#define BT_MAX_CHANNELS 79u

typedef struct {
  bool use_antenna_wired_offset;
  bd_addr accepted_bt_address_list[MAX_REFLECTOR_INSTANCES];
  uint32_t accepted_bt_address_count;
  uint32_t max_reflector_instances;
  uint32_t max_initiator_instances;
  bool trace;
} cs_host_config_t;

typedef struct {
  uint32_t num_reflector_connections;
  uint32_t num_initiator_connections;
  uint8_t reflector_conn_handles[MAX_REFLECTOR_INSTANCES];
  uint8_t initiator_conn_handles[MAX_INITIATOR_INSTANCES];
  bool trace;
} cs_host_state_t;

cs_host_config_t cs_host_config = {
  .use_antenna_wired_offset = false,
  .accepted_bt_address_count = 0u,
  .max_reflector_instances = 0u,
  .max_initiator_instances = 0u,
  .trace = false
};

cs_host_state_t cs_host_state = {
  .num_reflector_connections = 0u,
  .num_initiator_connections = 0u,
  .reflector_conn_handles = { SL_BT_INVALID_CONNECTION_HANDLE },
  .initiator_conn_handles = { SL_BT_INVALID_CONNECTION_HANDLE },
  .trace = false
};

static rtl_config_t rtl_config = RTL_CONFIG_DEFAULT;
static cs_reflector_config_t reflector_config = REFLECTOR_CONFIG_DEFAULT;
static cs_initiator_config_t initiator_config = INITIATOR_CONFIG_DEFAULT;

static bool initiator_cs_tone_antenna_config_index_set = false;
static bool initiator_cs_sync_antenna_req_set = false;

static char *antenna_usage_to_str(const cs_initiator_config_t *config);
static void enable_trace(void);
static void disable_trace(void);
static void cs_on_result(const cs_acp_result_evt_t *result, uint8_t connection);
static void cs_on_intermediate_result(const cs_acp_intermediate_result_evt_t *intermediate_result,
                                      uint8_t connection);
static void cs_on_error(uint8_t conn_handle,
                        cs_error_event_t err_evt,
                        sl_status_t sc);
static void on_connection_opened_with_initiator(uint8_t conn_handle);
static void on_connection_opened_with_reflector(uint8_t conn_handle);
static void on_connection_closed(uint8_t conn_handle);
static void stop_procedure(void);

/******************************************************************************
 * Application initialization
 *****************************************************************************/
void app_init(int argc, char *argv[])
{
  sl_status_t sc;
  int cli_opt;
  int cs_mode = sl_bt_cs_mode_pbr;
  bool max_initiator_instances_set = false;
  bool max_reflector_instances_set = false;
  unsigned int arg_data;
  int position;

  // Initialize the list of accepted BT addresses
  for (uint32_t i = 0u; i < MAX_REFLECTOR_INSTANCES; i++) {
    memset(cs_host_config.accepted_bt_address_list[i].addr, 0xFF, BT_ADDR_LEN);
  }

  app_log_info("+-[CS Host by Silicon Labs]------------------------+" APP_LOG_NL);
  app_log_info("+--------------------------------------------------+" APP_LOG_NL APP_LOG_NL);

  // Process command line options
  while ((cli_opt = getopt(argc, argv, OPTSTRING)) != -1) {
    switch (cli_opt) {
      // Print help
      case 'h':
        app_log(USAGE, argv[0]);
        app_log(OPTIONS);
        exit(EXIT_SUCCESS);

      case 'm':
        // Mode.
        // 1 - RTT
        // 2 - Phase based
        cs_mode = atoi(optarg);
        break;

      case 'R':
        cs_host_config.max_reflector_instances = atoi(optarg);
        max_reflector_instances_set = true;
        if (cs_host_config.max_reflector_instances > MAX_REFLECTOR_INSTANCES) {
          app_log_error(APP_PREFIX "Invalid number of 'maximum reflectors' argument (%d) provided, "
                                   "must be in the range of 0 to %u" APP_LOG_NL,
                        cs_host_config.max_reflector_instances,
                        MAX_REFLECTOR_INSTANCES);
          exit(EXIT_FAILURE);
        }
        break;

      case 'I':
        cs_host_config.max_initiator_instances = atoi(optarg);
        max_initiator_instances_set = true;
        if (cs_host_config.max_initiator_instances > MAX_INITIATOR_INSTANCES) {
          app_log_error(APP_PREFIX "Invalid number of 'maximum initiators' argument (%d) provided, "
                                   "must be in the range of 0 to %u" APP_LOG_NL,
                        cs_host_config.max_initiator_instances,
                        MAX_INITIATOR_INSTANCES);
          exit(EXIT_FAILURE);
        }
        break;

      case 'F':
      {
        // Check if we have room for more accepted addresses
        if (cs_host_config.accepted_bt_address_count >= MAX_REFLECTOR_INSTANCES) {
          app_log_error(APP_PREFIX "Maximum number of accepted BLE addresses (%u) reached, "
                                   "ignoring additional addresses" APP_LOG_NL,
                        MAX_REFLECTOR_INSTANCES);
          break;
        }
        // Add the accepted BLE address to the list
        bd_addr *bt_addr_to_add = &cs_host_config.accepted_bt_address_list[cs_host_config.accepted_bt_address_count];
        sc = ble_peer_manager_str_to_address(optarg, bt_addr_to_add);
        if (sc != SL_STATUS_OK) {
          app_log_error(APP_PREFIX "Invalid BLE address filter provided" APP_LOG_NL);
          exit(EXIT_FAILURE);
        }
        app_log_info(APP_PREFIX "BLE address accept filter added for: '%02X:%02X:%02X:%02X:%02X:%02X'" APP_LOG_NL,
                     bt_addr_to_add->addr[5],
                     bt_addr_to_add->addr[4],
                     bt_addr_to_add->addr[3],
                     bt_addr_to_add->addr[2],
                     bt_addr_to_add->addr[1],
                     bt_addr_to_add->addr[0]);
        cs_host_config.accepted_bt_address_count++;
      }
      break;

      case 'w':
        cs_host_config.use_antenna_wired_offset = true;
        break;

      case 'P':
        initiator_config.conn_phy = sl_bt_gap_phy_1m;
        break;

      case 'o':
      {
        int object_tracking_mode = atoi(optarg);

        if (object_tracking_mode != SL_RTL_CS_ALGO_MODE_REAL_TIME_BASIC
            && object_tracking_mode != SL_RTL_CS_ALGO_MODE_STATIC_HIGH_ACCURACY) {
          app_log_info(APP_PREFIX "Invalid object tracking mode (%d) provided!" APP_LOG_NL, object_tracking_mode);
          exit(EXIT_FAILURE);
        } else {
          rtl_config.algo_mode = (uint8_t)object_tracking_mode;
        }
      }
      break;

      case 'p':
      {
        int preset = atoi(optarg);
        if (preset != CS_CHANNEL_MAP_PRESET_LOW
            && preset != CS_CHANNEL_MAP_PRESET_MEDIUM
            && preset != CS_CHANNEL_MAP_PRESET_HIGH
            && preset != CS_CHANNEL_MAP_PRESET_CUSTOM) {
          app_log_error(APP_PREFIX "Unsupported preset (%d) provided!" APP_LOG_NL, preset);
          exit(EXIT_FAILURE);
        }
        rtl_config.channel_map_preset = preset;
        app_log_info(APP_PREFIX "Preset (%d) provided!" APP_LOG_NL, preset);
      }
      break;

      case 'a':
      {
        int cs_tone_antenna_config_idx_req = atoi(optarg);
        if (cs_tone_antenna_config_idx_req != CS_ANTENNA_CONFIG_INDEX_SINGLE_ONLY
            && cs_tone_antenna_config_idx_req != CS_ANTENNA_CONFIG_INDEX_DUAL_ONLY
            && cs_tone_antenna_config_idx_req != CS_ANTENNA_CONFIG_INDEX_DUAL_I_SINGLE_R
            && cs_tone_antenna_config_idx_req != CS_ANTENNA_CONFIG_INDEX_SINGLE_I_DUAL_R) {
          app_log_error(APP_PREFIX "Invalid antenna usage for PBR (%d) provided!" APP_LOG_NL, cs_tone_antenna_config_idx_req);
          exit(EXIT_FAILURE);
        } else {
          initiator_config.cs_tone_antenna_config_idx_req = (uint8_t)cs_tone_antenna_config_idx_req;
          initiator_cs_tone_antenna_config_index_set = true;
        }
      }
      break;

      case 'q':
      {
        int items_read = sscanf(optarg, "%x%n", &arg_data, &position);
        if (items_read != 1 || (size_t)position != strlen(optarg)) {
          app_log_error(APP_PREFIX "Invalid antenna usage for RTT (%s) provided!" APP_LOG_NL, optarg);
          exit(EXIT_FAILURE);
        } else {
          if (arg_data != CS_SYNC_ANTENNA_1
              && arg_data != CS_SYNC_ANTENNA_2
              && arg_data != CS_SYNC_SWITCHING) {
            app_log_error(APP_PREFIX "Invalid antenna usage for RTT (%d) provided!" APP_LOG_NL, arg_data);
            exit(EXIT_FAILURE);
          } else {
            initiator_config.cs_sync_antenna_req = (uint8_t)arg_data;
            reflector_config.cs_sync_antenna = (uint8_t)arg_data;
            initiator_cs_sync_antenna_req_set = true;
          }
        }
      }
      break;

      case 'T':
        cs_host_config.trace = true;
        break;

      default:
        sc = ncp_host_set_option((char)cli_opt, optarg);
        if (sc == SL_STATUS_NOT_FOUND) {
          sc = app_log_set_option((char)cli_opt, optarg);
        }
        if (sc != SL_STATUS_OK) {
          app_log(USAGE, argv[0]);
          exit(EXIT_FAILURE);
        }
        break;
    }
  }

  // Check for unknown parameters and write them to to console if there is any
  if (optind < argc) {
    app_log_critical("Unknown %d parameter", (argc - optind));
    if (argc - optind > 1) {
      app_log_append_critical("s \'");
      for (int i = optind; i < argc; i++) {
        app_log_append_critical("%s", argv[i]);
        if (i < argc - 1) {
          app_log_append_critical(", ");
        }
      }
      app_log_append_critical("\'");
    } else {
      app_log_append_critical(" \'%s\'", argv[optind]);
    }
    app_log_append_critical("! Usage:\n");

    app_log(USAGE, argv[0]);
    exit(EXIT_FAILURE);
  }

  // Sanity check of argument combinations
  if ((cs_host_config.max_initiator_instances + cs_host_config.max_reflector_instances) > MAX_CONNECTIONS) {
    app_log_info(APP_PREFIX "Sum of <max_initiator_instances> and <max_reflector_instances> "
                            "exceeds maximum connection count (%u)" APP_LOG_NL,
                 MAX_CONNECTIONS);
    exit(EXIT_FAILURE);
  }

  if (cs_host_config.max_initiator_instances == 0 && cs_host_config.max_reflector_instances == 0) {
    if (!max_initiator_instances_set && !max_reflector_instances_set) {
      cs_host_config.max_initiator_instances = 1;
      cs_host_config.max_reflector_instances = 1;
      app_log_info(APP_PREFIX "Not specified <max_initiator_instances> and "
                              "<max_reflector_instances>. Using 1-1 of each" APP_LOG_NL);
    } else {
      app_log_error(APP_PREFIX "<max_initiator_instances> and <max_reflector_instances>"
                               " cannot be both zero" APP_LOG_NL);
      exit(EXIT_FAILURE);
    }
  }

  if (cs_host_config.max_initiator_instances > 0) {
    if (cs_mode == (int)sl_bt_cs_mode_rtt) {
      initiator_config.cs_mode = sl_bt_cs_mode_rtt;
    } else if (cs_mode == (int)sl_bt_cs_mode_pbr) {
      initiator_config.cs_mode = sl_bt_cs_mode_pbr;
    } else {
      app_log_error(APP_PREFIX "Invalid CS mode argument (%d) provided" APP_LOG_NL, cs_mode);
      exit(EXIT_FAILURE);
    }
  } else {
    app_log_info(APP_PREFIX "Only reflector instances - CS mode will be configured by the initiator" APP_LOG_NL);
    initiator_config.cs_mode = cs_mode;
  }

  if (initiator_cs_tone_antenna_config_index_set && initiator_config.cs_mode == sl_bt_cs_mode_rtt) {
    app_log_warning(APP_PREFIX "PBR antenna configuration is omitted in RTT mode!" APP_LOG_NL);
  }
  if (initiator_cs_sync_antenna_req_set && initiator_config.cs_mode == sl_bt_cs_mode_pbr) {
    app_log_warning(APP_PREFIX "RTT antenna configuration is omitted in PBR mode!" APP_LOG_NL);
  }

  if (initiator_config.cs_mode == sl_bt_cs_mode_rtt && rtl_config.channel_map_preset != CS_CHANNEL_MAP_PRESET_HIGH) {
    app_log_error(APP_PREFIX "Only preset HIGH is supported with RTT mode!" APP_LOG_NL);
    exit(EXIT_FAILURE);
  }

  // Log configuration parameters
  app_log_info("+-------------------------------------------------------+" APP_LOG_NL);
  if (cs_host_config.accepted_bt_address_count > 0) {
    app_log_info(APP_PREFIX "BLE address filtering: enabled with %u addresses" APP_LOG_NL,
                 cs_host_config.accepted_bt_address_count);
  } else {
    app_log_info(APP_PREFIX "BLE address filtering: disabled" APP_LOG_NL);
  }
  app_log_info(APP_PREFIX "Maximum number of reflector instances: %u" APP_LOG_NL,
               cs_host_config.max_reflector_instances);
  app_log_info(APP_PREFIX "Maximum number of initiator instances: %u" APP_LOG_NL,
               cs_host_config.max_initiator_instances);
  app_log_info(APP_PREFIX "CS mode: %s (%u)" APP_LOG_NL,
               (initiator_config.cs_mode  == sl_bt_cs_mode_pbr) ? "PBR" : "RTT",
               (unsigned int)initiator_config.cs_mode);
  if ((initiator_config.cs_mode == sl_bt_cs_mode_pbr && initiator_cs_tone_antenna_config_index_set)
      || (initiator_config.cs_mode == sl_bt_cs_mode_rtt && initiator_cs_sync_antenna_req_set)) {
    app_log_info(APP_PREFIX "Requested antenna usage: %s" APP_LOG_NL,
                 antenna_usage_to_str(&initiator_config));
  } else {
    app_log_info(APP_PREFIX "Default antenna usage: %s" APP_LOG_NL,
                 antenna_usage_to_str(&initiator_config));
  }
  app_log_info(APP_PREFIX "CS channel map preset: %d" APP_LOG_NL, rtl_config.channel_map_preset);
  app_log_info(APP_PREFIX "Object tracking mode: %s" APP_LOG_NL,
               rtl_config.algo_mode == SL_RTL_CS_ALGO_MODE_STATIC_HIGH_ACCURACY
               ? "stationary object tracking"
               : "moving object tracking (up to 5 km/h)");

  app_log_info(APP_PREFIX "RSSI reference TX power @ 1m: %d dBm" APP_LOG_NL,
               (int)initiator_config.rssi_ref_tx_power);
  app_log_info("+-------------------------------------------------------+" APP_LOG_NL);

  // Initialize the list of reflector connection handles
  for (uint32_t i = 0u; i < cs_host_config.max_initiator_instances; i++) {
    cs_host_state.reflector_conn_handles[i] = SL_BT_INVALID_CONNECTION_HANDLE;
  }
  // Initialize the list of initiator connection handles
  for (uint32_t i = 0u; i < cs_host_config.max_reflector_instances; i++) {
    cs_host_state.initiator_conn_handles[i] = SL_BT_INVALID_CONNECTION_HANDLE;
  }

  // Initialize the NCP connection
  sc = ncp_host_init();
  if (sc == SL_STATUS_INVALID_PARAMETER) {
    app_log(USAGE, argv[0]);
    exit(EXIT_FAILURE);
  }
  app_assert_status(sc);
  app_log_info(APP_PREFIX "NCP host initialized" APP_LOG_NL);
  app_log_info(APP_PREFIX "Press Crtl+C to quit" APP_LOG_NL);
  app_log_info("+-------------------------------------------------------+" APP_LOG_NL APP_LOG_NL);
}

void sl_bt_on_event(sl_bt_msg_t *evt)
{
  sl_status_t sc;
  const char *device_name = INITIATOR_DEVICE_NAME;
  switch (SL_BT_MSG_ID(evt->header)) {
    // --------------------------------
    case sl_bt_evt_system_boot_id:
    {
      if (cs_host_config.trace) {
        enable_trace();
      }

      // Set TX power
      int16_t min_tx_power_x10 = CS_INITIATOR_MIN_TX_POWER_DBM * 10;
      int16_t max_tx_power_x10 = CS_INITIATOR_MAX_TX_POWER_DBM * 10;
      sc = sl_bt_system_set_tx_power(min_tx_power_x10,
                                     max_tx_power_x10,
                                     &min_tx_power_x10,
                                     &max_tx_power_x10);
      app_assert_status(sc);
      app_log_info(APP_PREFIX "Minimum system TX power is set to: %d dBm" APP_LOG_NL, min_tx_power_x10 / 10);
      app_log_info(APP_PREFIX "Maximum system TX power is set to: %d dBm" APP_LOG_NL, max_tx_power_x10 / 10);

      // Reset to initial state
      ble_peer_manager_central_init();
      ble_peer_manager_peripheral_init();
      ble_peer_manager_filter_init();
      cs_host_state.num_reflector_connections = 0u;
      cs_host_state.num_initiator_connections = 0u;

      // Initialize the list of reflector connection handles
      for (uint32_t i = 0u; i < cs_host_config.max_initiator_instances; i++) {
        cs_host_state.reflector_conn_handles[i] = SL_BT_INVALID_CONNECTION_HANDLE;
      }
      // Initialize the list of initiator connection handles
      for (uint32_t i = 0u; i < cs_host_config.max_reflector_instances; i++) {
        cs_host_state.initiator_conn_handles[i] = SL_BT_INVALID_CONNECTION_HANDLE;
      }

      // Print the Bluetooth address
      bd_addr address;
      uint8_t address_type;
      sc = sl_bt_gap_get_identity_address(&address, &address_type);
      app_assert_status(sc);
      app_log_info(APP_PREFIX "Bluetooth %s address: %02X:%02X:%02X:%02X:%02X:%02X\n",
                   address_type ? "static random" : "public device",
                   address.addr[5],
                   address.addr[4],
                   address.addr[3],
                   address.addr[2],
                   address.addr[1],
                   address.addr[0]);

      // Filter for advertised name (CS_RFLCT)
      sc = ble_peer_manager_set_filter_device_name(device_name, strlen(device_name), false);
      app_assert_status(sc);

      // Filter for advertised service UUID (RAS)
      sc = ble_peer_manager_set_filter_service_uuid128(&ras_service_uuid);
      app_assert_status(sc);

      // Filter for BLE address if enabled
      if (cs_host_config.accepted_bt_address_count > 0) {
        // Enable Peer Manager BT address filtering
        ble_peer_manager_set_filter_bt_address(true);
        // Add all user specified BT addresses to the allowed list
        for (uint32_t i = 0u; i < cs_host_config.accepted_bt_address_count; i++) {
          sc = ble_peer_manager_add_allowed_bt_address(&cs_host_config.accepted_bt_address_list[i]);
          app_assert_status(sc);
        }
      }

      // Start scanning for reflector connections
      if (cs_host_config.max_initiator_instances > 0 ) {
        sc = ble_peer_manager_central_create_connection();
        app_assert_status(sc);
        app_log_info(APP_PREFIX "Scanning started for reflector connections..." APP_LOG_NL);
      }

      // Start advertising for initiator connections
      if (cs_host_config.max_reflector_instances > 0 ) {
        // Set 2M PHY
        sc = ble_peer_manager_peripheral_set_advertiser_phy(initiator_config.conn_phy);
        app_assert_status(sc);
        sc = ble_peer_manager_peripheral_start_advertising(SL_BT_INVALID_ADVERTISING_SET_HANDLE);
        app_assert_status(sc);
        app_log_info(APP_PREFIX "Advertising started for initiator connections..." APP_LOG_NL);
      }

      // Set antenna configuration
      sc = cs_antenna_configure(cs_host_config.use_antenna_wired_offset);
      app_assert_status(sc);

      break;
    }

    // --------------------------------
    case sl_bt_evt_user_cs_service_message_to_host_id:
    {
      cs_acp_event_t *cs_evt = (cs_acp_event_t *)evt->data.evt_user_cs_service_message_to_host.message.data;
      switch (cs_evt->acp_evt_id) {
        case CS_ACP_EVT_RESULT_ID:
          cs_on_result(&cs_evt->data.result, cs_evt->connection_id);
          break;

        case CS_ACP_EVT_EXTENDED_RESULT_ID:
          on_extended_result_event(&cs_evt->data.ext_result, cs_evt->connection_id);
          if (cs_evt->data.ext_result.fragments_left & CS_ACP_FIRST_FRAGMENT_MASK
              && cs_evt->data.ext_result.fragment.len >= sizeof(cs_acp_result_evt_t)) {
            // The first element in the serialized extended result data is the result event.
            cs_on_result((cs_acp_result_evt_t *)cs_evt->data.ext_result.fragment.data, cs_evt->connection_id);
          }
          break;

        case CS_ACP_EVT_INTERMEDIATE_RESULT_ID:
          cs_on_intermediate_result(&cs_evt->data.intermediate_result, cs_evt->connection_id);
          break;

        case CS_ACP_EVT_STATUS_ID:
          cs_on_error(cs_evt->connection_id, cs_evt->data.stat.error, cs_evt->data.stat.sc);
          break;
        default:
          app_log(APP_PREFIX "Unknown ACP event" APP_LOG_NL);
          break;
      }
      break;
    }
  }
}

/******************************************************************************
 * Application process action
 *****************************************************************************/
void app_process_action(void)
{
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application code here!                              //
  // This is called infinitely.                                              //
  // Do not call blocking functions from here!                               //
  /////////////////////////////////////////////////////////////////////////////
}

/******************************************************************************
 * Application deinit
 *****************************************************************************/
void app_deinit(void)
{
  stop_procedure();
  disable_trace();
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application deinit code here!                       //
  // This is called once during termination.                                 //
  /////////////////////////////////////////////////////////////////////////////
}

static char *antenna_usage_to_str(const cs_initiator_config_t *config)
{
  if (config->cs_mode == sl_bt_cs_mode_rtt) {
    switch (config->cs_sync_antenna_req) {
      case CS_SYNC_ANTENNA_1:
        return "antenna ID 1";
      case CS_SYNC_ANTENNA_2:
        return "antenna ID 2";
      case CS_SYNC_SWITCHING:
        return "switch between all antenna IDs";
      default:
        return "unknown";
    }
  } else {
    switch (config->cs_tone_antenna_config_idx_req) {
      case CS_ANTENNA_CONFIG_INDEX_SINGLE_ONLY:
        return "single antenna on both sides (1:1)";
      case CS_ANTENNA_CONFIG_INDEX_DUAL_I_SINGLE_R:
        return "dual antenna initiator & single antenna reflector (2:1)";
      case CS_ANTENNA_CONFIG_INDEX_SINGLE_I_DUAL_R:
        return "single antenna initiator & dual antenna reflector (1:2)";
      case CS_ANTENNA_CONFIG_INDEX_DUAL_ONLY:
        return "dual antennas on both sides (2:2)";
      default:
        return "unknown";
    }
  }
}

static void enable_trace(void)
{
  if (cs_host_state.trace) {
    return;
  }
  sl_status_t sc;
  cs_acp_cmd_t cmd;
  cmd.cmd_id = CS_ACP_CMD_ENABLE_TRACE;
  cmd.data.enable_trace = 1;
  const size_t cmd_len = sizeof(cmd.cmd_id) + sizeof(cmd.data.enable_trace);

  sc = sl_bt_user_cs_service_message_to_target(cmd_len,
                                               (const uint8_t *)&cmd,
                                               0,
                                               NULL,
                                               NULL);
  if (sc == SL_STATUS_OK) {
    cs_host_state.trace = true;
  } else {
    app_log_status_error_f(sc, APP_PREFIX "Failed to enable trace" APP_LOG_NL);
  }
}

static void disable_trace(void)
{
  if (!cs_host_state.trace) {
    return;
  }
  sl_status_t sc;
  cs_acp_cmd_t cmd;
  cmd.cmd_id = CS_ACP_CMD_ENABLE_TRACE;
  cmd.data.enable_trace = 0;
  const size_t cmd_len = sizeof(cmd.cmd_id) + sizeof(cmd.data.enable_trace);

  sc = sl_bt_user_cs_service_message_to_target(cmd_len,
                                               (const uint8_t *)&cmd,
                                               0,
                                               NULL,
                                               NULL);
  if (sc == SL_STATUS_OK) {
    cs_host_state.trace = false;
  } else {
    app_log_status_error_f(sc, APP_PREFIX "Failed to disable trace" APP_LOG_NL);
  }
}

static void on_connection_opened_with_reflector(uint8_t conn_handle)
{
  sl_status_t sc;

  // Check if we can accept one more reflector connection
  if (cs_host_state.num_reflector_connections >= cs_host_config.max_initiator_instances) {
    app_log_error(APP_PREFIX "Maximum number of initiator instances (%u) reached, "
                             "dropping connection..." APP_LOG_NL,
                  cs_host_config.max_initiator_instances);
    sc = sl_bt_connection_close(conn_handle);
    app_assert_status(sc);
    return;
  }

  // Store the new reflector connection handle
  bool could_store = false;
  for (uint32_t i = 0u; i < cs_host_config.max_initiator_instances; i++) {
    if (cs_host_state.reflector_conn_handles[i] == SL_BT_INVALID_CONNECTION_HANDLE) {
      cs_host_state.reflector_conn_handles[i] = conn_handle;
      cs_host_state.num_reflector_connections++;
      could_store = true;
      break;
    }
  }
  if (!could_store) {
    app_log_error(APP_PREFIX "Failed to store reflector connection handle" APP_LOG_NL);
    return;
  }

  // Call the ACP API to create a new initiator instance for the connection handle
  cs_acp_cmd_t cmd;
  cmd.cmd_id = CS_ACP_CMD_CREATE_INITIATOR;
  cmd.data.initiator_cmd_data.connection_id = conn_handle;
  cmd.data.initiator_cmd_data.extended_result = EXTENDED_RESULT;
  memcpy(&cmd.data.initiator_cmd_data.initiator_config, &initiator_config, sizeof(cmd.data.initiator_cmd_data.initiator_config));
  memcpy(&cmd.data.initiator_cmd_data.rtl_config, &rtl_config, sizeof(cmd.data.initiator_cmd_data.rtl_config));
  cs_initiator_apply_channel_map_preset(cmd.data.initiator_cmd_data.rtl_config.channel_map_preset, cmd.data.initiator_cmd_data.rtl_config.cs_parameters.channel_map);
  uint8_t rsp[1];
  size_t rsp_len = 0;
  uint8_t instance_id = 0;

  sc = sl_bt_user_cs_service_message_to_target(CREATE_INITATOR_MSG_LEN,
                                               (const uint8_t *)&cmd,
                                               sizeof(rsp),
                                               &rsp_len,
                                               rsp);
  if (sc != SL_STATUS_OK) {
    app_log_status_error_f(sc, APP_PREFIX "Failed to create initiator instance" APP_LOG_NL);
    return;
  }
  if (rsp_len > 0) {
    instance_id = rsp[0];
  }

  app_log_info(APP_INSTANCE_PREFIX "New initiator instance created with RTL lib instance %u" APP_LOG_NL, conn_handle, instance_id);

  app_log_info(APP_PREFIX "Initiator instances in use: %u/%u" APP_LOG_NL,
               cs_host_state.num_reflector_connections,
               cs_host_config.max_initiator_instances);
  // Scan for new reflector connections if we have room for more
  if (cs_host_state.num_reflector_connections < cs_host_config.max_initiator_instances) {
    sl_status_t sc = ble_peer_manager_central_create_connection();
    app_assert_status(sc);
    app_log_info(APP_PREFIX "Scanning restarted for new reflector connections..." APP_LOG_NL);
  }
}

static void on_connection_opened_with_initiator(uint8_t conn_handle)
{
  // Check if we can accept one more initiator connection
  if (cs_host_state.num_initiator_connections >= cs_host_config.max_reflector_instances) {
    app_log_error(APP_PREFIX "Maximum number of reflector instances (%u) reached, "
                             "dropping connection..." APP_LOG_NL,
                  cs_host_config.max_reflector_instances);
    sl_status_t sc = sl_bt_connection_close(conn_handle);
    app_assert_status(sc);
    return;
  }

  // Store the new initiator connection handle
  for (uint32_t i = 0u; i < cs_host_config.max_reflector_instances; i++) {
    if (cs_host_state.initiator_conn_handles[i] == SL_BT_INVALID_CONNECTION_HANDLE) {
      cs_host_state.initiator_conn_handles[i] = conn_handle;
      cs_host_state.num_initiator_connections++;
      break;
    }
  }
  // Call the ACP API to create a new reflector instance for the connection handle
  sl_status_t sc;
  cs_acp_cmd_t cmd;

  cmd.cmd_id = CS_ACP_CMD_CREATE_REFLECTOR;
  cmd.data.reflector_cmd_data.connection_id = conn_handle;
  memcpy(&cmd.data.reflector_cmd_data.reflector_config, &reflector_config, sizeof(cmd.data.reflector_cmd_data.reflector_config));
  sc = sl_bt_user_cs_service_message_to_target(CREATE_REFLECTOR_MSG_LEN,
                                               (const uint8_t *)&cmd,
                                               0,
                                               NULL,
                                               NULL);
  app_assert_status(sc);
  app_log_info(APP_INSTANCE_PREFIX "New reflector instance created" APP_LOG_NL, conn_handle);

  app_log_info(APP_PREFIX "Reflector instances in use: %u/%u" APP_LOG_NL,
               cs_host_state.num_initiator_connections,
               cs_host_config.max_reflector_instances);

  // Advertise for new initiator connections if we have room for more
  if (cs_host_state.num_initiator_connections < cs_host_config.max_reflector_instances) {
    sl_status_t sc = ble_peer_manager_peripheral_start_advertising(SL_BT_INVALID_ADVERTISING_SET_HANDLE);
    app_assert_status(sc);
    app_log_info(APP_PREFIX "Advertising restarted for new initiator connections..." APP_LOG_NL);
  }
}

static void on_connection_closed(uint8_t conn_handle)
{
  // Check if the connection handle is a reflector connection - if yes remove the corresponding initiator instance
  for (uint32_t i = 0u; i < cs_host_config.max_initiator_instances; i++) {
    if (cs_host_state.reflector_conn_handles[i] == conn_handle) {
      cs_host_state.reflector_conn_handles[i] = SL_BT_INVALID_CONNECTION_HANDLE;
      app_log_info(APP_INSTANCE_PREFIX "Reflector connection closed" APP_LOG_NL, conn_handle);

      // Call the ACP API to remove the initiator instance for the connection handle
      cs_acp_cmd_t cmd;
      sl_status_t sc;

      cmd.cmd_id = CS_ACP_CMD_INITIATOR_ACTION;
      cmd.data.initiator_action_data.connection_id = conn_handle;
      cmd.data.initiator_action_data.initiator_action = CS_ACP_ACTION_DELETE_INITIATOR;
      sc = sl_bt_user_cs_service_message_to_target(INITATOR_ACTION_MSG_LEN,
                                                   (const uint8_t *)&cmd,
                                                   0,
                                                   NULL,
                                                   NULL);
      app_assert_status(sc);
      app_log_info(APP_INSTANCE_PREFIX "Initiator instance removed" APP_LOG_NL, conn_handle);

      // Restart scanning for new reflector connections
      (void)ble_peer_manager_central_create_connection();
      app_log_info(APP_PREFIX "Scanning restarted for new reflector connections..." APP_LOG_NL);

      cs_host_state.num_reflector_connections--;
      app_log_info(APP_PREFIX "Initiator instances in use: %u/%u" APP_LOG_NL,
                   cs_host_state.num_reflector_connections,
                   cs_host_config.max_initiator_instances);
      break;
    }
  }

  // Check if the connection handle is an initiator connection - if yes remove the corresponding reflector instance
  for (uint32_t i = 0u; i < cs_host_config.max_reflector_instances; i++) {
    if (cs_host_state.initiator_conn_handles[i] == conn_handle) {
      cs_host_state.initiator_conn_handles[i] = SL_BT_INVALID_CONNECTION_HANDLE;
      app_log_info(APP_INSTANCE_PREFIX "Initiator connection closed" APP_LOG_NL, conn_handle);

      // Call the ACP API to remove the reflector instance for the connection handle
      sl_status_t sc;
      cs_acp_cmd_t cmd;

      cmd.cmd_id = CS_ACP_CMD_REFLECTOR_ACTION;
      cmd.data.reflector_action_data.connection_id = conn_handle;
      cmd.data.reflector_action_data.reflector_action = CS_ACP_ACTION_DELETE_REFLECTOR;
      sc = sl_bt_user_cs_service_message_to_target(REFLECTOR_ACTION_MSG_LEN,
                                                   (const uint8_t *)&cmd,
                                                   0,
                                                   NULL,
                                                   NULL);
      app_assert_status(sc);
      app_log_info(APP_INSTANCE_PREFIX "Reflector instance removed" APP_LOG_NL, conn_handle);

      // Restart advertising for new initiator connections if we were at the limit
      if (cs_host_state.num_initiator_connections == cs_host_config.max_reflector_instances) {
        sl_status_t sc = ble_peer_manager_peripheral_start_advertising(SL_BT_INVALID_ADVERTISING_SET_HANDLE);
        app_assert_status(sc);
        app_log_info(APP_PREFIX "Advertising restarted for new initiator connections..." APP_LOG_NL);
      }

      cs_host_state.num_initiator_connections--;
      app_log_info(APP_PREFIX "Reflector instances in use: %u/%u" APP_LOG_NL,
                   cs_host_state.num_initiator_connections,
                   cs_host_config.max_reflector_instances);
      break;
    }
  }
}

void ble_peer_manager_on_event(ble_peer_manager_evt_type_t *event)
{
  const bd_addr *address;
  switch (event->evt_id) {
    case BLE_PEER_MANAGER_ON_CONN_OPENED_CENTRAL:
      address = ble_peer_manager_get_bt_address(event->connection_id);
      app_log_info(APP_INSTANCE_PREFIX "Connection opened as central with CS Reflector"
                                       " '%02X:%02X:%02X:%02X:%02X:%02X'" APP_LOG_NL,
                   event->connection_id,
                   address->addr[5],
                   address->addr[4],
                   address->addr[3],
                   address->addr[2],
                   address->addr[1],
                   address->addr[0]);
      on_connection_opened_with_reflector(event->connection_id);
      break;

    case BLE_PEER_MANAGER_ON_CONN_OPENED_PERIPHERAL:
      address = ble_peer_manager_get_bt_address(event->connection_id);
      app_log_info(APP_INSTANCE_PREFIX "Connection opened as peripheral with CS Initiator"
                                       " '%02X:%02X:%02X:%02X:%02X:%02X'" APP_LOG_NL,
                   event->connection_id,
                   address->addr[5],
                   address->addr[4],
                   address->addr[3],
                   address->addr[2],
                   address->addr[1],
                   address->addr[0]);
      on_connection_opened_with_initiator(event->connection_id);
      break;

    case BLE_PEER_MANAGER_ON_CONN_CLOSED:
      app_log_info(APP_INSTANCE_PREFIX "Connection closed" APP_LOG_NL, event->connection_id);
      on_connection_closed(event->connection_id);
      break;

    case BLE_PEER_MANAGER_ON_ADV_STOPPED:
      app_log_info(APP_INSTANCE_PREFIX "Advertisement stopped" APP_LOG_NL, event->connection_id);
      break;

    case BLE_PEER_MANAGER_ERROR:
      app_log_error(APP_INSTANCE_PREFIX "Peer Manager error" APP_LOG_NL, event->connection_id);
      break;

    default:
      app_log_info(APP_INSTANCE_PREFIX "Unhandled Peer Manager event (%u)" APP_LOG_NL, event->connection_id, event->evt_id);
      break;
  }
}

/******************************************************************************
 * Extract and display measurement results
 *****************************************************************************/
static void cs_on_result(const cs_acp_result_evt_t *result, uint8_t connection)
{
  app_log_info("---" APP_LOG_NL);
  app_log_info(APP_INSTANCE_PREFIX "Measurement result: %u mm" APP_LOG_NL,
               connection,
               (uint32_t)(result->distance * 1000.f));
  app_log_info(APP_INSTANCE_PREFIX "Measurement likeliness: %f" APP_LOG_NL,
               connection,
               result->likeliness);
  app_log_info(APP_INSTANCE_PREFIX "RSSI distance: %u mm" APP_LOG_NL,
               connection,
               (uint32_t)(result->rssi_distance * 1000.f));
  if (!isnan(result->bit_error_rate)) {
    app_log_info(APP_INSTANCE_PREFIX "CS bit error rate: %f" APP_LOG_NL,
                 connection,
                 result->bit_error_rate);
  }
}

/******************************************************************************
 * Extract and display intermediate results between measurement results
 * Note: only called when stationary object tracking used
 *****************************************************************************/
static void cs_on_intermediate_result(const cs_acp_intermediate_result_evt_t *intermediate_result,
                                      uint8_t connection)
{
  app_log_info("---" APP_LOG_NL);
  uint32_t percentage_whole = (uint32_t)intermediate_result->progress_percentage;
  uint32_t percentage_frac =
    (uint32_t)((intermediate_result->progress_percentage - (float)percentage_whole) * 100.0f);

  app_log_info(APP_INSTANCE_PREFIX "Estimation in progress: %lu.%02lu%%" APP_LOG_NL,
               connection,
               (unsigned long)percentage_whole,
               (unsigned long)percentage_frac);
}

/******************************************************************************
 * CS error handler
 *****************************************************************************/
static void cs_on_error(uint8_t conn_handle, cs_error_event_t err_evt, sl_status_t sc)
{
  switch (err_evt) {
    // Assert
    case CS_ERROR_EVENT_CS_PROCEDURE_STOP_TIMER_FAILED:
    case CS_ERROR_EVENT_CS_PROCEDURE_UNEXPECTED_DATA:
      app_assert(false,
                 APP_INSTANCE_PREFIX "Unrecoverable CS procedure error happened!"
                                     "[E: 0x%x sc: 0x%x]" APP_LOG_NL,
                 conn_handle,
                 err_evt,
                 sc);
      break;
    case CS_ERROR_EVENT_INITIATOR_FAILED_TO_GET_CHANNEL_MAP:
      app_assert(false,
                 APP_INSTANCE_PREFIX "Failed to create channel map!"
                                     "Input parameters might be wrong!"
                                     "[E: 0x%x sc: 0x%x]" APP_LOG_NL,
                 conn_handle,
                 err_evt,
                 sc);
      break;
    // Discard
    case CS_ERROR_EVENT_RTL_PROCESS_ERROR:
      app_log_error(APP_INSTANCE_PREFIX "RTL processing error happened!"
                                        "[E: 0x%x sc: 0x%x]" APP_LOG_NL,
                    conn_handle,
                    err_evt,
                    sc);
      break;
    // PBR antenna usage not supported
    case CS_ERROR_EVENT_INITIATOR_PBR_ANTENNA_USAGE_NOT_SUPPORTED:
      if (initiator_cs_tone_antenna_config_index_set) {
        app_log_error(APP_INSTANCE_PREFIX "The requested PBR antenna configuration is not supported!"
                                          " Will use the closest one and continue."
                                          "[E: 0x%x sc: 0x%x]" APP_LOG_NL,
                      conn_handle,
                      err_evt,
                      sc);
      } else {
        app_log_info(APP_INSTANCE_PREFIX "Default PBR antenna configuration not supported!"
                                         " Will use the closest one and continue."
                                         "[E: 0x%x sc: 0x%x]" APP_LOG_NL,
                     conn_handle,
                     err_evt,
                     sc);
      }
      break;
    // RTT antenna usage not supported
    case CS_ERROR_EVENT_INITIATOR_RTT_ANTENNA_USAGE_NOT_SUPPORTED:
      if (initiator_cs_sync_antenna_req_set) {
        app_log_error(APP_INSTANCE_PREFIX "The requested RTT antenna configuration is not supported!"
                                          " Will use the closest one and continue."
                                          "[E: 0x%x sc: 0x%x]" APP_LOG_NL,
                      conn_handle,
                      err_evt,
                      sc);
      } else {
        app_log_info(APP_INSTANCE_PREFIX "Default RTT antenna configuration not supported!"
                                         " Will use the closest one and continue."
                                         "[E: 0x%x sc: 0x%x]" APP_LOG_NL,
                     conn_handle,
                     err_evt,
                     sc);
      }
      break;

    // Close connection
    default:
      app_log_error(APP_INSTANCE_PREFIX "Error happened! Closing connection"
                                        "[E: 0x%x sc: 0x%x]" APP_LOG_NL,
                    conn_handle,
                    err_evt,
                    sc);
      // Common errors
      if (err_evt == CS_ERROR_EVENT_TIMER_ELAPSED) {
        app_log_error(APP_INSTANCE_PREFIX "Operation timeout." APP_LOG_NL, conn_handle);
      } else if (err_evt == CS_ERROR_EVENT_INITIATOR_FAILED_TO_INCREASE_SECURITY) {
        app_log_error(APP_INSTANCE_PREFIX "Security level increase failed." APP_LOG_NL, conn_handle);
      }
      // Close the connection
      (void)ble_peer_manager_central_close_connection(conn_handle);
      break;
  }
}

static void stop_procedure(void)
{
  cs_acp_cmd_t cmd;
  sl_status_t sc;
  uint8_t conn_handle;

  // Close all reflector connections, and delete initiator instances on the NCP
  for (uint32_t i = 0u; i < cs_host_config.max_initiator_instances; i++) {
    conn_handle = cs_host_state.reflector_conn_handles[i];
    if (conn_handle != SL_BT_INVALID_CONNECTION_HANDLE) {
      app_log_info(APP_PREFIX "Removing Initiator instance; connection_handle='%u'" APP_LOG_NL, conn_handle);
      cs_host_state.reflector_conn_handles[i] = SL_BT_INVALID_CONNECTION_HANDLE;

      // Call the ACP API to remove the initiator instance for the connection handle
      cmd.cmd_id = CS_ACP_CMD_INITIATOR_ACTION;
      cmd.data.initiator_action_data.connection_id = conn_handle;
      cmd.data.initiator_action_data.initiator_action = CS_ACP_ACTION_DELETE_INITIATOR;
      sc = sl_bt_user_cs_service_message_to_target(INITATOR_ACTION_MSG_LEN,
                                                   (const uint8_t *)&cmd,
                                                   0,
                                                   NULL,
                                                   NULL);
      app_assert_status(sc);
      app_log_info(APP_INSTANCE_PREFIX "Initiator instance removed" APP_LOG_NL, conn_handle);

      sc = ble_peer_manager_central_close_connection(conn_handle);
      app_assert_status(sc);
      app_log_info(APP_INSTANCE_PREFIX "Reflector connection closed" APP_LOG_NL, conn_handle);

      cs_host_state.num_reflector_connections--;
      app_log_info(APP_PREFIX "Initiator instances in use: %u/%u" APP_LOG_NL,
                   cs_host_state.num_reflector_connections,
                   cs_host_config.max_initiator_instances);
    }
  }

  // Close all initiator connections, and delete reflector instances on the NCP
  for (uint32_t i = 0u; i < cs_host_config.max_reflector_instances; i++) {
    conn_handle = cs_host_state.initiator_conn_handles[i];
    if (conn_handle != SL_BT_INVALID_CONNECTION_HANDLE) {
      cs_host_state.initiator_conn_handles[i] = SL_BT_INVALID_CONNECTION_HANDLE;

      // Call the ACP API to remove the initiator instance for the connection handle
      cmd.cmd_id = CS_ACP_CMD_REFLECTOR_ACTION;
      cmd.data.reflector_action_data.connection_id = conn_handle;
      cmd.data.reflector_action_data.reflector_action = CS_ACP_ACTION_DELETE_REFLECTOR;
      sc = sl_bt_user_cs_service_message_to_target(REFLECTOR_ACTION_MSG_LEN,
                                                   (const uint8_t *)&cmd,
                                                   0,
                                                   NULL,
                                                   NULL);
      app_assert_status(sc);
      app_log_info(APP_INSTANCE_PREFIX "Reflector instance removed" APP_LOG_NL, conn_handle);

      sc = ble_peer_manager_peripheral_close_connection(conn_handle);
      app_assert_status(sc);
      app_log_info(APP_INSTANCE_PREFIX "Initiator connection closed" APP_LOG_NL, conn_handle);

      cs_host_state.num_initiator_connections--;
      app_log_info(APP_PREFIX "Reflector instances in use: %u/%u" APP_LOG_NL,
                   cs_host_state.num_initiator_connections,
                   cs_host_config.max_reflector_instances);
    }
  }
}

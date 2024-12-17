/***************************************************************************//**
 * @file
 * @brief CS Reflector
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

#include "sl_bt_api.h"
#include "app_assert.h"
#include "app_log.h"
#include "cs_ras.h"
#include "cs_reflector.h"
#include "cs_reflector_config.h"
#include "cs_reflector_connmanager.h"
#include "cs_reflector_event_buf.h"
#include "sl_component_catalog.h"
#ifdef SL_CATALOG_APP_LOG_PRESENT
#include "app_log.h"
#endif // SL_CATALOG_APP_LOG_PRESENT

// -----------------------------------------------------------------------------
// Macros

// Component logging
#if defined(SL_CATALOG_APP_LOG_PRESENT) && CS_REFLECTOR_LOG
#define LOG_PREFIX                  CS_REFLECTOR_LOG_PREFIX " "
#define NL                          APP_LOG_NL
#define INSTANCE_PREFIX             "[%u] "
#define reflector_log_debug(...)    app_log_debug(LOG_PREFIX  __VA_ARGS__)
#define reflector_log_info(...)     app_log_info(LOG_PREFIX  __VA_ARGS__)
#define reflector_log_warning(...)  app_log_warning(LOG_PREFIX  __VA_ARGS__)
#define reflector_log_error(...)    app_log_error(LOG_PREFIX  __VA_ARGS__)
#define reflector_log_critical(...) app_log_critical(LOG_PREFIX  __VA_ARGS__)
#else
#define NL
#define INSTANCE_PREFIX
#define reflector_log_debug(...)
#define reflector_log_info(...)
#define reflector_log_warning(...)
#define reflector_log_error(...)
#define reflector_log_critical(...)
#endif

// -----------------------------------------------------------------------------
// Enums, structs, typedefs

SL_ENUM(cs_result_fragment_t) {
  cs_result_consecutive_fragment = 0,
  cs_result_first_fragment
};

// -----------------------------------------------------------------------------
// Static function declarations

static void cs_reflector_gattdb_init(void);
static bool cs_reflector_send_next_available(const uint8_t conn_handle);
static void cs_indicate_ranging_data_ready(const uint8_t conn_handle);
static void cs_reflector_find_and_send_cs_result(const uint8_t conn_handle,
                                                 const uint16_t procedure_index,
                                                 const uint16_t subevent_index);

// -----------------------------------------------------------------------------
// Static variables

static uint16_t gattdb_ras_control_point_chr_handle;
static uint16_t gattdb_ras_subevent_ranging_data_chr_handle;

// -----------------------------------------------------------------------------
// Private function definitions

// Finds the next available CS result and sends it to the initiator
static bool cs_reflector_send_next_available(const uint8_t conn_handle)
{
  cs_event_data_t *cs_event_buf_entry = cs_reflector_event_buf_get_next_element(conn_handle);
  if (cs_event_buf_entry == NULL) {
    reflector_log_warning(INSTANCE_PREFIX "No results left to transfer for this connection" NL,
                          conn_handle);
    return false;
  }

  reflector_log_debug(INSTANCE_PREFIX "Transferring next CS result to the initiator; index='%u' subevent_index='%u' len='%u'" NL,
                      conn_handle,
                      cs_event_buf_entry->procedure_index,
                      cs_event_buf_entry->subevent_index,
                      cs_event_buf_entry->data_len);

  reflector_log_debug(INSTANCE_PREFIX "CS packet details; proc_cnt='%u' proc_done='%u' se_done='%u' len='%u'" NL,
                      conn_handle,
                      cs_event_buf_entry->procedure_index,
                      cs_event_buf_entry->procedure_done,
                      cs_event_buf_entry->subevent_done,
                      cs_event_buf_entry->data_len);
  sl_status_t sc = sl_bt_gatt_server_send_notification(conn_handle,
                                                       gattdb_ras_subevent_ranging_data_chr_handle,
                                                       cs_event_buf_entry->data_len,
                                                       cs_event_buf_entry->data);
  // Give warning that we couldn't transfer the data
  // Data will be discarded anyways
  if (sc != SL_STATUS_OK) {
    reflector_log_warning(INSTANCE_PREFIX "Cannot transfer CS result, discarding; idx='%u' sc='0x%02lx'" NL,
                          conn_handle,
                          cs_event_buf_entry->procedure_index,
                          sc);
  }
  // Mark the event buffer element as consumed
  cs_event_buf_entry->empty = true;
  return true;
}

// Finds and sends the requested CS result (if available) to the initiator
static void cs_reflector_find_and_send_cs_result(const uint8_t conn_handle,
                                                 const uint16_t procedure_index,
                                                 const uint16_t subevent_index)
{
  cs_event_data_t *cs_event_buf_entry = cs_reflector_event_buf_find(conn_handle,
                                                                    procedure_index,
                                                                    subevent_index);
  if (cs_event_buf_entry == NULL) {
    reflector_log_warning(INSTANCE_PREFIX "Requested CS result cannot be found; index='%u' subevent_index='%u'" NL,
                          conn_handle, procedure_index, subevent_index);
    return;
  }

  reflector_log_debug(INSTANCE_PREFIX "Transferring CS result to the initiator; index='%u' subevent_index='%u'" NL,
                      conn_handle,
                      procedure_index,
                      subevent_index);
  sl_status_t sc = sl_bt_gatt_server_send_notification(conn_handle,
                                                       gattdb_ras_subevent_ranging_data_chr_handle,
                                                       cs_event_buf_entry->data_len,
                                                       cs_event_buf_entry->data);
  app_assert_status(sc);
  // Mark the event buffer element as consumed
  cs_event_buf_entry->empty = true;
}

static void cs_reflector_gattdb_init(void)
{
  // Look up the 'RAS control point' and the 'RAS subevent ranging data' characteristic handles from the GATT database
  sl_status_t sc;
  static const uint8_t ras_control_point_uuid[] = RAS_CONTROL_POINT_CHARACTERISTIC_UUID;
  sc =  sl_bt_gatt_server_find_attribute(0u,
                                         sizeof(ras_control_point_uuid),
                                         ras_control_point_uuid,
                                         &gattdb_ras_control_point_chr_handle);
  app_assert_status(sc);

  static const uint8_t ras_subevent_ranging_data_uuid[] = RAS_SE_RANGING_DATA_CHARACTERISTIC_UUID;
  sc =  sl_bt_gatt_server_find_attribute(0u,
                                         sizeof(ras_subevent_ranging_data_uuid),
                                         ras_subevent_ranging_data_uuid,
                                         &gattdb_ras_subevent_ranging_data_chr_handle);
  app_assert_status(sc);
}

// -----------------------------------------------------------------------------
// Public function definitions

bool cs_reflector_identify(uint8_t conn_handle)
{
  connection_ctx_t *conn_ctx = cs_rcm_get_connection_ctx(conn_handle);
  if (conn_ctx == NULL) {
    return false;
  }
  return true;
}

sl_status_t cs_reflector_create(uint8_t               conn_handle,
                                cs_reflector_config_t *reflector_config)
{
  sl_status_t sc;
  uint8_t num_antennas;
  reflector_log_info(INSTANCE_PREFIX "Creating new reflector instance; conn_handle='%u'" NL,
                     conn_handle,
                     conn_handle);

  // Prepare for the CS mode: RTT antenna usage
  sc = sl_bt_cs_read_local_supported_capabilities(NULL,
                                                  NULL,
                                                  &num_antennas,
                                                  NULL,
                                                  NULL,
                                                  NULL,
                                                  NULL,
                                                  NULL,
                                                  NULL,
                                                  NULL,
                                                  NULL,
                                                  NULL,
                                                  NULL,
                                                  NULL,
                                                  NULL,
                                                  NULL,
                                                  NULL,
                                                  NULL);

  if (sc != SL_STATUS_OK) {
    return sc;
  }

  switch (reflector_config->cs_sync_antenna) {
    case CS_SYNC_ANTENNA_1:
      reflector_log_info(INSTANCE_PREFIX "CS - RTT - Using the antenna ID 1" NL,
                         conn_handle);
      break;
    case CS_SYNC_ANTENNA_2:
      if (num_antennas >= 2) {
        reflector_log_info(INSTANCE_PREFIX "CS - RTT - 2-antenna device! Using the antenna ID 2" NL,
                           conn_handle);
      } else {
        reflector_log_info(INSTANCE_PREFIX "CS - RTT - only 1-antenna device! Using the antenna ID 1" NL,
                           conn_handle);
        reflector_config->cs_sync_antenna = CS_SYNC_ANTENNA_1;
      }
      break;
    case CS_SYNC_SWITCHING:
      reflector_log_info(INSTANCE_PREFIX "CS - RTT - switching between %u available antennas" NL,
                         conn_handle,
                         num_antennas);
      reflector_config->cs_sync_antenna = CS_SYNC_SWITCHING;
      break;
    default:
      reflector_log_info(INSTANCE_PREFIX "CS - RTT - unknown antenna usage! "
                                         "Using the default setting: antenna ID 1" NL,
                         conn_handle);
      reflector_config->cs_sync_antenna = CS_SYNC_ANTENNA_1;
      break;
  }

  sc = sl_bt_cs_set_default_settings(conn_handle,
                                     sl_bt_cs_role_status_disable,
                                     sl_bt_cs_role_status_enable,
                                     reflector_config->cs_sync_antenna,
                                     reflector_config->max_tx_power_dbm);
  if (sc != SL_STATUS_OK) {
    return sc;
  }

  // Add the new instance
  sc = cs_rcm_add_new_initiator_connection(conn_handle);
  if (sc != SL_STATUS_OK) {
    // New instance cannot be added - we're at capacity
    reflector_log_warning(INSTANCE_PREFIX "Cannot add new reflector connection" NL,
                          conn_handle);
    return SL_STATUS_ALLOCATION_FAILED;
  }

  cs_reflector_gattdb_init();

  reflector_log_info("Instances: %u/%u" NL,
                     cs_rcm_get_number_of_connections(),
                     CS_REFLECTOR_MAX_CONNECTIONS);
  return sc;
}

uint8_t cs_reflector_get_active_instance_count(void)
{
  return cs_rcm_get_number_of_connections();
}

sl_status_t cs_reflector_delete(uint8_t conn_handle)
{
  if (!cs_reflector_identify(conn_handle)) {
    return SL_STATUS_NOT_FOUND;
  }
  reflector_log_info(INSTANCE_PREFIX "Deleting instance" NL, conn_handle);
  cs_rcm_remove_initiator_connection(conn_handle);
  cs_reflector_event_buf_purge_data(conn_handle);
  reflector_log_info("Instances: %u/%u" NL,
                     cs_rcm_get_number_of_connections(),
                     CS_REFLECTOR_MAX_CONNECTIONS);
  return SL_STATUS_OK;
}

// Looks for the first result we haven't sent an indication about and sends it
void cs_indicate_ranging_data_ready(const uint8_t conn_handle)
{
  connection_ctx_t *conn_ctx = cs_rcm_get_connection_ctx(conn_handle);
  if (conn_ctx == NULL) {
    reflector_log_warning(INSTANCE_PREFIX "Couldn't find registered connection for handle '%u'" NL,
                          conn_handle,
                          conn_handle);
    return;
  }
  // If the RAS Control Point notifications/indications are enabled and not busy,
  // and the RAS Periodic Notification was not requested
  if (!conn_ctx->ras_control_point_indication_enabled
      || conn_ctx->indication_in_progress
      || conn_ctx->ras_periodic_notification_mode_enabled) {
    return;
  }

  // Get the next result we haven't indicated about
  cs_event_data_t *current_event = cs_reflector_event_buf_get_next_unindicated(conn_handle);
  // Return if there's no result we haven't indicated about
  if (current_event == NULL) {
    return;
  }

  // Prepare the indication data
  ras_ranging_data_ready_indication_t cmd;
  cmd.opcode = CS_RAS_OPCODE_RANGING_DATA_READY_INDICATION;
  cmd.procedure_index = current_event->procedure_index;
  cmd.number_of_subevents = 1u;
  cmd.subevent_index = current_event->subevent_index;
  cmd.subevent_index_data_size = current_event->data_len;
  reflector_log_info(INSTANCE_PREFIX "Sending RAS Ranging Data Ready indication; "
                                     "index='%u' num_se='%u' se_idx='%u' size='%u'" NL,
                     conn_handle,
                     cmd.procedure_index,
                     cmd.number_of_subevents,
                     cmd.subevent_index,
                     cmd.subevent_index_data_size);

  // Send the notification
  sl_status_t sc = sl_bt_gatt_server_send_indication(conn_handle,
                                                     gattdb_ras_control_point_chr_handle,
                                                     sizeof(ras_ranging_data_ready_indication_t),
                                                     (uint8_t*)&cmd);
  if (sc != SL_STATUS_OK) {
    reflector_log_warning(INSTANCE_PREFIX "Failed to send RAS Ranging Data Ready indication" NL,
                          conn_handle);
  } else {
    current_event->indication_sent = true;
    conn_ctx->indication_in_progress = true;
  }
}

// -----------------------------------------------------------------------------
// Event / callback definitions

bool cs_reflector_on_bt_event(sl_bt_msg_t *evt)
{
  bool handled = false;
  sl_status_t sc;
  (void)sc;

  switch (SL_BT_MSG_ID(evt->header)) {
    // -------------------------------
    // This event indicates that the BT stack has booted successfully
    case sl_bt_evt_system_boot_id:
    {
      cs_reflector_event_buf_clear();
    }
    break;

    // -------------------------------
    // This event indicates that a CS procedure was started by the initiator
    case sl_bt_evt_cs_procedure_enable_complete_id:
    {
      uint8_t conn_handle = evt->data.evt_cs_procedure_enable_complete.connection;
      if (!cs_reflector_identify(conn_handle)) {
        break;
      }
      // Set message handled
      handled = true;
      reflector_log_debug("-------------------------------------------------------" NL);
      reflector_log_debug(INSTANCE_PREFIX "CS procedure started" NL, conn_handle);
    }
    break;

    // -------------------------------
    // This event indicates the first fragment of Channel Sounding Result data is available
    case sl_bt_evt_cs_result_id:
    {
      uint8_t conn_handle = evt->data.evt_cs_result.connection;
      connection_ctx_t *conn_ctx = cs_rcm_get_connection_ctx(conn_handle);
      if (conn_ctx == NULL) {
        break;
      }
      // Set message handled
      handled = true;

      #ifdef SL_CATALOG_BLUETOOTH_FEATURE_CS_TEST_PRESENT
      handled = false;
      #endif //SL_CATALOG_BLUETOOTH_FEATURE_CS_TEST_PRESENT

      uint16_t current_procedure_index = conn_ctx->current_procedure_index;
      uint16_t current_cs_subevent_index = conn_ctx->current_cs_subevent_index;
      uint16_t received_procedure_index = evt->data.evt_cs_result.procedure_counter;

      // Count the real procedure index (stack returns 0 for consecutive subevents)
      // If the current index is not zero and we receive a zero that means
      // that it's a consecutive event
      if (current_procedure_index != 0 && received_procedure_index == 0) {
        received_procedure_index = current_procedure_index;
      } else {
        current_procedure_index = received_procedure_index;
      }

      reflector_log_debug(INSTANCE_PREFIX "CS procedure #%u first CS result available "
                                          "[subevent_idx:%u, procedure_done_sts: 0x%02x, subevent_done_sts:0x%02x]" NL,
                          conn_handle,
                          current_procedure_index,
                          current_cs_subevent_index,
                          evt->data.evt_cs_result.procedure_done_status,
                          evt->data.evt_cs_result.subevent_done_status);

      // Get a free space in the event buffer we can write
      cs_event_data_t *cs_event_buf_entry = cs_reflector_event_buf_get_element_for_write();
      if (cs_event_buf_entry == NULL) {
        reflector_log_error("CS event buffer overflown! No more space for new events!" NL);
        break;
      }

      // Get the size of the CS result type enumerator + CS result data
      size_t result_len = sizeof(evt->data.evt_cs_result)
                          + evt->data.evt_cs_result.data.len
                          + sizeof(cs_result_fragment_t);
      // Check if we can copy the whole result data
      app_assert((result_len <= CS_EVENT_BUF_DATA_MAX_LEN),
                 "Event data exceeds the event buffer size, possible data loss, aborting; "
                 "result_len='%u' cs_event_buf_data_len='%u'",
                 result_len, CS_EVENT_BUF_DATA_MAX_LEN);

      cs_event_buf_entry->data[0] = (uint8_t)cs_result_first_fragment;
      // Copy the result to the event buffer element
      memcpy(&cs_event_buf_entry->data[1], (uint8_t *)&evt->data.evt_cs_result, result_len - 1);
      // Fill the rest of the results
      cs_event_buf_entry->data_len = result_len;
      cs_event_buf_entry->procedure_index = current_procedure_index;
      cs_event_buf_entry->subevent_index = current_cs_subevent_index;
      cs_event_buf_entry->empty = false;
      cs_event_buf_entry->indication_sent = false;
      cs_event_buf_entry->conn_handle = evt->data.evt_cs_result.connection;
      cs_event_buf_entry->procedure_done = evt->data.evt_cs_result.procedure_done_status;
      cs_event_buf_entry->subevent_done = evt->data.evt_cs_result.subevent_done_status;

      reflector_log_debug("CS event buf: %lu/%u" NL,
                          CS_REFLECTOR_CS_EVENT_BUF_SIZE - cs_reflector_event_buf_get_free_space(),
                          CS_REFLECTOR_CS_EVENT_BUF_SIZE);

      current_cs_subevent_index = 0u;
      conn_ctx->current_procedure_index = current_procedure_index;
      conn_ctx->current_cs_subevent_index = current_cs_subevent_index;

      cs_indicate_ranging_data_ready(conn_ctx->conn_handle);

      // If the periodic RAS Periodic Notification mode is enabled send the results immediately
      if (conn_ctx->ras_periodic_notification_mode_enabled) {
        cs_reflector_send_next_available(conn_ctx->conn_handle);
      }

      sl_bt_cs_done_status_t cs_procedure_status =
        (sl_bt_cs_done_status_t)evt->data.evt_cs_result.procedure_done_status;
      if (cs_procedure_status == sl_bt_cs_done_status_complete) {
        reflector_log_debug(INSTANCE_PREFIX "CS procedure #%u finished" NL,
                            conn_handle,
                            current_procedure_index);
      } else if (cs_procedure_status == sl_bt_cs_done_status_partial_results_continue) {
        reflector_log_debug(INSTANCE_PREFIX "CS procedure #%u ongoing, waiting for further results..." NL,
                            conn_handle,
                            current_procedure_index);
      } else {
        reflector_log_error(INSTANCE_PREFIX "CS procedure #%u aborted! [E:0x%02x, abort_reason:0x%02x]" NL,
                            conn_handle,
                            current_procedure_index,
                            cs_procedure_status,
                            evt->data.evt_cs_result.abort_reason);
      }
    }
    break;

    // -------------------------------
    // This event indicates that a consecutive fragment for Channel Sounding Result is available
    case sl_bt_evt_cs_result_continue_id:
    {
      uint8_t conn_handle = evt->data.evt_cs_result_continue.connection;
      connection_ctx_t *conn_ctx = cs_rcm_get_connection_ctx(conn_handle);
      if (conn_ctx == NULL) {
        break;
      }
      // Set message handled
      handled = true;
      #ifdef SL_CATALOG_BLUETOOTH_FEATURE_CS_TEST_PRESENT
      handled = false;
      #endif //SL_CATALOG_BLUETOOTH_FEATURE_CS_TEST_PRESENT

      uint16_t current_procedure_index = conn_ctx->current_procedure_index;
      uint16_t current_cs_subevent_index = conn_ctx->current_cs_subevent_index;

      reflector_log_debug(INSTANCE_PREFIX "CS procedure #%u consecutive CS result arrived"
                                          "[subevent #%u, procedure_done_sts: 0x%02x, subevent_done_sts: 0x%02x]" NL,
                          conn_handle,
                          current_procedure_index,
                          current_cs_subevent_index,
                          evt->data.evt_cs_result_continue.procedure_done_status,
                          evt->data.evt_cs_result_continue.subevent_done_status);

      // Get a free space in the event buffer we can write
      cs_event_data_t *cs_event_buf_entry = cs_reflector_event_buf_get_element_for_write();
      if (cs_event_buf_entry == NULL) {
        reflector_log_error("CS event buffer overflown! No more space for new events!" NL);
        break;
      }

      // Get the size of the CS result type enumerator + CS result data
      size_t result_len = sizeof(evt->data.evt_cs_result_continue)
                          + evt->data.evt_cs_result_continue.data.len
                          + sizeof(cs_result_fragment_t);
      // Check if we can copy the whole result data
      app_assert((result_len < CS_EVENT_BUF_DATA_MAX_LEN),
                 "Event data exceeds the event buffer size, possible data loss, aborting; "
                 "result_len='%u' cs_event_buf_data_len='%u'",
                 result_len, CS_EVENT_BUF_DATA_MAX_LEN);

      cs_event_buf_entry->data[0] = (uint8_t)cs_result_consecutive_fragment;
      // Copy the result to the event buffer element
      memcpy(&cs_event_buf_entry->data[1], (uint8_t *)&evt->data.evt_cs_result_continue, result_len - 1);
      // Fill the rest of the results
      cs_event_buf_entry->data_len = (uint8_t)result_len;
      cs_event_buf_entry->procedure_index = current_procedure_index;
      cs_event_buf_entry->subevent_index = current_cs_subevent_index;
      cs_event_buf_entry->empty = false;
      cs_event_buf_entry->indication_sent = false;
      cs_event_buf_entry->conn_handle = evt->data.evt_cs_result_continue.connection;
      cs_event_buf_entry->procedure_done = evt->data.evt_cs_result_continue.procedure_done_status;
      cs_event_buf_entry->subevent_done = evt->data.evt_cs_result_continue.subevent_done_status;

      reflector_log_debug("CS event buf: %lu/%u" NL,
                          CS_REFLECTOR_CS_EVENT_BUF_SIZE - cs_reflector_event_buf_get_free_space(),
                          CS_REFLECTOR_CS_EVENT_BUF_SIZE);

      current_cs_subevent_index++;
      conn_ctx->current_cs_subevent_index = current_cs_subevent_index;

      cs_indicate_ranging_data_ready(conn_ctx->conn_handle);

      // If the periodic RAS Periodic Notification mode is enabled send the results immediately
      if (conn_ctx->ras_periodic_notification_mode_enabled) {
        cs_reflector_send_next_available(conn_ctx->conn_handle);
      }

      sl_bt_cs_done_status_t cs_procedure_status =
        (sl_bt_cs_done_status_t)evt->data.evt_cs_result_continue.procedure_done_status;
      if (cs_procedure_status == sl_bt_cs_done_status_complete) {
        reflector_log_debug(INSTANCE_PREFIX "CS procedure #%u finished" NL,
                            conn_handle,
                            current_procedure_index);
      } else if (cs_procedure_status == sl_bt_cs_done_status_partial_results_continue) {
        reflector_log_debug(INSTANCE_PREFIX "CS procedure #%u ongoing, waiting for further results..." NL,
                            conn_handle,
                            current_procedure_index);
      } else {
        reflector_log_error(INSTANCE_PREFIX "CS procedure #%u aborted!"
                                            "[E:0x%02x, abort_reason:0x%02x]" NL,
                            conn_handle,
                            current_procedure_index,
                            cs_procedure_status,
                            evt->data.evt_cs_result_continue.abort_reason);
      }
    }
    break;

    // -------------------------------
    // This event indicates that a value was changed in the local GATT by a remote device
    case sl_bt_evt_gatt_server_attribute_value_id:
    {
      uint8_t conn_handle = evt->data.evt_gatt_characteristic_value.connection;

      connection_ctx_t *conn_ctx = cs_rcm_get_connection_ctx(conn_handle);
      if (conn_ctx == NULL) {
        break;
      }
      // Set message handled
      handled = true;
      // If the RAS Control Point was written
      if (evt->data.evt_gatt_characteristic_value.characteristic == gattdb_ras_control_point_chr_handle) {
        reflector_log_info(INSTANCE_PREFIX "Received new RAS Control Point message" NL,
                           conn_handle);
        // Parse the incoming RAS message
        // The result - if valid - will tell the opcode and whether the
        // periodic notification mode was enabled / disabled
        ras_control_point_parse_result ras_result = cs_ras_parse_control_point_message(
          evt->data.evt_gatt_characteristic_value.value.data,
          evt->data.evt_gatt_characteristic_value.value.len);

        ras_opcode_t ras_opcode = ras_result.opcode;
        conn_ctx->ras_periodic_notification_mode_enabled = ras_result.periodic_notification_mode_set_enabled;

        // Initiator requesting a certain measurement
        // The initiator can optionally turn on the 'Periodic Notification Mode' while issuing this command
        if (ras_opcode == CS_RAS_OPCODE_RANGING_DATA_GET_COMMAND) {
          ras_ranging_data_get_command_t rx_cmd =
            *((ras_ranging_data_get_command_t*)(evt->data.evt_gatt_characteristic_value.value.data));
          cs_reflector_find_and_send_cs_result(conn_handle, rx_cmd.procedure_index, rx_cmd.subevent_index);

          // If the RAS Periodic Notification was enabled
          if (conn_ctx->ras_periodic_notification_mode_enabled) {
            reflector_log_info(INSTANCE_PREFIX "RAS Periodic Notification mode enabled" NL,
                               conn_handle);
            // Send out all the results we have
            while (cs_reflector_send_next_available(conn_handle)) ;
          } else {
            reflector_log_info(INSTANCE_PREFIX "RAS Periodic Notification mode disabled" NL,
                               conn_handle);
          }
        }
      }
    }
    break;

    // -------------------------------
    // This event is received when a GATT characteristic status changes
    case sl_bt_evt_gatt_server_characteristic_status_id:
    {
      uint8_t conn_handle = evt->data.evt_gatt_characteristic_value.connection;
      if (!cs_reflector_identify(conn_handle)) {
        break;
      }
      // Check for the RAS control point characteristic status changes
      if (evt->data.evt_gatt_server_characteristic_status.characteristic == gattdb_ras_control_point_chr_handle) {
        connection_ctx_t *conn_ctx = cs_rcm_get_connection_ctx(conn_handle);
        if (conn_ctx == NULL) {
          reflector_log_warning(INSTANCE_PREFIX "Couldn't find registered connection for handle '%u'" NL,
                                conn_handle,
                                conn_handle);
          break;
        }
        // Set message handled
        handled = true;
        if ((evt->data.evt_gatt_server_characteristic_status.client_config_flags == sl_bt_gatt_server_indication)
            && (evt->data.evt_gatt_server_characteristic_status.status_flags == sl_bt_gatt_server_client_config)) {
          reflector_log_info(INSTANCE_PREFIX "RAS control point indication enabled" NL, conn_handle);
          conn_ctx->ras_control_point_indication_enabled = true;
        } else if ((evt->data.evt_gatt_server_characteristic_status.client_config_flags == sl_bt_gatt_server_disable)
                   && (evt->data.evt_gatt_server_characteristic_status.status_flags == sl_bt_gatt_server_client_config)) {
          reflector_log_info(INSTANCE_PREFIX "RAS control point indication disabled" NL, conn_handle);
          conn_ctx->ras_control_point_indication_enabled = false;
        } else if (evt->data.evt_gatt_server_characteristic_status.status_flags == sl_bt_gatt_server_confirmation) {
          reflector_log_info(INSTANCE_PREFIX "RAS control point confirmation received" NL, conn_handle);
          conn_ctx->indication_in_progress = false;
          cs_indicate_ranging_data_ready(conn_handle);
        }
      }
    }
    break;

    // -------------------------------
    // This event indicates that the Channel Sounding Security Enable procedure has completed
    case sl_bt_evt_cs_security_enable_complete_id:
    {
      uint8_t conn_handle = evt->data.evt_cs_security_enable_complete.connection;
      if (!cs_reflector_identify(conn_handle)) {
        break;
      }
      // Set message handled
      handled = true;
      reflector_log_info(INSTANCE_PREFIX "CS Security Enable completed" NL, conn_handle);
    }
    break;

    // -------------------------------
    // This event indicates that the Channel Sounding configuration was completed
    case sl_bt_evt_cs_config_complete_id:
    {
      uint8_t conn_handle = evt->data.evt_cs_config_complete.connection;
      if (!cs_reflector_identify(conn_handle)) {
        break;
      }
      // Set message handled
      handled = true;
      reflector_log_info(INSTANCE_PREFIX "CS configuration completed" NL, conn_handle);
    }
    break;
  }
  // Return false if the message was handled above
  return !handled;
}

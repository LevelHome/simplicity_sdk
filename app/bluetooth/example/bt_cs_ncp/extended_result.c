/***************************************************************************//**
 * @file
 * @brief Extended result event serializer.
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

#include <stdint.h>
#include <string.h>
#include "sl_status.h"
#include "sl_common.h"
#include "sl_power_manager.h"
#include "sl_bt_api.h"
#include "app_log.h"
#include "cs_acp.h"
#include "cs_initiator_config.h"
#include "extended_result.h"

// This is not an exact calculation, but a good enough approximation that can
// safely store the procedure data coming from the CS initiator component.
// The factor of 2 stands for initiator + reflector.
#define EVT_DATA_BUFFER_MAX_SIZE (                                             \
    sizeof(cs_acp_result_evt_t)                                                \
    + sizeof(sl_rtl_cs_config)                                                 \
    + sizeof(sl_rtl_cs_subevent_data) * CS_INITIATOR_MAX_SUBEVENT_PER_PROC * 2 \
    + CS_INITIATOR_MAX_STEP_DATA_LEN * 2                                       \
    )

#define EVT_OVERHEAD             (sizeof(cs_acp_event_id_t) + 3)
#define EVT_MAX_DATA             (UINT8_MAX - EVT_OVERHEAD)

static uint8_t evt_data_buffer[EVT_DATA_BUFFER_MAX_SIZE];
static size_t evt_data_buffer_len = 0;
static uint8_t *evt_data_ptr;
static uint8_t connection;

static sl_status_t serialize_extended_result(const cs_result_t *result,
                                             const sl_rtl_cs_procedure *cs_procedure,
                                             size_t max_data_size,
                                             size_t *data_len,
                                             uint8_t *data);

void cs_on_extended_result(const cs_result_t *result,
                           const sl_rtl_cs_procedure *cs_procedure,
                           const void *user_data)
{
  sl_status_t sc;
  size_t data_len;

  (void)user_data;

  if (evt_data_buffer_len > 0) {
    app_log_error("Event data buffer busy" APP_LOG_NL);
    return;
  }

  sc = serialize_extended_result(result,
                                 cs_procedure,
                                 sizeof(evt_data_buffer),
                                 &data_len,
                                 evt_data_buffer);
  if (sc != SL_STATUS_OK) {
    app_log_status_error_f(sc, "Event data serialization failed" APP_LOG_NL);
    return;
  }

  connection = result->connection;
  evt_data_buffer_len = data_len;
  evt_data_ptr = evt_data_buffer;
  // Keep the MCU awake until all fragments are sent.
  sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM1);
}

void extended_result_step(void)
{
  if (evt_data_buffer_len == 0) {
    return;
  }

  // Avoid big buffer allocation on the stack by using static variable.
  static uint8_t evt_data[UINT8_MAX];
  uint8_t evt_data_len = (uint8_t)SL_MIN(evt_data_buffer_len, (size_t)EVT_MAX_DATA);
  cs_acp_event_t *evt = (cs_acp_event_t *)evt_data;
  evt->connection_id = connection;
  evt->acp_evt_id = CS_ACP_EVT_EXTENDED_RESULT_ID;
  memcpy(evt->data.ext_result.fragment.data, evt_data_ptr, evt_data_len);
  evt->data.ext_result.fragment.len = evt_data_len;
  evt_data_buffer_len -= evt_data_len;
  // Calculate remaining number of fragments using ceiling division.
  evt->data.ext_result.fragments_left = (uint8_t)((evt_data_buffer_len + EVT_MAX_DATA - 1) / EVT_MAX_DATA);
  if (evt_data_ptr == evt_data_buffer) {
    // First fragment
    evt->data.ext_result.fragments_left |= CS_ACP_FIRST_FRAGMENT_MASK;
  }
  evt_data_ptr += evt_data_len;
  sl_bt_send_evt_user_cs_service_message_to_host(evt_data_len + EVT_OVERHEAD, evt_data);

  if (evt_data_buffer_len == 0) {
    sl_power_manager_remove_em_requirement(SL_POWER_MANAGER_EM1);
  }
}

static sl_status_t serialize_extended_result(const cs_result_t *result,
                                             const sl_rtl_cs_procedure *cs_procedure,
                                             size_t max_data_size,
                                             size_t *data_len,
                                             uint8_t *data)
{
  // Size of the subevent data structure without the step data
  const size_t subevent_data_size = sizeof(sl_rtl_cs_subevent_data) - sizeof(uint8_t*);
  // Calculate serialized size of the CS result
  *data_len = sizeof(cs_acp_result_evt_t);
  // Calculate serialized size of the CS procedure
  *data_len += sizeof(cs_procedure->cs_config);
  *data_len += sizeof(cs_procedure->initiator_subevent_data_count);
  for (uint8_t i = 0; i < cs_procedure->initiator_subevent_data_count; i++) {
    *data_len += subevent_data_size;
    *data_len += cs_procedure->initiator_subevent_data[i].step_data_count;
  }
  *data_len += sizeof(cs_procedure->reflector_subevent_data_count);
  for (uint8_t i = 0; i < cs_procedure->reflector_subevent_data_count; i++) {
    *data_len += subevent_data_size;
    *data_len += cs_procedure->reflector_subevent_data[i].step_data_count;
  }
  if (*data_len > max_data_size) {
    return SL_STATUS_WOULD_OVERFLOW;
  }
  // Serialize result
  cs_acp_result_evt_t *result_evt = (cs_acp_result_evt_t *)data;
  result_evt->distance = result->distance;
  result_evt->rssi_distance = result->rssi_distance;
  result_evt->likeliness = result->likeliness;
  result_evt->bit_error_rate = result->bit_error_rate;
  data += sizeof(cs_acp_result_evt_t);
  // Serialize procedure data
  memcpy(data, &cs_procedure->cs_config, sizeof(cs_procedure->cs_config));
  data += sizeof(cs_procedure->cs_config);
  memcpy(data, &cs_procedure->initiator_subevent_data_count, sizeof(cs_procedure->initiator_subevent_data_count));
  data += sizeof(cs_procedure->initiator_subevent_data_count);
  for (uint8_t i = 0; i < cs_procedure->initiator_subevent_data_count; i++) {
    memcpy(data, &cs_procedure->initiator_subevent_data[i], subevent_data_size);
    data += subevent_data_size;
    memcpy(data, cs_procedure->initiator_subevent_data[i].step_data, cs_procedure->initiator_subevent_data[i].step_data_count);
    data += cs_procedure->initiator_subevent_data[i].step_data_count;
  }
  memcpy(data, &cs_procedure->reflector_subevent_data_count, sizeof(cs_procedure->reflector_subevent_data_count));
  data += sizeof(cs_procedure->reflector_subevent_data_count);
  for (uint8_t i = 0; i < cs_procedure->reflector_subevent_data_count; i++) {
    memcpy(data, &cs_procedure->reflector_subevent_data[i], subevent_data_size);
    data += subevent_data_size;
    memcpy(data, cs_procedure->reflector_subevent_data[i].step_data, cs_procedure->reflector_subevent_data[i].step_data_count);
    data += cs_procedure->reflector_subevent_data[i].step_data_count;
  }
  return SL_STATUS_OK;
}

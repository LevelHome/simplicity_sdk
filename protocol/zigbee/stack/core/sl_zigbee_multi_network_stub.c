/***************************************************************************//**
 * @file
 * @brief Code for multi-network support.
 *******************************************************************************
 * # License
 * <b>Copyright 2018 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/

#ifdef PLATFORM_HEADER
  #include PLATFORM_HEADER
#endif // PLATFORM_HEADER
#include "core/sl_zigbee_multi_network.h"
#include "hal/hal.h"
#include "upper-mac.h"
#include "stack/include/sl_zigbee_types_internal.h" // for PAN_ID_OKAY and NETWORK_INITIAL
#include "stack/internal/inc/internal-callbacks-patch.h"
#include "stack/internal/inc/internal-defs-patch.h"

extern sl_mac_tx_options_bitmask_t sli_802154mac_prepare_tx_handler(sli_zigbee_packet_header_t packet,
                                                                    uint8_t *flat_packet_buffer,
                                                                    uint8_t mac_payload_offset,
                                                                    uint8_t mac_index,
                                                                    uint8_t nwk_index,
                                                                    int8_t *tx_power);
extern bool sli_802154mac_passthrough_handler(uint8_t* macHeader, uint8_t macPayloadLength);
extern void sli_802154mac_packet_send_complete_callback(uint8_t mac_index, sl_status_t status, sli_zigbee_packet_header_t packet, uint8_t tag);
extern bool sli_zigbee_make_stack_jit_message(void);
extern bool sli_zigbee_packet_handoff_incoming_callback(sli_buffer_manager_buffer_t rawPacket, uint8_t index, void *data);
extern bool sli_zigbee_process_network_header(sli_zigbee_packet_header_t macHeaderOnly,
                                              const uint8_t* networkHeader);
extern void sli_zigbee_note_successful_poll_received(uint8_t childIndex, uint8_t nwk_index);
extern uint8_t sli_zigbee_get_phy_interface_by_node_id(sl_802154_short_addr_t nodeId);

const sl_zigbee_library_status_t sli_zigbee_multi_network_library_status = SL_ZIGBEE_LIBRARY_IS_STUB;

// A few internal targets without stripped need these dependencies brought in
#ifndef SL_ZIGBEE_MULTI_NETWORK_STRIPPED
extern sl_zigbee_neighbor_table_entry_info_t sli_zigbee_neighbor_data[];
extern uint8_t sli_zigbee_router_neighbor_table_size;
extern uint32_t sli_zigbee_frame_counters_table[];
#include "stack/core/multi-pan.h"
#endif // SL_ZIGBEE_MULTI_NETWORK_STRIPPED

//------------------------------------------------------------------------------
// SL_ZIGBEE_MULTI_NETWORK_STRIPPED is an optional #define
// that is used to conditionally compile out multi-network related source code.
// For flash-space constrained chips, we provide more flash savings by declaring
// a variable for each field in the sli_zigbee_network_info_t struct so that we avoid using
// the (->) operator everywhere in the code.
//------------------------------------------------------------------------------
#if defined(SL_ZIGBEE_MULTI_NETWORK_STRIPPED)
uint8_t sli_zigbee_node_type = SL_ZIGBEE_UNKNOWN_DEVICE;
uint8_t sli_zigbee_state = NETWORK_INITIAL;
uint8_t sli_zigbee_dynamic_capabilities = 0;
uint32_t sli_zigbee_security_state_bitmask = 0;
uint8_t sli_zigbee_sequence_number = 0;
uint8_t sli_zigbee_aps_sequence_number = 0;
uint8_t sli_zigbee_network_security_level = 0;
uint16_t sli_zigbee_unicast_tx_attempts = 0;
uint16_t sli_zigbee_unicast_tx_failures = 0;
uint8_t sli_zigbee_security_key_sequence_number = 0;
uint32_t sli_zigbee_incoming_tc_link_key_frame_counter = 0;
uint32_t sli_zigbee_last_child_age_time_ms = 0;
uint32_t sli_zigbee_ms_since_last_poll = 0;
uint32_t sli_zigbee_next_nwk_frame_counter = 0;
uint8_t sli_zigbee_neighbor_count = 0;
uint8_t sli_zigbee_end_device_child_count = 0;
uint16_t parentNwkInformation = 0;
uint32_t sli_zigbee_broadcast_age_cutoff_indices;
uint8_t sli_zigbee_broadcast_head = 0;
// This is zero if the time since bootup is longer than the broadcast
// table timeout.
uint8_t sli_zigbee_in_initial_broadcast_timeout;
uint8_t sli_zigbee_fast_link_status_count = 0;
bool sli_zigbee_permit_joining = false;
bool sli_802154mac_permit_association = false;
// A bool of whether we allow trust center (insecure) rejoins for devices
// using the well-known link key. Sending the network key encrypted with the
// well-known key is a security hole, so we ideally want to reject the TC
// rejoin. Allowing it provides backwards compatibility with R20 (HA) devices
// Setting this value to true is done through a setter, which arms a timer that,
// when fired, sets the variable back to false
// This variable corresponds to the allowRejoins attribute in the Zigbee spec
bool sli_zigbee_allow_rejoins_with_well_known_key = false;
uint8_t sli_zigbee_parent_announce_index = 0;
// The number of children for the parent announce messages can change if children
// get deleted between consecutive parent Announce messages.
uint8_t sli_zigbee_total_initial_children = 0;
sl_802154_pan_id_t sli_zigbee_new_pan_id = PAN_ID_OKAY;
extern sl_mac_child_entry_t sli_zigbee_child_table_data[];
extern uint16_t sli_zigbee_child_status_data[];
#else
uint8_t sli_zigbee_current_network_index = 0;
#endif // defined(SL_ZIGBEE_MULTI_NETWORK_STRIPPED)

//------------------------------------------------------------------------------
// Network descriptor init function.
//------------------------------------------------------------------------------
#if defined(SL_ZIGBEE_MULTI_NETWORK_STRIPPED)
void sli_zigbee_network_descriptor_init(void)
{
  memset(&sli_zigbee_parent_eui64, 0, sizeof(sl_802154_long_addr_t));
  sli_zigbee_sequence_number = sli_zigbee_stack_get_pseudo_random_number() & 0x00FF;
  sli_zigbee_aps_sequence_number = sli_zigbee_stack_get_pseudo_random_number() & 0x00FF;
  sli_zigbee_network_security_level = sli_zigbee_default_security_level;
}
#else
void sli_zigbee_network_descriptor_init(void)
{
  // Initialize the current network pointers to point to the 0-index network.
  sli_zigbee_current_network_index = 0;

  memset(&sli_zigbee_current_network, 0, sizeof(sl_zigbee_network_info_t));
  sli_zigbee_current_network.stackProfile = sli_zigbee_default_stack_profile;
  //sli_zigbee_current_network.parentId = SL_ZIGBEE_NULL_NODE_ID;
  sli_zigbee_current_network.nodeType = SL_ZIGBEE_UNKNOWN_DEVICE;
  sli_zigbee_current_network.zigbeeState = NETWORK_INITIAL;
  //sli_zigbee_current_network.radioChannel = SL_ZIGBEE_MIN_802_15_4_CHANNEL_NUMBER; // default to 802.15.4 ch 11
  //sli_zigbee_current_network.radioPower = MAX_RADIO_POWER;
  //sli_zigbee_current_network.localNodeId = EM_USE_LONG_ADDRESS;
  //sli_zigbee_current_network.localPanId = EM_BROADCAST_PAN_ID;
  //sli_zigbee_current_network.macDataSequenceNumber = sli_zigbee_stack_get_pseudo_random_number() & 0x00FF;
  sli_zigbee_current_network.zigbeeSequenceNumber = sli_zigbee_stack_get_pseudo_random_number() & 0x00FF;
  sli_zigbee_current_network.apsSequenceNumber = sli_zigbee_stack_get_pseudo_random_number() & 0x00FF;
  sli_zigbee_current_network.zigbeeNetworkSecurityLevel = sli_zigbee_default_security_level;
  sli_zigbee_current_network.neighborTable = sli_zigbee_neighbor_data;
  sli_zigbee_current_network.neighborTableSize = sli_zigbee_router_neighbor_table_size;
  sli_zigbee_current_network.frameCounters = sli_zigbee_frame_counters_table;
  sli_zigbee_current_network.panInfoData = &(sl_zigbee_pan_info_table[sli_zigbee_current_network_index]);

  sli_zigbee_network_pan_info_data_init(sli_zigbee_current_network_index, &sli_zigbee_current_network);
  sli_mac_init_child_table_pointers(sli_zigbee_current_network_index, sli_zigbee_child_table, sli_zigbee_child_status);
}
#endif // defined(SL_ZIGBEE_MULTI_NETWORK_STRIPPED)

sl_status_t sli_zigbee_multinetwork_radio_init(void)
{
  sl_status_t status = SL_STATUS_OK;
  sl_mac_radio_parameters_t radio_params;

#ifdef MAC_DUAL_PRESENT
  status = sli_mac_get_nwk_radio_parameters(PHY_INDEX_NATIVE, 0, &radio_params);
  if (status != SL_STATUS_OK) {
    return SL_STATUS_FAIL;
  }

  // This function only sets the following fields. Other fields like channel
  // and tx_power are set by other functions.
  radio_params.prepare_tx_callback = sli_802154mac_prepare_tx_handler;
  radio_params.passthrough_filter_callback = sli_802154mac_passthrough_handler;
  radio_params.poll_handler_callback = sli_zigbee_stack_poll_handler;
  radio_params.indirect_tx_complete_callback = sli_802154mac_packet_send_complete_callback;
  radio_params.make_jit_message_callback = sli_zigbee_make_stack_jit_message;
  radio_params.packet_handoff_incoming_callback = sli_zigbee_packet_handoff_incoming_callback;
  radio_params.process_network_header_callback = sli_zigbee_process_network_header;
  radio_params.poll_tx_complete_callback = sli_802154mac_packet_send_complete_callback;
  radio_params.poll_rx_callback = sli_zigbee_note_successful_poll_received;

  status = sli_mac_set_nwk_radio_parameters(PHY_INDEX_NATIVE, 0, &radio_params);
  if (status != SL_STATUS_OK) {
#ifdef SL_ZIGBEE_TEST
    assert(0);
#endif // SL_ZIGBEE_TEST
    return SL_STATUS_FAIL;
  }

  status = sli_mac_get_nwk_radio_parameters(PHY_INDEX_PRO2PLUS, 0, &radio_params);
  if (status != SL_STATUS_OK) {
    return SL_STATUS_FAIL;
  }

  radio_params.prepare_tx_callback = sli_802154mac_prepare_tx_handler;
  radio_params.passthrough_filter_callback = sli_802154mac_passthrough_handler;
  radio_params.poll_handler_callback = sli_zigbee_stack_poll_handler;
  radio_params.indirect_tx_complete_callback = sli_802154mac_packet_send_complete_callback;
  radio_params.make_jit_message_callback = sli_zigbee_make_stack_jit_message;
  radio_params.packet_handoff_incoming_callback = sli_zigbee_packet_handoff_incoming_callback;
  radio_params.process_network_header_callback = sli_zigbee_process_network_header;
  radio_params.poll_tx_complete_callback = sli_802154mac_packet_send_complete_callback;
  radio_params.poll_rx_callback = sli_zigbee_note_successful_poll_received;

  status = sli_mac_set_nwk_radio_parameters(PHY_INDEX_PRO2PLUS, 0, &radio_params);
  if (status != SL_STATUS_OK) {
#ifdef SL_ZIGBEE_TEST
    assert(0);
#endif // SL_ZIGBEE_TEST
    return SL_STATUS_FAIL;
  }
#else // MAC_DUAL_PRESENT
  status = sli_mac_get_nwk_radio_parameters(0, 0, &radio_params);
  if (status != SL_STATUS_OK) {
    return SL_STATUS_FAIL;
  }

  // This function only sets the following fields. Other fields like channel
  // and tx_power are set by other functions.
  radio_params.prepare_tx_callback = sli_802154mac_prepare_tx_handler;
  radio_params.passthrough_filter_callback = sli_802154mac_passthrough_handler;
  radio_params.poll_handler_callback = sli_zigbee_stack_poll_handler;
  radio_params.indirect_tx_complete_callback = sli_802154mac_packet_send_complete_callback;
  radio_params.make_jit_message_callback = sli_zigbee_make_stack_jit_message;
  radio_params.packet_handoff_incoming_callback = sli_zigbee_packet_handoff_incoming_callback;
  radio_params.process_network_header_callback = sli_zigbee_process_network_header;
  radio_params.poll_tx_complete_callback = sli_802154mac_packet_send_complete_callback;
  radio_params.poll_rx_callback = sli_zigbee_note_successful_poll_received;

  status = sli_mac_set_nwk_radio_parameters(0, 0, &radio_params);
  if (status != SL_STATUS_OK) {
#ifdef SL_ZIGBEE_TEST
    assert(0);
#endif // SL_ZIGBEE_TEST
    return SL_STATUS_FAIL;
  }
#endif // MAC_DUAL_PRESENT
  return status;
}

uint8_t sli_zigbee_get_active_always_on_network_index(void)
{
  if (sli_zigbee_is_network_always_on(0) && sli_zigbee_is_network_joined(0)) {
    return 0;
  }

  return SL_ZIGBEE_NULL_NETWORK_INDEX;
}

bool sli_zigbee_is_network_joined(uint8_t nwkIndex)
{
  return (sli_zigbee_state == NETWORK_JOINED
          || sli_zigbee_state == NETWORK_JOINED_UNAUTHENTICATED
          || sli_zigbee_state == NETWORK_REJOINED_UNAUTHENTICATED
          || sli_zigbee_state == NETWORK_JOINED_S2S_INITIATOR
          || sli_zigbee_state == NETWORK_JOINED_S2S_TARGET);
}

bool sli_zigbee_is_network_always_on(uint8_t nwkIndex)
{
  return (sli_zigbee_node_type == SL_ZIGBEE_COORDINATOR
          || sli_zigbee_node_type == SL_ZIGBEE_ROUTER
          || sli_zigbee_node_type == SL_ZIGBEE_END_DEVICE);
}

// SL_ZIGBEE_MULTI_NETWORK_STRIPPED is an optional #define
// that is used on flash-space constrained chips
// to conditionally compile out multi-network related source code.
// These functions are #defined in sl_zigbee_multi_network.h or not #defined at all.
//------------------------------------------------------------------------------
//#if !defined(SL_ZIGBEE_MULTI_NETWORK_STRIPPED)
// Public multi-network APIs.
//------------------------------------------------------------------------------
uint8_t sli_zigbee_stack_get_current_network(void)
{
  return 0;
}
sl_status_t sli_zigbee_stack_set_current_network(uint8_t index)
{
  return SL_STATUS_INVALID_STATE;
}

uint8_t sli_zigbee_stack_get_callback_network(void)
{
  return 0;
}

#if !defined(SL_ZIGBEE_MULTI_NETWORK_STRIPPED)
// Public multi-network APIs.
void sli_zigbee_set_zigbee_event_network_index(uint8_t offset)
{
}
uint8_t sli_zigbee_get_zigbee_event_network_index(uint8_t offset)
{
  return 0;
}
bool sli_zigbee_is_current_network_stack_empty(void)
{
  return true;
}
void sli_zigbee_enable_application_current_network(void)
{
}
uint8_t sli_zigbee_get_current_network_index(void)
{
  return 0;
}
void sli_zigbee_set_current_network_internal(uint8_t nwkIndex)
{
}
void sli_zigbee_restore_current_network_internal(void)
{
}
bool sli_zigbee_association_in_progress(void)
{
  return false;
}

bool sli_zigbee_active_always_on_network_found(void)
{
  return (sli_zigbee_is_network_joined(0) && sli_zigbee_is_network_always_on(0));
}

uint8_t sli_zigbee_get_first_active_network_index(void)
{
  if (sli_zigbee_is_network_joined(0)) {
    return 0;
  } else {
    return 0xFF;
  }
}

bool sli_zigbee_is_active_coordinator_or_router_network_index(uint8_t index)
{
  if (sli_zigbee_is_network_joined(0)
      && (sli_zigbee_node_type == SL_ZIGBEE_COORDINATOR
          || sli_zigbee_node_type == SL_ZIGBEE_ROUTER)
      && index == 0) {
    return true;
  }
  return false;
}

uint8_t sli_zigbee_get_active_coordinator_or_router_network_index(void)
{
  if (sli_zigbee_is_network_joined(0) && sli_zigbee_node_type <= SL_ZIGBEE_ROUTER) {
    return 0;
  } else {
    return SL_ZIGBEE_NULL_NETWORK_INDEX;
  }
}

// Polling stuff
//------------------------------------------------------------------------------

void sli_zigbee_schedule_next_poll(void)
{
  sli_mac_request_poll(0, 0);
}
// Tables allocation stuff
//------------------------------------------------------------------------------

extern void sli_zigbee_reset_nwk_incoming_frame_counters(void);

void sli_zigbee_note_node_type_change(sl_zigbee_node_type_t nodeType)
{
  if (nodeType != SL_ZIGBEE_UNKNOWN_DEVICE) {
    sli_zigbee_reset_nwk_incoming_frame_counters();
  }
}

// Handlers stubs
//------------------------------------------------------------------------------
void sli_zigbee_call_zigbee_key_establishment_handler(sl_802154_long_addr_t partner,
                                                      sl_zigbee_key_status_t status)
{
  sli_zigbee_stack_zigbee_key_establishment_handler(partner, status);
}

void sli_zigbee_call_stack_status_handler(sl_status_t status)
{
  sli_zigbee_stack_stack_status_handler(status);
}

sl_zigbee_packet_action_t sli_zigbee_call_packet_handoff_incoming_handler(
  sl_zigbee_zigbee_packet_type_t packetType,
  sli_buffer_manager_buffer_t packetBuffer,
  uint8_t index,
  // Return:
  void *data)
{
  return sl_zigbee_internal_packet_handoff_incoming_handler(packetType,
                                                            packetBuffer,
                                                            index,
                                                            data);
}

sl_zigbee_packet_action_t sli_zigbee_call_packet_handoff_outgoing_handler(
  sl_zigbee_zigbee_packet_type_t packetType,
  sli_buffer_manager_buffer_t packetBuffer,
  uint8_t index,
  // Return:
  void *data)
{
  return sl_zigbee_internal_packet_handoff_outgoing_handler(packetType,
                                                            packetBuffer,
                                                            index,
                                                            data);
}
#endif // !defined(SL_ZIGBEE_MULTI_NETWORK_STRIPPED)

extern void sli_zigbee_stack_populate_counters(sl_zigbee_counter_type_t type, sl_zigbee_counter_info_t info);
// This is called also from the HAL so we need to provide an implementation
// for both stripped and non-stripped versions of the stack.
/*
 * This function is here to support the direct calls from platform/base
 * (for a few counters that only need the data field).
 * The following function should never be called from the stack, within the
 * stack
 * we only use sli_zigbee_build_and_send_counter_info
 */
void sli_zigbee_stack_call_populate_counters(sl_zigbee_counter_type_t type, uint8_t data)
{
  sl_zigbee_counter_info_t info;
  info.data = data;
  info.otherFields = NULL;
  sli_zigbee_stack_populate_counters(type, info);
}
void sli_zigbee_build_and_send_counter_info(sl_zigbee_counter_type_t counter, sl_802154_short_addr_t dst, uint8_t data)
{
  sl_zigbee_counter_info_t info;
  sl_zigbee_extra_counters_info_t other;
  uint8_t tmpIndex;

  info.data = data;
  bool requirePhyIndex = sli_zigbee_stack_counter_requires_phy_index(counter);
  bool requireDestination = sli_zigbee_stack_counter_requires_destination_node_id(counter);
  if (requirePhyIndex
      && !requireDestination) {
    tmpIndex  = sli_zigbee_get_phy_interface_by_node_id(dst);
    // default to phy index 0 if dst is not found in neighbor or child table,
    // dst should always be present for phyIndex required counters though.
    // However, it's possible for dst to be populated with SL_ZIGBEE_UNKNOWN_NODE_ID if no short
    // destination address was available for the packet in question, making PHY lookup impossible.
    if (tmpIndex == SL_MAC_CHILD_INVALID_INDEX) {
      tmpIndex = PHY_INDEX_NATIVE;
    }
    info.otherFields = &tmpIndex;
  } else if (!requirePhyIndex
             && requireDestination) {
    info.otherFields = &dst;
  } else if (requirePhyIndex
             && requireDestination) {
    tmpIndex  = sli_zigbee_get_phy_interface_by_node_id(dst);
    // default to phy index 0 if dst is not found in neighbor or child table,
    // dst should always be present for phyIndex required counters though.
    // However, it's possible for dst to be populated with SL_ZIGBEE_UNKNOWN_NODE_ID if no short
    // destination address was available for the packet in question, making PHY lookup impossible.
    if (tmpIndex == SL_MAC_CHILD_INVALID_INDEX) {
      tmpIndex = PHY_INDEX_NATIVE;
    }
    other.phy_index = tmpIndex;
    other.destinationNodeId = dst;
    info.otherFields = &other;
  } else {
    // otherFields is not used -- need this branch for MISRA compliance
    info.otherFields = NULL;
  }

  sli_zigbee_stack_populate_counters(counter, info);
}

// sli_zigbee_build_and_send_counter_info() is used to log all counter types within stack,
// including phyIndex and/or destination Id required counters. For phyIndex required
// counters, we get the phyIndex by destination/node id look up.
// Following internal function is used for the counters where destination
// Id is not applicable/valid but counter requires the phyIndex.
// For all other counters, call sli_zigbee_build_and_send_counter_info().
void sli_zigbee_build_and_send_counter_info_with_phy_index(sl_zigbee_counter_type_t counter,
                                                           uint8_t phyIndex,
                                                           uint8_t data)
{
  bool requirePhyIndex = sli_zigbee_stack_counter_requires_phy_index(counter);
  bool requireDestination = sli_zigbee_stack_counter_requires_destination_node_id(counter);

  if (requirePhyIndex
      && !requireDestination) {
    sl_zigbee_counter_info_t info;
    info.data = data;
    info.otherFields = &phyIndex;

    sli_zigbee_stack_populate_counters(counter, info);
    return;
  }

  assert(0);  // wrong api! call sli_zigbee_build_and_send_counter_info build with correct counter info.
}
//------------------------------------------------------------------------------

/***************************************************************************//**
 * @file zigbee_direct_tunneling.c
 * @brief Zigbee Direct - Zigbee Direct tunneling code
 *******************************************************************************
 * # License
 * <b>Copyright 2023 Silicon Laboratories Inc. www.silabs.com</b>
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
#include PLATFORM_HEADER
#include "rail.h"
#include "sl_zigbee.h"
#include "app/framework/include/af.h"
#include "sl_component_catalog.h"
#include "sl_bluetooth.h"
#include "gatt_db.h"
#ifdef SL_CATALOG_ZIGBEE_DEBUG_PRINT_PRESENT
#include "sl_zigbee_debug_print.h"
#endif // SL_CATALOG_ZIGBEE_DEBUG_PRINT_PRESENT
#include "stack/include/sl_zigbee_tlv_core.h"
#include "stack/include/sl_zigbee_stack_specific_tlv.h"
#include "stack/include/sl_zigbee_address_info.h"
#include "stack/include/zigbee-security-manager.h"
#include "zigbee_direct_tlv.h"
#include "zigbee_direct_common.h"

#define SL_ZIGBEE_NETWORK_COMMISSIONING_REQUEST  0x0E
#define LONG_SOURCE_NOT_INCLUDED 0x10
#define NPDU_INDEX_FLAGS 2
#define PACKET_LENGTH_INDEX 3
#define NWK_PACKET_INDEX 4
#define NODE_ID_INDEX 8
#define INCOMING_EUI64_INDEX 12
#define INCOMING_COMMAND_ID_INDEX 20
#define ATTACHED_TLVS_INDEX 23
#define MAX_MTU 254 //maximum usable MTU size
#define NPDU_TLV_OVERHEAD 4
#define OUTGOING_NPDU_QUEUE_SIZE 1024
#define MAX_FLAT_PACKET_SIZE 127

//prototypes
extern void sl_zigbee_direct_send_commissioning_response(uint8_t status, sl_802154_short_addr_t dst, sl_802154_long_addr_t longDest);
extern bool sl_zigbee_direct_send_ephemeral_key(sl_802154_short_addr_t destinationShortId,
                                                sl_802154_long_addr_t destinationLongId,
                                                sl_802154_long_addr_t sourceOrPartnerLongId,
                                                uint8_t keyType,
                                                sl_zigbee_key_data_t* keyData,
                                                uint8_t options);
extern sl_status_t sl_zigbee_transient_device_mgmt_finish(const sl_zigbee_address_info *device_ids,
                                                          bool store);
static void ble_tx_handler(uint8_t * packetData,
                           uint8_t mac_index,
                           uint8_t nwk_index);

//global variables
extern sli_zigbee_ble_connect_table_t bleConnectionTable[SL_BT_CONFIG_MAX_CONNECTIONS];
extern sl_802154_long_addr_t zdd_eui;
static bool commissioning_request_received = false;
static uint8_t commissioning_tlvs[100];
static uint8_t commissioning_tlvs_size;
static uint8_t packet_from_zvd[MAX_MTU];
static uint16_t packet_from_zvd_length = 0;
static uint8_t outgoing_npdu_queue[OUTGOING_NPDU_QUEUE_SIZE];
static uint16_t outgoing_npdu_queue_index = 0;
static uint8_t outgoing_flat_packet[MAX_FLAT_PACKET_SIZE];
static uint8_t in_connection;
static uint8_t outgoing_unicast[MAX_FLAT_PACKET_SIZE];

//local events
static sl_zigbee_af_event_t zb_incoming_tunnel_data_event;
static sl_zigbee_af_event_t zb_schedule_indication_event;
static sl_zigbee_af_event_t zb_tunnel_indicate_event;
static sl_zigbee_af_event_t zb_tunnel_broadcast_to_zvd_event;

//static void sli_zigbee_af_incoming_tunnel_data_event_handler();//sl_zigbee_af_event_t * event) protoype
//this is called when data is arriving via the BLE tunneling service
void sl_zigbee_direct_tunnel_write(uint8_t connection, uint8array * writeValue)
{
  if (!(sl_zigbee_direct_security_decrypt_packet(sl_zvd_eui, writeValue->data, writeValue->len, gattdb_zigbee_tunnel_2))) {
    sl_bt_gatt_server_send_user_write_response(connection, gattdb_zigbee_tunnel_2, ES_ERR_APPLICATION_SPECIFIC);
    return;
  }

  in_connection = connection;

#ifdef SL_CATALOG_ZIGBEE_DEBUG_PRINT_PRESENT
  sl_zigbee_app_debug_print("Received tunnel data [");
  for (uint8_t i = 0; i < writeValue->len - 8; i++) {
    sl_zigbee_app_debug_print(" %02X", writeValue->data[i]);
  }
  sl_zigbee_app_debug_println("]");
#endif //SL_CATALOG_ZIGBEE_

  if (packet_from_zvd_length == 0) {
    //store incoming data in global variable and trigger event to deal with it in the application task
    packet_from_zvd_length = writeValue->len - 8;
    if (packet_from_zvd_length > MAX_MTU) {
      sl_zigbee_app_debug_println("Error: Incoming ZD tunnel message's MTU larger than we can handle");
      sl_bt_gatt_server_send_user_write_response(connection, gattdb_zigbee_tunnel_2, ES_ERR_APPLICATION_SPECIFIC);
      return;
    }
    memcpy(packet_from_zvd, writeValue->data, packet_from_zvd_length);
    sl_zigbee_af_event_set_active(&zb_incoming_tunnel_data_event);
    sl_zigbee_wakeup_common_task();
  } else {
    sl_zigbee_app_debug_println("Had to drop incoming data as I have not submitted the previous one!!!");
  }
}

//the event to handle data coming over BLE from the ZVD
static void sli_zigbee_af_incoming_tunnel_data_event_handler(sl_zigbee_af_event_t * event)
{
  sl_status_t status;
  uint8_t pointer = 0;
  sl_nwk_packet_exchange_t my_nwk_packet = { .mac_interface_id = 0, .nwk_index = 0 };
  uint8_t tlv_length = 0;

  while ((packet_from_zvd_length > 4) && (packet_from_zvd[pointer] == SL_ZIGBEE_DIRECT_TUNNELING_TLV_NPDU_MESSAGE_TAG_ID)) {
    /** General packet structure from tunneled data for nwk commissioning
     *
     * packet_from_zvd[0] Tag ID
     * packet_from_zvd[2] NPDU Flag value
     * packet_from_zvd[3] Length of Packet
     * packet_from_zvd[4] NWK Packet
     * packet_from_zvd[8] Node ID
     * packet_from_zvd[12] EUI 64
     * packet_from_zvd[20] Type of request
     * packet_from_zvd[23] Attached TLVs
     *
     **/
#ifdef SL_CATALOG_ZIGBEE_DEBUG_PRINT_PRESENT
    sl_zigbee_app_debug_print("Received a NPDU message with flags: %02X and length: %02X Message [", packet_from_zvd[pointer + NPDU_INDEX_FLAGS], packet_from_zvd[pointer + PACKET_LENGTH_INDEX]);
    for (uint8_t i = 0; i < packet_from_zvd[pointer + PACKET_LENGTH_INDEX]; i++) {
      sl_zigbee_app_debug_print(" %02X", packet_from_zvd[i + NWK_PACKET_INDEX + pointer]);
    }
    sl_zigbee_app_debug_println("]");
#endif // SL_CATALOG_ZIGBEE_DEBUG_PRINT_PRESENT

    if ((packet_from_zvd[pointer + INCOMING_COMMAND_ID_INDEX] == SL_ZIGBEE_NETWORK_COMMISSIONING_REQUEST) && (packet_from_zvd[pointer + 5] == LONG_SOURCE_NOT_INCLUDED)) {
      commissioning_tlvs_size = packet_from_zvd_length - 22;
      sl_zigbee_app_debug_print("Attached TLVs:");
      for (uint8_t i = 0; i < commissioning_tlvs_size; i++) {
        sl_zigbee_app_debug_print(" %02X", packet_from_zvd[pointer + i + ATTACHED_TLVS_INDEX]);
        commissioning_tlvs[i] = packet_from_zvd[pointer + i + ATTACHED_TLVS_INDEX];
      }
      sl_zigbee_app_debug_println("");

      // extract the EUI64 from packet
      memcpy(sl_zvd_eui, &packet_from_zvd[pointer + INCOMING_EUI64_INDEX], EUI64_SIZE);

      sl_zigbee_core_debug_print("Extracted EUI64:");
      sl_zigbee_af_print_ieee_line(sl_zvd_eui);

      sl_zvd_node_id = sl_util_fetch_low_high_int16u(&packet_from_zvd[pointer + NODE_ID_INDEX]);
      sl_zigbee_app_debug_println("Received Network Commissioning Request 1. Identified ZVDs Network Address to be %04X", sl_zvd_node_id);
      my_nwk_packet.bitmask = (packet_from_zvd[pointer + NPDU_INDEX_FLAGS] & 0x01);
      my_nwk_packet.network_packet = &packet_from_zvd[pointer + NWK_PACKET_INDEX];
      my_nwk_packet.network_packet_length = packet_from_zvd[pointer + PACKET_LENGTH_INDEX];
      my_nwk_packet.nwk_short_address = sl_zvd_node_id;

      status = slx_zigbee_add_to_incoming_network_queue(-40, 255, &my_nwk_packet);
      sl_zigbee_core_debug_println("Submitted commissioning request with Status:%04X", status);
      packet_from_zvd_length = 0;
      incoming_counter = 0;
      sl_bt_gatt_server_send_user_write_response(in_connection, gattdb_zigbee_tunnel_2, ES_WRITE_OK);
      return;
    }

    my_nwk_packet.bitmask = (packet_from_zvd[pointer + NPDU_INDEX_FLAGS] & 0x01);
    my_nwk_packet.network_packet = &packet_from_zvd[pointer + NWK_PACKET_INDEX];
    my_nwk_packet.network_packet_length = packet_from_zvd[pointer + PACKET_LENGTH_INDEX];
    my_nwk_packet.nwk_short_address = sl_zvd_node_id;

    status = slx_zigbee_add_to_incoming_network_queue(-40, 255, &my_nwk_packet);

    tlv_length = packet_from_zvd[pointer + PACKET_LENGTH_INDEX] + 4; // + 3
    pointer = pointer + tlv_length;
    packet_from_zvd_length = packet_from_zvd_length - tlv_length;

    sl_zigbee_app_debug_println("Submitted incoming NPDU with Status:%2X, remaining length %04X", status, packet_from_zvd_length);
  }
  packet_from_zvd_length = 0;
  sl_bt_gatt_server_send_user_write_response(in_connection, gattdb_zigbee_tunnel_2, ES_WRITE_OK);
}

//queue NPDU(s) to be sent to the ZVD
static void sli_zigbee_direct_queue_outgoing_npdu(uint8_t * packet, uint8_t length)
{
  //todo support more than a single connection
  // we don't keep queuing in case connection is closed
  if (bleConnectionTable[0].inUse == false) {
    outgoing_npdu_queue_index = 0;
    sl_zigbee_app_debug_println("BLE connection is down, flushing queue");
    return;
  }

  if ((outgoing_npdu_queue_index + length + 6) > OUTGOING_NPDU_QUEUE_SIZE) {
    // this may happen as there could easily be a lot more Zigbee traffic than the low throughput BLE indications can transport,
    // at the same time we can sensibly only buffer so much
    sl_zigbee_app_debug_println("Can't queue further outgoing messages, buffer full");
    return;
  }

  outgoing_npdu_queue[outgoing_npdu_queue_index++] = SL_ZIGBEE_DIRECT_TUNNELING_TLV_NPDU_MESSAGE_TAG_ID;   //Type
  outgoing_npdu_queue[outgoing_npdu_queue_index++] = length + 1;   //Length
  if (packet[1] & 0x02) {   //Value: Flags
    sl_zigbee_app_debug_println("Setting NPDU security flag");
    outgoing_npdu_queue[outgoing_npdu_queue_index++] = 0x01;    //Value: Flags -> Security enabled
  } else {
    sl_zigbee_app_debug_println("Clearing NPDU security flag");
    outgoing_npdu_queue[outgoing_npdu_queue_index++] = 0x00;
  }
  outgoing_npdu_queue[outgoing_npdu_queue_index++] = length;    //Value: NPDU length

  memcpy(&outgoing_npdu_queue[outgoing_npdu_queue_index], packet, length);    //Value: NPDU
  outgoing_npdu_queue[outgoing_npdu_queue_index + 1] &= 0xFD;     // always set security bit in NWK header (bit 9) to 0
  outgoing_npdu_queue_index += length;

  //trigger event for sending if not already active
  if ((sl_zigbee_af_event_is_scheduled(&zb_schedule_indication_event) == false) && (sl_zigbee_af_event_is_scheduled(&zb_tunnel_indicate_event) == false)) {
    sl_zigbee_af_event_set_active(&zb_schedule_indication_event);
    sl_zigbee_wakeup_common_task();
  }
}

//schedule an indication to be sent for the next connection interval
void sli_zigbee_af_schedule_indication_event_handler(sl_zigbee_af_event_t * event)
{
  uint32_t access_address;
  uint8_t role;
  uint32_t crc_init;
  uint16_t interval;
  uint16_t supervision_timeout;
  uint8_t central_clock_accuracy;
  uint8_t central_phy;
  uint8_t peripheral_phy;
  uint8_t channel_selection_algorithm;
  uint8_t hop;
  sl_bt_connection_channel_map_t channel_map;
  uint8_t channel;
  uint16_t event_counter;
  uint32_t time_to_next_connection_event;
  RAIL_Time_t rail_time;

  sl_zigbee_af_event_set_inactive(&zb_schedule_indication_event);

  sl_bt_connection_get_scheduling_details(bleConnectionTable[0].connectionHandle,
                                          &access_address,
                                          &role,
                                          &crc_init,
                                          &interval,
                                          &supervision_timeout,
                                          &central_clock_accuracy,
                                          &central_phy,
                                          &peripheral_phy,
                                          &channel_selection_algorithm,
                                          &hop,
                                          &channel_map,
                                          &channel,
                                          &event_counter,
                                          &time_to_next_connection_event);

  rail_time = RAIL_GetTime();
  if (RAIL_GetTime() >= time_to_next_connection_event) { //we missed the current one
    sl_zigbee_af_event_set_inactive(&zb_schedule_indication_event);
    return;
  }

  time_to_next_connection_event = time_to_next_connection_event - rail_time;
  time_to_next_connection_event = (time_to_next_connection_event >> 10);

  sl_zigbee_app_debug_println("time_to_next_connection_event ms %08X", time_to_next_connection_event);

  if (time_to_next_connection_event <= 2) {
    sl_zigbee_af_event_set_active(&zb_tunnel_indicate_event);
  } else {
    sl_zigbee_af_event_set_delay_ms(&zb_tunnel_indicate_event, (time_to_next_connection_event - 2));   //allow 2ms slack
  }
}

// and send send NPDU(s) via the tunnel to the ZVD
void sli_zigbee_af_tunnel_indicate_event_handler(sl_zigbee_af_event_t * event)
{
  uint8_t outgoing_indication[MAX_MTU + 12];
  uint16_t outgoing_npdu_queue_read_index = 0;
  sl_status_t status;
  uint16_t negotiated_max_mtu;
  uint16_t outgoing_indication_length = NPDU_TLV_OVERHEAD;
  uint16_t length;

  sl_zigbee_af_event_set_inactive(&zb_tunnel_indicate_event);

  status =  sl_bt_gatt_server_get_mtu(bleConnectionTable[0].connectionHandle, &negotiated_max_mtu);
  if (status != SL_STATUS_OK) {
    sl_zigbee_app_debug_println("ERROR: Can't get MAX_MTU");
    sl_zigbee_af_event_set_active(&zb_tunnel_indicate_event);   //restart event to prevent lock-up
  }

  while ((outgoing_npdu_queue_index > outgoing_npdu_queue_read_index) && ((outgoing_indication_length + 12 + outgoing_npdu_queue[outgoing_npdu_queue_read_index + 1]) < negotiated_max_mtu)) {
    length = outgoing_npdu_queue[outgoing_npdu_queue_read_index + 1] + 3;  //used to be +3
    sl_zigbee_app_debug_println("De-Queuing by reading from outgoing queue at %04X, placing %04X characters into %04X. Total length of outgoing queue: %04X", outgoing_npdu_queue_read_index, length, outgoing_indication_length, outgoing_npdu_queue_index);
    memcpy(&outgoing_indication[outgoing_indication_length], &outgoing_npdu_queue[outgoing_npdu_queue_read_index], length);

    outgoing_indication_length += length;
    outgoing_npdu_queue_read_index += length;
  }

  sl_zigbee_app_debug_println("Done de-queuing at outgoing queue %04X, placed into %04X", outgoing_npdu_queue_read_index, outgoing_indication_length);
  if (outgoing_indication_length < 5) {
    return;
  }

#ifdef SL_CATALOG_ZIGBEE_DEBUG_PRINT_PRESENT

  sl_zigbee_app_debug_print("Sending NPDU(s) over BLE  with length: %02X [", outgoing_indication_length);
  for (uint8_t i = 4; i < outgoing_indication_length; i++) {
    sl_zigbee_app_debug_print(" %02X", outgoing_indication[i]);
  }
  sl_zigbee_app_debug_println("]");

#endif // SL_CATALOG_ZIGBEE_DEBUG_PRINT_PRESENT

  if (sl_zigbee_direct_security_encrypt_packet(zdd_eui, &outgoing_indication[4], outgoing_indication_length - 4, gattdb_zigbee_tunnel_2) != SL_STATUS_OK) {
    sl_zigbee_app_debug_println("ERROR: Something went wrong during encryption", status);
    return;
  }

  status = sl_bt_gatt_server_send_indication(bleConnectionTable[0].connectionHandle,
                                             gattdb_zigbee_tunnel_2,
                                             outgoing_indication_length + 4,
                                             outgoing_indication);
  sl_zigbee_app_debug_println("Indication sent with status %04X", status);

  if (status == SL_STATUS_OK) {
    memcpy(outgoing_npdu_queue, &outgoing_npdu_queue[outgoing_npdu_queue_read_index], outgoing_npdu_queue_index - outgoing_npdu_queue_read_index);
    outgoing_npdu_queue_index = outgoing_npdu_queue_index - outgoing_npdu_queue_read_index;
    sl_zigbee_app_debug_println("New outgoing queue index %04X", outgoing_npdu_queue_index);
  }
}

sl_zigbee_packet_action_t sl_zigbee_af_incoming_packet_filter_cb(sl_zigbee_zigbee_packet_type_t packetType,
                                                                 uint8_t * packetData,
                                                                 uint8_t * size_p,
                                                                 void* data)
{
  /*
   #ifdef SL_CATALOG_ZIGBEE_DEBUG_PRINT_PRESENT
     sl_zigbee_app_debug_print("Incoming packet type %X:", packetType);
     for (uint8_t j = 0; j < *size_p; j++) {
     sl_zigbee_app_debug_print(" %X", packetData[j]);
     }
     sl_zigbee_app_debug_println("");
   #endif // SL_CATALOG_ZIGBEE_DEBUG_PRINT_PRESENT
   */
  // catch tunneling transport key message for ZVD
  if ((packetType == SL_ZIGBEE_ZIGBEE_PACKET_TYPE_APS_COMMAND)
      && (packetData[0] == 0x0e)   // 0x0e = APS_CMD_TUNNEL
      && (memcmp(&packetData[1], sl_zvd_eui, EUI64_SIZE) == 0)) {
    sl_zigbee_app_debug_println("Caught aps tunnel command for ZVD");
    // If the APS command frame is not using APS layer security, the APSME of the ZDD SHALL examine the APSME command identifier,
    // and in case of a Transport Key message, the StandardKeyType field of that message.
    // If the StandardKeyType equals 'Standard network key' (0x01) the message is considered a Transport Key message conveying
    // an active or prospective network key and the APSME SHALL decline forwarding of this frame to the ZVD.
    if ((packetData[9] & 0x20) == 0x00) {   // No APS security
      if ((packetData[11] == 0x05) && (packetData[12] == 0x01)) {    // 0x05 = APS_CMD_TRANSPORT_KEY //0x01 Standard network key
        sl_zigbee_app_debug_println("Caught unencrypted Transport Key Message with standard key type, dropping and disconnecting!");
        sl_bt_connection_close(sl_my_connection);
        return SL_ZIGBEE_DROP_PACKET;
      }
    } else {   //APS security
      // If the APS command frame is using APS layer encryption, the APSME of the ZDD SHALL examine the auxiliary security header,
      // and in particular the Key Identifier sub-field of the Security Control field. If the Key Identifier equals 'key-transport key' (0x02)
      // the message is already decrypted here, so I am here assuming it was decrypted using the well-known key.
      sl_zigbee_app_debug_println("Tunnel command is using APS crypto");
      uint8_t pkt = packetData[11] & 0x18;
      if (sli_zigbee_direct_tc_is_zigbee_direct_aware == false) {
        sl_zigbee_app_debug_println("It is a legacy network");
        if (pkt == 0x10) {          //key-transport key
          sl_zigbee_app_debug_println("Caught Key Transport Key Message!");

          sl_status_t status = SL_STATUS_OK;
          if (status == SL_STATUS_OK) {
            sl_zigbee_app_debug_println("Decryption with default key succeeded!");
          } else {
            sl_zigbee_app_debug_println("No luck decrypting with default key", status);
          }

          sl_zigbee_sec_man_key_t zigbeeAlliance09Key = ZIGBEE_3_CENTRALIZED_SECURITY_LINK_KEY;
          status = sl_zigbee_sec_man_import_link_key(0xFF,
                                                     sl_zvd_eui,
                                                     &zigbeeAlliance09Key);
          sl_zigbee_app_debug_println("Adding well known key with status: %02X", status);
          status = sl_zigbee_direct_send_ephemeral_key(sl_zvd_node_id,
                                                       sl_zvd_eui,
                                                       NULL,   //should be ok as this is the TC address
                                                       0xB0,   //KEY_TRANSPORT_EPHEMERAL_GLOBAL_LINK_KEY
                                                       NULL,
                                                       0x02);  //ENCRYPTION_APS
          sl_zigbee_app_debug_println("Sent transport key with type KEY_TRANSPORT_EPHEMERAL_GLOBAL_LINK_KEY and status %02X", status);
          return SL_ZIGBEE_DROP_PACKET;
        } else if (pkt == 0x20) {         //key-load key
          sl_zigbee_app_debug_println("Caught  Key Load Key Message!");
          sl_zigbee_address_info my_zvd_address_info;
          memcpy(my_zvd_address_info.device_long, sl_zvd_eui, EUI64_SIZE);
          my_zvd_address_info.device_short = sl_zvd_node_id;
          sl_zigbee_transient_device_mgmt_finish(&my_zvd_address_info, true);
        }
      } else {   //TC is Zigbee Direct aware
        if ( pkt == 0x10) {           //key-transport key
          sl_zigbee_app_debug_println("Caught suspect Transport Key Message, dropping and disconnecting!");
          sl_bt_connection_close(sl_my_connection);
          return SL_ZIGBEE_DROP_PACKET;
        } else {
          sl_zigbee_app_debug_println("Moving device on from transient table");   //this is not happening in ZD aware situation
          sl_zigbee_address_info my_zvd_address_info;
          memcpy(my_zvd_address_info.device_long, sl_zvd_eui, EUI64_SIZE);
          my_zvd_address_info.device_short = sl_zvd_node_id;
          sl_zigbee_transient_device_mgmt_finish(&my_zvd_address_info, true);
        }
      }
    }
  }
  return SL_ZIGBEE_ACCEPT_PACKET;
}

uint8_t outgoing_broadcast[MAX_FLAT_PACKET_SIZE];
static void  sli_zigbee_af_tunnel_broadcast_to_zvd_event_handler(sl_zigbee_af_event_t * event)
{
  sli_zigbee_direct_queue_outgoing_npdu(&outgoing_broadcast[1], outgoing_broadcast[0]);
}

sl_zigbee_packet_action_t sl_zigbee_af_outgoing_packet_filter_cb(sl_zigbee_zigbee_packet_type_t  packetType,
                                                                 uint8_t * packetData,
                                                                 uint8_t * size_p,
                                                                 void* data)
{
  if (((packetType == SL_ZIGBEE_ZIGBEE_PACKET_TYPE_NWK_DATA) || (packetType == SL_ZIGBEE_ZIGBEE_PACKET_TYPE_NWK_COMMAND)) && (sl_util_fetch_low_high_int16u(&packetData[2]) == sl_zvd_node_id)) {
    #ifdef SL_CATALOG_ZIGBEE_DEBUG_PRINT_PRESENT
    sl_zigbee_app_debug_print("Outgoing Packet Filter NWK");
    for (uint8_t i = 0; i < *size_p; i++) {
      sl_zigbee_app_debug_print(" %02X", packetData[i]);
    }
    sl_zigbee_app_debug_println("");
    #endif //SL_CATALOG_ZIGBEE_DEBUG_PRINT_PRESENT

    //queue unicast data for transmission to ZVD
    memcpy(&outgoing_unicast[1], packetData, *size_p);
    outgoing_unicast[0] = *size_p;
    sl_zigbee_app_debug_println("Queuing length of %02X", outgoing_unicast[0]);
    // the sli_zigbee_af_tunnel_broadcast_to_zvd_event_handler is not going through the IPC, and can be called on quick succession
    // so handling this via an af event is not an option. Quickest way to make this thread safe is to handle in an atomic block
    ATOMIC(
      sli_zigbee_direct_queue_outgoing_npdu(&outgoing_unicast[1], outgoing_unicast[0]);
      )
    return SL_ZIGBEE_DROP_PACKET;
  }
  return SL_ZIGBEE_ACCEPT_PACKET;
}

//this function is handling outgoing packets targeting the ZVD
static void ble_tx_handler(uint8_t * packetData,
                           uint8_t mac_index,
                           uint8_t nwk_index)
{
  uint8_t len = packetData[0];
  uint8_t networkPayloadIndex = sli_mac_flat_payload_offset(packetData, true);
  // Outgoing Network Packet Filtering
  uint16_t sl_nodeId = sl_util_fetch_low_high_int16u(&packetData[networkPayloadIndex + 2]);
  sl_zigbee_app_debug_print("ble_tx_handler sl_nodeId %04X networkPayloadIndex %d len %d\n ",
                            sl_nodeId, networkPayloadIndex, len);
  if (sl_nodeId >= SL_ZIGBEE_BROADCAST_ADDRESS) {
    // there is no other way to check if this is broadcast
    // this has already also been submitted to macSubmit
    sl_zigbee_app_debug_println("Forwarding Broadcast to ZVD");

    memcpy(&outgoing_broadcast[1], packetData, len);
    outgoing_broadcast[0] = len;
    sl_zigbee_af_event_set_active(&zb_tunnel_broadcast_to_zvd_event);
    sl_zigbee_wakeup_common_task();
  } else {   //this message has arrived here, being aimed for BLE interface ID
    uint8_t network_packet_length = len - networkPayloadIndex;
    sl_zigbee_app_debug_println("Forwarding Unicast to ZVD");
    for (uint8_t i = networkPayloadIndex; i < network_packet_length; i++) {
      sl_zigbee_app_debug_print(" %02X", packetData[i]);
    }
    sl_zigbee_app_debug_println("");
    memcpy(&outgoing_broadcast[1], packetData + networkPayloadIndex, network_packet_length);
    outgoing_broadcast[0] = network_packet_length;
    sl_zigbee_af_event_set_active(&zb_tunnel_broadcast_to_zvd_event);
    sl_zigbee_wakeup_common_task();
  }
}

// This is called by the Zigbee Stack (via the IPC, so it is thread safe)
// at this moment in time only multicasts are forwarded this way
void sli_zigbee_af_plugin_zdd_tunneling_redirect_outgoing_message_callback(uint8_t mac_index,
                                                                           uint8_t packet_length,
                                                                           uint8_t * packet_contents,
                                                                           sl_zigbee_transmit_priority_t priority)
{
  // a mac level raw packet is received here
  memcpy(outgoing_flat_packet + 1, packet_contents, packet_length);
  outgoing_flat_packet[0] = packet_length;
  ble_tx_handler(outgoing_flat_packet, mac_index, 0);
}

// Initialization of tunneling events used by the tunneling component
void sli_zigbee_af_zdd_tunneling_init(uint8_t init_level)
{
  switch (init_level) {
    case SL_ZIGBEE_INIT_LEVEL_EVENT:
    {
      sl_zigbee_af_event_init(&zb_incoming_tunnel_data_event, sli_zigbee_af_incoming_tunnel_data_event_handler);
      sl_zigbee_af_event_init(&zb_schedule_indication_event, sli_zigbee_af_schedule_indication_event_handler);
      sl_zigbee_af_event_init(&zb_tunnel_indicate_event, sli_zigbee_af_tunnel_indicate_event_handler);
      sl_zigbee_af_event_init(&zb_tunnel_broadcast_to_zvd_event, sli_zigbee_af_tunnel_broadcast_to_zvd_event_handler);
      break;
    }
    default:
      break;
  }
}

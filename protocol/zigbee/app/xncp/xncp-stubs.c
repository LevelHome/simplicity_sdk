/***************************************************************************//**
 * @file
 * @brief Programmable NCP stubs.
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

#include "xncp.h"
#include "app/util/ezsp/ezsp-frame-utilities.h"

bool sli_zigbee_xncp_allow_incoming_ezsp_command(void)
{
  return true;
}

bool sli_zigbee_xncp_allow_outgoing_ezsp_callback(uint8_t *callback,
                                                  uint8_t callbackLength)
{
  (void)callback;
  (void)callbackLength;
  return true;
}

bool sli_zigbee_af_xncp_incoming_message_callback(sl_zigbee_incoming_message_type_t type,
                                                  sl_zigbee_aps_frame_t *apsFrame,
                                                  sl_zigbee_rx_packet_info_t *packetInfo,
                                                  uint8_t messageLength,
                                                  uint8_t *message)
{
  (void)type;
  (void)apsFrame;
  (void)packetInfo;
  (void)messageLength;
  (void)message;
  return false;
}

void sli_zigbee_xncp_handle_incoming_custom_ezsp_message(void)
{
  appendInt32u(SL_STATUS_INVALID_STATE);
  appendInt8u(0); // Bug 14838: We always need to append the reply length
}
void sli_zigbee_xncp_handle_get_info_command(void)
{
  appendInt32u(SL_STATUS_INVALID_STATE);
}

void sl_zigbee_af_xncp_get_xncp_information(uint16_t *manufacturerId,
                                            uint16_t *versionNumber)
{
  *versionNumber = *manufacturerId = 0;
}

/*
 * SPDX-License-Identifier: LicenseRef-MSLA
 * Copyright (c) 2022 Silicon Laboratories Inc. (www.silabs.com)
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of the Silicon Labs Master Software License
 * Agreement (MSLA) available at [1].  This software is distributed to you in
 * Object Code format and/or Source Code format and is governed by the sections
 * of the MSLA applicable to Object Code, Source Code and Modified Open Source
 * Code. By using this software, you agree to the terms of the MSLA.
 *
 * [1]: https://www.silabs.com/about-us/legal/master-software-license-agreement
 */

#include <assert.h>
#include <cmsis_os2.h>
#include "sl_memory_manager.h"
#include "sl_wisun_trace_api.h"
#include "sl_wisun_ip6string.h"
#include "sl_wisun_types.h"
#include "socket/socket.h"
#include "common/iobuf.h"
#include "sl_wisun_br_dhcpv6_server.h"

static int dhcpv6_server_socket = -1;
static uint8_t server_prefix[8], server_DUID[8];
static uint32_t valid_lifetime, prefered_lifetime;

// Messages types (RFC3315, section 5.3)
#define DHCPV6_MSG_SOLICIT      1
#define DHCPV6_MSG_ADVERT       2  /* Unused */
#define DHCPV6_MSG_REQUEST      3  /* Unused */
#define DHCPV6_MSG_CONFIRM      4  /* Unused */
#define DHCPV6_MSG_RENEW        5  /* Unused */
#define DHCPV6_MSG_REBIND       6  /* Unused */
#define DHCPV6_MSG_REPLY        7
#define DHCPV6_MSG_RELEASE      8  /* Unused */
#define DHCPV6_MSG_DECLINE      9  /* Unused */
#define DHCPV6_MSG_RECONFIGURE  10 /* Unused */
#define DHCPV6_MSG_INFO_REQUEST 11 /* Unused */
#define DHCPV6_MSG_RELAY_FWD    12
#define DHCPV6_MSG_RELAY_REPLY  13

// Options IDs (RFC3315, section 24.3)
#define DHCPV6_OPT_CLIENT_ID                  0x0001
#define DHCPV6_OPT_SERVER_ID                  0x0002
#define DHCPV6_OPT_IA_NA                      0x0003
#define DHCPV6_OPT_IA_TA                      0x0004 /* Unused */
#define DHCPV6_OPT_IA_ADDRESS                 0x0005
#define DHCPV6_OPT_ORO                        0x0006 /* Unused */
#define DHCPV6_OPT_PREFERENCE                 0x0007 /* Unused */
#define DHCPV6_OPT_ELAPSED_TIME               0x0008
#define DHCPV6_OPT_RELAY                      0x0009
#define DHCPV6_OPT_RESERVED1                  0x000a /* Unused */
#define DHCPV6_OPT_AUTH                       0x000b /* Unused */
#define DHCPV6_OPT_UNICAST                    0x000c /* Unused */
#define DHCPV6_OPT_STATUS_CODE                0x000d
#define DHCPV6_OPT_RAPID_COMMIT               0x000e
#define DHCPV6_OPT_USER_CLASS                 0x000f /* Unused */
#define DHCPV6_OPT_VENDOR_CLASS               0x0010 /* Unused */
#define DHCPV6_OPT_VENDOR_SPECIFIC            0x0011 /* Unused */
#define DHCPV6_OPT_INTERFACE_ID               0x0012
#define DHCPV6_OPT_RECONF_MSG                 0x0013 /* Unused */
#define DHCPV6_OPT_RECONF_ACCEPT              0x0014 /* Unused */

#define DHCPV6_DUID_TYPE_LINK_LAYER_PLUS_TIME 0x0001 /* Unused */
#define DHCPV6_DUID_TYPE_EN                   0x0002 /* Unused */
#define DHCPV6_DUID_TYPE_LINK_LAYER           0x0003
#define DHCPV6_DUID_TYPE_UUID                 0x0004 /* Unused */

#define DHCPV6_DUID_HW_TYPE_EUI48             0x0001 /* Unused */
#define DHCPV6_DUID_HW_TYPE_IEEE802           0x0006
#define DHCPV6_DUID_HW_TYPE_EUI64             0x001b

static int dhcp_handle_request(struct iobuf_read *req, struct iobuf_write *reply);
static int dhcp_handle_request_fwd(struct iobuf_read *req, struct iobuf_write *reply);

static int dhcp_get_option(const uint8_t *data, size_t len, uint16_t option, struct iobuf_read *option_payload)
{
  uint16_t opt_type, opt_len;
  struct iobuf_read input = {
    .data_size = len,
    .data = data,
  };

  memset(option_payload, 0, sizeof(struct iobuf_read));
  option_payload->err = true;
  while (iobuf_remaining_size(&input)) {
    opt_type = iobuf_pop_be16(&input);
    opt_len = iobuf_pop_be16(&input);
    if (opt_type == option) {
      option_payload->data = iobuf_pop_data_ptr(&input, opt_len);
      if (!option_payload->data) {
        return -1;
      }
      option_payload->err = false;
      option_payload->data_size = opt_len;
      return opt_len;
    }
    iobuf_pop_data_ptr(&input, opt_len);
  }
  return -1;
}

static int dhcp_get_client_hwaddr(const uint8_t *req, size_t req_len, const uint8_t **hwaddr)
{
  struct iobuf_read opt;
  uint16_t duid_type, ll_type;

  dhcp_get_option(req, req_len, DHCPV6_OPT_CLIENT_ID, &opt);
  if (opt.err) {
    sl_wisun_trace_error("dhcp_get_client_hwaddr: missing client ID option");
    return -1;
  }
  duid_type = iobuf_pop_be16(&opt);
  ll_type = iobuf_pop_be16(&opt);
  if (duid_type != DHCPV6_DUID_TYPE_LINK_LAYER ||
    (ll_type != DHCPV6_DUID_HW_TYPE_EUI64 && ll_type != DHCPV6_DUID_HW_TYPE_IEEE802)) {
    sl_wisun_trace_error("dhcp_get_client_hwaddr: unsupported client ID option %"PRIu16"", ll_type);
    return -1;
  }
  *hwaddr = iobuf_pop_data_ptr(&opt, 8);
  return ll_type;
}

static uint32_t dhcp_get_identity_association_id(const uint8_t *req, size_t req_len)
{
  struct iobuf_read opt;
  uint32_t ia_id;

  dhcp_get_option(req, req_len, DHCPV6_OPT_IA_NA, &opt);
  ia_id = iobuf_pop_be32(&opt);
  if (opt.err) {
    sl_wisun_trace_error("dhcp_get_identity_association_id: missing IA_NA option");
    return UINT32_MAX;
  }
  return ia_id;
}

static int dhcp_check_rapid_commit(const uint8_t *req, size_t req_len)
{
  struct iobuf_read opt;

  dhcp_get_option(req, req_len, DHCPV6_OPT_RAPID_COMMIT, &opt);
  if (opt.err) {
    sl_wisun_trace_error("dhcp_check_rapid_commit: missing rapid commit option");
    return -1;
  }
  return 0;
}

static int dhcp_check_status_code(const uint8_t *req, size_t req_len)
{
  struct iobuf_read opt;
  uint16_t status;

  dhcp_get_option(req, req_len, DHCPV6_OPT_STATUS_CODE, &opt);
  if (opt.err) {
    return 0;
  }
  status = iobuf_pop_be16(&opt);
  if (status) {
    sl_wisun_trace_error("dhcp_check_status_code: status code: %d", status);
    return -1;
  }
  return 0;
}

static int dhcp_check_elapsed_time(const uint8_t *req, size_t req_len)
{
  struct iobuf_read opt;

  dhcp_get_option(req, req_len, DHCPV6_OPT_ELAPSED_TIME, &opt);
  if (opt.err) {
    sl_wisun_trace_error("dhcp_check_elapsed_time: missing elapsed time option");
    return -1; // Elapsed Time option is mandatory
  }
  return 0;
}

static void dhcp_fill_server_id(struct iobuf_write *reply)
{
  iobuf_push_be16(reply, DHCPV6_OPT_SERVER_ID);
  iobuf_push_be16(reply, 2 + 2 + 8);
  iobuf_push_be16(reply, DHCPV6_DUID_TYPE_LINK_LAYER);
  iobuf_push_be16(reply, DHCPV6_DUID_HW_TYPE_IEEE802);
  iobuf_push_data(reply, server_DUID, 8);
}

static void dhcp_fill_client_id(struct iobuf_write *reply,
                                uint16_t hwaddr_type, const uint8_t *hwaddr)
{
  iobuf_push_be16(reply, DHCPV6_OPT_CLIENT_ID);
  iobuf_push_be16(reply, 2 + 2 + 8);
  iobuf_push_be16(reply, DHCPV6_DUID_TYPE_LINK_LAYER);
  iobuf_push_be16(reply, hwaddr_type);
  iobuf_push_data(reply, hwaddr, 8);
  }

static void dhcp_fill_rapid_commit(struct iobuf_write *reply)
{
  iobuf_push_be16(reply, DHCPV6_OPT_RAPID_COMMIT);
  iobuf_push_be16(reply, 0);
}

static void dhcp_fill_identity_association(struct iobuf_write *reply,
                                           const uint8_t *hwaddr, uint32_t ia_id)
{
  uint8_t ipv6[16];
  memcpy(ipv6, server_prefix, 8);
  memcpy(ipv6 + 8, hwaddr, 8);
  ipv6[8] ^= 0x02;
  iobuf_push_be16(reply, DHCPV6_OPT_IA_NA);
  iobuf_push_be16(reply, 4 + 4 + 4 + 2 + 2 + 16 + 4 + 4);
  iobuf_push_be32(reply, ia_id);
  iobuf_push_be32(reply, 0); // T1
  iobuf_push_be32(reply, 0); // T2
  iobuf_push_be16(reply, DHCPV6_OPT_IA_ADDRESS);
  iobuf_push_be16(reply, 16 + 4 + 4);
  iobuf_push_data(reply, ipv6, 16);
  iobuf_push_be32(reply, prefered_lifetime);
  iobuf_push_be32(reply, valid_lifetime);
}

static int dhcp_send_reply(struct sockaddr_in6 *dest,
                           struct iobuf_write *reply)
{
  char dst_addr_str[MAX_IPV6_STRING_LEN_WITH_TRAILING_NULL];
  int32_t retval = sendto(dhcpv6_server_socket, reply->data, reply->len, 0, (struct sockaddr *)dest, sizeof(struct sockaddr_in6));
  if (retval <= 0) {
    sl_wisun_trace_error("dhcp_send_reply: sendto failed %d", retval);
    return -1;
  }
  ip6tos(dest->sin6_addr.address, dst_addr_str);
  sl_wisun_trace_info("dhcp_send_reply: sending reply to %s", dst_addr_str);
  return 0;
}

static int dhcp_handle_request_fwd(struct iobuf_read *req, struct iobuf_write *reply)
{
  uint8_t buf[350];
  struct iobuf_read opt_interface_id, opt_relay;
  struct iobuf_write relay_reply = {
    .data_size = 350,
    .data = buf,
  };
  const uint8_t *linkaddr, *peeraddr;
  uint8_t hopcount;

  hopcount = iobuf_pop_u8(req);
  linkaddr = iobuf_pop_data_ptr(req, 16);
  peeraddr = iobuf_pop_data_ptr(req, 16);
  iobuf_push_u8(reply, DHCPV6_MSG_RELAY_REPLY);
  iobuf_push_u8(reply, hopcount);
  iobuf_push_data(reply, linkaddr, 16);
  iobuf_push_data(reply, peeraddr, 16);
  if (dhcp_get_option(iobuf_ptr(req), iobuf_remaining_size(req),
                      DHCPV6_OPT_INTERFACE_ID, &opt_interface_id) > 0) {
    iobuf_push_be16(reply, DHCPV6_OPT_INTERFACE_ID);
    iobuf_push_be16(reply, opt_interface_id.data_size);
    iobuf_push_data(reply, opt_interface_id.data, opt_interface_id.data_size);
  }
  if (dhcp_get_option(iobuf_ptr(req), iobuf_remaining_size(req),
                      DHCPV6_OPT_RELAY, &opt_relay) < 0) {
    sl_wisun_trace_error("dhcp_handle_request_fwd: missing relay option");
    return -1;
  }
  if (dhcp_handle_request(&opt_relay, &relay_reply) < 0) {
    return -1;
  }
  iobuf_push_be16(reply, DHCPV6_OPT_RELAY);
  iobuf_push_be16(reply, relay_reply.len);
  iobuf_push_data(reply, relay_reply.data, relay_reply.len);
  return 0;
}

static int dhcp_handle_request(struct iobuf_read *req, struct iobuf_write *reply)
{
  uint24_t transaction;
  uint8_t msg_type;
  uint32_t iaid;
  const uint8_t *hwaddr;
  int hwaddr_type;

  msg_type = iobuf_pop_u8(req);
  if (msg_type == DHCPV6_MSG_RELAY_FWD) {
    return dhcp_handle_request_fwd(req, reply);
  }
  if (msg_type != DHCPV6_MSG_SOLICIT) {
    sl_wisun_trace_error("dhcp_handle_request: unsupported message type %d", msg_type);
    return -1;
  }

  transaction = iobuf_pop_be24(req);
  if (dhcp_check_status_code(iobuf_ptr(req), iobuf_remaining_size(req))) {
    return -1;
  }
  if (dhcp_check_rapid_commit(iobuf_ptr(req), iobuf_remaining_size(req))) {
    return -1;
  }
  if (dhcp_check_elapsed_time(iobuf_ptr(req), iobuf_remaining_size(req))) {
    return -1;
  }
  iaid = dhcp_get_identity_association_id(iobuf_ptr(req), iobuf_remaining_size(req));
  if (iaid == UINT32_MAX) {
    return -1;
  }
  hwaddr_type = dhcp_get_client_hwaddr(iobuf_ptr(req), iobuf_remaining_size(req), &hwaddr);
  if (hwaddr_type < 0) {
    return -1;
  }

  iobuf_push_u8(reply, DHCPV6_MSG_REPLY);
  iobuf_push_be24(reply, transaction);
  dhcp_fill_server_id(reply);
  dhcp_fill_client_id(reply, hwaddr_type, hwaddr);
  dhcp_fill_identity_association(reply, hwaddr, iaid);
  dhcp_fill_rapid_commit(reply);
  return 0;
}

void sl_wisun_br_dhcpv6_server_on_recv(uint8_t *buffer, ssize_t length, in6_addr_t peer_address, in_port_t remote_port)
{
  char src_addr_str[MAX_IPV6_STRING_LEN_WITH_TRAILING_NULL];

  void iobuf_reset(struct iobuf_write *buf);
  sockaddr_in6_t src_addr = {
    .sin6_family = AF_INET6,
    .sin6_port = remote_port,
    .sin6_flowinfo = 0,
    .sin6_addr = IN6ADDR_ANY_INIT,
    .sin6_scope_id = 0,
  };
  uint8_t buf[350];

  ip6tos(peer_address.address, src_addr_str);
  sl_wisun_trace_info("sl_wisun_br_dhcpv6_server_on_recv: received msg from %s", src_addr_str);
  memcpy (src_addr.sin6_addr.address, peer_address.address, 16);
  struct iobuf_read req = {
    .data_size = length,
    .data = buffer,
   };
  struct iobuf_write reply = {
    .data_size = 350,
    .data = buf,
  };
  if (!dhcp_handle_request(&req, &reply)) {
    dhcp_send_reply(&src_addr, &reply);
  }
  req.data = buffer;
}


sl_status_t sl_wisun_br_dhcpv6_server_init(void)
{
  return SL_STATUS_OK;
}

sl_status_t sl_wisun_br_dhcpv6_server_start(int socket, uint8_t prefix[8], uint8_t DUID[8], uint32_t dhcp_address_lifetime)
{
  dhcpv6_server_socket = socket;
  valid_lifetime = dhcp_address_lifetime;
  prefered_lifetime = dhcp_address_lifetime / 2;
  memcpy(server_prefix, prefix, 8);
  memcpy(server_DUID, DUID, 8);
  return SL_STATUS_OK;
}

sl_status_t sl_wisun_br_dhcpv6_server_stop(void) {
  dhcpv6_server_socket = -1;
  return SL_STATUS_OK;
}

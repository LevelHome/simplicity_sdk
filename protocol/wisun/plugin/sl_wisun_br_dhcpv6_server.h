/***************************************************************************//**
 * @file sl_wisun_br_dhcpv6_server.h
 * @brief Components that implements a DHCPv6 server for Wi-SUN Border Router
 *******************************************************************************
 * # License
 * <b>Copyright 2021 Silicon Laboratories Inc. www.silabs.com</b>
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
#ifndef __SL_WISUN_BR_DHCPV6_SERVER_H__
#define __SL_WISUN_BR_DHCPV6_SERVER_H__
#include "sl_status.h"
#include "sl_wisun_types.h"

/**  UDP Port Number definition */
#define DHCPV6_SERVER_PORT 547
#define DHCPV6_CLIENT_PORT 546

/***************************************************************************//**
 * Initialize DHCPv6 Server.
 *
 * @return SL_STATUS_OK if successful, an error code otherwise
 ******************************************************************************/
sl_status_t sl_wisun_br_dhcpv6_server_init(void);

/***************************************************************************//**
 * Start DHCPv6 Server.
 *
 * @param[in] socket DHCPv6 Server's socket
 * @param[in] prefix DHCPv6 Server's IPv6 prefix
 * @param[in] DUID DHCPv6 Server's DUID
 * @param[in] dhcp_address_lifetime Addresses' valid lifetime
 * @return SL_STATUS_OK if successful, an error code otherwise
 ******************************************************************************/
sl_status_t sl_wisun_br_dhcpv6_server_start(int socket, uint8_t prefix[8], uint8_t DUID[8], uint32_t dhcp_address_lifetime);

/***************************************************************************//**
 * Handle DHCPv6 relay-forward or solicit messages.
 *
 * @param[in] socket Message's socket
 * @param[in] buffer Message's buffer
 * @param[in] length Message length
 * @param[in] peer_address Peer's IPv6 address
 * @param[in] remote_port Port
 ******************************************************************************/
void sl_wisun_br_dhcpv6_server_on_recv(uint8_t *buffer, ssize_t length, in6_addr_t peer_address, in_port_t remote_port);

/***************************************************************************//**
 * Stop DHCPv6 Server.
 *
 * @return SL_STATUS_OK if successful, an error code otherwise
 ******************************************************************************/
sl_status_t sl_wisun_br_dhcpv6_server_stop(void);

#endif  /* __SL_WISUN_BR_DHCPV6_SERVER_H__ */

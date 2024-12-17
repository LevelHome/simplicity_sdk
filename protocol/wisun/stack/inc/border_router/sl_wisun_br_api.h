/***************************************************************************//**
 * @file sl_wisun_br_api.h
 * @brief Wi-SUN Border Router API
 *******************************************************************************
 * # License
 * <b>Copyright 2021 Silicon Laboratories Inc. www.silabs.com</b>
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

#ifndef SL_WISUN_BR_API_H
#define SL_WISUN_BR_API_H

#include "sl_wisun_types.h"
#include "sl_wisun_br_msg_api.h"
#include "sl_status.h"

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************//**
 * @addtogroup SL_WISUN_BR_API Border Router API
 * @ingroup SL_WISUN_API
 *
 * Wi-SUN Border Router API contains functions for Border Router specific
 * functionality. This API is an extension, not a replacement of @ref SL_WISUN_API.
 * An application implementing Border Router capability may utilize functions
 * from both APIs. See @ref API_AVAILABILITY for further information.
 *
 * @{
 *****************************************************************************/

/**************************************************************************//**
 * Start the Border Router.
 *
 * @param[in] name Name of the Wi-SUN network as a zero-terminated string
 * @param[in] phy_config Pointer to PHY configuration
 * @return SL_STATUS_OK if successful, an error code otherwise
 *****************************************************************************/
sl_status_t sl_wisun_br_start(const uint8_t *name, const sl_wisun_phy_config_t *phy_config);

/**************************************************************************//**
 * Stop the Border Router.
 *
 * @return SL_STATUS_OK if successful, an error code otherwise
 *****************************************************************************/
sl_status_t sl_wisun_br_stop(void);

/**************************************************************************//**
 * Set Group Transient Key.
 *
 * @param[in] gtk Group Transient Key
 * @param[in] index Index of the key from 0 to 3 for GTKs and 4 to 6 for LGTKs
 * @note This API is provided for test and debug purposes: setting a specific
 *       key in production is not recommended. Furthermore, using this API to
 *       set a GTK or LGTK disables the rotation of this key type so the
 *       application is responsible for setting new ones when suitable.
 * @return SL_STATUS_OK if successful, an error code otherwise
 *****************************************************************************/
sl_status_t sl_wisun_br_set_gtk(const uint8_t *gtk, uint8_t index);

/**************************************************************************//**
 * Set the size of the Wi-SUN network.
 *
 * @param[in] size Size of the network
 *    <br/><b>SL_WISUN_NETWORK_SIZE_AUTOMATIC</b>: network size is managed automatically
 *    <br/><b>SL_WISUN_NETWORK_SIZE_SMALL</b>: less than 100 nodes
 *    <br/><b>SL_WISUN_NETWORK_SIZE_MEDIUM</b>: 100 to 800 nodes
 *    <br/><b>SL_WISUN_NETWORK_SIZE_LARGE</b>: 800 to 1500 nodes
 * @return SL_STATUS_OK if successful, an error code otherwise
 *
 * This function sets the size of the network. The size is to used set various
 * stack parameters, such as timing parameters to optimize device behavior
 * in regards to node count. The device will function with any setting but
 * may exhibit non-optimal behavior. Setting the size too large may cause
 * slow connection speed and increased latency. Conversely, a value too
 * small may cause increased network traffic.
 *****************************************************************************/
sl_status_t sl_wisun_br_set_network_size(sl_wisun_network_size_t size);

/**************************************************************************//**
 * Get the unicast IPv6 addresses configured in the Border Router.
 *
 * @param[out] addr_ll Pointer to Link-Local address
 * @param[out] addr_gua Pointer to Unique Local Address or Global Unicast Address
 * @param[out] addr_dodagid Pointer to DODAG ID
 * @return SL_STATUS_OK if successful, an error code otherwise
 *****************************************************************************/
sl_status_t sl_wisun_br_get_ip_addresses(uint8_t *addr_ll, uint8_t *addr_gua, uint8_t *addr_dodagid);

/**************************************************************************//**
 * Get Group AES Key.
 *
 * @param[in] index Index of the key from 0 to 3 for GAKs and 4 to 6 for LFN GAKs
 * @param[out] gak Pointer to Group AES Key
 * @return SL_STATUS_OK if successful, an error code otherwise
 *
 * This function exports (L)GAK as a plaintext key.
 *
 * @note This functionality is only available when using the debug library.
 *****************************************************************************/
sl_status_t sl_wisun_br_get_gak(uint8_t index, uint8_t *gak);

/**************************************************************************//**
 * Get the Border Router state.
 *
 * @param[out] state Pointer to state
 *   - #SL_WISUN_BR_STATE_INITIALIZED: ready to start
 *   - #SL_WISUN_BR_STATE_OPERATIONAL: started
 * @return SL_STATUS_OK if successful, an error code otherwise
 *****************************************************************************/
sl_status_t sl_wisun_br_get_state(sl_wisun_br_state_t *state);

/**************************************************************************//**
 * Set the Border Router device certificate.
 *
 * @param[in] certificate_options Options for the certificate.
 *   <br/><b>SL_WISUN_CERTIFICATE_OPTION_APPEND</b>: Append the certificate to the list of server certificates
 *                                                   instead of replacing the previous entries
 *   <br/><b>SL_WISUN_CERTIFICATE_OPTION_IS_REF</b>: The application guarantees the certificate data will remain
 *                                                   in scope and can therefore be referenced instead of copied
 *   <br/><b>SL_WISUN_CERTIFICATE_OPTION_HAS_KEY</b>: The certificate has a private key
 * @param[in] certificate_length Size of the certificate data
 * @param[in] certificate Pointer to the certificate data
 * @return SL_STATUS_OK if successful, an error code otherwise
 *****************************************************************************/
sl_status_t sl_wisun_br_set_device_certificate(uint16_t certificate_options,
                                               uint16_t certificate_length,
                                               const uint8_t *certificate);

/**
 * Define sl_wisun_set_br_device_certificate for legacy.
 */
#define sl_wisun_set_br_device_certificate  sl_wisun_br_set_device_certificate

/**************************************************************************//**
 * Set broadcast settings.
 *
 * @param[in] interval_ms Broadcast Interval (BDI < BI <= 16777215 ms)
 *                        (default: 1020 ms)
 * @param[in] dwell_interval_ms Broadcast Dwell Interval (100-255 ms)
 *                              (default: 255 ms)
 * @return SL_STATUS_OK if successful, an error code otherwise
 *
 * This function sets the broadcast channel hopping schedule. The
 * Broadcast Interval is the duration, between Broadcast Dwell Intervals
 * within the advertising node’s broadcast schedule. Its resolution is
 * limited to 24 bits (16777215 ms). The Broadcast Dwell Interval specifies
 * the duration during which the node may transmit a broadcast message.
 * It is recommended a BI value of 4.0 times the maximum of (UDI, BDI).
 * These values affect the whole PAN, since they are propagated from
 * parent to children. This function must be called before starting the
 * Border Router.
 *****************************************************************************/
sl_status_t sl_wisun_br_set_broadcast_settings(uint32_t interval_ms,
                                               uint8_t dwell_interval_ms);

/**************************************************************************//**
 * Configure the Border Router parameter set.
 *
 * @param[in] params Parameter set to use
 * @return SL_STATUS_OK if successful, an error code otherwise
 *
 * This functions sets the Border Router parameter set. These parameters
 * impact connection time, bandwidth usage, and latency. Use of a predefined
 * parameter set is recommended (@ref SL_WISUN_BR_PARAMETER_SETS).
 * This function must be called before starting the Border Router.
 *****************************************************************************/
sl_status_t sl_wisun_br_set_connection_parameters(const sl_wisun_br_connection_params_t *params);

/**************************************************************************//**
 * Configure LFN parenting parameter set.
 *
 * @param[in] params Parameter set to use
 * @return SL_STATUS_OK if successful, an error code otherwise
 *
 * This function sets the LFN parenting parameter set. These parameters
 * impact battery life of all LFNs in the PAN. Use of a predefined parameter
 * set is recommended (@ref SL_WISUN_BR_LFN_PARAMETER_SETS). This function
 * must be called before starting the Border Router.
 *****************************************************************************/
sl_status_t sl_wisun_br_set_lfn_parameters(const sl_wisun_br_lfn_params_t *params);

/**************************************************************************//**
 * Configure LFN parenting support.
 *
 * @param[in] lfn_limit Maximum number of LFN children
 *   - **0**: LFN parenting is disabled in the Border Router
 *   - **1 - 10**: Maximum number of LFN children the Border Router can parent
 * @param[in] lfn_support_pan  LFN parenting support in the PAN
 *   - **true**: LFN parenting is enabled in the PAN
 *   - **false**: LFN parenting is disabled in the PAN
 * @return SL_STATUS_OK if successful, an error code otherwise
 *
 * This function configures LFN parenting support in the Border Router
 * and in the PAN.
 *****************************************************************************/
sl_status_t sl_wisun_br_set_lfn_support(uint8_t lfn_limit, bool lfn_support_pan);

/**************************************************************************//**
 * Set DHCPv6 Vendor Data
 *
 * @param[in] enterprise_number Vendor Specific Enterprise Number
 * @param[in] data_length Length of Vendor specific data
 * @param[in] data Vendor specific data
 * @return SL_STATUS_OK if successful, an error code otherwise
 *
 * This function sets the vendor-specific information transmitted during
 * DHCPv6 exchange. This function must be called before starting the
 * Border Router.
 *****************************************************************************/
sl_status_t sl_wisun_br_set_dhcp_vendor_data(uint32_t enterprise_number, uint16_t data_length, const uint8_t *data);

/**************************************************************************//**
 * Start or stop PAN Defect advertisement.
 *
 * @param[in] defect_advertise True to start PAN Defect procedure,
 *                             false to stop it.
 * @param[in] min_scan_duration_s Minimum duration after which devices
 *                                are allowed to switch their PAN if an
 *                                alternative one is available.
 * @param[in] max_scan_duration_s Duration after which devices should
 *                                systemically switch to the alternative
 *                                PAN if one is available.
 * @return SL_STATUS_OK if successful, an error code otherwise.
 *
 * @note The values for `min_scan_duration_s` and `max_scan_duration_s` should
 *       be chosen based on the estimated performance of the backup power source.
 *       Specifically, `max_scan_duration_s` should be set to a value smaller than
 *       the estimated lifetime of the backup power source to ensure a seamless
 *       transition to the alternative PAN without long connection interruptions.
 *****************************************************************************/
sl_status_t sl_wisun_br_pan_defect_advertise(bool defect_advertise,
                                             uint32_t min_scan_duration_s,
                                             uint32_t max_scan_duration_s);

/**************************************************************************//**
 * Set IPv6 prefix for DODAG.
 *
 * @param[in] ipv6_prefix IPv6 prefix
 * @param[in] prefix_length Prefix length. Currently, only 64 bit prefixes are
 *                          supported.
 * @note  Must not be called after the border router is started
 * @return SL_STATUS_OK if successful, an error code otherwise
 *
 * This function sets the prefix used to generate IP addresses for RPL traffic
 * (DODAGID will derive from it). This function must be called before starting
 * the Border Router.
 *****************************************************************************/
sl_status_t sl_wisun_br_set_ipv6_prefix(const uint8_t *ipv6_prefix, uint8_t prefix_length);

/**************************************************************************//**
 * Register a function to handle IPv6 packets received from Wi-SUN network.
 *
 * @param[in] handler Pointer to handler function
 * @return SL_STATUS_OK if successful, an error code otherwise
 *
 * This function registers a handler function that will be called when the
 * destination address of a received IPv6 packet is outside the DODAG prefix.
 * Setting the handler function to NULL disables the forwarding. This
 * function is used for handling uplink packets towards the backhaul
 * interface, see sl_wisun_br_ipv6_down() for downlink.
 *****************************************************************************/
sl_status_t sl_wisun_br_set_ipv6_up_handler(sl_wisun_ipv6_up_handler_t handler);

/**************************************************************************//**
 * Forward an IPv6 packet to Wi-SUN network.
 *
 * @param[in] data Pointer to IPv6 header
 * @param[out] data_length Length of IPv6 packet data in bytes
 * @return SL_STATUS_OK if successful, an error code otherwise
 *
 * This function forwards an IPv6 packet to Wi-SUN network. The destination
 * address must belong to the DODAG prefix. This function is used for handling
 * downlink packets from the backhaul interface, see
 * sl_wisun_br_set_ipv6_up_handler() for uplink. It's recommended to use
 * sl_wisun_br_ipv6_route_exists() before forwarding a packet.
 *****************************************************************************/
sl_status_t sl_wisun_br_ipv6_down(const uint8_t* data, size_t data_length);

/**************************************************************************//**
 * Check if the IPv6 destination address is routable in the Wi-SUN network.
 *
 * @param[in] ipv6_address Pointer to IPv6 destination address
 * @param[out] exists Whether the address is routable
 *   - **true**: address is routable
 *   - **false**: address is not routable
 * @return SL_STATUS_OK if successful, an error code otherwise
 *
 * This function checks if the given IPv6 address is a valid destination
 * in the Wi-SUN network. The destination address must belong to the
 * Border Router or one of the connected nodes.
 *****************************************************************************/
sl_status_t sl_wisun_br_ipv6_route_exists(const uint8_t *ipv6_address, bool *exists);

/** @} (end addtogroup SL_WISUN_BR_API) */

#ifdef __cplusplus
}
#endif

#endif  // SL_WISUN_BR_API_H

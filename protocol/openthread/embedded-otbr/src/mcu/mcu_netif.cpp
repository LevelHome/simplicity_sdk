/*******************************************************************************
 * @file
 * @brief Network interface
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

#include <openthread/logging.h>
#include <openthread/openthread-system.h>

#include <net/if.h>

#include "cmsis_os2.h"
#include "sl_cmsis_os2_common.h"
#include "sl_component_catalog.h"

#include "platform-si91x.h"

#define WIFI_SSID "SILABS_OTBR"
#define WIFI_PASSWD "123456789"

#ifdef __cplusplus
extern "C" {
#endif

#include "sl_net.h"
#include "sl_status.h"
#include "sl_utility.h"
#include "sl_wifi.h"
#include "sl_wifi_callback_framework.h"

#include "sl_board_configuration.h"
#include "sl_net_for_lwip.h"
#include "sl_net_wifi_types.h"
#include "sl_si91x_driver.h"

#include "lwip/dhcp.h"
#include "lwip/etharp.h"
#include "lwip/ethip6.h"
#include "lwip/ip6_addr.h"
#include "lwip/netif.h"
#include "lwip/netifapi.h"
#include "lwip/tcpip.h"
#include "lwip/timeouts.h"

unsigned int gNetifIndex = 1;
char         gNetifName[IFNAMSIZ];

static const sl_wifi_device_configuration_t otWifiConfig = {
    LOAD_NWP_FW,               // boot_option
    NULL,                      // mac_address
    SL_SI91X_WIFI_BAND_2_4GHZ, // band
    US,                        // region_code
    {
        // boot_config
        SL_SI91X_CLIENT_MODE,    // oper_mode
        SL_SI91X_WLAN_ONLY_MODE, // coex_mode
        (SL_SI91X_FEAT_SECURITY_OPEN | SL_SI91X_FEAT_AGGREGATION | SL_SI91X_FEAT_ULP_GPIO_BASED_HANDSHAKE
#ifdef SLI_SI91X_MCU_INTERFACE
         | SL_SI91X_FEAT_WPS_DISABLE
#endif
         ), // feature_bit_map
        (SL_SI91X_TCP_IP_FEAT_BYPASS | SL_SI91X_TCP_IP_FEAT_DHCPV4_CLIENT
#if SL_CATALOG_OPENTHREAD_BORDER_ROUTER_PRESENT
         | SL_SI91X_TCP_IP_FEAT_DHCPV6_CLIENT | SL_SI91X_TCP_IP_FEAT_IPV6
#endif
         | SL_SI91X_TCP_IP_FEAT_DNS_CLIENT | SL_SI91X_TCP_IP_FEAT_EXTENSION_VALID), // tcp_ip_feature_bit_map
        SL_SI91X_CUSTOM_FEAT_EXTENTION_VALID,                                       // custom_feature_bit_map
        (SL_SI91X_EXT_FEAT_LOW_POWER_MODE | SL_SI91X_EXT_FEAT_XTAL_CLK | SL_SI91X_EXT_FEAT_UART_SEL_FOR_DEBUG_PRINTS
         | MEMORY_CONFIG
#ifdef SLI_SI917
         | SL_SI91X_EXT_FEAT_FRONT_END_SWITCH_PINS_ULP_GPIO_4_5_0
#endif
         ),                                   // ext_custom_feature_bit_map
        0,                                    // bt_feature_bit_map
        SL_SI91X_CONFIG_FEAT_EXTENTION_VALID, // ext_tcp_ip_feature_bit_map
        0,                                    // ble_feature_bit_map
        0,                                    // ble_ext_feature_bit_map
        SL_SI91X_FEAT_SLEEP_GPIO_SEL_BITMAP   // config_feature_bit_map
    }};

#if SL_CATALOG_OPENTHREAD_BORDER_ROUTER_PRESENT

static sl_net_wifi_lwip_context_t otWifiContext;

static void otWifiApp(void *arg);

static sl_net_wifi_client_profile_t otWifiProfile = {
    {{WIFI_SSID, sizeof(WIFI_SSID) - 1},                                // ssid
     {SL_WIFI_AUTO_CHANNEL, SL_WIFI_AUTO_BAND, SL_WIFI_AUTO_BANDWIDTH}, // channel
     {{0}},                                                             // bssid
     SL_WIFI_BSS_TYPE_INFRASTRUCTURE,                                   // bss_type
     SL_WIFI_WPA2,                                                      // security
     SL_WIFI_DEFAULT_ENCRYPTION,                                        // encryption
     (sl_wifi_client_flag_t)0,
     SL_NET_DEFAULT_WIFI_CLIENT_CREDENTIAL_ID},
    {SL_IP_MANAGEMENT_DHCP, SL_IPV4, NULL, {{{0}}}}};

const osThreadAttr_t otWifiThreadAttributes = {
    "WiFiApp",
    0,
    0,
    0,
    0,
    4096,
    osPriorityLow,
    0,
    0,
};
#endif /*SL_CATALOG_OPENTHREAD_BORDER_ROUTER_PRESENT*/

void platformNetifInitialize()
{
    int32_t status;

    otLogPlat(OT_LOG_LEVEL_INFO, "app", "Init WiFi driver...");
#if SL_CATALOG_OPENTHREAD_BORDER_ROUTER_PRESENT
    status = sl_net_init((sl_net_interface_t)SL_NET_WIFI_CLIENT_INTERFACE, &otWifiConfig, &otWifiContext, NULL);
#else
    status = sl_net_init((sl_net_interface_t)SL_NET_WIFI_CLIENT_INTERFACE, &otWifiConfig, NULL, NULL);
#endif
    if (status != SL_STATUS_OK)
    {
        otLogPlat(OT_LOG_LEVEL_INFO, "app", "Failed to start Wi-Fi Client interface: 0x%lx", status);
        return;
    }

    otLogPlat(OT_LOG_LEVEL_INFO, "app", "Wi-Fi Client interface success");
}

const char *otSysGetThreadNetifName(void)
{
    return gNetifName;
}

unsigned int otSysGetThreadNetifIndex(void)
{
    return gNetifIndex;
}

#if SL_CATALOG_OPENTHREAD_BORDER_ROUTER_PRESENT

void platformNetifStart(void)
{
    osThreadNew((osThreadFunc_t)otWifiApp, NULL, &otWifiThreadAttributes);
}

bool platformNetifHasIPv6(void)
{
    return ip6_addr_ispreferred(netif_ip6_addr_state(&otWifiContext.netif, 0));
}

static sl_status_t ap_connected_event_handler(sl_wifi_event_t event, void *data, uint32_t data_length, void *arg)
{
    UNUSED_PARAMETER(data);
    UNUSED_PARAMETER(data_length);
    UNUSED_PARAMETER(arg);
    UNUSED_PARAMETER(event);

    otLogPlat(OT_LOG_LEVEL_INFO, "app", "Wi-Fi connected");

    return SL_STATUS_OK;
}

static sl_status_t ap_disconnected_event_handler(sl_wifi_event_t event, void *data, uint32_t data_length, void *arg)
{
    UNUSED_PARAMETER(data);
    UNUSED_PARAMETER(data_length);
    UNUSED_PARAMETER(arg);
    UNUSED_PARAMETER(event);

    otLogPlat(OT_LOG_LEVEL_INFO, "app", "Wi-Fi disconnected");

    return SL_STATUS_OK;
}

static void platformNetifLinkUp(sl_net_wifi_client_profile_t *profile)
{
    if (SL_IP_MANAGEMENT_STATIC_IP == profile->ip.mode)
    {
        ip4_addr_t ipaddr  = {0};
        ip4_addr_t gateway = {0};
        ip4_addr_t netmask = {0};
        uint8_t   *address = &(profile->ip.ip.v4.ip_address.bytes[0]);

        IP4_ADDR(&ipaddr, address[0], address[1], address[2], address[3]);
        address = &(profile->ip.ip.v4.gateway.bytes[0]);
        IP4_ADDR(&gateway, address[0], address[1], address[2], address[3]);
        address = &(profile->ip.ip.v4.netmask.bytes[0]);
        IP4_ADDR(&netmask, address[0], address[1], address[2], address[3]);

        netifapi_netif_set_addr(&(otWifiContext.netif), &ipaddr, &netmask, &gateway);
    }

    netifapi_netif_set_up(&(otWifiContext.netif));
    netifapi_netif_set_link_up(&(otWifiContext.netif));

    if (SL_IP_MANAGEMENT_DHCP == profile->ip.mode)
    {
#if LWIP_IPV4 && LWIP_DHCP
        ip_addr_set_zero_ip4(&(otWifiContext.netif.ip_addr));
        ip_addr_set_zero_ip4(&(otWifiContext.netif.netmask));
        ip_addr_set_zero_ip4(&(otWifiContext.netif.gw));
        dhcp_start(&(otWifiContext.netif));
#endif /* LWIP_IPV4 && LWIP_DHCP */
        /*
         * Enable IPV6
         */
        //! Wait for DHCP to acquire IP Address
        while (!dhcp_supplied_address(&(otWifiContext.netif)))
        {
            osDelay(100);
        }
    }

#if LWIP_IPV6
#if LWIP_IPV6_AUTOCONFIG
    otWifiContext.netif.ip6_autoconfig_enabled = 1;
#endif /* LWIP_IPV6_AUTOCONFIG */
    netif_create_ip6_linklocal_address(&(otWifiContext.netif), 1);
#endif
    return;
}

static void otWifiApp(void *argument)
{
    UNUSED_PARAMETER(argument);
    sl_status_t                   status;
    sl_wifi_performance_profile_t otWifiPerformance = {HIGH_PERFORMANCE, 0, 0, 0, 0};

    otLogPlat(OT_LOG_LEVEL_INFO, "app", "Wi-Fi app enter");

    sl_net_set_profile((sl_net_interface_t)SL_NET_WIFI_CLIENT_INTERFACE,
                       SL_NET_DEFAULT_WIFI_CLIENT_PROFILE_ID,
                       &otWifiProfile);
    sl_wifi_set_callback(SL_WIFI_CLIENT_CONNECTED_EVENTS, ap_connected_event_handler, NULL);
    sl_wifi_set_callback(SL_WIFI_CLIENT_DISCONNECTED_EVENTS, ap_disconnected_event_handler, NULL);
    sl_net_set_credential(SL_NET_DEFAULT_WIFI_CLIENT_CREDENTIAL_ID,
                          SL_NET_WIFI_PSK,
                          WIFI_PASSWD,
                          sizeof(WIFI_PASSWD) - 1);

#ifdef SLI_SI91X_MCU_INTERFACE
    uint8_t xtalEnable = 1;
    status             = sl_si91x_m4_ta_secure_handshake(SL_SI91X_ENABLE_XTAL, 1, &xtalEnable, 0, NULL);
    if (status != SL_STATUS_OK)
    {
        otLogPlat(OT_LOG_LEVEL_INFO, "app", "Failed to bring m4_ta_secure_handshake: 0x%lx", status);
        return;
    }
    otLogPlat(OT_LOG_LEVEL_INFO, "app", "sl_si91x_m4_ta_secure_handshake success");
#endif

    status = sl_wifi_connect(SL_WIFI_CLIENT_INTERFACE, &otWifiProfile.config, 10000);
    if (status != SL_STATUS_OK)
    {
        otLogPlat(OT_LOG_LEVEL_INFO, "app", "Failed to bring Wi-Fi client interface up: 0x%lx", status);
        return;
    }
    otLogPlat(OT_LOG_LEVEL_INFO, "app", "Wi-Fi client connected");

    platformNetifLinkUp(&otWifiProfile);

    status = sl_wifi_filter_broadcast(5000, 1, 1);
    if (status != SL_STATUS_OK)
    {
        otLogPlat(OT_LOG_LEVEL_INFO, "app", "sl_wifi_filter_broadcast Failed, Error Code : 0x%lX", status);
        return;
    }
    otLogPlat(OT_LOG_LEVEL_INFO, "app", "Filter Broadcast Done");

    // set performance profile
    status = sl_wifi_set_performance_profile(&otWifiPerformance);
    if (status != SL_STATUS_OK)
    {
        otLogPlat(OT_LOG_LEVEL_INFO, "app", "Power save configuration Failed, Error Code : 0x%lX", status);
        return;
    }
    otLogPlat(OT_LOG_LEVEL_INFO, "app", "Performance profile set");

    while (1)
    {
        // if (platformNetifHasIPv6())
        // {
        //     char addrStr[46] = { 0 };
        //     if (ip6addr_ntoa_r(netif_ip6_addr(&otWifiContext.netif, 0), addrStr, sizeof(addrStr)) != NULL)
        //     {
        //         otLogPlat(OT_LOG_LEVEL_INFO, "app", "SLAAC OK: linklocal addr: %s", addrStr);
        //     }
        // }
        // else
        // {
        //     otLogPlat(OT_LOG_LEVEL_INFO, "app", "IPv6 Fail");
        // }
        sys_check_timeouts();
        // osDelay(1000);
    }
}

#endif /*SL_CATALOG_OPENTHREAD_BORDER_ROUTER_PRESENT*/
#ifdef __cplusplus
}
#endif

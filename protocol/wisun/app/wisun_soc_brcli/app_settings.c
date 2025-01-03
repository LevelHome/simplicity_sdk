/***************************************************************************//**
 * @file app_settings.c
 * @brief Application settings handler
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sl_wisun_ip6string.h"
#include "nvm3.h"
#include "app.h"
#include "app_settings.h"
#include "border_router/sl_wisun_br_api.h"
#include "sl_wisun_default_phy.h"
#include "sl_wisun_cli_util.h"
#include "sl_wisun_types.h"
#include "sl_wisun_trace_api.h"
#include "sl_wisun_keychain.h"
#include "rail_features.h"
#include "sl_memory_manager.h"

#ifdef SL_CATALOG_WISUN_BR_WIFI_PRESENT
#include "sl_wisun_br_wifi.h"
#endif

#ifdef WISUN_FAN_CERTIFICATION

  // Defaults for Wi-SUN FAN Certification.

  #define APP_SETTINGS_WISUN_DEFAULT_NETWORK_NAME  "WiSUN PAN"
  #define APP_SETTINGS_WISUN_DEFAULT_PHY_CONFIG_TYPE  SL_WISUN_PHY_CONFIG_FAN11
  #define APP_SETTINGS_WISUN_DEFAULT_REGULATORY_DOMAIN  1 // NA
  #define APP_SETTINGS_WISUN_DEFAULT_OPERATING_CLASS  1
  #define APP_SETTINGS_WISUN_DEFAULT_OPERATING_MODE  0x1b
  #define APP_SETTINGS_WISUN_DEFAULT_CHAN_PLAN_ID  1
  #define APP_SETTINGS_WISUN_DEFAULT_PHY_MODE_ID  2
  #define APP_SETTINGS_WISUN_DEFAULT_NETWORK_SIZE  SL_WISUN_NETWORK_SIZE_CERTIFICATION
  #define APP_SETTINGS_WISUN_DEFAULT_ALLOWED_CHANNELS  "0"

#elif defined SL_CATALOG_WISUN_CONFIG_PRESENT

  // Defaults from Wi-SUN Configurator.

  #include "sl_wisun_config.h"

  #ifdef WISUN_CONFIG_NETWORK_NAME
    #define APP_SETTINGS_WISUN_DEFAULT_NETWORK_NAME  WISUN_CONFIG_NETWORK_NAME
  #endif
  #ifdef WISUN_CONFIG_DEFAULT_PHY_FAN10
    #define APP_SETTINGS_WISUN_DEFAULT_PHY_CONFIG_TYPE  SL_WISUN_PHY_CONFIG_FAN10
    #define APP_SETTINGS_WISUN_DEFAULT_REGULATORY_DOMAIN  WISUN_CONFIG_REGULATORY_DOMAIN
    #define APP_SETTINGS_WISUN_DEFAULT_OPERATING_CLASS  WISUN_CONFIG_OPERATING_CLASS
    #define APP_SETTINGS_WISUN_DEFAULT_OPERATING_MODE  WISUN_CONFIG_OPERATING_MODE
  #elif defined WISUN_CONFIG_DEFAULT_PHY_FAN11
    #define APP_SETTINGS_WISUN_DEFAULT_PHY_CONFIG_TYPE  SL_WISUN_PHY_CONFIG_FAN11
    #define APP_SETTINGS_WISUN_DEFAULT_REGULATORY_DOMAIN  WISUN_CONFIG_REGULATORY_DOMAIN
    #define APP_SETTINGS_WISUN_DEFAULT_CHAN_PLAN_ID  WISUN_CONFIG_CHANNEL_PLAN_ID
    #define APP_SETTINGS_WISUN_DEFAULT_PHY_MODE_ID  WISUN_CONFIG_PHY_MODE_ID
  #endif
  #ifdef WISUN_CONFIG_NETWORK_SIZE
    #define APP_SETTINGS_WISUN_DEFAULT_NETWORK_SIZE  WISUN_CONFIG_NETWORK_SIZE
  #endif
  #ifdef WISUN_CONFIG_ALLOWED_CHANNELS
    #define APP_SETTINGS_WISUN_DEFAULT_ALLOWED_CHANNELS  WISUN_CONFIG_ALLOWED_CHANNELS
  #endif
  #ifdef WISUN_CONFIG_TX_POWER
    #define APP_SETTINGS_WISUN_DEFAULT_TX_POWER_DDBM  (WISUN_CONFIG_TX_POWER * 10)
  #endif
  #ifdef WISUN_CONFIG_MODE_SWITCH_PHYS
    #define APP_SETTINGS_WISUN_DEFAULT_RX_PHY_MODE_IDS  WISUN_CONFIG_MODE_SWITCH_PHYS
  #endif
  #ifdef WISUN_CONFIG_MODE_SWITCH_PHYS_NUMBER
    #define APP_SETTINGS_WISUN_DEFAULT_RX_PHY_MODE_IDS_COUNT  WISUN_CONFIG_MODE_SWITCH_PHYS_NUMBER
  #endif
  #ifdef WISUN_CONFIG_DEVICE_PROFILE
    #define APP_SETTINGS_WISUN_DEFAULT_LFN_PROFILE  WISUN_CONFIG_DEVICE_PROFILE
  #endif

#endif

// Defaults from wisun_default_phy component.

#include "sl_wisun_default_phy.h"

#ifndef APP_SETTINGS_WISUN_DEFAULT_NETWORK_NAME
  #define APP_SETTINGS_WISUN_DEFAULT_NETWORK_NAME  "Wi-SUN Network"
#endif
#ifndef APP_SETTINGS_WISUN_DEFAULT_PHY_CONFIG_TYPE
  #define APP_SETTINGS_WISUN_DEFAULT_PHY_CONFIG_TYPE  SL_WISUN_PHY_CONFIG_FAN11
#endif
#ifndef APP_SETTINGS_WISUN_DEFAULT_REGULATORY_DOMAIN
  #define APP_SETTINGS_WISUN_DEFAULT_REGULATORY_DOMAIN  SL_WISUN_DEFAULT_REGULATORY_DOMAIN
#endif
#ifndef APP_SETTINGS_WISUN_DEFAULT_OPERATING_CLASS
  #define APP_SETTINGS_WISUN_DEFAULT_OPERATING_CLASS  SL_WISUN_DEFAULT_OPERATING_CLASS
#endif
#ifndef APP_SETTINGS_WISUN_DEFAULT_OPERATING_MODE
  #define APP_SETTINGS_WISUN_DEFAULT_OPERATING_MODE  SL_WISUN_DEFAULT_OPERATING_MODE
#endif
#ifndef APP_SETTINGS_WISUN_DEFAULT_CHAN_PLAN_ID
  #define APP_SETTINGS_WISUN_DEFAULT_CHAN_PLAN_ID  SL_WISUN_DEFAULT_CHAN_PLAN_ID
#endif
#ifndef APP_SETTINGS_WISUN_DEFAULT_PHY_MODE_ID
  #define APP_SETTINGS_WISUN_DEFAULT_PHY_MODE_ID  SL_WISUN_DEFAULT_PHY_MODE_ID
#endif

#ifndef APP_SETTINGS_WISUN_DEFAULT_NETWORK_SIZE
  #define APP_SETTINGS_WISUN_DEFAULT_NETWORK_SIZE  SL_WISUN_NETWORK_SIZE_SMALL
#endif
#ifndef APP_SETTINGS_WISUN_DEFAULT_ALLOWED_CHANNELS
  #define APP_SETTINGS_WISUN_DEFAULT_ALLOWED_CHANNELS  "0-255"
#endif
#ifndef APP_SETTINGS_WISUN_DEFAULT_TX_POWER_DDBM
  #define APP_SETTINGS_WISUN_DEFAULT_TX_POWER_DDBM  200
#endif
#ifndef APP_SETTINGS_WISUN_DEFAULT_RX_PHY_MODE_IDS
  #define APP_SETTINGS_WISUN_DEFAULT_RX_PHY_MODE_IDS  { 0 }
#endif
#ifndef APP_SETTINGS_WISUN_DEFAULT_RX_PHY_MODE_IDS_COUNT
  #define APP_SETTINGS_WISUN_DEFAULT_RX_PHY_MODE_IDS_COUNT  0
#endif
#ifndef APP_SETTINGS_WISUN_DEFAULT_LFN_PROFILE
  #define APP_SETTINGS_WISUN_DEFAULT_LFN_PROFILE SL_WISUN_LFN_PROFILE_TEST
#endif

#define APP_SETTINGS_WISUN_DEFAULT_UC_DWELL_INTERVAL  255
#define APP_SETTINGS_WISUN_DEFAULT_BC_INTERVAL  1020
#define APP_SETTINGS_WISUN_DEFAULT_BC_DWELL_INTERVAL  255
#define APP_SETTINGS_WISUN_DEFAULT_CH0_FREQUENCY  863100
#define APP_SETTINGS_WISUN_DEFAULT_NUMBER_OF_CHANNELS  69
#define APP_SETTINGS_WISUN_DEFAULT_CHANNEL_SPACING  100
#define APP_SETTINGS_WISUN_DEFAULT_REGULATION  SL_WISUN_REGULATION_NONE
#define APP_SETTINGS_WISUN_DEFAULT_DEVICE_TYPE SL_WISUN_BORDER_ROUTER
#define APP_SETTINGS_WISUN_DEFAULT_CRC_TYPE SL_WISUN_4_BYTES_CRC
#define APP_SETTINGS_WISUN_DEFAULT_STF_LENGTH 4
#define APP_SETTINGS_WISUN_DEFAULT_PREAMBLE_LENGTH 56
#define APP_SETTINGS_WISUN_NEIGHBOR_TABLE_SIZE 22
#define APP_SETTINGS_WISUN_DEFAULT_KEYCHAIN SL_WISUN_KEYCHAIN_AUTOMATIC
#define APP_SETTINGS_WISUN_DEFAULT_KEYCHAIN_INDEX 0
#define APP_SETTINGS_WISUN_DEFAULT_IPV6_PREFIX  "fd12:3456::/64"
#define APP_SETTINGS_WISUN_DEFAULT_DHCPV6_SERVER  ""

#ifndef APP_SETTINGS_APP_DEFAULT_AUTOSTART
  #define APP_SETTINGS_APP_DEFAULT_AUTOSTART  false
#endif

#define SL_WISUN_WIFI_DEFAULT_SSID  "DEFAULT_SSID"
#define SL_WISUN_WIFI_DEFAULT_PASSPHRASE  "DEFAULT_PASSPHRASE"
#define SL_WISUN_WIFI_DEFAULT_SECURITY_TYPE  WIFI_SECURITY_TYPE_WPA2_CCMP

typedef enum
{
  app_settings_domain_wisun       = 0x00,
  app_settings_domain_statistics  = 0x01,
  app_settings_domain_app         = 0x02,
  app_settings_domain_info        = 0x03,
  app_settings_domain_ping        = 0x04,
  app_settings_domain_wifi        = 0x05,
} app_settings_domain_t;

const char *app_settings_domain_str[] =
{
  "wisun",
  "statistics",
  "app",
  "info",
  "ping",
  "wifi",
  NULL,
};

typedef enum
{
  app_statistics_domain_phy        = 0x00,
  app_statistics_domain_mac        = 0x01,
  app_statistics_domain_fhss       = 0x02,
  app_statistics_domain_wisun      = 0x03,
  app_statistics_domain_network    = 0x04,
  app_statistics_domain_regulation = 0x05,
  app_statistics_domain_heap       = 0x06
} app_statistics_domain_t;

const char *app_statistics_domain_str[] =
{
  "phy",
  "mac",
  "fhss",
  "wisun",
  "network",
  "regulation",
  "heap",
  NULL,
};

typedef enum
{
  app_info_domain_network = 0x00,
  app_info_domain_rpl     = 0x01
} app_info_domain_t;

const char *app_info_domain_str[] =
{
  "network",
  "rpl",
  NULL,
};

static const app_settings_wisun_t app_settings_wisun_default = {
  .network_name = APP_SETTINGS_WISUN_DEFAULT_NETWORK_NAME,
  .regulatory_domain = APP_SETTINGS_WISUN_DEFAULT_REGULATORY_DOMAIN,
  .operating_class = APP_SETTINGS_WISUN_DEFAULT_OPERATING_CLASS,
  .operating_mode = APP_SETTINGS_WISUN_DEFAULT_OPERATING_MODE,
  .network_size = APP_SETTINGS_WISUN_DEFAULT_NETWORK_SIZE,
  .tx_power_ddbm = APP_SETTINGS_WISUN_DEFAULT_TX_POWER_DDBM,
  .uc_dwell_interval_ms = APP_SETTINGS_WISUN_DEFAULT_UC_DWELL_INTERVAL,
  .bc_interval_ms = APP_SETTINGS_WISUN_DEFAULT_BC_INTERVAL,
  .bc_dwell_interval_ms = APP_SETTINGS_WISUN_DEFAULT_BC_DWELL_INTERVAL,
  .gtks = {
    {0xBB, 0x06, 0x08, 0x57, 0x2C, 0xE1, 0x4D, 0x7B, 0xA2, 0xD1, 0x55, 0x49, 0x9C, 0xC8, 0x51, 0x9B},  // GTK[0] from Conformance Test Plan.
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
  },
  .lgtks = {
    {0xA6, 0xE9, 0xAE, 0x41, 0x56, 0xB2, 0x74, 0xC3, 0x6B, 0x49, 0x40, 0xC7, 0xDA, 0x2B, 0xA3, 0x09},  // LGTK[0] from Conformance Test Plan.
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
  },
  .gtk_set = 0x11,
  .state = SL_WISUN_BR_STATE_INITIALIZED,
  .channel_spacing = APP_SETTINGS_WISUN_DEFAULT_CHANNEL_SPACING,
  .number_of_channels = APP_SETTINGS_WISUN_DEFAULT_NUMBER_OF_CHANNELS,
  .ch0_frequency = APP_SETTINGS_WISUN_DEFAULT_CH0_FREQUENCY,
  .allowed_channels = APP_SETTINGS_WISUN_DEFAULT_ALLOWED_CHANNELS,
  .trace_filter = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
  .ipv6_prefix = APP_SETTINGS_WISUN_DEFAULT_IPV6_PREFIX,
  .dhcpv6_server = APP_SETTINGS_WISUN_DEFAULT_DHCPV6_SERVER,
  .regulation = APP_SETTINGS_WISUN_DEFAULT_REGULATION,
  .chan_plan_id = APP_SETTINGS_WISUN_DEFAULT_CHAN_PLAN_ID,
  .phy_mode_id = APP_SETTINGS_WISUN_DEFAULT_PHY_MODE_ID,
  .phy_config_type = APP_SETTINGS_WISUN_DEFAULT_PHY_CONFIG_TYPE,
  .rx_phy_mode_ids = APP_SETTINGS_WISUN_DEFAULT_RX_PHY_MODE_IDS,
  .rx_phy_mode_ids_count = APP_SETTINGS_WISUN_DEFAULT_RX_PHY_MODE_IDS_COUNT,
  .lfn_profile = APP_SETTINGS_WISUN_DEFAULT_LFN_PROFILE,
  .crc_type = APP_SETTINGS_WISUN_DEFAULT_CRC_TYPE,
  .stf_length = APP_SETTINGS_WISUN_DEFAULT_STF_LENGTH,
  .preamble_length = APP_SETTINGS_WISUN_DEFAULT_PREAMBLE_LENGTH,
  .neighbor_table_size = APP_SETTINGS_WISUN_NEIGHBOR_TABLE_SIZE,
  .keychain = APP_SETTINGS_WISUN_DEFAULT_KEYCHAIN,
  .keychain_index = APP_SETTINGS_WISUN_DEFAULT_KEYCHAIN_INDEX
};

static const app_settings_ping_t app_settings_ping_default = {
  .identifier = 1,
  .sequence_number = 1,
  .packet_interval = 1000,
  .packet_length = 40,
  .pattern = "0123456789"
};

static const app_settings_app_t app_settings_app_default = {
  .printable_data_as_hex = false,
  .printable_data_length = APP_UTIL_PRINTABLE_DATA_MAX_LENGTH,
  .autostart = APP_SETTINGS_APP_DEFAULT_AUTOSTART,
  .pti_state = true
};

static const app_settings_wifi_t app_settings_wifi_default = {
  .ssid = SL_WISUN_WIFI_DEFAULT_SSID,
  .passphrase = SL_WISUN_WIFI_DEFAULT_PASSPHRASE,
#ifdef SL_CATALOG_WISUN_BR_WIFI_PRESENT
  .security_type = SL_WISUN_WIFI_DEFAULT_SECURITY_TYPE
#endif
};

app_settings_wisun_t app_settings_wisun;
app_settings_ping_t app_settings_ping;
app_settings_app_t app_settings_app;
app_settings_wifi_t app_settings_wifi;

//static sl_wisun_statistics_t app_statistics;

const app_saving_item_t app_saving_item_wisun = {
  .data = &app_settings_wisun,
  .data_size = sizeof(app_settings_wisun),
  .default_val = &app_settings_wisun_default
};

const app_saving_item_t app_saving_item_ping = {
  .data = &app_settings_ping,
  .data_size = sizeof(app_settings_ping),
  .default_val = &app_settings_ping_default
};

const app_saving_item_t app_saving_item_app = {
  .data = &app_settings_app,
  .data_size = sizeof(app_settings_app),
  .default_val = &app_settings_app_default
};

const app_saving_item_t app_saving_item_wifi = {
  .data = &app_settings_wifi,
  .data_size = sizeof(app_settings_wifi),
  .default_val = &app_settings_wifi_default
};

const app_saving_item_t *saving_settings[] = {
  &app_saving_item_wisun,
  &app_saving_item_ping,
  &app_saving_item_app,
  &app_saving_item_wifi,
  NULL
};

static sl_wisun_statistics_t app_statistics;
static sl_wisun_network_info_t app_network_info;
static sl_wisun_rpl_info_t app_rpl_info;

#if SLI_WISUN_DISABLE_SECURITY
uint32_t app_security_state = 1;
#endif

static const app_enum_t app_settings_wisun_phy_config_type_enum[] =
{
  { "FAN 1.0", SL_WISUN_PHY_CONFIG_FAN10 },
  { "FAN 1.1", SL_WISUN_PHY_CONFIG_FAN11 },
  { "explicit", SL_WISUN_PHY_CONFIG_EXPLICIT },
  { "IDs", SL_WISUN_PHY_CONFIG_IDS},
  { "Custom FSK", SL_WISUN_PHY_CONFIG_CUSTOM_FSK},
  { "Custom OFDM", SL_WISUN_PHY_CONFIG_CUSTOM_OFDM},
  { "Custom OQPSK", SL_WISUN_PHY_CONFIG_CUSTOM_OQPSK},
  { NULL, 0 }
};

static const app_enum_t app_settings_wisun_network_size_enum[] =
{
  { "automatic", SL_WISUN_NETWORK_SIZE_AUTOMATIC },
  { "small", SL_WISUN_NETWORK_SIZE_SMALL },
  { "medium", SL_WISUN_NETWORK_SIZE_MEDIUM },
  { "large", SL_WISUN_NETWORK_SIZE_LARGE },
  { "test", SL_WISUN_NETWORK_SIZE_TEST },
  { "certification", SL_WISUN_NETWORK_SIZE_CERTIFICATION },
  { NULL, 0 }
};

static const app_enum_t app_settings_wisun_reg_domain_enum[] =
{
  { "WW", SL_WISUN_REGULATORY_DOMAIN_WW },
  { "NA", SL_WISUN_REGULATORY_DOMAIN_NA },
  { "JP", SL_WISUN_REGULATORY_DOMAIN_JP },
  { "EU", SL_WISUN_REGULATORY_DOMAIN_EU },
  { "CN", SL_WISUN_REGULATORY_DOMAIN_CN },
  { "IN", SL_WISUN_REGULATORY_DOMAIN_IN },
  { "MX", SL_WISUN_REGULATORY_DOMAIN_MX },
  { "BZ", SL_WISUN_REGULATORY_DOMAIN_BZ },
  { "AZ/NZ", SL_WISUN_REGULATORY_DOMAIN_AZ },
  { "AZ", SL_WISUN_REGULATORY_DOMAIN_AZ },
  { "NZ", SL_WISUN_REGULATORY_DOMAIN_NZ },
  { "KR", SL_WISUN_REGULATORY_DOMAIN_KR },
  { "PH", SL_WISUN_REGULATORY_DOMAIN_PH },
  { "MY", SL_WISUN_REGULATORY_DOMAIN_MY },
  { "HK", SL_WISUN_REGULATORY_DOMAIN_HK },
  { "SG", SL_WISUN_REGULATORY_DOMAIN_SG },
  { "TH", SL_WISUN_REGULATORY_DOMAIN_TH },
  { "VN", SL_WISUN_REGULATORY_DOMAIN_VN },
  { "application", SL_WISUN_REGULATORY_DOMAIN_APP },
  { NULL, 0 }
};

static const app_enum_t app_settings_wisun_operating_class_enum[] =
{
  { "application", SL_WISUN_OPERATING_CLASS_APP },
  { NULL, 0 }
};

static const app_enum_t app_settings_wisun_state_enum[] =
{
  { "initialized", SL_WISUN_BR_STATE_INITIALIZED },
  { "operational", SL_WISUN_BR_STATE_OPERATIONAL },
  { NULL, 0 }
};

const app_enum_t app_settings_wisun_crc_type_enum[] =
{
  { "no-crc", SL_WISUN_NO_CRC },
  { "2-bytes", SL_WISUN_2_BYTES_CRC },
  { "4-bytes", SL_WISUN_4_BYTES_CRC },
  { NULL, 0 }
};

const app_enum_t app_settings_wisun_neighbor_type_enum[] =
{
  { "Primary parent", SL_WISUN_NEIGHBOR_TYPE_PRIMARY_PARENT },
  { "Secondary parent", SL_WISUN_NEIGHBOR_TYPE_SECONDARY_PARENT },
  { "Child", SL_WISUN_NEIGHBOR_TYPE_CHILD },
  { NULL, 0 }
};

const app_enum_t app_settings_wisun_is_lfn_enum[] =
{
  { "FFN", 0 },
  { "LFN", 1 },
  { NULL, 0 }
};

static const app_enum_t app_settings_wisun_regulation_enum[] =
{
  { "none", SL_WISUN_REGULATION_NONE },
  { "arib", SL_WISUN_REGULATION_ARIB },
  { "wpc", SL_WISUN_REGULATION_WPC },
  { NULL, 0 }
};

static const app_enum_t app_settings_wisun_lfn_profile_enum[] =
{
  { "test", SL_WISUN_LFN_PROFILE_TEST },
  { "balanced", SL_WISUN_LFN_PROFILE_BALANCED },
  { "eco", SL_WISUN_LFN_PROFILE_ECO },
  { NULL, 0 }
};

static const app_enum_t app_settings_wisun_keychain_enum[] =
{
  { "automatic", SL_WISUN_KEYCHAIN_AUTOMATIC },
  { "builtin", SL_WISUN_KEYCHAIN_BUILTIN },
  { "nvm", SL_WISUN_KEYCHAIN_NVM },
  { NULL, 0 }
};

#ifdef SL_CATALOG_WISUN_BR_WIFI_PRESENT

static const app_enum_t app_settings_wifi_security_type_enum[] = {
  { "NONE", WIFI_SECURITY_TYPE_NONE },
  { "WPA_PSK", WIFI_SECURITY_TYPE_WPA_PSK },
  { "WPA2_TKIP", WIFI_SECURITY_TYPE_WPA2_TKIP },
  { "WPA2_CCMP", WIFI_SECURITY_TYPE_WPA2_CCMP },
  { NULL, 0 }
};

#endif

static sl_status_t app_settings_get_fan10_phy_config(char *value_str,
                                                     const char *key_str,
                                                     const app_settings_entry_t *entry);
static sl_status_t app_settings_get_fan11_phy_config(char *value_str,
                                                     const char *key_str,
                                                     const app_settings_entry_t *entry);
static sl_status_t app_settings_get_explicit_phy_config(char *value_str,
                                                        const char *key_str,
                                                        const app_settings_entry_t *entry);
static sl_status_t app_settings_get_custom_phy_config(char *value_str,
                                                      const char *key_str,
                                                      const app_settings_entry_t *entry);
static sl_status_t app_settings_get_custom_fsk_oqpsk_phy_config(char *value_str,
                                                                const char *key_str,
                                                                const app_settings_entry_t *entry);
static sl_status_t app_settings_get_custom_ofdm_phy_config(char *value_str,
                                                           const char *key_str,
                                                           const app_settings_entry_t *entry);
static sl_status_t app_settings_get_ids_phy_config(char *value_str,
                                                   const char *key_str,
                                                   const app_settings_entry_t *entry);
static sl_status_t app_settings_get_fan10_and_fan11_phy_config(char *value_str,
                                                               const char *key_str,
                                                               const app_settings_entry_t *entry);
static sl_status_t app_settings_get_fan11_and_explicit_and_ids_phy_config(char *value_str,
                                                                          const char *key_str,
                                                                          const app_settings_entry_t *entry);
static sl_status_t app_settings_set_gtk(const char *value_str,
                                        const char *key_str,
                                        const app_settings_entry_t *entry);
static sl_status_t app_settings_get_gtk(char *value_str,
                                        const char *key_str,
                                        const app_settings_entry_t *entry);
static sl_status_t app_settings_set_lgtk(const char *value_str,
                                         const char *key_str,
                                         const app_settings_entry_t *entry);
static sl_status_t app_settings_get_lgtk(char *value_str,
                                         const char *key_str,
                                         const app_settings_entry_t *entry);
static sl_status_t app_settings_get_gak(char *value_str,
                                        const char *key_str,
                                        const app_settings_entry_t *entry);
static sl_status_t app_settings_get_lgak(char *value_str,
                                         const char *key_str,
                                         const app_settings_entry_t *entry);
static sl_status_t app_settings_get_mac_address(char *value_str,
                                                const char *key_str,
                                                const app_settings_entry_t *entry);
static sl_status_t app_settings_get_ip_addresses(char *value_str,
                                                 const char *key_str,
                                                 const app_settings_entry_t *entry);
static sl_status_t app_settings_get_neighbors(char *value_str,
                                              const char *key_str,
                                              const app_settings_entry_t *entry);
static sl_status_t app_settings_get_statistics(char *value_str,
                                               const char *key_str,
                                               const app_settings_entry_t *entry);
static sl_status_t app_settings_get_statistics_regulation(char *value_str,
                                                          const char *key_str,
                                                          const app_settings_entry_t *entry);
static sl_status_t app_settings_get_state(char *value_str,
                                          const char *key_str,
                                          const app_settings_entry_t *entry);
static sl_status_t app_settings_get_async_channel_mask_str(char *value_str,
                                                           const char *key_str,
                                                           const app_settings_entry_t *entry);
static sl_status_t app_settings_get_unicast_channel_mask_str(char *value_str,
                                                             const char *key_str,
                                                             const app_settings_entry_t *entry);
static sl_status_t app_settings_get_broadcast_channel_mask_str(char *value_str,
                                                               const char *key_str,
                                                               const app_settings_entry_t *entry);
static sl_status_t app_settings_set_allowed_channels(const char *value_str,
                                                     const char *key_str,
                                                     const app_settings_entry_t *entry);
static sl_status_t app_settings_set_trace_filter(const char *value_str,
                                                 const char *key_str,
                                                 const app_settings_entry_t *entry);
static sl_status_t app_settings_get_trace_filter(char *value_str,
                                                 const char *key_str,
                                                 const app_settings_entry_t *entry);
static sl_status_t app_settings_set_regulation(const char *value_str,
                                               const char *key_str,
                                               const app_settings_entry_t *entry);
static sl_status_t app_settings_set_regulation_warning_threshold(const char *value_str,
                                                                 const char *key_str,
                                                                 const app_settings_entry_t *entry);
static sl_status_t app_settings_set_regulation_alert_threshold(const char *value_str,
                                                               const char *key_str,
                                                               const app_settings_entry_t *entry);
#if RAIL_IEEE802154_SUPPORTS_G_MODESWITCH
static sl_status_t app_settings_set_rx_mdr_capable(const char *value_str,
                                                   const char *key_str,
                                                   const app_settings_entry_t *entry);
static sl_status_t app_settings_set_rx_phy_mode_ids(const char *value_str,
                                                    const char *key_str,
                                                    const app_settings_entry_t *entry);
static sl_status_t app_settings_get_rx_phy_mode_ids(char *value_str,
                                                    const char *key_str,
                                                    const app_settings_entry_t *entry);
#endif

static sl_status_t app_settings_get_network_info(char *value_str,
                                                 const char *key_str,
                                                 const app_settings_entry_t *entry);
static sl_status_t app_settings_get_rpl_info(char *value_str,
                                             const char *key_str,
                                             const app_settings_entry_t *entry);
#ifdef SL_CATALOG_WISUN_BR_WIFI_PRESENT
static sl_status_t app_settings_get_wifi_info(char *value_str,
                                              const char *key_str,
                                              const app_settings_entry_t *entry);
#endif
static sl_status_t app_settings_set_tx_power(const char *value_str,
                                             const char *key_str,
                                             const app_settings_entry_t *entry);
static sl_status_t app_settings_get_tx_power(char *value_str,
                                             const char *key_str,
                                             const app_settings_entry_t *entry);

const app_settings_entry_t app_settings_entries[] =
{
  {
    .key = "state",
    .domain = app_settings_domain_wisun,
    .value_size = APP_SETTINGS_VALUE_SIZE_NONE,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_settings_wisun.state,
    .input_enum_list = NULL,
    .output_enum_list = app_settings_wisun_state_enum,
    .set_handler = NULL,
    .get_handler = app_settings_get_state,
    .description = "Border router state"
  },
  {
    .key = "network_name",
    .domain = app_settings_domain_wisun,
    .value_size = SL_WISUN_NETWORK_NAME_SIZE+1,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_settings_wisun.network_name,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = app_settings_set_string,
    .get_handler = app_settings_get_string,
    .description = "Wi-SUN network [string] max 32"
  },
  {
    .key = "phy_config_type",
    .domain = app_settings_domain_wisun,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT8,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_settings_wisun.phy_config_type,
    .input_enum_list = NULL,
    .output_enum_list = app_settings_wisun_phy_config_type_enum,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "PHY configuration type (FAN 1.0|FAN 1.1|explicit|IDs)"
  },
  {
    .key = "regulatory_domain",
    .domain = app_settings_domain_wisun,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT8,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_settings_wisun.regulatory_domain,
    .input_enum_list = app_settings_wisun_reg_domain_enum,
    .output_enum_list = app_settings_wisun_reg_domain_enum,
    .set_handler = app_settings_set_integer,
    .get_handler = app_settings_get_fan10_and_fan11_phy_config,
    .description = "Wi-SUN regulatory domain [uint8]"
  },
  {
    .key = "operating_class",
    .domain = app_settings_domain_wisun,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT8,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_settings_wisun.operating_class,
    .input_enum_list = app_settings_wisun_operating_class_enum,
    .output_enum_list = app_settings_wisun_operating_class_enum,
    .set_handler = app_settings_set_integer,
    .get_handler = app_settings_get_fan10_phy_config,
    .description = "Wi-SUN operation class [uint8]"
  },
  {
    .key = "operating_mode",
    .domain = app_settings_domain_wisun,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT16,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT | APP_SETTINGS_OUTPUT_FLAG_HEX,
    .value = &app_settings_wisun.operating_mode,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = app_settings_set_integer,
    .get_handler = app_settings_get_fan10_phy_config,
    .description = "Wi-SUN operating mode [uint16]"
  },
  {
    .key = "fec",
    .domain = app_settings_domain_wisun,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT8,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_settings_wisun.fec,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = app_settings_set_integer,
    .get_handler = app_settings_get_fan10_phy_config,
    .description = "1 to use FEC, 0 otherwise [bool] (0|1)"
  },
  {
    .key = "chan_plan_id",
    .domain = app_settings_domain_wisun,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT8,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_settings_wisun.chan_plan_id,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = app_settings_set_integer,
    .get_handler = app_settings_get_fan11_phy_config,
    .description = "Wi-SUN channel plan ID [uint8]"
  },
  {
    .key = "phy_mode_id",
    .domain = app_settings_domain_wisun,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT8,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_settings_wisun.phy_mode_id,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = app_settings_set_integer,
    .get_handler = app_settings_get_fan11_and_explicit_and_ids_phy_config,
    .description = "Wi-SUN PHY mode ID [uint8]"
  },
  {
    .key = "ch0_frequency",
    .domain = app_settings_domain_wisun,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT32,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_settings_wisun.ch0_frequency,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = app_settings_set_integer,
    .get_handler = app_settings_get_explicit_phy_config,
    .description = "Ch0 frequency in kHz [uint32]"
  },
  {
    .key = "number_of_channels",
    .domain = app_settings_domain_wisun,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT16,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_settings_wisun.number_of_channels,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = app_settings_set_integer,
    .get_handler = app_settings_get_explicit_phy_config,
    .description = "Number of channels [uint16]"
  },
  {
    .key = "channel_spacing",
    .domain = app_settings_domain_wisun,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT16,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_settings_wisun.channel_spacing,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = app_settings_set_integer,
    .get_handler = app_settings_get_explicit_phy_config,
    .description = "Channel spacing in kHz [uint16]"
  },
  {
    .key = "crc_type",
    .domain = app_settings_domain_wisun,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT8,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_settings_wisun.crc_type,
    .input_enum_list = app_settings_wisun_crc_type_enum,
    .output_enum_list = app_settings_wisun_crc_type_enum,
    .set_handler = app_settings_set_integer,
    .get_handler = app_settings_get_custom_phy_config,
    .description = "Custom CRC type [uint8]"
  },
  {
    .key = "preamble_length",
    .domain = app_settings_domain_wisun,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT8,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_settings_wisun.preamble_length,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = app_settings_set_integer,
    .get_handler = app_settings_get_custom_fsk_oqpsk_phy_config,
    .description = "Custom preamble length [uint8]"
  },
  {
    .key = "stf_length",
    .domain = app_settings_domain_wisun,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT8,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_settings_wisun.stf_length,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = app_settings_set_integer,
    .get_handler = app_settings_get_custom_ofdm_phy_config,
    .description = "Custom STF length [uint8]"
  },
  {
    .key = "protocol_id",
    .domain = app_settings_domain_wisun,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT16,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_settings_wisun.protocol_id,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = app_settings_set_integer,
    .get_handler = app_settings_get_ids_phy_config,
    .description = "Radioconf protocol ID [uint16]"
  },
  {
    .key = "channel_id",
    .domain = app_settings_domain_wisun,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT16,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_settings_wisun.channel_id,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = app_settings_set_integer,
    .get_handler = app_settings_get_ids_phy_config,
    .description = "Radioconf channel ID [uint16]"
  },
  {
    .key = "network_size",
    .domain = app_settings_domain_wisun,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT8,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_settings_wisun.network_size,
    .input_enum_list = app_settings_wisun_network_size_enum,
    .output_enum_list = app_settings_wisun_network_size_enum,
    .set_handler = app_settings_set_integer,
    .get_handler = app_settings_get_integer,
    .description = "Wi-SUN network size [uint8]"
  },
  {
    .key = "tx_power",
    .domain = app_settings_domain_wisun,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT8,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT | APP_SETTINGS_INPUT_FLAG_SIGNED,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT | APP_SETTINGS_OUTPUT_FLAG_SIGNED,
    .value = &app_settings_wisun.tx_power_ddbm,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = app_settings_set_tx_power,
    .get_handler = app_settings_get_tx_power,
    .description = "TX power in dBm [int8]"
  },
  {
    .key = "tx_power_ddbm",
    .domain = app_settings_domain_wisun,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT16,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT | APP_SETTINGS_INPUT_FLAG_SIGNED,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT | APP_SETTINGS_OUTPUT_FLAG_SIGNED,
    .value = &app_settings_wisun.tx_power_ddbm,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = app_settings_set_tx_power,
    .get_handler = app_settings_get_tx_power,
    .description = "TX power in ddBm [int16]"
  },
  {
    .key = "unicast_dwell_interval",
    .domain = app_settings_domain_wisun,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT8,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_settings_wisun.uc_dwell_interval_ms,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = app_settings_set_integer,
    .get_handler = app_settings_get_integer,
    .description = "Unicast dwell interval in ms [uint8]"
  },
  {
    .key = "broadcast_interval",
    .domain = app_settings_domain_wisun,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT32,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_settings_wisun.bc_interval_ms,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = app_settings_set_integer,
    .get_handler = app_settings_get_integer,
    .description = "Broadcast interval in ms [uint32]"
  },
  {
    .key = "broadcast_dwell_interval",
    .domain = app_settings_domain_wisun,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT8,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_settings_wisun.bc_dwell_interval_ms,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = app_settings_set_integer,
    .get_handler = app_settings_get_integer,
    .description = "Broadcast dwell interval in ms [uint8]"
  },
  {
    .key = "mac_address",
    .domain = app_settings_domain_wisun,
    .value_size = APP_SETTINGS_VALUE_SIZE_NONE,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = NULL,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_mac_address,
    .description = "MAC address (EUI-64)"
  },
  {
    .key = "ip_addresses",
    .domain = app_settings_domain_wisun,
    .value_size = APP_SETTINGS_VALUE_SIZE_NONE,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = NULL,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_ip_addresses,
    .description = "IPv6 addresses"
  },
  {
    .key = "neighbors",
    .domain = app_settings_domain_wisun,
    .value_size = APP_SETTINGS_VALUE_SIZE_NONE,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = NULL,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_neighbors,
    .description = "RPL neighbors (parents and children)"
  },
  {
    .key = "async_channel_mask",
    .domain = app_settings_domain_wisun,
    .value_size = APP_UTIL_PRINTABLE_DATA_MAX_LENGTH+1,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = NULL,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_async_channel_mask_str,
    .description = "Excluded channel mask applied to async frames"
  },
  {
    .key = "unicast_channel_mask",
    .domain = app_settings_domain_wisun,
    .value_size = APP_UTIL_PRINTABLE_DATA_MAX_LENGTH+1,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = NULL,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_unicast_channel_mask_str,
    .description = "Excluded channel mask applied to unicast frequency hopping"
  },
  {
    .key = "broadcast_channel_mask",
    .domain = app_settings_domain_wisun,
    .value_size = APP_UTIL_PRINTABLE_DATA_MAX_LENGTH+1,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = NULL,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_broadcast_channel_mask_str,
    .description = "Excluded channel mask applied to broadcast frequency hopping"
  },
  {
    .key = "allowed_channels",
    .domain = app_settings_domain_wisun,
    .value_size = APP_UTIL_PRINTABLE_DATA_MAX_LENGTH+1,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = app_settings_wisun.allowed_channels,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = app_settings_set_allowed_channels,
    .get_handler = app_settings_get_string,
    .description = "Allowed channel ranges (e.g. 0-54,57-60,64,67-68)"
  },
  {
    .key = "gtk1",
    .domain = app_settings_domain_wisun,
    .value_size = APP_SETTINGS_VALUE_SIZE_NONE,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = app_settings_wisun.gtks[0],
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = app_settings_set_gtk,
    .get_handler = app_settings_get_gtk,
    .description = "GTK to install, 'none' to uninstall [string]"
  },
  {
    .key = "gtk2",
    .domain = app_settings_domain_wisun,
    .value_size = APP_SETTINGS_VALUE_SIZE_NONE,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = app_settings_wisun.gtks[1],
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = app_settings_set_gtk,
    .get_handler = app_settings_get_gtk,
    .description = "GTK to install, 'none' to uninstall [string]"
  },
  {
    .key = "gtk3",
    .domain = app_settings_domain_wisun,
    .value_size = APP_SETTINGS_VALUE_SIZE_NONE,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = app_settings_wisun.gtks[2],
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = app_settings_set_gtk,
    .get_handler = app_settings_get_gtk,
    .description = "GTK to install, 'none' to uninstall [string]"
  },
  {
    .key = "gtk4",
    .domain = app_settings_domain_wisun,
    .value_size = APP_SETTINGS_VALUE_SIZE_NONE,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = app_settings_wisun.gtks[3],
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = app_settings_set_gtk,
    .get_handler = app_settings_get_gtk,
    .description = "GTK to install, 'none' to uninstall [string]"
  },
  {
    .key = "lgtk1",
    .domain = app_settings_domain_wisun,
    .value_size = APP_SETTINGS_VALUE_SIZE_NONE,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = app_settings_wisun.lgtks[0],
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = app_settings_set_lgtk,
    .get_handler = app_settings_get_lgtk,
    .description = "LGTK to install, 'none' to uninstall [string]"
  },
  {
    .key = "lgtk2",
    .domain = app_settings_domain_wisun,
    .value_size = APP_SETTINGS_VALUE_SIZE_NONE,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = app_settings_wisun.lgtks[1],
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = app_settings_set_lgtk,
    .get_handler = app_settings_get_lgtk,
    .description = "LGTK to install, 'none' to uninstall [string]"
  },
  {
    .key = "lgtk3",
    .domain = app_settings_domain_wisun,
    .value_size = APP_SETTINGS_VALUE_SIZE_NONE,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = app_settings_wisun.lgtks[2],
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = app_settings_set_lgtk,
    .get_handler = app_settings_get_lgtk,
    .description = "LGTK to install, 'none' to uninstall [string]"
  },
  {
    .key = "lgtk4",
    .domain = app_settings_domain_wisun,
    .value_size = APP_SETTINGS_VALUE_SIZE_NONE,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = app_settings_wisun.lgtks[3],
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = app_settings_set_lgtk,
    .get_handler = app_settings_get_lgtk,
    .description = "LGTK to install, 'none' to uninstall [string]"
  },
  {
    .key = "gak1",
    .domain = app_settings_domain_wisun,
    .value_size = APP_SETTINGS_VALUE_SIZE_NONE,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = app_settings_wisun.gaks[0],
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_gak,
    .description = "GAK derived from GTK1 [string]"
  },
  {
    .key = "gak2",
    .domain = app_settings_domain_wisun,
    .value_size = APP_SETTINGS_VALUE_SIZE_NONE,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = app_settings_wisun.gaks[1],
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_gak,
    .description = "GAK derived from GTK2 [string]"
  },
  {
    .key = "gak3",
    .domain = app_settings_domain_wisun,
    .value_size = APP_SETTINGS_VALUE_SIZE_NONE,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = app_settings_wisun.gaks[2],
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_gak,
    .description = "GAK derived from GTK3 [string]"
  },
  {
    .key = "gak4",
    .domain = app_settings_domain_wisun,
    .value_size = APP_SETTINGS_VALUE_SIZE_NONE,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = app_settings_wisun.gaks[3],
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_gak,
    .description = "GAK derived from GTK4 [string]"
  },
  {
    .key = "lgak1",
    .domain = app_settings_domain_wisun,
    .value_size = APP_SETTINGS_VALUE_SIZE_NONE,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = app_settings_wisun.lgaks[0],
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_lgak,
    .description = "GAK derived from LGTK1 [string]"
  },
  {
    .key = "lgak2",
    .domain = app_settings_domain_wisun,
    .value_size = APP_SETTINGS_VALUE_SIZE_NONE,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = app_settings_wisun.lgaks[1],
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_lgak,
    .description = "GAK derived from LGTK2 [string]"
  },
  {
    .key = "lgak3",
    .domain = app_settings_domain_wisun,
    .value_size = APP_SETTINGS_VALUE_SIZE_NONE,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = app_settings_wisun.lgaks[2],
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_lgak,
    .description = "GAK derived from LGTK3 [string]"
  },
  {
    .key = "trace_filter",
    .domain = app_settings_domain_wisun,
    .value_size = APP_SETTINGS_VALUE_SIZE_NONE,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = NULL,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = app_settings_set_trace_filter,
    .get_handler = app_settings_get_trace_filter,
    .description = "Trace filter group list, 1 bit per group"
  },
  {
    .key = "ipv6_prefix",
    .domain = app_settings_domain_wisun,
    .value_size = APP_IPV6_PREFIX_SIZE,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_settings_wisun.ipv6_prefix,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = app_settings_set_string,
    .get_handler = app_settings_get_string,
    .description = "IPv6 prefix [string] max 43 (prefix length of 64 is required)"
  },
#ifdef SL_CATALOG_WISUN_BR_DHCPV6_RELAY_PRESENT
  {
    .key = "dhcpv6_server",
    .domain = app_settings_domain_wisun,
    .value_size = APP_IPV6_ADDR_SIZE+1,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_settings_wisun.dhcpv6_server,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = app_settings_set_string,
    .get_handler = app_settings_get_string,
    .description = "DHCPv6 Server's IPv6 address [string] max 39"
  },
#endif
  {
    .key = "regulation",
    .domain = app_settings_domain_wisun,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT8,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_settings_wisun.regulation,
    .input_enum_list = app_settings_wisun_regulation_enum,
    .output_enum_list = app_settings_wisun_regulation_enum,
    .set_handler = app_settings_set_regulation,
    .get_handler = app_settings_get_integer,
    .description = "Regional regulation [uint8]"
  },
  {
    .key = "regulation_warning_threshold",
    .domain = app_settings_domain_wisun,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT8,
    .input = APP_SETTINGS_OUTPUT_FLAG_SIGNED,
    .output = APP_SETTINGS_OUTPUT_FLAG_SIGNED,
    .value = &app_settings_wisun.regulation_warning_threshold,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = app_settings_set_regulation_warning_threshold,
    .get_handler = app_settings_get_integer,
    .description = "Transmission warning threshold in percent (-1 to disable) [int8]"
  },
  {
    .key = "regulation_alert_threshold",
    .domain = app_settings_domain_wisun,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT8,
    .input = APP_SETTINGS_OUTPUT_FLAG_SIGNED,
    .output = APP_SETTINGS_OUTPUT_FLAG_SIGNED,
    .value = &app_settings_wisun.regulation_alert_threshold,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = app_settings_set_regulation_alert_threshold,
    .get_handler = app_settings_get_integer,
    .description = "Transmission alert threshold in percent (-1 to disable) [int8]"
  },
#if RAIL_IEEE802154_SUPPORTS_G_MODESWITCH
  {
    .key = "rx_mdr_capable",
    .domain = app_settings_domain_wisun,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT8,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_settings_wisun.rx_mdr_capable,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = app_settings_set_rx_mdr_capable,
    .get_handler = app_settings_get_integer,
    .description = "Indicate if MDR Command is supported"
  },
  {
    .key = "rx_phy_mode_ids_count",
    .domain = app_settings_domain_wisun,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT8,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_settings_wisun.rx_phy_mode_ids_count,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = NULL,
    .description = "Number of PhyModeId configured"
  },
  {
    .key = "rx_phy_mode_ids",
    .domain = app_settings_domain_wisun,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT8,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_settings_wisun.rx_phy_mode_ids,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = app_settings_set_rx_phy_mode_ids,
    .get_handler = app_settings_get_rx_phy_mode_ids,
    .description = "List of PhyModeId to advertise in POM-IE"
  },
#endif
  {
    .key = "lfn_profile",
    .domain = app_settings_domain_wisun,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT8,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_settings_wisun.lfn_profile,
    .input_enum_list = app_settings_wisun_lfn_profile_enum,
    .output_enum_list = app_settings_wisun_lfn_profile_enum,
    .set_handler = app_settings_set_integer,
    .get_handler = app_settings_get_integer,
    .description = "Wi-SUN LFN profile [uint8]"
  },
  {
    .key = "phy",
    .domain = app_settings_domain_statistics,
    .value_size = APP_SETTINGS_VALUE_SIZE_NONE,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = (void *)SL_WISUN_STATISTICS_TYPE_PHY,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_statistics,
    .description = "PHY statistics and errors"
  },
  {
    .key = "mac",
    .domain = app_settings_domain_statistics,
    .value_size = APP_SETTINGS_VALUE_SIZE_NONE,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = (void *)SL_WISUN_STATISTICS_TYPE_MAC,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_statistics,
    .description = "MAC statistics and errors"
  },
  {
    .key = "fhss",
    .domain = app_settings_domain_statistics,
    .value_size = APP_SETTINGS_VALUE_SIZE_NONE,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = (void *)SL_WISUN_STATISTICS_TYPE_FHSS,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_statistics,
    .description = "FHSS statistics and errors"
  },
  {
    .key = "wisun",
    .domain = app_settings_domain_statistics,
    .value_size = APP_SETTINGS_VALUE_SIZE_NONE,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = (void *)SL_WISUN_STATISTICS_TYPE_WISUN,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_statistics,
    .description = "Wi-SUN statistics and errors"
  },
  {
    .key = "network",
    .domain = app_settings_domain_statistics,
    .value_size = APP_SETTINGS_VALUE_SIZE_NONE,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = (void *)SL_WISUN_STATISTICS_TYPE_NETWORK,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_statistics,
    .description = "Network statistics and errors"
  },
  {
    .key = "heap",
    .domain = app_settings_domain_statistics,
    .value_size = APP_SETTINGS_VALUE_SIZE_NONE,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = (void *)SL_WISUN_STATISTICS_TYPE_HEAP,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_statistics,
    .description = "Heap usage statistics"
  },
  {
    .key = "regulation",
    .domain = app_settings_domain_statistics,
    .value_size = APP_SETTINGS_VALUE_SIZE_NONE,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = (void *)SL_WISUN_STATISTICS_TYPE_REGULATION,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_statistics_regulation,
    .description = "Regional regulation statistics and errors"
  },
  {
    .key = "printable_data_as_hex",
    .domain = app_settings_domain_app,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT8,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_settings_app.printable_data_as_hex,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = app_settings_set_integer,
    .get_handler = app_settings_get_integer,
    .description = "Print socket data as hex [bool] (0|1)"
  },
  {
    .key = "printable_data_length",
    .domain = app_settings_domain_app,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT8,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_settings_app.printable_data_length,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = app_settings_set_integer,
    .get_handler = app_settings_get_integer,
    .description = "Socket data line length [uint8]"
  },
  {
    .key = "autostart",
    .domain = app_settings_domain_app,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT8,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_settings_app.autostart,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = app_settings_set_integer,
    .get_handler = app_settings_get_integer,
    .description = "Disable or enable autostart [bool] (0|1)"
  },
  {
    .key = "pti_state",
    .domain = app_settings_domain_app,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT8,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_settings_app.pti_state,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = app_settings_set_integer,
    .get_handler = app_settings_get_integer,
    .description = "Disable or enable PTI [bool] (0|1)"
  },
#if SLI_WISUN_DISABLE_SECURITY
  {
    .key = "security_state",
    .domain = app_settings_domain_app,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT32,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_security_state,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = app_settings_set_integer,
    .get_handler = app_settings_get_integer,
    .description = "Set the security state [uint32]"
  },
#endif
  {
    .key = "network",
    .domain = app_settings_domain_info,
    .value_size = APP_SETTINGS_VALUE_SIZE_NONE,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = NULL,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_network_info,
    .description = "Wi-SUN network information"
  },
  {
    .key = "rpl",
    .domain = app_settings_domain_info,
    .value_size = APP_SETTINGS_VALUE_SIZE_NONE,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = NULL,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_rpl_info,
    .description = "Wi-SUN RPL information"
  },
  {
    .key = "neighbor_table_size",
    .domain = app_settings_domain_wisun,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT8,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_settings_wisun.neighbor_table_size,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = app_settings_set_integer,
    .get_handler = app_settings_get_integer,
    .description = "Neighbor table size [uint8]"
  },
  {
    .key = "identifier",
    .domain = app_settings_domain_ping,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT16,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_settings_ping.identifier,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = app_settings_set_integer,
    .get_handler = app_settings_get_integer,
    .description = "Identifier [uint16]"
  },
  {
    .key = "sequence_number",
    .domain = app_settings_domain_ping,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT16,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_settings_ping.sequence_number,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = app_settings_set_integer,
    .get_handler = app_settings_get_integer,
    .description = "Sequence number [uint16]"
  },
  {
    .key = "packet_interval",
    .domain = app_settings_domain_ping,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT16,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_settings_ping.packet_interval,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = app_settings_set_integer,
    .get_handler = app_settings_get_integer,
    .description = "Packet interval in ms [uint16]"
  },
  {
    .key = "packet_length",
    .domain = app_settings_domain_ping,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT16,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_settings_ping.packet_length,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = app_settings_set_integer,
    .get_handler = app_settings_get_integer,
    .description = "Packet length [uint16]"
  },
  {
    .key = "pattern",
    .domain = app_settings_domain_ping,
    .value_size = APP_PING_PATTERN_SIZE+1,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = app_settings_ping.pattern,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = app_settings_set_string,
    .get_handler = app_settings_get_string,
    .description = "Pattern [string] max 16"
  },
  {
    .key = "keychain",
    .domain = app_settings_domain_wisun,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT8,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_settings_wisun.keychain,
    .input_enum_list = app_settings_wisun_keychain_enum,
    .output_enum_list = app_settings_wisun_keychain_enum,
    .set_handler = app_settings_set_integer,
    .get_handler = app_settings_get_integer,
    .description = "Keychain [uint8]"
  },
  {
    .key = "keychain_index",
    .domain = app_settings_domain_wisun,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT8,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_settings_wisun.keychain_index,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = app_settings_set_integer,
    .get_handler = app_settings_get_integer,
    .description = "Built-in keychain index [uint8]"
  },
#ifdef SL_CATALOG_WISUN_BR_WIFI_PRESENT
  {
    .key = "ssid",
    .domain = app_settings_domain_wifi,
    .value_size = SL_WISUN_WIFI_SSID_SIZE+1,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_settings_wifi.ssid,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = app_settings_set_string,
    .get_handler = app_settings_get_string,
    .description = "Wi-Fi SSID [string] max 32"
  },
  {
    .key = "security",
    .domain = app_settings_domain_wifi,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT8+1,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_settings_wifi.security_type,
    .input_enum_list = app_settings_wifi_security_type_enum,
    .output_enum_list = app_settings_wifi_security_type_enum,
    .set_handler = app_settings_set_integer,
    .get_handler = app_settings_get_integer,
    .description = "Wi-Fi security type [uint8] (WPA2_TKIP|WPA2_CCMP)"
  },
  {
    .key = "passphrase",
    .domain = app_settings_domain_wifi,
    .value_size = SL_WISUN_WIFI_PASSPHRASE_SIZE+1,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_settings_wifi.passphrase,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = app_settings_set_string,
    .get_handler = app_settings_get_string,
    .description = "Wi-Fi passphrase [string] max 32"
  },
  {
    .key = "wlan_state",
    .domain = app_settings_domain_wifi,
    .value_size = APP_SETTINGS_VALUE_SIZE_NONE,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = NULL,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_wifi_info,
    .description = "WLAN state"
  },
  {
    .key = "channel_number",
    .domain = app_settings_domain_wifi,
    .value_size = APP_SETTINGS_VALUE_SIZE_NONE,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = NULL,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_wifi_info,
    .description = "Channel number of connected AP"
  },
  {
    .key = "mac_address",
    .domain = app_settings_domain_wifi,
    .value_size = APP_SETTINGS_VALUE_SIZE_NONE,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = NULL,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_wifi_info,
    .description = "MAC address (EUI-48)"
  },
  {
    .key = "ipv6_address",
    .domain = app_settings_domain_wifi,
    .value_size = APP_SETTINGS_VALUE_SIZE_NONE,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = NULL,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_wifi_info,
    .description = "IPv6 address"
  },
#endif
  {
    .key = NULL,
    .domain = 0,
    .value_size = APP_SETTINGS_VALUE_SIZE_NONE,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = NULL,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = NULL
  }
};

static const app_settings_entry_t app_trace_filter_entries[] = {
  {
    .key = "000-031",
    .domain = app_settings_domain_wisun,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT32,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT | APP_SETTINGS_OUTPUT_FLAG_HEX | APP_SETTINGS_OUTPUT_FLAG_FIXEDSIZE,
    .value = &app_settings_wisun.trace_filter[0],
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = app_settings_set_integer,
    .get_handler = app_settings_get_integer,
    .description = "Trace filter bitfield 000-031 [uint32]"
  },
  {
    .key = "032-063",
    .domain = app_settings_domain_wisun,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT32,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT | APP_SETTINGS_OUTPUT_FLAG_HEX | APP_SETTINGS_OUTPUT_FLAG_FIXEDSIZE,
    .value = &app_settings_wisun.trace_filter[4],
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = app_settings_set_integer,
    .get_handler = app_settings_get_integer,
    .description = "Trace filter bitfield 032-063 [uint32]"
  },
  {
    .key = NULL,
    .domain = 0,
    .value_size = APP_SETTINGS_VALUE_SIZE_NONE,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = NULL,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = NULL,
    .description = NULL
  }
};

static const app_settings_entry_t app_statistics_entries[] =
{
  {
    .key = "crc_fails",
    .domain = app_statistics_domain_phy,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT32,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_statistics.phy.crc_fails,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "Crc fails"
  },
  {
    .key = "tx_timeouts",
    .domain = app_statistics_domain_phy,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT32,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_statistics.phy.tx_timeouts,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "Tx timeouts"
  },
  {
    .key = "rx_timeouts",
    .domain = app_statistics_domain_phy,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT32,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_statistics.phy.rx_timeouts,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "Rx timeouts"
  },
  {
    .key = "tx_queue_size",
    .domain = app_statistics_domain_mac,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT16,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_statistics.mac.tx_queue_size,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "Tx queue size"
  },
  {
    .key = "tx_queue_peak",
    .domain = app_statistics_domain_mac,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT16,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_statistics.mac.tx_queue_peak,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "Tx queue peak"
  },
  {
    .key = "rx_ms_count",
    .domain = app_statistics_domain_mac,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT32,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_statistics.mac.rx_ms_count,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "Rx count using mode switch"
  },
  {
    .key = "tx_ms_count",
    .domain = app_statistics_domain_mac,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT32,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_statistics.mac.tx_ms_count,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "Tx count using mode switch"
  },
  {
    .key = "rx_ms_failed",
    .domain = app_statistics_domain_mac,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT32,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_statistics.mac.rx_ms_failed_count,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "Rx failed count using mode switch"
  },
  {
    .key = "tx_ms_failed",
    .domain = app_statistics_domain_mac,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT32,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_statistics.mac.tx_ms_failed_count,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "Tx failed count using mode switch"
  },
  {
    .key = "rx_count",
    .domain = app_statistics_domain_mac,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT32,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_statistics.mac.rx_count,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "Rx count"
  },
  {
    .key = "tx_count",
    .domain = app_statistics_domain_mac,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT32,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_statistics.mac.tx_count,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "Tx count"
  },
  {
    .key = "bc_rx_count",
    .domain = app_statistics_domain_mac,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT32,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_statistics.mac.bc_rx_count,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "Bc rx count"
  },
  {
    .key = "bc_tx_count",
    .domain = app_statistics_domain_mac,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT32,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_statistics.mac.bc_tx_count,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "Bc tx count"
  },
  {
    .key = "rx_drop_count",
    .domain = app_statistics_domain_mac,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT32,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_statistics.mac.rx_drop_count,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "Rx drop count"
  },
  {
    .key = "tx_bytes",
    .domain = app_statistics_domain_mac,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT32,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_statistics.mac.tx_bytes,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "Tx bytes"
  },
  {
    .key = "rx_bytes",
    .domain = app_statistics_domain_mac,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT32,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_statistics.mac.rx_bytes,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "Rx bytes"
  },
  {
    .key = "tx_failed_count",
    .domain = app_statistics_domain_mac,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT32,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_statistics.mac.tx_failed_count,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "Tx failed count"
  },
  {
    .key = "retry_count",
    .domain = app_statistics_domain_mac,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT32,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_statistics.mac.retry_count,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "Retry count"
  },
  {
    .key = "cca_attempts_count",
    .domain = app_statistics_domain_mac,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT32,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_statistics.mac.cca_attempts_count,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "Cca attempts count"
  },
  {
    .key = "failed_cca_count",
    .domain = app_statistics_domain_mac,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT32,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_statistics.mac.failed_cca_count,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "Failed cca count"
  },
  {
    .key = "drift_compensation",
    .domain = app_statistics_domain_fhss,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT16,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT | APP_SETTINGS_INPUT_FLAG_SIGNED,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT | APP_SETTINGS_OUTPUT_FLAG_SIGNED,
    .value = &app_statistics.fhss.drift_compensation,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "Drift compensation"
  },
  {
    .key = "hop_count",
    .domain = app_statistics_domain_fhss,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT16,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_statistics.fhss.hop_count,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "Hop count"
  },
  {
    .key = "synch_interval",
    .domain = app_statistics_domain_fhss,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT16,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_statistics.fhss.synch_interval,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "Synch interval"
  },
  {
    .key = "prev_avg_synch_fix",
    .domain = app_statistics_domain_fhss,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT16,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT | APP_SETTINGS_INPUT_FLAG_SIGNED,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT | APP_SETTINGS_OUTPUT_FLAG_SIGNED,
    .value = &app_statistics.fhss.prev_avg_synch_fix,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "Prev avf synch fix"
  },
  {
    .key = "synch_lost",
    .domain = app_statistics_domain_fhss,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT32,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_statistics.fhss.synch_lost,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "Synch lost"
  },
  {
    .key = "unknown_neighbor",
    .domain = app_statistics_domain_fhss,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT32,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_statistics.fhss.unknown_neighbor,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "Unknown neighbor"
  },
  {
    .key = "pan_control_rx_count",
    .domain = app_statistics_domain_wisun,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT32,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_statistics.wisun.pan_control_rx_count,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "PAN control rx count"
  },
  {
    .key = "pan_control_tx_count",
    .domain = app_statistics_domain_wisun,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT32,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_statistics.wisun.pan_control_tx_count,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "PAN control tx count"
  },
  {
    .key = "ip_rx_count",
    .domain = app_statistics_domain_network,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT32,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_statistics.network.ip_rx_count,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "Ip rx count"
  },
  {
    .key = "ip_tx_count",
    .domain = app_statistics_domain_network,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT32,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_statistics.network.ip_tx_count,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "Ip tx count"
  },
  {
    .key = "ip_rx_drop",
    .domain = app_statistics_domain_network,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT32,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_statistics.network.ip_rx_drop,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "Ip rx drop"
  },
  {
    .key = "ip_cksum_error",
    .domain = app_statistics_domain_network,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT32,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_statistics.network.ip_cksum_error,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "Ip cksum error"
  },
  {
    .key = "ip_tx_bytes",
    .domain = app_statistics_domain_network,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT32,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_statistics.network.ip_tx_bytes,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "Ip tx bytes"
  },
  {
    .key = "ip_rx_bytes",
    .domain = app_statistics_domain_network,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT32,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_statistics.network.ip_rx_bytes,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "Ip rx bytes"
  },
  {
    .key = "ip_routed_up",
    .domain = app_statistics_domain_network,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT32,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_statistics.network.ip_routed_up,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "Ip routed up"
  },
  {
    .key = "ip_no_route",
    .domain = app_statistics_domain_network,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT32,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_statistics.network.ip_no_route,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "Ip no route"
  },
  {
    .key = "frag_rx_errors",
    .domain = app_statistics_domain_network,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT32,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_statistics.network.frag_rx_errors,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "Frag rx errors"
  },
  {
    .key = "frag_tx_errors",
    .domain = app_statistics_domain_network,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT32,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_statistics.network.frag_tx_errors,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "Frag tx errors"
  },
  {
    .key = "rpl_route_routecost_better_change",
    .domain = app_statistics_domain_network,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT32,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_statistics.network.rpl_route_routecost_better_change,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "Rpl route routecost better change"
  },
  {
    .key = "ip_routeloop_detect",
    .domain = app_statistics_domain_network,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT32,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_statistics.network.ip_routeloop_detect,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "Ip routeloop detect"
  },
  {
    .key = "rpl_memory_overflow",
    .domain = app_statistics_domain_network,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT32,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_statistics.network.rpl_memory_overflow,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "Rpl memory overflow"
  },
  {
    .key = "rpl_parent_tx_fail",
    .domain = app_statistics_domain_network,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT32,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_statistics.network.rpl_parent_tx_fail,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "Rpl parent tx fail"
  },
  {
    .key = "rpl_unknown_instance",
    .domain = app_statistics_domain_network,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT32,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_statistics.network.rpl_unknown_instance,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "Rpl unknown instance"
  },
  {
    .key = "rpl_local_repair",
    .domain = app_statistics_domain_network,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT32,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_statistics.network.rpl_local_repair,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "Rpl local repair"
  },
  {
    .key = "rpl_global_repair",
    .domain = app_statistics_domain_network,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT32,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_statistics.network.rpl_global_repair,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "Rpl global repair"
  },
  {
    .key = "rpl_malformed_message",
    .domain = app_statistics_domain_network,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT32,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_statistics.network.rpl_malformed_message,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "Rpl malformed message"
  },
  {
    .key = "rpl_time_no_next_hop",
    .domain = app_statistics_domain_network,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT32,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_statistics.network.rpl_time_no_next_hop,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "Rpl time no next hop"
  },
  {
    .key = "rpl_total_memory",
    .domain = app_statistics_domain_network,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT32,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_statistics.network.rpl_total_memory,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "Rpl total memory"
  },
  {
    .key = "buf_alloc",
    .domain = app_statistics_domain_network,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT32,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_statistics.network.buf_alloc,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "Buf alloc"
  },
  {
    .key = "buf_headroom_realloc",
    .domain = app_statistics_domain_network,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT32,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_statistics.network.buf_headroom_realloc,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "Buf headroom realloc"
  },
  {
    .key = "buf_headroom_shuffle",
    .domain = app_statistics_domain_network,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT32,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_statistics.network.buf_headroom_shuffle,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "Buf headroom shuffle"
  },
  {
    .key = "buf_headroom_fail",
    .domain = app_statistics_domain_network,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT32,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_statistics.network.buf_headroom_fail,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "Buf headroom fail"
  },
  {
    .key = "etx_1st_parent",
    .domain = app_statistics_domain_network,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT16,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_statistics.network.etx_1st_parent,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "Etx 1st parent"
  },
  {
    .key = "etx_2nd_parent",
    .domain = app_statistics_domain_network,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT16,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_statistics.network.etx_2nd_parent,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "Etx 2nd parent"
  },
  {
    .key = "adapt_layer_tx_queue_size",
    .domain = app_statistics_domain_network,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT16,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_statistics.network.adapt_layer_tx_queue_size,
    .input_enum_list =  NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "Adapt layer tx queue size"
  },
  {
    .key = "adapt_layer_tx_queue_peak",
    .domain = app_statistics_domain_network,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT16,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_statistics.network.adapt_layer_tx_queue_peak,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "Adapt layer tx queue peak"
  },
  {
    .key = "tx_duration_ms",
    .domain = app_statistics_domain_regulation,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT32,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_statistics.regulation.arib.tx_duration_ms,
    .input_enum_list = (void *)SL_WISUN_REGULATION_ARIB,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "Total transmission duration during last hour in milliseconds"
  },
  {
    .key = "arena",
    .domain = app_statistics_domain_heap,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT32,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_statistics.heap.arena,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "Maximum heap usage"
  },
  {
    .key = "uordblks",
    .domain = app_statistics_domain_heap,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT32,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_statistics.heap.uordblks,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "Current heap usage"
  },
  {
    .key = NULL,
    .domain = 0,
    .value_size = APP_SETTINGS_VALUE_SIZE_NONE,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = NULL,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = NULL,
    .description = NULL
  }
};

static const app_settings_entry_t app_info_entries[] =
{
  {
    .key = "pan_id",
    .domain = app_info_domain_network,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT16,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_network_info.pan_id,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "PAN ID"
  },
  {
    .key = NULL,
    .domain = 0,
    .value_size = APP_SETTINGS_VALUE_SIZE_NONE,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = NULL,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = NULL,
    .description = NULL
  }
};

static const app_settings_entry_t app_rpl_entries[] =
{
  {
    .key = "instance_id",
    .domain = app_info_domain_rpl,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT8,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_rpl_info.instance_id,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "Instance ID"
  },
  {
    .key = "dodag_version_number",
    .domain = app_info_domain_rpl,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT8,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_rpl_info.dodag_version_number,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "DODAG Version Number"
  },
  {
    .key = "dodag_rank",
    .domain = app_info_domain_rpl,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT16,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_rpl_info.dodag_rank,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "DODAG Rank"
  },
  {
    .key = "grounded",
    .domain = app_info_domain_rpl,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT8,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_rpl_info.grounded,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "Grounded"
  },
  {
    .key = "mode_of_operation",
    .domain = app_info_domain_rpl,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT8,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_rpl_info.mode_of_operation,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "Mode of Operation (MOF)"
  },
  {
    .key = "dodag_preference",
    .domain = app_info_domain_rpl,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT8,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_rpl_info.dodag_preference,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "DODAG Preference"
  },
  {
    .key = "dodag_dtsn",
    .domain = app_info_domain_rpl,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT8,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_rpl_info.dodag_dtsn,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "Destination Advertisement Trigger Sequence Number (DTSN)"
  },
  {
    .key = "dio_interval_min",
    .domain = app_info_domain_rpl,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT8,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_rpl_info.dio_interval_min,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "DIO Minimum Interval"
  },
  {
    .key = "dio_interval_doublings",
    .domain = app_info_domain_network,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT8,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_rpl_info.dio_interval_doublings,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "DIO Interval Doublings"
  },
  {
    .key = "dio_redundancy_constant",
    .domain = app_info_domain_rpl,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT8,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_rpl_info.dio_redundancy_constant,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "DIO Redundancy Constant"
  },
  {
    .key = "default_lifetime",
    .domain = app_info_domain_rpl,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT8,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_rpl_info.default_lifetime,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "Default Lifetime"
  },
  {
    .key = "lifetime_unit",
    .domain = app_info_domain_rpl,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT16,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = &app_rpl_info.lifetime_unit,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_integer,
    .description = "Lifetime Unit"
  },
  {
    .key = NULL,
    .domain = 0,
    .value_size = APP_SETTINGS_VALUE_SIZE_NONE,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = NULL,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = NULL,
    .description = NULL
  }
};

static sl_status_t app_settings_get_phy_config(char *value_str,
                                               const char *key_str,
                                               const app_settings_entry_t *entry,
                                               bool unused)
{
  sl_status_t status;

  status = app_settings_get_integer(value_str, key_str, entry);
  if (status == SL_STATUS_OK && unused) {
    strcat(value_str, " (unused)");
  }

  return status;
}

static sl_status_t app_settings_get_fan10_phy_config(char *value_str,
                                                     const char *key_str,
                                                     const app_settings_entry_t *entry)
{
  bool used = app_settings_wisun.phy_config_type == SL_WISUN_PHY_CONFIG_FAN10;
  return app_settings_get_phy_config(value_str, key_str, entry, !used);
}

static sl_status_t app_settings_get_fan11_phy_config(char *value_str,
                                                     const char *key_str,
                                                     const app_settings_entry_t *entry)
{
  bool used = app_settings_wisun.phy_config_type == SL_WISUN_PHY_CONFIG_FAN11;
  return app_settings_get_phy_config(value_str, key_str, entry, !used);
}

static sl_status_t app_settings_get_explicit_phy_config(char *value_str,
                                                        const char *key_str,
                                                        const app_settings_entry_t *entry)
{
  bool used = (app_settings_wisun.phy_config_type == SL_WISUN_PHY_CONFIG_EXPLICIT
               || app_settings_wisun.phy_config_type == SL_WISUN_PHY_CONFIG_CUSTOM_FSK
               || app_settings_wisun.phy_config_type == SL_WISUN_PHY_CONFIG_CUSTOM_OFDM
               || app_settings_wisun.phy_config_type == SL_WISUN_PHY_CONFIG_CUSTOM_OQPSK);
  return app_settings_get_phy_config(value_str, key_str, entry, !used);
}

static sl_status_t app_settings_get_custom_phy_config(char *value_str,
                                                      const char *key_str,
                                                      const app_settings_entry_t *entry)
{
  bool used = (app_settings_wisun.phy_config_type == SL_WISUN_PHY_CONFIG_CUSTOM_FSK
               || app_settings_wisun.phy_config_type == SL_WISUN_PHY_CONFIG_CUSTOM_OFDM
               || app_settings_wisun.phy_config_type == SL_WISUN_PHY_CONFIG_CUSTOM_OQPSK);
  return app_settings_get_phy_config(value_str, key_str, entry, !used);
}

static sl_status_t app_settings_get_custom_fsk_oqpsk_phy_config(char *value_str,
                                                                const char *key_str,
                                                                const app_settings_entry_t *entry)
{
  bool used = (app_settings_wisun.phy_config_type == SL_WISUN_PHY_CONFIG_CUSTOM_FSK
               || app_settings_wisun.phy_config_type == SL_WISUN_PHY_CONFIG_CUSTOM_OQPSK);
  return app_settings_get_phy_config(value_str, key_str, entry, !used);
}

static sl_status_t app_settings_get_custom_ofdm_phy_config(char *value_str,
                                                           const char *key_str,
                                                           const app_settings_entry_t *entry)
{
  bool used = (app_settings_wisun.phy_config_type == SL_WISUN_PHY_CONFIG_CUSTOM_OFDM);
  return app_settings_get_phy_config(value_str, key_str, entry, !used);
}

static sl_status_t app_settings_get_ids_phy_config(char *value_str,
                                                   const char *key_str,
                                                   const app_settings_entry_t *entry)
{
  bool used = app_settings_wisun.phy_config_type == SL_WISUN_PHY_CONFIG_IDS;
  return app_settings_get_phy_config(value_str, key_str, entry, !used);
}

static sl_status_t app_settings_get_fan10_and_fan11_phy_config(char *value_str,
                                                               const char *key_str,
                                                               const app_settings_entry_t *entry)
{
  bool used = app_settings_wisun.phy_config_type == SL_WISUN_PHY_CONFIG_FAN10
           || app_settings_wisun.phy_config_type == SL_WISUN_PHY_CONFIG_FAN11;
  return app_settings_get_phy_config(value_str, key_str, entry, !used);
}

static sl_status_t app_settings_get_fan11_and_explicit_and_ids_phy_config(char *value_str,
                                                                          const char *key_str,
                                                                          const app_settings_entry_t *entry)
{
  bool used = app_settings_wisun.phy_config_type == SL_WISUN_PHY_CONFIG_FAN11
           || app_settings_wisun.phy_config_type == SL_WISUN_PHY_CONFIG_EXPLICIT
           || app_settings_wisun.phy_config_type == SL_WISUN_PHY_CONFIG_CUSTOM_FSK
           || app_settings_wisun.phy_config_type == SL_WISUN_PHY_CONFIG_CUSTOM_OFDM
           || app_settings_wisun.phy_config_type == SL_WISUN_PHY_CONFIG_CUSTOM_OQPSK
           || app_settings_wisun.phy_config_type == SL_WISUN_PHY_CONFIG_IDS;
  return app_settings_get_phy_config(value_str, key_str, entry, !used);
}

static sl_status_t app_settings_set_gtk(const char *value_str,
                                        const char *key_str,
                                        const app_settings_entry_t *entry)
{
  sl_status_t status;
  uint16_t key_index;
  int ret;

  (void)key_str;

  ret = sscanf(entry->key, "gtk%hu", &key_index);
  if (ret != 1) {
    return SL_STATUS_FAIL;
  }

  if (strcmp(value_str, "none") == 0){
    app_settings_wisun.gtk_set &= ~(1 << (key_index - 1));
    return SL_STATUS_OK;
  } else {
    status = app_util_get_byte_array(value_str, entry->value, 16);
    if (status == SL_STATUS_OK) {
      app_settings_wisun.gtk_set |= 1 << (key_index - 1);
    }
    return status;
  }
}

static sl_status_t app_settings_get_gtk(char *value_str,
                                        const char *key_str,
                                        const app_settings_entry_t *entry)
{
  (void)key_str;

  const uint8_t *gtk = (uint8_t *) entry->value;
  uint16_t key_index;
  int ret;

  ret = sscanf(entry->key, "gtk%hu", &key_index);
  if (ret != 1) {
    return SL_STATUS_FAIL;
  }

  if (app_settings_wisun.gtk_set & (1 << (key_index - 1))) {
    sprintf(value_str, "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
            *gtk,        *(gtk + 1),  *(gtk + 2),  *(gtk + 3),
            *(gtk + 4),  *(gtk + 5),  *(gtk + 6),  *(gtk + 7),
            *(gtk + 8),  *(gtk + 9),  *(gtk + 10), *(gtk + 11),
            *(gtk + 12), *(gtk + 13), *(gtk + 14), *(gtk + 15));
    return SL_STATUS_OK;
  } else {
    return SL_STATUS_FAIL;
  }
}

static sl_status_t app_settings_set_lgtk(const char *value_str,
                                         const char *key_str,
                                         const app_settings_entry_t *entry)
{
  sl_status_t status;
  uint16_t key_index;
  int ret;

  (void)key_str;

  ret = sscanf(entry->key, "lgtk%hu", &key_index);
  if (ret != 1) {
    return SL_STATUS_FAIL;
  }

  key_index += 4;

  if (strcmp(value_str, "none") == 0){
    app_settings_wisun.gtk_set &= ~(1 << (key_index - 1));
    return SL_STATUS_OK;
  } else {
    status = app_util_get_byte_array(value_str, entry->value, 16);
    if (status == SL_STATUS_OK) {
      app_settings_wisun.gtk_set |= 1 << (key_index - 1);
    }
    return status;
  }
}

static sl_status_t app_settings_get_lgtk(char *value_str,
                                         const char *key_str,
                                         const app_settings_entry_t *entry)
{
  (void)key_str;

  const uint8_t *lgtk = (uint8_t *) entry->value;
  uint16_t key_index;
  int ret;

  ret = sscanf(entry->key, "lgtk%hu", &key_index);
  if (ret != 1) {
    return SL_STATUS_FAIL;
  }

  key_index += 4;

  if (app_settings_wisun.gtk_set & (1 << (key_index - 1))) {
    sprintf(value_str, "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
            *lgtk,        *(lgtk + 1),  *(lgtk + 2),  *(lgtk + 3),
            *(lgtk + 4),  *(lgtk + 5),  *(lgtk + 6),  *(lgtk + 7),
            *(lgtk + 8),  *(lgtk + 9),  *(lgtk + 10), *(lgtk + 11),
            *(lgtk + 12), *(lgtk + 13), *(lgtk + 14), *(lgtk + 15));
    return SL_STATUS_OK;
  } else {
    return SL_STATUS_FAIL;
  }
}

static sl_status_t app_settings_get_gak(char *value_str,
                                        const char *key_str,
                                        const app_settings_entry_t *entry)
{
  (void)key_str;

  sl_wisun_br_state_t state;
  uint8_t *gak = (uint8_t *) entry->value;
  uint16_t key_index;
  int ret;

  if(!sl_wisun_br_get_state(&state) && state != SL_WISUN_BR_STATE_OPERATIONAL) {
    return SL_STATUS_FAIL;
  }

  ret = sscanf(entry->key, "gak%hu", &key_index);
  if (ret != 1) {
    return SL_STATUS_FAIL;
  }

  sl_wisun_br_get_gak(key_index - 1, gak);

  sprintf(value_str, "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
          *gak,        *(gak + 1),  *(gak + 2),  *(gak + 3),
          *(gak + 4),  *(gak + 5),  *(gak + 6),  *(gak + 7),
          *(gak + 8),  *(gak + 9),  *(gak + 10), *(gak + 11),
          *(gak + 12), *(gak + 13), *(gak + 14), *(gak + 15));

  return SL_STATUS_OK;
}

static sl_status_t app_settings_get_lgak(char *value_str,
                                         const char *key_str,
                                         const app_settings_entry_t *entry)
{
  (void)key_str;

  sl_wisun_br_state_t state;
  uint8_t *lgak = (uint8_t *) entry->value;
  uint16_t key_index;
  int ret;

  if(!sl_wisun_br_get_state(&state) && state != SL_WISUN_BR_STATE_OPERATIONAL) {
    return SL_STATUS_FAIL;
  }

  ret = sscanf(entry->key, "lgak%hu", &key_index);
  if (ret != 1) {
    return SL_STATUS_FAIL;
  }

  key_index += 4;

  sl_wisun_br_get_gak(key_index - 1, lgak);

  sprintf(value_str, "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
          *lgak,        *(lgak + 1),  *(lgak + 2),  *(lgak + 3),
          *(lgak + 4),  *(lgak + 5),  *(lgak + 6),  *(lgak + 7),
          *(lgak + 8),  *(lgak + 9),  *(lgak + 10), *(lgak + 11),
          *(lgak + 12), *(lgak + 13), *(lgak + 14), *(lgak + 15));

  return SL_STATUS_OK;
}

static sl_status_t app_settings_get_mac_address(char *value_str,
                                                const char *key_str,
                                                const app_settings_entry_t *entry)
{
  sl_status_t ret;
  sl_wisun_mac_address_t address;
  (void)key_str;
  (void)entry;

  ret = sl_wisun_get_mac_address(&address);
  if (ret == SL_STATUS_OK) {
    ret = app_util_get_mac_address_string(value_str, &address);
  }

  return ret;
}

static sl_status_t app_settings_get_ip_addresses(char *value_str,
                                                 const char *key_str,
                                                 const app_settings_entry_t *entry)
{
  uint8_t addr_ll[16], addr_gua[16], addr_dodagid[16];
  char str_ll[40], str_gua[40], str_dodagid[40];

  int count;
  (void)key_str;
  (void)entry;

  count = sprintf(value_str, "[");
  value_str += count;

  sl_wisun_br_get_ip_addresses(addr_ll, addr_gua, addr_dodagid);
  ip6tos(addr_ll, str_ll);
  ip6tos(addr_gua, str_gua);
  ip6tos(addr_dodagid, str_dodagid);

  count = sprintf(value_str,"ll: %s gua: %s dodagid: %s", str_ll, str_gua, str_dodagid);
  value_str += count;

  count = sprintf(value_str, "]");
  value_str += count;

  return SL_STATUS_OK;
}

static sl_status_t app_settings_get_state(char *value_str,
                                          const char *key_str,
                                          const app_settings_entry_t *entry)
{
  (void)key_str;

  sl_wisun_br_state_t state;
  uint8_t value_length = sizeof(sl_wisun_br_state_t);

  if (sl_wisun_br_get_state(&state) != SL_STATUS_OK) {
    return SL_STATUS_FAIL;
  }

  return app_util_get_string(value_str,
                            state,
                            entry->output_enum_list,
                            false,
                            false,
                            value_length);
}

static sl_status_t app_settings_get_neighbors(char *value_str,
                                              const char *key_str,
                                              const app_settings_entry_t *entry)
{
  sl_status_t ret;
  uint8_t neighbor_count, idx, j;
  sl_wisun_mac_address_t *neighbor_mac_addresses;
  (void)key_str;
  (void)entry;

  ret = sl_wisun_get_neighbor_count(&neighbor_count);
  if (ret == SL_STATUS_OK && neighbor_count) {
    neighbor_mac_addresses = sl_malloc(neighbor_count * sizeof(sl_wisun_mac_address_t));
    if (neighbor_mac_addresses) {
      ret = sl_wisun_get_neighbors(&neighbor_count, neighbor_mac_addresses);
      if (ret == SL_STATUS_OK) {
        printf("%s.%s = [\r\n", app_settings_domain_str[entry->domain], entry->key);
        for (idx = 0; idx < neighbor_count; idx++) {
          sl_wisun_neighbor_info_t neighbor_info;
          app_util_get_mac_address_string(value_str, neighbor_mac_addresses + idx);
          printf("  %s\r\n", value_str);
          ret = sl_wisun_get_neighbor_info(neighbor_mac_addresses + idx, &neighbor_info);
          if (ret == SL_STATUS_OK)  {
            app_util_get_string(value_str, neighbor_info.type, app_settings_wisun_neighbor_type_enum, false, false, 0);
            printf("    type = %s\r\n", value_str);
            ip6tos(neighbor_info.link_local_address.address, value_str);
            printf("    ll = %s\r\n", value_str);
            ip6tos(neighbor_info.global_address.address, value_str);
            if (strcmp(value_str, "::")) {
              printf("    gua = %s\r\n", value_str);
            }
            printf("    lifetime = %lu\r\n", neighbor_info.lifetime);
            printf("    mac_tx_count = %lu\r\n", neighbor_info.mac_tx_count);
            printf("    mac_tx_failed_count = %lu\r\n", neighbor_info.mac_tx_failed_count);
            printf("    mac_tx_ms_count = %lu\r\n", neighbor_info.mac_tx_ms_count);
            printf("    mac_tx_ms_failed_count = %lu\r\n", neighbor_info.mac_tx_ms_failed_count);
            printf("    mac_rx_count = %lu\r\n", neighbor_info.mac_rx_count);
            if (neighbor_info.rpl_rank != 0xFFFF) {
              printf("    rpl_rank = %hu\r\n", neighbor_info.rpl_rank);
            }
            if (neighbor_info.etx != 0xFFFF) {
              printf("    etx = %hu\r\n", neighbor_info.etx);
            }
            if (neighbor_info.rsl_out != 0xFF) {
              printf("    rsl_out = %d dBm\r\n", (int16_t)neighbor_info.rsl_out - 174);
            }
            if (neighbor_info.rsl_in != 0xFF) {
              printf("    rsl_in = %d dBm\r\n", (int16_t)neighbor_info.rsl_in - 174);
            }
            app_util_get_string(value_str, neighbor_info.is_lfn, app_settings_wisun_is_lfn_enum, false, false, 0);
            printf("    device_type = %s\r\n", value_str);
            printf("    phy_mode_ids = ");
            if (neighbor_info.phy_mode_id_count != 0) {
              for (j = 0; j < neighbor_info.phy_mode_id_count - 1; j++) {
                printf("0x%02x,", neighbor_info.phy_mode_ids[j]);
              }
              // Last one without coma
              printf("0x%02x\r\n", neighbor_info.phy_mode_ids[j]);
            } else {
              printf("none\r\n");
            }
            printf("    is_mdr_command_capable = %s\r\n", neighbor_info.is_mdr_command_capable ? "true" : "false");
          }
        }
        printf("]\r\n");
      }
      sl_free(neighbor_mac_addresses);
    }
  }

  // Prevent parent from printing anything
  return SL_STATUS_FAIL;
}

static sl_status_t app_settings_get_statistics(char *value_str,
                                               const char *key_str,
                                               const app_settings_entry_t *entry)
{
  sl_status_t ret;
  const app_settings_entry_t *iter;
  sl_wisun_statistics_type_t statistics_type;

  // API statistics type is stored in the settings entry value.
  #ifdef __GNUC__
  statistics_type = (sl_wisun_statistics_type_t) entry->value;
  #elif defined __ICCARM__
  statistics_type = (sl_wisun_statistics_type_t)((uint32_t) entry->value & 0xFF);
  #endif

  // Update statistics
  ret = sl_wisun_get_statistics(statistics_type, &app_statistics);
  if (ret != SL_STATUS_OK) {
    printf("[Failed to retrieve statistics: %lu]\r\n", ret);
    return SL_STATUS_FAIL;
  }

  iter = app_statistics_entries;
  while (iter->key) {
    if (!strcmp(entry->key, app_statistics_domain_str[iter->domain])) {
      if (!key_str || !strcmp(iter->key, key_str)) {
        if (iter->get_handler) {
          ret = iter->get_handler(value_str, NULL, iter);
          if (ret == SL_STATUS_OK) {
            printf("%s.%s.%s = %s\r\n", app_settings_domain_str[entry->domain], app_statistics_domain_str[iter->domain], iter->key, value_str);
          }
        }
      }
    }
    iter++;
  }

  // Prevent parent from printing anything
  return SL_STATUS_FAIL;
}

static sl_status_t app_settings_get_statistics_regulation(char *value_str,
                                                          const char *key_str,
                                                          const app_settings_entry_t *entry)
{
  sl_status_t ret;
  const app_settings_entry_t *iter;

  // Update statistics
  ret = sl_wisun_get_statistics(SL_WISUN_STATISTICS_TYPE_REGULATION, &app_statistics);
  if (ret != SL_STATUS_OK) {
    return ret;
  }

  iter = app_statistics_entries;
  while (iter->key) {
    if (!strcmp(entry->key, app_statistics_domain_str[iter->domain])) {
      // Associated regional regulation is coded in input_enum_list.
      if ((uintptr_t)app_settings_wisun.regulation == (uintptr_t)iter->input_enum_list) {
        if (!key_str || !strcmp(iter->key, key_str)) {
          if (iter->get_handler) {
            ret = iter->get_handler(value_str, NULL, iter);
            if (ret == SL_STATUS_OK) {
              printf("%s.%s.%s = %s\r\n", app_settings_domain_str[entry->domain], app_statistics_domain_str[iter->domain], iter->key, value_str);
            }
          }
        }
      }
    }
    iter++;
  }

  // Prevent parent from printing anything
  return SL_STATUS_FAIL;
}

static sl_status_t app_settings_get_channel_mask_str(sl_wisun_channel_mask_type_t type, char *value_str)


{
  sl_status_t status;
  sl_wisun_channel_mask_t channel_mask;
  uint8_t channel_count;
  uint8_t index;

  status = sl_wisun_get_excluded_channel_mask(type, &channel_mask, &channel_count);

  strcpy(value_str, "--");

  if (status == SL_STATUS_OK && channel_count > 0) {
    sprintf(value_str, "%02x", channel_mask.mask[0]);
    value_str += 2;
    for (index = 1; index < (channel_count + 7) / 8; index++) {
      sprintf(value_str, ":%02x", channel_mask.mask[index]);
      value_str += 3;
    }
  }

  return SL_STATUS_OK;
}

static sl_status_t app_settings_get_async_channel_mask_str(char *value_str,
                                                           const char *key_str,
                                                           const app_settings_entry_t *entry)
{
  (void)key_str;
  (void)entry;

  return app_settings_get_channel_mask_str(SL_WISUN_CHANNEL_MASK_TYPE_EFFECTIVE_ASYNC, value_str);
}

static sl_status_t app_settings_get_unicast_channel_mask_str(char *value_str,
                                                             const char *key_str,
                                                             const app_settings_entry_t *entry)
{
  (void)key_str;
  (void)entry;

  return app_settings_get_channel_mask_str(SL_WISUN_CHANNEL_MASK_TYPE_EFFECTIVE_UNICAST, value_str);
}

static sl_status_t app_settings_get_broadcast_channel_mask_str(char *value_str,
                                                               const char *key_str,
                                                               const app_settings_entry_t *entry)
{
  (void)key_str;
  (void)entry;

  return app_settings_get_channel_mask_str(SL_WISUN_CHANNEL_MASK_TYPE_EFFECTIVE_BROADCAST, value_str);
}

static sl_status_t app_ranges_to_mask(const char *str, uint8_t *mask, uint32_t size)
{
  char *endptr;
  uint32_t cur, end, index;
  memset(mask, 0, size * sizeof(uint8_t));
  do {
    if (*str == '\0') {
      return SL_STATUS_FAIL;
    }
    cur = strtoul(str, &endptr, 0);
    if (*endptr == '-') {
      str = endptr + 1;
      end = strtoul(str, &endptr, 0);
    } else {
      end = cur;
    }
    if (*endptr != '\0' && *endptr != ',') {
      return SL_STATUS_FAIL;
    }
    if (cur > end) {
      return SL_STATUS_FAIL;
    }
    for (; cur <= end; cur++) {
      index = cur / 8;
      if (index < size) {
        mask[index] |= 1 << (cur % 8);
      } else {
        return SL_STATUS_FAIL;
      }
    }
    str = endptr + 1;
  } while (*endptr != '\0');

  return SL_STATUS_OK;
}

sl_status_t app_settings_get_channel_mask(const char *str, sl_wisun_channel_mask_t *channel_mask)
{
  return app_ranges_to_mask(str, channel_mask->mask, SL_WISUN_CHANNEL_MASK_SIZE);
}

static sl_status_t app_settings_set_allowed_channels(const char *value_str,
                                                     const char *key_str,
                                                     const app_settings_entry_t *entry)
{
  sl_status_t ret;
  (void)key_str;
  (void)entry;
  sl_wisun_channel_mask_t channel_mask;

  ret = app_settings_get_channel_mask(value_str, &channel_mask);
  if (ret == SL_STATUS_OK) {
    ret = app_settings_set_string(value_str, key_str, entry);
  }

  return ret;
}

static sl_status_t app_settings_set_trace_filter(const char *value_str,
                                                 const char *key_str,
                                                 const app_settings_entry_t *entry)
{
  sl_status_t ret;
  const app_settings_entry_t *iter;
  (void)entry;

  if (!value_str) {
    return SL_STATUS_INVALID_KEY;
  }

  iter = app_trace_filter_entries;
  while (iter->key) {
    if (!key_str || !strcmp(iter->key, key_str)) {
      if (iter->set_handler) {
        ret = iter->set_handler(value_str, key_str, iter);
        if (ret != SL_STATUS_OK) {
          return ret;
        } else {
          return sl_wisun_set_trace_filter(app_settings_wisun.trace_filter);
        }
      } else {
        return SL_STATUS_PERMISSION;
      }
    }
    iter++;
  }

  return SL_STATUS_OK;
}

static sl_status_t app_settings_get_trace_filter(char *value_str,
                                                 const char *key_str,
                                                 const app_settings_entry_t *entry)
{
  sl_status_t ret;
  const app_settings_entry_t *iter;
  char trace_filter_string[128];
  (void)value_str;

  iter = app_trace_filter_entries;
  while (iter->key) {
    if (!key_str || !strcmp(iter->key, key_str)) {
      if (iter->get_handler) {
        ret = iter->get_handler(trace_filter_string, NULL, iter);
        if (ret == SL_STATUS_OK) {
          printf("%s.%s.%s = %s\r\n", app_settings_domain_str[entry->domain], entry->key, iter->key, trace_filter_string);
        }
      }
    }
    iter++;
  }

  // Prevent parent from printing anything
  return SL_STATUS_FAIL;
}

static sl_status_t app_settings_set_regulation(const char *value_str,
                                               const char *key_str,
                                               const app_settings_entry_t *entry)
{
  sl_status_t ret;
  sl_wisun_br_state_t state;

  sl_wisun_br_get_state(&state);

  if (state == SL_WISUN_BR_STATE_OPERATIONAL) {
    return SL_STATUS_INVALID_STATE;
  }

  ret = app_settings_set_integer(value_str, key_str, entry);

  return ret;
}


static sl_status_t app_settings_set_regulation_warning_threshold(const char *value_str,
                                                                 const char *key_str,
                                                                 const app_settings_entry_t *entry)
{
  sl_status_t ret;

  ret = app_settings_set_integer(value_str, key_str, entry);
  if (ret == SL_STATUS_OK) {
    ret = sl_wisun_set_regulation_tx_thresholds(app_settings_wisun.regulation_warning_threshold,
                                                app_settings_wisun.regulation_alert_threshold);
  }

  return ret;
}

static sl_status_t app_settings_set_regulation_alert_threshold(const char *value_str,
                                                               const char *key_str,
                                                               const app_settings_entry_t *entry)
{
  sl_status_t ret;

  ret = app_settings_set_integer(value_str, key_str, entry);
  if (ret == SL_STATUS_OK) {
    ret = sl_wisun_set_regulation_tx_thresholds(app_settings_wisun.regulation_warning_threshold,
                                                app_settings_wisun.regulation_alert_threshold);
  }

  return ret;
}

#if RAIL_IEEE802154_SUPPORTS_G_MODESWITCH

static sl_status_t app_settings_set_rx_mdr_capable(const char *value_str,
                                                   const char *key_str,
                                                   const app_settings_entry_t *entry)
{
  sl_status_t ret;
  uint8_t phy_mode_id_count, is_mdr_command_capable;
  uint8_t phy_mode_id[SL_WISUN_MAX_PHY_MODE_ID_COUNT];
  uint8_t *phy_mode_id_p, *phy_mode_id_count_p;

  ret = app_settings_set_integer(value_str, key_str, entry);

  if (ret == SL_STATUS_OK) {

    if (app_settings_wisun.rx_phy_mode_ids_count == 0) {
      // Check if default PhyList can be retrieved from device
      if (sl_wisun_get_pom_ie(&phy_mode_id_count, phy_mode_id, &is_mdr_command_capable) == SL_STATUS_OK) {
        phy_mode_id_p = phy_mode_id;
        phy_mode_id_count_p = &phy_mode_id_count;
      } else {
        // POM-IE not available yet
        return ret;
      }
    } else {
      phy_mode_id_p = app_settings_wisun.rx_phy_mode_ids;
      phy_mode_id_count_p = &app_settings_wisun.rx_phy_mode_ids_count;
    }

    ret = sl_wisun_set_pom_ie(*phy_mode_id_count_p,
                              phy_mode_id_p,
                              app_settings_wisun.rx_mdr_capable);
  }

  return ret;
}

static sl_status_t app_settings_set_rx_phy_mode_ids(const char *value_str,
                                                    const char *key_str,
                                                    const app_settings_entry_t *entry)
{
  sl_status_t ret = SL_STATUS_OK;
  char *end_str, *start_str;
  uint8_t phy_mode_id;
  uint8_t phy_mode_ids_count = 0;
  uint8_t phy_mode_ids[SL_WISUN_MAX_PHY_MODE_ID_COUNT];


  (void)key_str;
  (void)entry;

  start_str = (char *) value_str;

  do {
    phy_mode_id = strtoul(start_str, &end_str, 0);

    if (end_str == start_str) {
      // No integer found
      ret = SL_STATUS_INVALID_PARAMETER;
      break;
    }

    phy_mode_ids[phy_mode_ids_count] = phy_mode_id;
    phy_mode_ids_count++;

    if (*end_str == ',') {
      // list separator, point to next element
      end_str++;
    }
    start_str = end_str;
  } while (*end_str != '\0');

  if (ret == SL_STATUS_OK) {
    memcpy(app_settings_wisun.rx_phy_mode_ids, phy_mode_ids, phy_mode_ids_count);
    app_settings_wisun.rx_phy_mode_ids_count = phy_mode_ids_count;

    ret = sl_wisun_set_pom_ie(app_settings_wisun.rx_phy_mode_ids_count,
                              app_settings_wisun.rx_phy_mode_ids,
                              app_settings_wisun.rx_mdr_capable);
  }

  return ret;
}

static sl_status_t app_settings_get_rx_phy_mode_ids(char *value_str,
                                                    const char *key_str,
                                                    const app_settings_entry_t *entry)
{
  int i;
  sl_status_t ret = SL_STATUS_OK;
  uint8_t phy_mode_id_count, is_mdr_command_capable;
  uint8_t phy_mode_id[SL_WISUN_MAX_PHY_MODE_ID_COUNT];
  uint8_t *phy_mode_id_p, *phy_mode_id_count_p;

  (void) key_str;
  (void) entry;


  if (app_settings_wisun.rx_phy_mode_ids_count == 0) {
    // Check if default PhyList can be retrieved from device
    if (sl_wisun_get_pom_ie(&phy_mode_id_count, phy_mode_id, &is_mdr_command_capable) == SL_STATUS_OK) {
      phy_mode_id_p = phy_mode_id;
      phy_mode_id_count_p = &phy_mode_id_count;
    } else {
      ret = SL_STATUS_FAIL;
    }
  } else {
    phy_mode_id_p = app_settings_wisun.rx_phy_mode_ids;
    phy_mode_id_count_p = &app_settings_wisun.rx_phy_mode_ids_count;
  }

  if (ret == SL_STATUS_OK) {
    if (*phy_mode_id_count_p == 0) {
      sprintf(value_str, "no PhyModeId set");
    } else {
      for (i = 0; i < *phy_mode_id_count_p - 1; i ++) {
        sprintf(value_str, "0x%02x,", phy_mode_id_p[i]);
        value_str += 5;
      }
      // Last one without coma
      sprintf(value_str, "0x%02x", phy_mode_id_p[i]);
    }
  }

  return SL_STATUS_OK;
}
#endif

static sl_status_t app_settings_get_network_info(char *value_str,
                                                 const char *key_str,
                                                 const app_settings_entry_t *entry)
{
  sl_status_t ret;
  const app_settings_entry_t *iter;

  ret = sl_wisun_get_network_info(&app_network_info);
  if (ret != SL_STATUS_OK) {
    printf("[Failed to retrieve Wi-SUN network information: %lu]\r\n", ret);
    return SL_STATUS_FAIL;
  }

  iter = app_info_entries;
  while (iter->key) {
    if (!strcmp(entry->key, app_info_domain_str[iter->domain])) {
      if (!key_str || !strcmp(iter->key, key_str)) {
        if (iter->get_handler) {
          ret = iter->get_handler(value_str, NULL, iter);
          if (ret == SL_STATUS_OK) {
            printf("%s.%s.%s = %s\r\n", app_settings_domain_str[entry->domain], app_info_domain_str[iter->domain], iter->key, value_str);
          }
        }
      }
    }
    iter++;
  }

  // Prevent parent from printing anything
  return SL_STATUS_FAIL;
}

static sl_status_t app_settings_get_rpl_info(char *value_str,
                                             const char *key_str,
                                             const app_settings_entry_t *entry)
{
  sl_status_t ret;
  const app_settings_entry_t *iter;

  ret = sl_wisun_get_rpl_info(&app_rpl_info);
  if (ret != SL_STATUS_OK) {
    printf("[Failed to retrieve Wi-SUN RPL information: %lu]\r\n", ret);
    return SL_STATUS_FAIL;
  }

  iter = app_rpl_entries;
  while (iter->key) {
    if (!strcmp(entry->key, app_info_domain_str[iter->domain])) {
      if (!key_str || !strcmp(iter->key, key_str)) {
        if (iter->get_handler) {
          ret = iter->get_handler(value_str, NULL, iter);
          if (ret == SL_STATUS_OK) {
            printf("%s.%s.%s = %s\r\n", app_settings_domain_str[entry->domain], app_info_domain_str[iter->domain], iter->key, value_str);
          }
        }
      }
    }
    iter++;
  }

  // Prevent parent from printing anything
  return SL_STATUS_FAIL;
}

#ifdef SL_CATALOG_WISUN_BR_WIFI_PRESENT

sl_status_t app_settings_get_mac_address_string(char *value_str,
                                                const uint8_t *value)
{
  for (uint8_t i = 0; i < 6; ++i) {
    sprintf(value_str, "%02x:", value[i]);
    value_str += 3;
  }

  // Remove the last colon
  *(value_str - 1) = '\0';

  return SL_STATUS_OK;
}

static sl_status_t app_settings_get_wifi_info(char *value_str,
                                              const char *key_str,
                                              const app_settings_entry_t *entry)
{
  sl_status_t status;
  bool connected;
  uint16_t channel_number;
  uint8_t mac_address[6];
  uint8_t ipv6_address[16];
  char str[40];

  (void)value_str;
  (void)key_str;

  status = sl_wisun_br_wifi_get_info(&connected, &channel_number, mac_address, ipv6_address);
  if (status != SL_STATUS_OK) {
    printf("[Failed to retrieve Wi-Fi information: %lu]\r\n", status);
    return SL_STATUS_FAIL;
  }

  if (strcmp(entry->key, "wlan_state") == 0) {
    printf("%s.%s = %s\r\n", app_settings_domain_str[entry->domain], entry->key, connected ? "connected" : "disconnected");
  } else if (strcmp(entry->key, "channel_number") == 0) {
    printf("%s.%s = %d\r\n", app_settings_domain_str[entry->domain], entry->key, channel_number);
  } else if (strcmp(entry->key, "mac_address") == 0) {
    app_settings_get_mac_address_string(str, mac_address);
    printf("%s.%s = %s\r\n", app_settings_domain_str[entry->domain], entry->key, str);
  } else if (strcmp(entry->key, "ipv6_address") == 0) {
    ip6tos(ipv6_address, str);
    printf("%s.%s = %s\r\n", app_settings_domain_str[entry->domain], entry->key, str);
  }

  // Prevent parent from printing anything
  return SL_STATUS_FAIL;
}

#endif  // SL_CATALOG_WISUN_BR_WIFI_PRESENT

static sl_status_t app_settings_set_tx_power(const char *value_str,
                                             const char *key_str,
                                             const app_settings_entry_t *entry)
{
  (void)key_str;
  sl_status_t ret;
  uint32_t value;

  ret = app_util_get_integer(&value,
                             value_str,
                             entry->input_enum_list,
                             entry->input & APP_SETTINGS_INPUT_FLAG_SIGNED);

  if (ret == SL_STATUS_OK) {
    // If entry is in dBm, multiply by 10 to have ddBm
    if (entry->value_size == APP_SETTINGS_VALUE_SIZE_UINT8) {
      value = value * 10;
    }
    app_settings_wisun.tx_power_ddbm = (int16_t)value;
  }

  return ret;
}

static sl_status_t app_settings_get_tx_power(char *value_str,
                                             const char *key_str,
                                             const app_settings_entry_t *entry)
{
  sl_status_t status = SL_STATUS_OK;
  int16_t tx_power_ddbm;
  (void)key_str;

  tx_power_ddbm = app_settings_wisun.tx_power_ddbm;
  if (entry->value_size == APP_SETTINGS_VALUE_SIZE_UINT8) {
    // If entry is in dBm, print in float format
    sprintf(value_str, "%d.%d",
            tx_power_ddbm / 10,
            abs(tx_power_ddbm % 10));
  } else if (entry->value_size == APP_SETTINGS_VALUE_SIZE_UINT16) {
    sprintf(value_str, "%d", tx_power_ddbm);
  } else {
    status = SL_STATUS_FAIL;
  }

  return status;
}
/***************************************************************************//**
 * @file sl_wisun_cli.c
 * @brief Wi-SUN CLI handler
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

// -----------------------------------------------------------------------------
//                                   Includes
// -----------------------------------------------------------------------------
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "sl_wisun_cli.h"
#include "sl_component_catalog.h"
#include "sl_cli.h"
#include "sl_string.h"
#include "sl_wisun_api.h"
#include "sl_wisun_trace_util.h"
#include "sl_wisun_app_setting.h"
#include "sl_wisun_cli_util.h"
#include "sl_wisun_cli_core.h"
#include "sl_wisun_cli_config.h"
#include "sl_wisun_cli_settings.h"

#if defined(SL_CATALOG_WISUN_APP_CORE_PRESENT)
#include "sl_wisun_app_core_util.h"
#include "sl_wisun_app_core_config.h"
#endif

#if SL_WISUN_CLI_ENABLED

#if defined(SL_CATALOG_WISUN_PING_PRESENT) \
  && SL_WISUN_PING_CLI_ENABLED
#include "sl_wisun_ping_cli.h"
#endif

#if defined(SL_CATALOG_IPERF_PRESENT) \
  && SL_WISUN_IPERF_CLI_ENABLED
#include "sl_iperf_cli.h"
#endif

#if defined(SL_CATALOG_WISUN_OTA_DFU_PRESENT) \
  && SL_WISUN_OTA_DFU_CLI_ENABLED
#include "sl_wisun_ota_dfu_cli.h"
#include "sl_wisun_ota_dfu_config.h"
#endif

#if defined(SL_CATALOG_WISUN_NETWORK_MEASUREMENT_PRESENT) \
  && SL_WISUN_NETWORK_MEASUREMENT_CLI_ENABLED
#include "sl_wisun_network_measurement_cli.h"
#endif

#if defined(SL_CATALOG_WISUN_APP_STATUS_PRESENT) \
  && SL_WISUN_APP_STATUS_CLI_ENABLED
#include "sl_wisun_app_status_cli.h"
#endif

#if defined(SL_CATALOG_WISUN_CLI_UTIL_PRESENT) \
  && SL_WISUN_CLI_UTIL_CLI_ENABLED
#include "sl_wisun_cli_util_cli.h"
#endif

#if (defined(SL_CATALOG_WISUN_TCP_CLIENT_PRESENT) || defined(SL_CATALOG_WISUN_UDP_CLIENT_PRESENT)) \
  && SL_WISUN_CLIENT_CLI_ENABLED
#include "sl_wisun_client_cli.h"
#endif

#if defined(SL_CATALOG_WISUN_COLLECTOR_PRESENT) \
  && defined(SL_CATALOG_WISUN_COAP_PRESENT)     \
  && SL_WISUN_COLLECTOR_CLI_ENABLED
#include "sl_wisun_coap_collector_cli.h"
#endif

#if defined(SL_CATALOG_WISUN_COLLECTOR_PRESENT) \
  && !defined(SL_CATALOG_WISUN_COAP_PRESENT)    \
  && SL_WISUN_COLLECTOR_CLI_ENABLED
#include "sl_wisun_collector_cli.h"
#endif

#endif // SL_WISUN_CLI_ENABLED
// -----------------------------------------------------------------------------
//                              Macros and Typedefs
// -----------------------------------------------------------------------------

/// Internal Config: Enable FAN 1.1 configuration
#define APP_CLI_ENABLE_FAN11_CONFIG         (1U)

/// Internal Config: Enable Explicit configuration
#define APP_CLI_ENABLE_EXPLICIT_CONFIG      (1U)

/// Wisun CLI domain ID
#define APP_CLI_WISUN_DOMAIN_ID             (0U)

/// App CLI input flag: None
#define APP_CLI_INPUT_FLAG_NONE             APP_SETTINGS_INPUT_FLAG_NONE

/// App CLI input flag: Signed
#define APP_CLI_INPUT_FLAG_SIGNED           APP_SETTINGS_INPUT_FLAG_SIGNED

/// App CLI input flag: Default
#define APP_CLI_INPUT_FLAG_DEFAULT          APP_SETTINGS_INPUT_FLAG_DEFAULT

/// App CLI output flag: None
#define APP_CLI_OUTPUT_FLAG_NONE            APP_SETTINGS_OUTPUT_FLAG_NONE

/// App CLI output flag: Signed
#define APP_CLI_OUTPUT_FLAG_SIGNED          APP_SETTINGS_OUTPUT_FLAG_SIGNED

/// App CLI output flag: Hex
#define APP_CLI_OUTPUT_FLAG_HEX             APP_SETTINGS_OUTPUT_FLAG_HEX

/// App CLI output flag: Fixed size
#define APP_CLI_OUTPUT_FLAG_FIXEDSIZE       APP_SETTINGS_OUTPUT_FLAG_FIXEDSIZE

/// App CLI output flag: Default
#define APP_CLI_OUTPUT_FLAG_DEFAULT         APP_SETTINGS_OUTPUT_FLAG_DEFAULT

/// App CLI value: None
#define APP_CLI_VALUE_SIZE_NONE             APP_SETTINGS_VALUE_SIZE_NONE

/// App CLI output flag: uint8
#define APP_CLI_VALUE_SIZE_UINT8            APP_SETTINGS_VALUE_SIZE_UINT8

/// App CLI output flag: uint16
#define APP_CLI_VALUE_SIZE_UINT16           APP_SETTINGS_VALUE_SIZE_UINT16

/// App CLI output flag: uint32
#define APP_CLI_VALUE_SIZE_UINT32           APP_SETTINGS_VALUE_SIZE_UINT32

/// App CLI string value length
#define APP_CLI_STR_VALUE_LENGTH            APP_SETTINGS_STR_VALUE_LENGTH

/// App CLI paramater string name: regulatory domain
#define APP_CLI_PHY_PARAM_REG_DOMAIN_STR    "regulatory_domain"

/// App CLI paramater string name: operating mode
#define APP_CLI_PHY_PARAM_OP_MODE_STR       "operating_mode"

/// App CLI paramater string name: operating class
#define APP_CLI_PHY_PARAM_OP_CLASS_STR      "operating_class"

#if APP_CLI_ENABLE_EXPLICIT_CONFIG
/// App CLI paramater string name: number of channels
#define APP_CLI_PHY_PARAM_NUM_OF_CH_STR     "number_of_channels"

/// App CLI paramater string name: CH0 frequency
#define APP_CLI_PHY_PARAM_CH0_FREQ_STR      "ch0_frequency"

/// App CLI paramater string name: channel spacing
#define APP_CLI_PHY_PARAM_CH_SPACING_STR    "channel_spacing"
#endif

#if APP_CLI_ENABLE_EXPLICIT_CONFIG || APP_CLI_ENABLE_FAN11_CONFIG
/// App CLI paramater string name: PHY mode ID
#define APP_CLI_PHY_PARAM_PHY_MODE_ID_STR   "phy_mode_id"
#endif

#if APP_CLI_ENABLE_FAN11_CONFIG
/// App CLI paramater string name: channel plan ID
#define APP_CLI_PHY_PARAM_CH_PLAN_ID_STR    "chan_plan_id"
#endif

/// App CLI paramater string name: PHY config type
#define APP_CLI_PHY_PARAM_PHY_CFG_TYPE_STR  "phy_config_type"

/// App CLI entry
typedef app_settings_entry_t app_cli_entry_t;

/// Structure typedef to store common PHY parameters, like reg. domain and PHY mode ID
typedef struct app_cli_phy_common_params {
  /// Regulatory domain
  uint8_t regulatory_domain;
  /// Operating class
  uint8_t operating_class;
  /// Operating mode
  uint16_t operating_mode;
#if APP_CLI_ENABLE_EXPLICIT_CONFIG
  /// Number of channels
  uint16_t number_of_channels;
  /// CH0 frequency
  uint32_t ch0_frequency;
  /// Channel spacing
  uint8_t channel_spacing;
#endif
#if APP_CLI_ENABLE_FAN11_CONFIG
  /// Channel plan ID
  uint8_t chan_plan_id;
#endif
#if APP_CLI_ENABLE_EXPLICIT_CONFIG || APP_CLI_ENABLE_FAN11_CONFIG
  /// PHY mode ID
  uint8_t phy_mode_id;
#endif
  /// PHY config type
  uint8_t phy_config_type;
} app_cli_phy_common_params_t;

/// Function pointer for CLI handler functions
typedef void (*sl_wisun_app_cli_handler)(const sl_cli_command_arg_t *arguments);

/// Structure for Wi-SUN CLI handler properties
typedef struct sl_wisun_cli_handler_property {
  /// CLI group
  const char *cli_group;
  /// CLI group shortcut
  const char *cli_group_shortcut;
  /// CLI command
  const char *cli_command;
  /// CLI command shortcut
  const char *cli_command_shortcut;
  /// CLI handler
  sl_wisun_app_cli_handler cli_handler_fnc;
} sl_wisun_cli_handler_property_t;

// -----------------------------------------------------------------------------
//                          Static Function Declarations
// -----------------------------------------------------------------------------

/**************************************************************************//**
 * @brief Set CLI common PHY parameters from PHY config
 * @details Synchronization is required because of common parameters,
 *          like reg. domain and PHY mode ID
 * @return sl_status_t SL_STATUS_OK on succes, SL_STATUS_FAIL otherwise
 *****************************************************************************/
static sl_status_t _load_common_params_from_phy_cfg(void);

/**************************************************************************//**
 * @brief Store CLI common PHY parameters to PHY config
 * @details Synchronization is required because of common parameters,
 *          like reg domain and phy mode id
 * @return sl_status_t SL_STATUS_OK on succes, SL_STATUS_FAIL otherwise
 *****************************************************************************/
static sl_status_t _store_common_params_to_phy_cfg(void);

/**************************************************************************//**
 * @brief Helper to get connection status for getter
 * @param[out] *value_str is the desired value string
 * @param[in] *key_str is the key string of the set command
 * @param[in] *entry is the settings entry
 * @return SL_STATUS_OK if the setting is success.
 * @return SL_STATUS_FAIL if setting failed.
 *****************************************************************************/
static sl_status_t _app_cli_get_connection(char *value_str,
                                           const char *key_str,
                                           const app_cli_entry_t *entry);

/**************************************************************************//**
 * @brief Helper to set network name for setter
 * @param[out] *value_str is the desired value string
 * @param[in] *key_str is the key string of the set command
 * @param[in] *entry is the settings entry
 * @return SL_STATUS_OK if the setting is success.
 * @return SL_STATUS_FAIL if setting failed.
 *****************************************************************************/
static sl_status_t _app_cli_set_network_name(const char *value_str,
                                             const char *key_str,
                                             const app_cli_entry_t *entry);

/**************************************************************************//**
 * @brief Helper to set network size for setter
 * @param[out] *value_str is the desired value string
 * @param[in] *key_str is the key string of the set command
 * @param[in] *entry is the settings entry
 * @return SL_STATUS_OK if the setting is success.
 * @return SL_STATUS_FAIL if setting failed.
 *****************************************************************************/
static sl_status_t _app_cli_set_network_size(const char *value_str,
                                             const char *key_str,
                                             const app_cli_entry_t *entry);

/**************************************************************************//**
 * @brief Helper to set TX power for getter
 * @param[out] *value_str is the desired value string
 * @param[in] *key_str is the key string of the set command
 * @param[in] *entry is the settings entry
 * @return SL_STATUS_OK if the setting is success.
 * @return SL_STATUS_FAIL if setting failed.
 *****************************************************************************/
static sl_status_t _app_cli_set_tx_power(const char *value_str,
                                         const char *key_str,
                                         const app_cli_entry_t *entry);

/**************************************************************************//**
 * @brief Helper to get network name for getter
 * @param[out] *value_str is the desired value string
 * @param[in] *key_str is the key string of the set command
 * @param[in] *entry is the settings entry
 * @return SL_STATUS_OK if the setting is success.
 * @return SL_STATUS_FAIL if setting failed.
 *****************************************************************************/
static sl_status_t _app_cli_get_network_name(char *value_str,
                                             const char *key_str,
                                             const app_cli_entry_t *entry);

/**************************************************************************//**
 * @brief Helper to get network size for getter
 * @param[out] *value_str is the desired value string
 * @param[in] *key_str is the key string of the set command
 * @param[in] *entry is the settings entry
 * @return SL_STATUS_OK if the setting is success.
 * @return SL_STATUS_FAIL if setting failed.
 *****************************************************************************/
static sl_status_t _app_cli_get_network_size(char *value_str,
                                             const char *key_str,
                                             const app_cli_entry_t *entry);

/**************************************************************************//**
 * @brief Helper to get TX power for getter
 * @param[out] *value_str is the desired value string
 * @param[in] *key_str is the key string of the set command
 * @param[in] *entry is the settings entry
 * @return SL_STATUS_OK if the setting is success.
 * @return SL_STATUS_FAIL if setting failed.
 *****************************************************************************/
static sl_status_t _app_cli_get_tx_power(char *value_str,
                                         const char *key_str,
                                         const app_cli_entry_t *entry);

/**************************************************************************//**
 * @brief Is CLI setting parameter is used.
 * @details It indicates the parameteris used in the current setup.
 * @param param_str Parameter string.
 * @return true Parameter is used.
 * @return false Parameter is not used.
 *****************************************************************************/
static bool _is_param_used(const char *param_str);

/**************************************************************************//**
 * @brief Get param used string flag
 * @details Helper function to get string representation of parameter usage
 * @return const char*
 *****************************************************************************/
__STATIC_INLINE const char *_param_used_flag_str(const char *param_str);

/**************************************************************************//**
 * @brief Helper to get wisun phy for getter
 * @param[out] *value_str is the desired value string
 * @param[in] *key_str is the key string of the set command
 * @param[in] *entry is the settings entry
 * @return SL_STATUS_OK if the setting is success.
 * @return SL_STATUS_FAIL if setting failed.
 *****************************************************************************/
static sl_status_t _app_cli_get_phy(char *value_str,
                                    const char *key_str,
                                    const app_cli_entry_t *entry);

#if defined(SL_CATALOG_WISUN_MODE_SWITCH_PRESENT)
/**************************************************************************//**
 * @brief Helper to get wisun mode switch counters for getter
 * @param[out] *value_str is the desired value string
 * @param[in] *key_str is the key string of the set command
 * @param[in] *entry is the settings entry
 * @return SL_STATUS_OK if the setting is success.
 * @return SL_STATUS_FAIL if setting failed.
 *****************************************************************************/
static sl_status_t _app_ms_get_counters(char *value_str,
                                        const char *key_str,
                                        const app_cli_entry_t *entry);
#endif

/**************************************************************************//**
 * @brief Helper to set TX power for getter
 * @param[out] *value_str is the desired value string
 * @param[in] *key_str is the key string of the set command
 * @param[in] *entry is the settings entry
 * @return SL_STATUS_OK if the setting is success.
 * @return SL_STATUS_FAIL if setting failed.
 *****************************************************************************/
static sl_status_t _app_cli_set_phy(const char *value_str,
                                    const char *key_str,
                                    const app_settings_entry_t *entry);

#if defined(SL_CATALOG_WISUN_APP_CORE_PRESENT)
/**************************************************************************//**
 * @brief Helper to set regulation for setter
 * @param[out] *value_str is the desired value string
 * @param[in] *key_str is the key string of the set command
 * @param[in] *entry is the settings entry
 * @return SL_STATUS_OK if the setting is success.
 * @return SL_STATUS_FAIL if setting failed.
 *****************************************************************************/
static sl_status_t _app_set_regulation(const char *value_str,
                                       const char *key_str,
                                       const app_settings_entry_t *entry);

/**************************************************************************//**
 * @brief Helper to set regulation warning threshold for setter
 * @param[out] *value_str is the desired value string
 * @param[in] *key_str is the key string of the set command
 * @param[in] *entry is the settings entry
 * @return SL_STATUS_OK if the setting is success.
 * @return SL_STATUS_FAIL if setting failed.
 *****************************************************************************/
static sl_status_t _app_set_regulation_warning_threshold(const char *value_str,
                                                         const char *key_str,
                                                         const app_settings_entry_t *entry);

/**************************************************************************//**
 * @brief Helper to set regulation alert threshold for setter
 * @param[out] *value_str is the desired value string
 * @param[in] *key_str is the key string of the set command
 * @param[in] *entry is the settings entry
 * @return SL_STATUS_OK if the setting is success.
 * @return SL_STATUS_FAIL if setting failed.
 *****************************************************************************/
static sl_status_t _app_set_regulation_alert_threshold(const char *value_str,
                                                       const char *key_str,
                                                       const app_settings_entry_t *entry);

/**************************************************************************//**
 * @brief Helper to get regulation for getter
 * @param[out] *value_str is the desired value string
 * @param[in] *key_str is the key string of the set command
 * @param[in] *entry is the settings entry
 * @return SL_STATUS_OK if the setting is success.
 * @return SL_STATUS_FAIL if setting failed.
 *****************************************************************************/
static sl_status_t _app_get_regulation(char *value_str,
                                       const char *key_str,
                                       const app_cli_entry_t *entry);

/**************************************************************************//**
 * @brief Helper to get regulation warning threshold for setter
 * @param[out] *value_str is the desired value string
 * @param[in] *key_str is the key string of the set command
 * @param[in] *entry is the settings entry
 * @return SL_STATUS_OK if the setting is success.
 * @return SL_STATUS_FAIL if setting failed.
 *****************************************************************************/
static sl_status_t _app_get_regulation_warning_threshold(char *value_str,
                                                         const char *key_str,
                                                         const app_cli_entry_t *entry);

/**************************************************************************//**
 * @brief Helper to get regulation alert threshold for setter
 * @param[out] *value_str is the desired value string
 * @param[in] *key_str is the key string of the set command
 * @param[in] *entry is the settings entry
 * @return SL_STATUS_OK if the setting is success.
 * @return SL_STATUS_FAIL if setting failed.
 *****************************************************************************/
static sl_status_t _app_get_regulation_alert_threshold(char *value_str,
                                                       const char *key_str,
                                                       const app_cli_entry_t *entry);

/**************************************************************************//**
 * @brief Helper to set MAC address
 * @param[out] *value_str is the desired value string
 * @param[in] *key_str is the key string of the set command
 * @param[in] *entry is the settings entry
 * @return SL_STATUS_OK if the setting is success.
 * @return SL_STATUS_FAIL if setting failed.
 *****************************************************************************/
static sl_status_t _app_settings_set_mac_address(const char *value_str,
                                                 const char *key_str,
                                                 const app_settings_entry_t *entry);

/**************************************************************************//**
 * @brief Helper to get MAC address
 * @param[out] *value_str is the desired value string
 * @param[in] *key_str is the key string of the set command
 * @param[in] *entry is the settings entry
 * @return SL_STATUS_OK if the setting is success.
 * @return SL_STATUS_FAIL if setting failed.
 *****************************************************************************/
static sl_status_t _app_settings_get_mac_address(char *value_str,
                                                 const char *key_str,
                                                 const app_settings_entry_t *entry);
#endif

#if defined(SL_CATALOG_WISUN_LFN_DEVICE_SUPPORT_PRESENT)
/**************************************************************************//**
 * @brief Helper to get device type
 * @param[out] *value_str is the desired value string
 * @param[in] *key_str is the key string of the set command
 * @param[in] *entry is the settings entry
 * @return SL_STATUS_OK if the setting is success.
 * @return SL_STATUS_FAIL if setting failed.
 *****************************************************************************/
static sl_status_t _app_get_device_type(char *value_str,
                                        const char *key_str,
                                        const app_cli_entry_t *entry);

/**************************************************************************//**
 * @brief Helper to get LFN profile
 * @param[out] *value_str is the desired value string
 * @param[in] *key_str is the key string of the set command
 * @param[in] *entry is the settings entry
 * @return SL_STATUS_OK if the setting is success.
 * @return SL_STATUS_FAIL if setting failed.
 *****************************************************************************/
static sl_status_t _app_get_lfn_profile(char *value_str,
                                        const char *key_str,
                                        const app_cli_entry_t *entry);
#endif

/**************************************************************************//**
 * @brief Connect to Wi-SUN network by PHY configuration type
 * @details CLI callback helper function
 * @param config_type PHY Configuration type
 *****************************************************************************/
static void _app_connect(const sl_wisun_phy_config_type_t config_type);

/**************************************************************************//**
 * @brief Helper function to iterate through CLI handlers
 * @details Helper function
 * @param[in] args CLI arguments pointer
 * @param[in] size CLI arguments size
 * @param[in] props CLI handler properties pointer
 *****************************************************************************/
static void _cli_handler_iterator(const sl_cli_command_arg_t *args,
                                  const size_t size,
                                  const sl_wisun_cli_handler_property_t *props);

// -----------------------------------------------------------------------------
//                                Static Variables
// -----------------------------------------------------------------------------

#if defined(SL_CATALOG_WISUN_APP_CORE_PRESENT)
/// Wi-SUN application regulation
static sl_wisun_regulation_t _app_regulation = (sl_wisun_regulation_t)SL_WISUN_APP_CORE_REGULATION;
#endif

/// Common PHY parameters for CLI setter/getter
static app_cli_phy_common_params_t _phy_common_params = { 0U };

/// Mapping PHY config parameters by PHY config type
static const app_enum_t _phy_cfg_map[] = {
  // FAN 1.0 parameters
  { APP_CLI_PHY_PARAM_REG_DOMAIN_STR, SL_WISUN_PHY_CONFIG_FAN10 },
  { APP_CLI_PHY_PARAM_OP_CLASS_STR, SL_WISUN_PHY_CONFIG_FAN10 },
  { APP_CLI_PHY_PARAM_OP_MODE_STR, SL_WISUN_PHY_CONFIG_FAN10 },

  // FAN 1.1 parameters
#if APP_CLI_ENABLE_FAN11_CONFIG
  { APP_CLI_PHY_PARAM_REG_DOMAIN_STR, SL_WISUN_PHY_CONFIG_FAN11 },
  { APP_CLI_PHY_PARAM_CH_PLAN_ID_STR, SL_WISUN_PHY_CONFIG_FAN11 },
  { APP_CLI_PHY_PARAM_PHY_MODE_ID_STR, SL_WISUN_PHY_CONFIG_FAN11 },
#endif

  // Explicit parameters
#if APP_CLI_ENABLE_EXPLICIT_CONFIG
  { APP_CLI_PHY_PARAM_CH0_FREQ_STR, SL_WISUN_PHY_CONFIG_EXPLICIT },
  { APP_CLI_PHY_PARAM_NUM_OF_CH_STR, SL_WISUN_PHY_CONFIG_EXPLICIT },
  { APP_CLI_PHY_PARAM_CH_SPACING_STR, SL_WISUN_PHY_CONFIG_EXPLICIT },
  { APP_CLI_PHY_PARAM_PHY_MODE_ID_STR, SL_WISUN_PHY_CONFIG_EXPLICIT },
#endif
  // End list
  { NULL, 0 }
};

#if defined(SL_CATALOG_IPERF_PRESENT) \
  && SL_WISUN_CLI_ENABLED             \
  && SL_WISUN_IPERF_CLI_ENABLED
/// iPerf CLI handler properties
static sl_wisun_cli_handler_property_t _iperf_cli_handler_properties[] = {
  {
    .cli_group = "iperf",
    .cli_group_shortcut = "i",
    .cli_command = "get",
    .cli_command_shortcut = "g",
    .cli_handler_fnc = &sl_iperf_cli_get
  },
  {
    .cli_group = "iperf",
    .cli_group_shortcut = "i",
    .cli_command = "set",
    .cli_command_shortcut = "s",
    .cli_handler_fnc = &sl_iperf_cli_set
  },
  {
    .cli_group = "iperf",
    .cli_group_shortcut = "i",
    .cli_command = "server",
    .cli_command_shortcut = "-s",
    .cli_handler_fnc = &sl_iperf_cli_exec_server
  },
  {
    .cli_group = "iperf",
    .cli_group_shortcut = "i",
    .cli_command = "client",
    .cli_command_shortcut = "-c",
    .cli_handler_fnc = &sl_iperf_cli_exec_client
  },
  {
    .cli_group = "N/A",
    .cli_group_shortcut = "N/A",
    .cli_command = "N/A",
    .cli_command_shortcut = "N/A",
    .cli_handler_fnc = NULL
  }
};

/// iPerf CLI handler properties size
static const size_t _iperf_cli_handler_properties_size = sizeof(_iperf_cli_handler_properties) / sizeof(sl_wisun_cli_handler_property_t);
#endif

#if defined(SL_CATALOG_WISUN_OTA_DFU_PRESENT) \
  && SL_WISUN_CLI_ENABLED                     \
  && SL_WISUN_OTA_DFU_CLI_ENABLED
/// OTA DFU CLI handler properties
static sl_wisun_cli_handler_property_t _ota_dfu_cli_handler_properties[] = {
  {
    .cli_group = "ota-dfu",
    .cli_group_shortcut = "od",
    .cli_command = "start_firmware_update",
    .cli_command_shortcut = "fb",
    .cli_handler_fnc = &app_fw_update_start
  },
  {
    .cli_group = "ota-dfu",
    .cli_group_shortcut = "od",
    .cli_command = "stop_firmware_update",
    .cli_command_shortcut = "fq",
    .cli_handler_fnc = &app_fw_update_stop
  },
  {
    .cli_group = "ota-dfu",
    .cli_group_shortcut = "od",
    .cli_command = "reboot_and_install_fw",
    .cli_command_shortcut = "fr",
    .cli_handler_fnc = &app_fw_reboot_and_install
  },
  {
    .cli_group = "ota-dfu",
    .cli_group_shortcut = "od",
    .cli_command = "firmware_update_status",
    .cli_command_shortcut = "fs",
    .cli_handler_fnc = &app_fw_update_status
  },
  {
    .cli_group = "ota-dfu",
    .cli_group_shortcut = "od",
    .cli_command = "set_host_addr",
    .cli_command_shortcut = "sha",
    .cli_handler_fnc = &app_set_host_addr
  },
  {
    .cli_group = "ota-dfu",
    .cli_group_shortcut = "od",
    .cli_command = "get_host_addr",
    .cli_command_shortcut = "gha",
    .cli_handler_fnc = &app_get_host_addr
  },
  {
    .cli_group = "ota-dfu",
    .cli_group_shortcut = "od",
    .cli_command = "set_host_uri",
    .cli_command_shortcut = "shu",
    .cli_handler_fnc = &app_set_host_uri
  },
  {
    .cli_group = "ota-dfu",
    .cli_group_shortcut = "od",
    .cli_command = "get_host_uri",
    .cli_command_shortcut = "ghu",
    .cli_handler_fnc = &app_get_host_uri
  },
  {
    .cli_group = "ota-dfu",
    .cli_group_shortcut = "od",
    .cli_command = "set_gbl_path",
    .cli_command_shortcut = "sgp",
    .cli_handler_fnc = &app_set_gbl_path
  },
  {
    .cli_group = "ota-dfu",
    .cli_group_shortcut = "od",
    .cli_command = "get_gbl_path",
    .cli_command_shortcut = "ggp",
    .cli_handler_fnc = &app_get_gbl_path
  },
#if SL_WISUN_OTA_DFU_HOST_NOTIFY_ENABLED
  {
    .cli_group = "ota-dfu",
    .cli_group_shortcut = "od",
    .cli_command = "set_notify_host_addr",
    .cli_command_shortcut = "snha",
    .cli_handler_fnc = &app_set_notify_host_addr
  },
  {
    .cli_group = "ota-dfu",
    .cli_group_shortcut = "od",
    .cli_command = "get_notify_host_addr",
    .cli_command_shortcut = "gnha",
    .cli_handler_fnc = &app_get_notify_host_addr
  },
  {
    .cli_group = "ota-dfu",
    .cli_group_shortcut = "od",
    .cli_command = "set_notify_host_uri",
    .cli_command_shortcut = "snhu",
    .cli_handler_fnc = &app_set_notify_host_uri
  },
  {
    .cli_group = "ota-dfu",
    .cli_group_shortcut = "od",
    .cli_command = "get_notify_host_uri",
    .cli_command_shortcut = "gnhu",
    .cli_handler_fnc = &app_get_notify_host_uri
  },
  {
    .cli_group = "ota-dfu",
    .cli_group_shortcut = "od",
    .cli_command = "set_notify_download_chunk",
    .cli_command_shortcut = "sndc",
    .cli_handler_fnc = &app_set_notify_download_chunk
  },
  {
    .cli_group = "ota-dfu",
    .cli_group_shortcut = "od",
    .cli_command = "get_notify_download_chunk",
    .cli_command_shortcut = "gndc",
    .cli_handler_fnc = &app_get_notify_download_chunk
  },
#endif
  {
    .cli_group = "N/A",
    .cli_group_shortcut = "N/A",
    .cli_command = "N/A",
    .cli_command_shortcut = "N/A",
    .cli_handler_fnc = NULL
  }
};

/// OTA DFU CLI handler properties size
static const size_t _ota_dfu_cli_handler_properties_size = sizeof(_ota_dfu_cli_handler_properties) / sizeof(sl_wisun_cli_handler_property_t);
#endif

/// Wi-SUN app CLI handler properties
static sl_wisun_cli_handler_property_t _wisun_app_cli_handler_properties[] = {
  {
    .cli_group = "wisun",
    .cli_group_shortcut = "w",
    .cli_command = "join_fan10",
    .cli_command_shortcut = "j10",
    .cli_handler_fnc = &app_join_fan10
  },
  {
    .cli_group = "wisun",
    .cli_group_shortcut = "w",
    .cli_command = "join_fan11",
    .cli_command_shortcut = "j11",
    .cli_handler_fnc = &app_join_fan11
  },
  {
    .cli_group = "wisun",
    .cli_group_shortcut = "w",
    .cli_command = "join_explicit",
    .cli_command_shortcut = "je",
    .cli_handler_fnc = &app_join_explicit
  },
  {
    .cli_group = "wisun",
    .cli_group_shortcut = "w",
    .cli_command = "disconnect",
    .cli_command_shortcut = "d",
    .cli_handler_fnc = &app_disconnect
  },
  {
    .cli_group = "wisun",
    .cli_group_shortcut = "w",
    .cli_command = "mac_allow",
    .cli_command_shortcut = "ma",
    .cli_handler_fnc = &app_mac_allow
  },
  {
    .cli_group = "wisun",
    .cli_group_shortcut = "w",
    .cli_command = "mac_deny",
    .cli_command_shortcut = "md",
    .cli_handler_fnc = &app_mac_deny
  },
#if defined(SL_CATALOG_WISUN_PING_PRESENT) \
  && SL_WISUN_CLI_ENABLED                  \
  && SL_WISUN_PING_CLI_ENABLED
  {
    .cli_group = "wisun",
    .cli_group_shortcut = "w",
    .cli_command = "ping",
    .cli_command_shortcut = "p",
    .cli_handler_fnc = &app_ping
  },
#endif
#if defined(SL_CATALOG_WISUN_NETWORK_MEASUREMENT_PRESENT) \
  && SL_WISUN_CLI_ENABLED                                 \
  && SL_WISUN_NETWORK_MEASUREMENT_CLI_ENABLED
  {
    .cli_group = "wisun",
    .cli_group_shortcut = "w",
    .cli_command = "measure",
    .cli_command_shortcut = "m",
    .cli_handler_fnc = &app_measure
  },
  {
    .cli_group = "wisun",
    .cli_group_shortcut = "w",
    .cli_command = "print_nodeinfo",
    .cli_command_shortcut = "pni",
    .cli_handler_fnc = &app_print_node_info
  },
#endif
#if defined(SL_CATALOG_WISUN_APP_STATUS_PRESENT) \
  && SL_WISUN_CLI_ENABLED                        \
  && SL_WISUN_APP_STATUS_CLI_ENABLED
  {
    .cli_group = "wisun",
    .cli_group_shortcut = "w",
    .cli_command = "set_app_status_address",
    .cli_command_shortcut = "sasa",
    .cli_handler_fnc = &sl_app_status_cli_set_address
  },
  {
    .cli_group = "wisun",
    .cli_group_shortcut = "w",
    .cli_command = "set_app_status_schedule",
    .cli_command_shortcut = "sast",
    .cli_handler_fnc = &sl_app_status_cli_set_schedule
  },
  {
    .cli_group = "wisun",
    .cli_group_shortcut = "w",
    .cli_command = "get_app_status_address",
    .cli_command_shortcut = "gasa",
    .cli_handler_fnc = &sl_app_status_cli_get_address
  },
  {
    .cli_group = "wisun",
    .cli_group_shortcut = "w",
    .cli_command = "get_app_status_schedule",
    .cli_command_shortcut = "gast",
    .cli_handler_fnc = &sl_app_status_cli_get_schedule
  },
#endif
#if defined(SL_CATALOG_WISUN_CLI_UTIL_PRESENT) \
  && SL_WISUN_CLI_ENABLED                      \
  && SL_WISUN_CLI_UTIL_CLI_ENABLED
  {
    .cli_group = "wisun",
    .cli_group_shortcut = "w",
    .cli_command = "set",
    .cli_command_shortcut = "s",
    .cli_handler_fnc = &app_set
  },
  {
    .cli_group = "wisun",
    .cli_group_shortcut = "w",
    .cli_command = "get",
    .cli_command_shortcut = "g",
    .cli_handler_fnc = &app_get
  },
  {
    .cli_group = "wisun",
    .cli_group_shortcut = "w",
    .cli_command = "save",
    .cli_command_shortcut = "a",
    .cli_handler_fnc = &app_save
  },
  {
    .cli_group = "wisun",
    .cli_group_shortcut = "w",
    .cli_command = "reset",
    .cli_command_shortcut = "r",
    .cli_handler_fnc = &app_reset
  },
#endif
#if defined(SL_CATALOG_WISUN_TCP_CLIENT_PRESENT) \
  && SL_WISUN_CLI_ENABLED                        \
  && SL_WISUN_CLIENT_CLI_ENABLED
  {
    .cli_group = "wisun",
    .cli_group_shortcut = "w",
    .cli_command = "tcp_client",
    .cli_command_shortcut = "tc",
    .cli_handler_fnc = &app_tcp_client
  },
#endif
#if defined(SL_CATALOG_WISUN_UDP_CLIENT_PRESENT) \
  && SL_WISUN_CLI_ENABLED                        \
  && SL_WISUN_CLIENT_CLI_ENABLED
  {
    .cli_group = "wisun",
    .cli_group_shortcut = "w",
    .cli_command = "udp_client",
    .cli_command_shortcut = "uc",
    .cli_handler_fnc = &app_udp_client
  },
#endif
#if (defined(SL_CATALOG_WISUN_TCP_CLIENT_PRESENT) || defined(SL_CATALOG_WISUN_UDP_CLIENT_PRESENT)) \
  && SL_WISUN_CLI_ENABLED && SL_WISUN_CLIENT_CLI_ENABLED
  {
    .cli_group = "wisun",
    .cli_group_shortcut = "w",
    .cli_command = "socket_close",
    .cli_command_shortcut = "sc",
    .cli_handler_fnc = &app_socket_close
  },
  {
    .cli_group = "wisun",
    .cli_group_shortcut = "w",
    .cli_command = "socket_write",
    .cli_command_shortcut = "sw",
    .cli_handler_fnc = &app_socket_write
  },
  {
    .cli_group = "wisun",
    .cli_group_shortcut = "w",
    .cli_command = "socket_read",
    .cli_command_shortcut = "sr",
    .cli_handler_fnc = &app_socket_read
  },
#endif
#if defined(SL_CATALOG_WISUN_COLLECTOR_PRESENT) \
  && defined(SL_CATALOG_WISUN_COAP_PRESENT)     \
  && SL_WISUN_CLI_ENABLED                       \
  && SL_WISUN_COLLECTOR_CLI_ENABLED
  {
    .cli_group = "wisun",
    .cli_group_shortcut = "w",
    .cli_command = "register_meter",
    .cli_command_shortcut = "r",
    .cli_handler_fnc = &app_register_meter
  },
  {
    .cli_group = "wisun",
    .cli_group_shortcut = "w",
    .cli_command = "remove_meter",
    .cli_command_shortcut = "d",
    .cli_handler_fnc = &app_remove_meter
  },
  {
    .cli_group = "wisun",
    .cli_group_shortcut = "w",
    .cli_command = "async_request",
    .cli_command_shortcut = "a",
    .cli_handler_fnc = &app_async_request
  },
  {
    .cli_group = "wisun",
    .cli_group_shortcut = "w",
    .cli_command = "list_meters",
    .cli_command_shortcut = "p",
    .cli_handler_fnc = &app_list_meters
  },
  {
    .cli_group = "wisun",
    .cli_group_shortcut = "w",
    .cli_command = "led_toggle",
    .cli_command_shortcut = "l",
    .cli_handler_fnc = &app_led_toggle
  },
#endif
#if defined(SL_CATALOG_WISUN_COLLECTOR_PRESENT) \
  && !defined(SL_CATALOG_WISUN_COAP_PRESENT)    \
  && SL_WISUN_CLI_ENABLED                       \
  && SL_WISUN_COLLECTOR_CLI_ENABLED
  {
    .cli_group = "wisun",
    .cli_group_shortcut = "w",
    .cli_command = "register_meter",
    .cli_command_shortcut = "r",
    .cli_handler_fnc = &app_register_meter
  },
  {
    .cli_group = "wisun",
    .cli_group_shortcut = "w",
    .cli_command = "remove_meter",
    .cli_command_shortcut = "d",
    .cli_handler_fnc = &app_remove_meter
  },
  {
    .cli_group = "wisun",
    .cli_group_shortcut = "w",
    .cli_command = "async_request",
    .cli_command_shortcut = "a",
    .cli_handler_fnc = &app_async_request
  },
#endif
  {
    .cli_group = "N/A",
    .cli_group_shortcut = "N/A",
    .cli_command = "N/A",
    .cli_command_shortcut = "N/A",
    .cli_handler_fnc = NULL
  }
};

/// iPerf CLI handler properties size
static const size_t _wisun_app_cli_handler_properties_size = sizeof(_wisun_app_cli_handler_properties) / sizeof(sl_wisun_cli_handler_property_t);

// -----------------------------------------------------------------------------
//                                Global Variables
// -----------------------------------------------------------------------------

/// Wi-SUN application settings domain string
const char *app_settings_domain_str[] = {
  "wisun",
  NULL,
};

///  Wi-SUN application settings entry/entries
const app_cli_entry_t app_settings_entries[] =
{
  {
    .key = "network_name",
    .domain = APP_CLI_WISUN_DOMAIN_ID,
    .value_size = SL_WISUN_NETWORK_NAME_SIZE + 1,
    .input = APP_CLI_INPUT_FLAG_DEFAULT,
    .output = APP_CLI_OUTPUT_FLAG_DEFAULT,
    .value = NULL,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = _app_cli_set_network_name,
    .get_handler = _app_cli_get_network_name,
    .description = "Wi-SUN network [string] max 32"
  },
  {
    .key = APP_CLI_PHY_PARAM_PHY_CFG_TYPE_STR,
    .domain = APP_CLI_WISUN_DOMAIN_ID,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT8,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = NULL,
    .input_enum_list = NULL,
    .output_enum_list = app_wisun_phy_config_type_enum,
    .set_handler = NULL,
    .get_handler = _app_cli_get_phy,
    .description = "PHY configuration type (FAN 1.0|FAN 1.1|explicit)"
  },
  {
    .key = "network_size",
    .domain = APP_CLI_WISUN_DOMAIN_ID,
    .value_size = APP_CLI_VALUE_SIZE_UINT8,
    .input = APP_CLI_INPUT_FLAG_DEFAULT,
    .output = APP_CLI_OUTPUT_FLAG_DEFAULT,
    .value = NULL,
    .input_enum_list = app_wisun_nw_size_enum,
    .output_enum_list = app_wisun_nw_size_enum,
    .set_handler = _app_cli_set_network_size,
    .get_handler = _app_cli_get_network_size,
    .description = "Wi-SUN network size [uint8]"
  },
  {
    .key = "tx_power",
    .domain = APP_CLI_WISUN_DOMAIN_ID,
    .value_size = APP_CLI_VALUE_SIZE_UINT16,
    .input = APP_CLI_INPUT_FLAG_DEFAULT | APP_CLI_INPUT_FLAG_SIGNED,
    .output = APP_CLI_OUTPUT_FLAG_DEFAULT | APP_CLI_OUTPUT_FLAG_SIGNED,
    .value = NULL,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = _app_cli_set_tx_power,
    .get_handler = _app_cli_get_tx_power,
    .description = "TX power in ddBm [uint16]"
  },
  {
    .key = APP_CLI_PHY_PARAM_REG_DOMAIN_STR,
    .domain = APP_CLI_WISUN_DOMAIN_ID,
    .value_size = APP_CLI_VALUE_SIZE_UINT8,
    .input = APP_CLI_INPUT_FLAG_DEFAULT,
    .output = APP_CLI_OUTPUT_FLAG_DEFAULT,
    .value = NULL,
    .input_enum_list = app_wisun_phy_reg_domain_enum,
    .output_enum_list = app_wisun_phy_reg_domain_enum,
    .set_handler = _app_cli_set_phy,
    .get_handler = _app_cli_get_phy,
    .description = "Wi-SUN regulatory domain [uint8]"
  },
  {
    .key = APP_CLI_PHY_PARAM_OP_CLASS_STR,
    .domain = APP_CLI_WISUN_DOMAIN_ID,
    .value_size = APP_CLI_VALUE_SIZE_UINT8,
    .input = APP_CLI_INPUT_FLAG_DEFAULT,
    .output = APP_CLI_OUTPUT_FLAG_DEFAULT,
    .value = NULL,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = _app_cli_set_phy,
    .get_handler = _app_cli_get_phy,
    .description = "Wi-SUN operation class [uint8]"
  },
  {
    .key = APP_CLI_PHY_PARAM_OP_MODE_STR,
    .domain = APP_CLI_WISUN_DOMAIN_ID,
    .value_size = APP_CLI_VALUE_SIZE_UINT8,
    .input = APP_CLI_INPUT_FLAG_DEFAULT,
    .output = APP_CLI_OUTPUT_FLAG_DEFAULT,
    .value = NULL,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = _app_cli_set_phy,
    .get_handler = _app_cli_get_phy,
    .description = "Wi-SUN operating mode [uint16]"
  },
#if APP_CLI_ENABLE_FAN11_CONFIG
  {
    .key = APP_CLI_PHY_PARAM_CH_PLAN_ID_STR,
    .domain = APP_CLI_WISUN_DOMAIN_ID,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT8,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = NULL,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = _app_cli_set_phy,
    .get_handler = _app_cli_get_phy,
    .description = "Wi-SUN channel plan ID [uint8]"
  },
#endif
#if APP_CLI_ENABLE_FAN11_CONFIG || APP_CLI_ENABLE_EXPLICIT_CONFIG
  {
    .key = APP_CLI_PHY_PARAM_PHY_MODE_ID_STR,
    .domain = APP_CLI_WISUN_DOMAIN_ID,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT8,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = NULL,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = _app_cli_set_phy,
    .get_handler = _app_cli_get_phy,
    .description = "Wi-SUN PHY mode ID [uint8]"
  },
#endif
#if APP_CLI_ENABLE_EXPLICIT_CONFIG
  {
    .key = APP_CLI_PHY_PARAM_CH0_FREQ_STR,
    .domain = APP_CLI_WISUN_DOMAIN_ID,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT32,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = NULL,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = _app_cli_set_phy,
    .get_handler = _app_cli_get_phy,
    .description = "Ch0 frequency in kHz [uint32]"
  },
  {
    .key = APP_CLI_PHY_PARAM_NUM_OF_CH_STR,
    .domain = APP_CLI_WISUN_DOMAIN_ID,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT16,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = NULL,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = _app_cli_set_phy,
    .get_handler = _app_cli_get_phy,
    .description = "Number of channels [uint16]"
  },
  {
    .key = APP_CLI_PHY_PARAM_CH_SPACING_STR,
    .domain = APP_CLI_WISUN_DOMAIN_ID,
    .value_size = APP_SETTINGS_VALUE_SIZE_UINT8,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = NULL,
    .input_enum_list = app_wisun_phy_channel_spacing_enum,
    .output_enum_list = app_wisun_phy_channel_spacing_enum,
    .set_handler = _app_cli_set_phy,
    .get_handler = _app_cli_get_phy,
    .description = "Channel spacing [string] (100kHz|200kHz|400kHz|600kHz)"
  },
#endif
  {
    .key = "join_state",
    .domain = APP_CLI_WISUN_DOMAIN_ID,
    .value_size = APP_CLI_VALUE_SIZE_UINT8,
    .input = APP_CLI_INPUT_FLAG_DEFAULT,
    .output = APP_CLI_OUTPUT_FLAG_DEFAULT,
    .value = NULL,
    .input_enum_list = app_wisun_conn_status_enum,
    .output_enum_list = app_wisun_conn_status_enum,
    .set_handler = NULL,
    .get_handler = _app_cli_get_connection,
    .description = "Wi-SUN join state"
  },
  {
    .key = "ip_address_global",
    .domain = APP_CLI_WISUN_DOMAIN_ID,
    .value_size = APP_CLI_VALUE_SIZE_NONE,
    .input = APP_CLI_INPUT_FLAG_DEFAULT,
    .output = APP_CLI_OUTPUT_FLAG_DEFAULT,
    .value = NULL,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_ip_address,
    .description = "Global IPv6 address"
  },
  {
    .key = "ip_address_link_local",
    .domain = APP_CLI_WISUN_DOMAIN_ID,
    .value_size = APP_CLI_VALUE_SIZE_NONE,
    .input = APP_CLI_INPUT_FLAG_DEFAULT,
    .output = APP_CLI_OUTPUT_FLAG_DEFAULT,
    .value = NULL,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_ip_address,
    .description = "Link local IPv6"
  },
  {
    .key = "ip_address_border_router",
    .domain = APP_CLI_WISUN_DOMAIN_ID,
    .value_size = APP_CLI_VALUE_SIZE_NONE,
    .input = APP_CLI_INPUT_FLAG_DEFAULT,
    .output = APP_CLI_OUTPUT_FLAG_DEFAULT,
    .value = NULL,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_ip_address,
    .description = "Border Router IPv6 address"
  },
  {
    .key = "ip_address_primary_parent",
    .domain = APP_CLI_WISUN_DOMAIN_ID,
    .value_size = APP_CLI_VALUE_SIZE_NONE,
    .input = APP_CLI_INPUT_FLAG_DEFAULT,
    .output = APP_CLI_OUTPUT_FLAG_DEFAULT,
    .value = NULL,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = app_settings_get_ip_address,
    .description = "Primary Parent IPv6 address"
  },
#if defined(SL_CATALOG_WISUN_MODE_SWITCH_PRESENT)
  {
    .key = "mode_switch_tx_counter",
    .domain = APP_CLI_WISUN_DOMAIN_ID,
    .value_size = APP_CLI_VALUE_SIZE_UINT32,
    .input = APP_CLI_INPUT_FLAG_DEFAULT,
    .output = APP_CLI_OUTPUT_FLAG_DEFAULT,
    .value = NULL,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = _app_ms_get_counters,
    .description = "Mode Switch Tx count"
  },
  {
    .key = "mode_switch_tx_failed_counter",
    .domain = APP_CLI_WISUN_DOMAIN_ID,
    .value_size = APP_CLI_VALUE_SIZE_UINT32,
    .input = APP_CLI_INPUT_FLAG_DEFAULT,
    .output = APP_CLI_OUTPUT_FLAG_DEFAULT,
    .value = NULL,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = _app_ms_get_counters,
    .description = "Mode Switch Tx failed count"
  },
  {
    .key = "mode_switch_rx_counter",
    .domain = APP_CLI_WISUN_DOMAIN_ID,
    .value_size = APP_CLI_VALUE_SIZE_UINT32,
    .input = APP_CLI_INPUT_FLAG_DEFAULT,
    .output = APP_CLI_OUTPUT_FLAG_DEFAULT,
    .value = NULL,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = _app_ms_get_counters,
    .description = "Mode Switch Rx count"
  },
  {
    .key = "mode_switch_rx_failed_counter",
    .domain = APP_CLI_WISUN_DOMAIN_ID,
    .value_size = APP_CLI_VALUE_SIZE_UINT32,
    .input = APP_CLI_INPUT_FLAG_DEFAULT,
    .output = APP_CLI_OUTPUT_FLAG_DEFAULT,
    .value = NULL,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = _app_ms_get_counters,
    .description = "Mode Switch Rx failed count"
  },
#endif
#if defined(SL_CATALOG_WISUN_APP_CORE_PRESENT)
  {
    .key = "regulation",
    .domain = APP_CLI_WISUN_DOMAIN_ID,
    .value_size = APP_CLI_VALUE_SIZE_UINT8,
    .input = APP_CLI_INPUT_FLAG_DEFAULT,
    .output = APP_CLI_OUTPUT_FLAG_DEFAULT,
    .value = NULL,
    .input_enum_list = app_regulation_enum,
    .output_enum_list = app_regulation_enum,
    .set_handler = _app_set_regulation,
    .get_handler = _app_get_regulation,
    .description = "Regional regulation [uint8]"
  },
  {
    .key = "regulation_warning_threshold",
    .domain = APP_CLI_WISUN_DOMAIN_ID,
    .value_size = APP_CLI_VALUE_SIZE_UINT8,
    .input = APP_CLI_INPUT_FLAG_SIGNED,
    .output = APP_CLI_OUTPUT_FLAG_SIGNED,
    .value = NULL,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = _app_set_regulation_warning_threshold,
    .get_handler = _app_get_regulation_warning_threshold,
    .description = "Transmission warning threshold in percent (-1 to disable) [int8]"
  },
  {
    .key = "regulation_alert_threshold",
    .domain = APP_CLI_WISUN_DOMAIN_ID,
    .value_size = APP_CLI_VALUE_SIZE_UINT8,
    .input = APP_CLI_INPUT_FLAG_SIGNED,
    .output = APP_CLI_OUTPUT_FLAG_SIGNED,
    .value = NULL,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = _app_set_regulation_alert_threshold,
    .get_handler = _app_get_regulation_alert_threshold,
    .description = "Transmission alert threshold in percent (-1 to disable) [int8]"
  },
  {
    .key = "mac_address",
    .domain = APP_CLI_WISUN_DOMAIN_ID,
    .value_size = APP_CLI_VALUE_SIZE_NONE,
    .input = APP_CLI_INPUT_FLAG_SIGNED,
    .output = APP_CLI_OUTPUT_FLAG_SIGNED,
    .value = NULL,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = _app_settings_set_mac_address,
    .get_handler = _app_settings_get_mac_address,
    .description = "MAC address"
  },
#endif
#if defined(SL_CATALOG_WISUN_LFN_DEVICE_SUPPORT_PRESENT)
  {
    .key = "device_type",
    .domain = APP_CLI_WISUN_DOMAIN_ID,
    .value_size = APP_CLI_VALUE_SIZE_UINT8,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = NULL,
    .input_enum_list = app_wisun_device_type_enum,
    .output_enum_list = app_wisun_device_type_enum,
    .set_handler = NULL,
    .get_handler = _app_get_device_type,
    .description = "Device type"
  },
  {
    .key = "lfn_profile",
    .domain = APP_CLI_WISUN_DOMAIN_ID,
    .value_size = APP_CLI_VALUE_SIZE_UINT8,
    .input = APP_SETTINGS_INPUT_FLAG_DEFAULT,
    .output = APP_SETTINGS_OUTPUT_FLAG_DEFAULT,
    .value = NULL,
    .input_enum_list = app_wisun_lfn_profile_enum,
    .output_enum_list = app_wisun_lfn_profile_enum,
    .set_handler = NULL,
    .get_handler = _app_get_lfn_profile,
    .description = "Wi-SUN LFN profile"
  },
#endif
  {
    .key = NULL,
    .domain = 0,
    .value_size = APP_CLI_VALUE_SIZE_NONE,
    .input = APP_CLI_INPUT_FLAG_DEFAULT,
    .output = APP_CLI_OUTPUT_FLAG_DEFAULT,
    .value = NULL,
    .input_enum_list = NULL,
    .output_enum_list = NULL,
    .set_handler = NULL,
    .get_handler = NULL,
    .description = NULL
  }
};

// -----------------------------------------------------------------------------
//                          Public Function Definitions
// -----------------------------------------------------------------------------

#if defined(SL_CATALOG_APP_PROJECT_INFO_PRESENT)
/* CLI app about handler */
void app_about(void)
{
  sl_wisun_app_core_util_project_info_print(false);
}
#endif

#if defined(SL_CATALOG_WISUN_APP_CORE_PRESENT)
/* CLI app connect to FAN 1.0 handler */
void app_join_fan10(const sl_cli_command_arg_t *arguments)
{
  (void) arguments;
  _app_connect(SL_WISUN_PHY_CONFIG_FAN10);
}

/* CLI app connect to FAN 1.1 handler */
void app_join_fan11(const sl_cli_command_arg_t *arguments)
{
  (void) arguments;
#if APP_CLI_ENABLE_FAN11_CONFIG
  _app_connect(SL_WISUN_PHY_CONFIG_FAN11);
#else
  printf("[FAN 1.1 configuration is not supported]\n");
#endif
}

/* CLI app connect explicit handler */
void app_join_explicit(const sl_cli_command_arg_t *arguments)
{
  (void) arguments;
#if APP_CLI_ENABLE_EXPLICIT_CONFIG
  _app_connect(SL_WISUN_PHY_CONFIG_EXPLICIT);
#else
  printf("[Explicit PHY configuration is not supported]\n");
#endif
}

/* CLI app disconnect handler */
void app_disconnect(const sl_cli_command_arg_t *arguments)
{
  sl_status_t ret;
  sl_wisun_join_state_t join_state = SL_WISUN_JOIN_STATE_DISCONNECTED;
  (void) arguments;

  ret = sl_wisun_get_join_state(&join_state);
  if (ret != SL_STATUS_OK) {
    printf("[Failed: Getting join state failed]\n");
    return;
  }
  if (join_state == SL_WISUN_JOIN_STATE_DISCONNECTED) {
    printf("[Failed: already disconnected]\n");
    return;
  }

  ret = sl_wisun_disconnect();
  if (ret == SL_STATUS_OK) {
    printf("[Disconnecting]\n");
  } else {
    printf("[Disconnection failed: %lu]\n", ret);
  }
}

void app_mac_allow(const sl_cli_command_arg_t *arguments)
{
  sl_status_t ret;
  const char *address_str = NULL;
  sl_wisun_mac_address_t address;
  const app_enum_t *value_enum;

  address_str = sl_cli_get_argument_string(arguments, 0);

  value_enum = app_util_get_enum_by_string(app_mac_enum, address_str);
  if (value_enum) {
    // Assume enumeration means a broadcast address
    address = APP_BROADCAST_MAC;
  } else {
    // Attempt to convert the MAC address string
    ret = app_util_get_mac_address(&address, address_str);
    if (ret != SL_STATUS_OK) {
      printf("[Failed: unable to parse the MAC address: %lu]\n", ret);
      return;
    }
  }

  ret = sl_wisun_allow_mac_address(&address);
  if (ret != SL_STATUS_OK) {
    printf("[Failed: unable to add the MAC address to the access list: %lu]\n", ret);
    return;
  }

  printf("[MAC address added to the access list]\n");
}

void app_mac_deny(const sl_cli_command_arg_t *arguments)
{
  sl_status_t ret;
  const char *address_str = NULL;
  sl_wisun_mac_address_t address;
  const app_enum_t *value_enum;

  address_str = sl_cli_get_argument_string(arguments, 0);

  value_enum = app_util_get_enum_by_string(app_mac_enum, address_str);
  if (value_enum) {
    // Assume enumeration means a broadcast address
    address = APP_BROADCAST_MAC;
  } else {
    // Attempt to convert the MAC address string
    ret = app_util_get_mac_address(&address, address_str);
    if (ret != SL_STATUS_OK) {
      printf("[Failed: unable to parse the MAC address: %lu]\n", ret);
      return;
    }
  }

  ret = sl_wisun_deny_mac_address(&address);
  if (ret != SL_STATUS_OK) {
    printf("[Failed: unable to add the MAC address to the deny list: %lu]\n", ret);
    return;
  }

  printf("[MAC address added to the deny list]\n");
}

#endif

/* App CLI handler */
void app_cli_handler(const sl_cli_command_arg_t *args)
{
#if defined(SL_CATALOG_APP_PROJECT_INFO_PRESENT)
  if ((sl_strcasecmp(args->argv[0], "about") == 0) || (sl_strcasecmp(args->argv[0], "a") == 0)) {
    app_about();
  }
#endif

  _cli_handler_iterator(args,
                        _wisun_app_cli_handler_properties_size,
                        _wisun_app_cli_handler_properties);
}

/* OTA DFU CLI handler */
void sl_wisun_ota_dfu_cli_handler(const sl_cli_command_arg_t *args)
{
#if defined(SL_CATALOG_WISUN_OTA_DFU_PRESENT) \
  && SL_WISUN_CLI_ENABLED                     \
  && SL_WISUN_OTA_DFU_CLI_ENABLED
  _cli_handler_iterator(args,
                        _ota_dfu_cli_handler_properties_size,
                        _ota_dfu_cli_handler_properties);
#else
  (void) args;
#endif
}

/* iPerf CLI handler */
void sl_iperf_cli_handler(const sl_cli_command_arg_t *args)
{
#if defined(SL_CATALOG_IPERF_PRESENT) \
  && SL_WISUN_CLI_ENABLED             \
  && SL_WISUN_IPERF_CLI_ENABLED
  _cli_handler_iterator(args,
                        _iperf_cli_handler_properties_size,
                        _iperf_cli_handler_properties);
#else
  (void) args;
#endif
}

// -----------------------------------------------------------------------------
//                          Static Function Definitions
// -----------------------------------------------------------------------------

#if defined(SL_CATALOG_WISUN_APP_CORE_PRESENT)
static void _app_connect(const sl_wisun_phy_config_type_t config_type)
{
  sl_status_t stat = SL_STATUS_FAIL;

  stat = _load_common_params_from_phy_cfg();
  if (stat != SL_STATUS_OK) {
    printf("[Failed: Loading PHY config]\n");
    return;
  }

  // Set expected config type
  _phy_common_params.phy_config_type = (uint8_t) config_type;

  // Store CLI cfg to PHY cfg
  stat = _store_common_params_to_phy_cfg();

  if (stat != SL_STATUS_OK) {
    printf("[Failed: Storing PHY config]\n");
    return;
  }
  // call connect API
  sl_wisun_app_core_network_connect();
}
#endif

static sl_status_t _load_common_params_from_phy_cfg(void)
{
  static sl_wisun_phy_config_t phy_cfg = { 0U };

  if (!app_wisun_setting_is_notified(APP_SETTING_NOTIFICATION_SET_PHY_CFG,
                                     APP_SETTING_DEFAULT_SUBSCRIPT_CH)) {
    return SL_STATUS_OK;
  }

  (void) app_wisun_setting_get_phy(&phy_cfg);

  // FAN 1.0 setting
  if (phy_cfg.type == SL_WISUN_PHY_CONFIG_FAN10) {
    _phy_common_params.regulatory_domain  = phy_cfg.config.fan10.reg_domain;
    _phy_common_params.operating_mode     = phy_cfg.config.fan10.op_mode;
    _phy_common_params.operating_class    = phy_cfg.config.fan10.op_class;

    // FAN 1.1 setting
  } else if (phy_cfg.type == SL_WISUN_PHY_CONFIG_FAN11) {
#if APP_CLI_ENABLE_FAN11_CONFIG
    _phy_common_params.regulatory_domain  = phy_cfg.config.fan11.reg_domain;
    _phy_common_params.chan_plan_id       = phy_cfg.config.fan11.chan_plan_id;
    _phy_common_params.phy_mode_id        = phy_cfg.config.fan11.phy_mode_id;
#else
    (void) 0U;
#endif
    // Explicit setting
  } else if (phy_cfg.type == SL_WISUN_PHY_CONFIG_EXPLICIT) {
#if APP_CLI_ENABLE_EXPLICIT_CONFIG
    _phy_common_params.ch0_frequency      = phy_cfg.config.explicit_plan.ch0_frequency_khz;
    _phy_common_params.number_of_channels = phy_cfg.config.explicit_plan.number_of_channels;
    _phy_common_params.channel_spacing    = phy_cfg.config.explicit_plan.channel_spacing;
    _phy_common_params.phy_mode_id        = phy_cfg.config.explicit_plan.phy_mode_id;
#else
    (void) 0U;
#endif
    // Invalid setting
  } else {
    return SL_STATUS_FAIL;
  }
  _phy_common_params.phy_config_type = phy_cfg.type;

  app_wisun_setting_ack_notification(APP_SETTING_NOTIFICATION_SET_PHY_CFG,
                                     APP_SETTING_DEFAULT_SUBSCRIPT_CH);
  return SL_STATUS_OK;
}

static sl_status_t _store_common_params_to_phy_cfg(void)
{
  static sl_wisun_phy_config_t phy_cfg = { 0U };

  // FAN 1.0 setting
  if (_phy_common_params.phy_config_type == SL_WISUN_PHY_CONFIG_FAN10) {
    phy_cfg.config.fan10.reg_domain            = _phy_common_params.regulatory_domain;
    phy_cfg.config.fan10.op_mode               = _phy_common_params.operating_mode;
    phy_cfg.config.fan10.op_class              = _phy_common_params.operating_class;

    // FAN 1.1 setting
  } else if (_phy_common_params.phy_config_type == SL_WISUN_PHY_CONFIG_FAN11) {
#if APP_CLI_ENABLE_FAN11_CONFIG
    phy_cfg.config.fan11.reg_domain            = _phy_common_params.regulatory_domain;
    phy_cfg.config.fan11.chan_plan_id          = _phy_common_params.chan_plan_id;
    phy_cfg.config.fan11.phy_mode_id           = _phy_common_params.phy_mode_id;
#else
    (void) 0U;
#endif
    // Explicit setting
  } else if (_phy_common_params.phy_config_type == SL_WISUN_PHY_CONFIG_EXPLICIT) {
#if APP_CLI_ENABLE_EXPLICIT_CONFIG
    phy_cfg.config.explicit_plan.ch0_frequency_khz  = _phy_common_params.ch0_frequency;
    phy_cfg.config.explicit_plan.number_of_channels = _phy_common_params.number_of_channels;
    phy_cfg.config.explicit_plan.channel_spacing    = _phy_common_params.channel_spacing;
    phy_cfg.config.explicit_plan.phy_mode_id        = _phy_common_params.phy_mode_id;
#else
    (void) 0U;
#endif
    // Invalid setting
  } else {
    return SL_STATUS_FAIL;
  }

  phy_cfg.type = _phy_common_params.phy_config_type;

  app_wisun_setting_set_phy(&phy_cfg);

  return SL_STATUS_OK;
}

/* App CLI getting connection state (join state) */
static sl_status_t _app_cli_get_connection(char *value_str,
                                           const char *key_str,
                                           const app_cli_entry_t *entry)
{
  sl_status_t res = SL_STATUS_FAIL;
  uint32_t value;
  const app_enum_t* value_enum;
  sl_wisun_join_state_t join_state = SL_WISUN_JOIN_STATE_DISCONNECTED;
  (void)key_str;

  if ((value_str == NULL) || (entry == NULL)) {
    return SL_STATUS_FAIL;
  }

  // gets the connection state from the core
  res = sl_wisun_get_join_state(&join_state);
  if (res != SL_STATUS_OK) {
    return res;
  }
  value = (uint32_t)join_state;

  // finds the proper string for the value
  value_enum = entry->output_enum_list;

  if (value_enum->value_str != NULL) {
    while (value_enum) {
      if (value_enum->value == value) {
        // Matching enumeration found
        break;
      }
      value_enum++;
    }
  } else {
    return SL_STATUS_FAIL;
  }

  // creates the value string
  snprintf(value_str, APP_CLI_STR_VALUE_LENGTH, "%s (%d)", value_enum->value_str, (uint8_t)value_enum->value);

  return SL_STATUS_OK;
}

/* App CLI setting network name */
static sl_status_t _app_cli_set_network_name(const char *value_str,
                                             const char *key_str,
                                             const app_cli_entry_t *entry)
{
  (void)key_str;
  (void)entry;

  return app_wisun_setting_set_network_name(value_str);
}

/* App CLI setting network size */
static sl_status_t _app_cli_set_network_size(const char *value_str,
                                             const char *key_str,
                                             const app_cli_entry_t *entry)
{
  sl_status_t res = SL_STATUS_FAIL;
  uint32_t value = 0U;
  (void)key_str;
  (void)entry;

  res = app_util_get_integer(&value,
                             value_str,
                             entry->input_enum_list,
                             entry->input & APP_CLI_INPUT_FLAG_SIGNED);
  if (res == SL_STATUS_OK) {
    // sets the network name
    res = app_wisun_setting_set_network_size((uint8_t*)&value);
  }

  return res;
}

/* App CLI setting tx power */
static sl_status_t _app_cli_set_tx_power(const char *value_str,
                                         const char *key_str,
                                         const app_cli_entry_t *entry)
{
  sl_status_t res = SL_STATUS_FAIL;
  int32_t value = 0U;
  (void)key_str;
  (void)entry;

  res = app_util_get_integer((uint32_t *)&value,
                             value_str,
                             entry->input_enum_list,
                             entry->input & APP_CLI_INPUT_FLAG_SIGNED);
  if (res == SL_STATUS_OK) {
    // sets the tx power
    res = app_wisun_setting_set_tx_power((int16_t *)&value);
  }

  return res;
}

/* App CLI setting network name */
static sl_status_t _app_cli_get_network_name(char *value_str,
                                             const char *key_str,
                                             const app_cli_entry_t *entry)
{
  sl_status_t res = SL_STATUS_FAIL;
  char name_buffer[APP_SETTING_NETWORK_NAME_MAX_SIZE] = { 0 };
  (void)key_str;
  (void)entry;

  // gets the network name
  res = app_wisun_setting_get_network_name(name_buffer, sizeof(name_buffer));

  if (res == SL_STATUS_OK) {
    snprintf(value_str, APP_CLI_STR_VALUE_LENGTH, "%s", name_buffer);
  }

  return res;
}

/* App CLI setting network size */
static sl_status_t _app_cli_get_network_size(char *value_str,
                                             const char *key_str,
                                             const app_cli_entry_t *entry)
{
  sl_status_t res = SL_STATUS_FAIL;
  const app_enum_t* value_enum;
  uint8_t value = 0U;
  (void)key_str;

  res = app_wisun_setting_get_network_size(&value);
  // finds the proper string for the value
  value_enum = entry->output_enum_list;

  if (res != SL_STATUS_OK || value_enum->value_str == NULL) {
    return SL_STATUS_FAIL;
  }

  while (value_enum) {
    if (value_enum->value == value) {
      // Matching enumeration found
      break;
    }
    value_enum++;
  }

  // creates the value string
  snprintf(value_str, APP_CLI_STR_VALUE_LENGTH, "%s (%d)", value_enum->value_str, (uint8_t)value_enum->value);

  return res;
}

/* App CLI setting tx power */
static sl_status_t _app_cli_get_tx_power(char *value_str,
                                         const char *key_str,
                                         const app_cli_entry_t *entry)
{
  sl_status_t res = SL_STATUS_FAIL;
  int16_t value = 0;
  (void)key_str;
  (void)entry;

  res = app_wisun_setting_get_tx_power(&value);
  if (res == SL_STATUS_OK) {
    snprintf(value_str, APP_CLI_STR_VALUE_LENGTH, "%d ddBm", value);
  } else {
    snprintf(value_str, APP_CLI_STR_VALUE_LENGTH, "N/A");
  }

  return res;
}

static bool _is_param_used(const char *param_str)
{
  const app_enum_t *iter = NULL;

  iter = (const app_enum_t *) _phy_cfg_map;

  while (iter != NULL && iter->value_str != NULL) {
    if (iter->value == _phy_common_params.phy_config_type
        && !strncmp(param_str, iter->value_str, APP_CLI_STR_VALUE_LENGTH)) {
      return true;
    }
    ++iter;
  }

  return false;
}

__STATIC_INLINE const char *_param_used_flag_str(const char *param_str)
{
  return _is_param_used(param_str) ? "" : "(unused)";
}

/* App CLI getting PHY parameters */
static sl_status_t _app_cli_get_phy(char *value_str,
                                    const char *key_str,
                                    const app_cli_entry_t *entry)
{
  sl_status_t res = SL_STATUS_FAIL;
  (void)key_str;

  if ((value_str == NULL) || (entry == NULL) || (entry->key == NULL)) {
    return res;
  }

  res = _load_common_params_from_phy_cfg();
  if (res != SL_STATUS_OK) {
    return res;
  }

  // Regulatory domain
  if (strstr(entry->key, APP_CLI_PHY_PARAM_REG_DOMAIN_STR)) {
    snprintf(value_str, APP_CLI_STR_VALUE_LENGTH, "%s (%u) %s",
             app_wisun_trace_util_reg_domain_to_str(_phy_common_params.regulatory_domain),
             _phy_common_params.regulatory_domain,
             _param_used_flag_str(APP_CLI_PHY_PARAM_REG_DOMAIN_STR));

    // Operating class
  } else if (strstr(entry->key, APP_CLI_PHY_PARAM_OP_CLASS_STR)) {
    snprintf(value_str, APP_CLI_STR_VALUE_LENGTH, "%u %s",
             _phy_common_params.operating_class,
             _param_used_flag_str(APP_CLI_PHY_PARAM_OP_CLASS_STR));

    // Operating mode
  } else if (strstr(entry->key, APP_CLI_PHY_PARAM_OP_MODE_STR)) {
    snprintf(value_str, APP_CLI_STR_VALUE_LENGTH, "0x%x %s",
             _phy_common_params.operating_mode,
             _param_used_flag_str(APP_CLI_PHY_PARAM_OP_MODE_STR));

#if APP_CLI_ENABLE_EXPLICIT_CONFIG
    // Number of channels
  } else if (strstr(entry->key, APP_CLI_PHY_PARAM_NUM_OF_CH_STR)) {
    snprintf(value_str, APP_CLI_STR_VALUE_LENGTH, "%u %s",
             _phy_common_params.number_of_channels,
             _param_used_flag_str(APP_CLI_PHY_PARAM_NUM_OF_CH_STR));

    // CH0 Frequency
  } else if (strstr(entry->key, APP_CLI_PHY_PARAM_CH0_FREQ_STR)) {
    snprintf(value_str, APP_CLI_STR_VALUE_LENGTH, "%lu %s",
             _phy_common_params.ch0_frequency,
             _param_used_flag_str(APP_CLI_PHY_PARAM_CH0_FREQ_STR));

    // Channel spacing
  } else if (strstr(entry->key, APP_CLI_PHY_PARAM_CH_SPACING_STR)) {
    snprintf(value_str, APP_CLI_STR_VALUE_LENGTH, "%s (%u) %s",
             app_wisun_trace_util_ch_spacing_to_str(_phy_common_params.channel_spacing),
             _phy_common_params.channel_spacing,
             _param_used_flag_str(APP_CLI_PHY_PARAM_CH_SPACING_STR));
#endif

#if APP_CLI_ENABLE_FAN11_CONFIG
    // Chan plan ID
  } else if (strstr(entry->key, APP_CLI_PHY_PARAM_CH_PLAN_ID_STR)) {
    snprintf(value_str, APP_CLI_STR_VALUE_LENGTH, "%u %s",
             _phy_common_params.chan_plan_id,
             _param_used_flag_str(APP_CLI_PHY_PARAM_CH_PLAN_ID_STR));
#endif

#if APP_CLI_ENABLE_EXPLICIT_CONFIG || APP_CLI_ENABLE_FAN11_CONFIG
    // PHY mode ID
  } else if (strstr(entry->key, APP_CLI_PHY_PARAM_PHY_MODE_ID_STR)) {
    snprintf(value_str, APP_CLI_STR_VALUE_LENGTH, "%u %s",
             _phy_common_params.phy_mode_id,
             _param_used_flag_str(APP_CLI_PHY_PARAM_PHY_MODE_ID_STR));
#endif

    // PHY config type
  } else if (strstr(entry->key, APP_CLI_PHY_PARAM_PHY_CFG_TYPE_STR)) {
    snprintf(value_str, APP_CLI_STR_VALUE_LENGTH, "%s (%d)",
             app_wisun_trace_util_phy_cfg_type_to_str(_phy_common_params.phy_config_type),
             _phy_common_params.phy_config_type);

    // Not found
  } else {
    return SL_STATUS_FAIL;
  }

  return SL_STATUS_OK;
}

/* App CLI getting PHY parameters */
static sl_status_t _app_cli_set_phy(const char *value_str,
                                    const char *key_str,
                                    const app_settings_entry_t *entry)
{
  sl_status_t res = SL_STATUS_FAIL;
  int32_t value = 0U;
  (void)key_str;

  if ((value_str == NULL) || (entry == NULL) || (entry->key == NULL)) {
    return res;
  }
  res = _load_common_params_from_phy_cfg();
  if (res != SL_STATUS_OK) {
    return res;
  }

  res = app_util_get_integer((uint32_t *)&value,
                             value_str,
                             entry->input_enum_list,
                             entry->input & APP_SETTINGS_INPUT_FLAG_SIGNED);
  if (res != SL_STATUS_OK) {
    return res;
  }

  // Regulatory domain
  if (strstr(entry->key, APP_CLI_PHY_PARAM_REG_DOMAIN_STR)) {
    _phy_common_params.regulatory_domain = value;

    // Operating class
  } else if (strstr(entry->key, APP_CLI_PHY_PARAM_OP_CLASS_STR)) {
    _phy_common_params.operating_class = value;

    // Operating mode
  } else if (strstr(entry->key, APP_CLI_PHY_PARAM_OP_MODE_STR)) {
    _phy_common_params.operating_mode = value;

#if APP_CLI_ENABLE_EXPLICIT_CONFIG
    // Number of channels
  } else if (strstr(entry->key, APP_CLI_PHY_PARAM_NUM_OF_CH_STR)) {
    _phy_common_params.number_of_channels = value;

    // CH0 Frequency
  } else if (strstr(entry->key, APP_CLI_PHY_PARAM_CH0_FREQ_STR)) {
    _phy_common_params.ch0_frequency = value;

    // Channel spacing
  } else if (strstr(entry->key, APP_CLI_PHY_PARAM_CH_SPACING_STR)) {
    _phy_common_params.channel_spacing = value;
#endif

#if APP_CLI_ENABLE_FAN11_CONFIG
    // Chan plan ID
  } else if (strstr(entry->key, APP_CLI_PHY_PARAM_CH_PLAN_ID_STR)) {
    _phy_common_params.chan_plan_id = value;
#endif

#if APP_CLI_ENABLE_EXPLICIT_CONFIG || APP_CLI_ENABLE_FAN11_CONFIG
    // PHY mode ID
  } else if (strstr(entry->key, APP_CLI_PHY_PARAM_PHY_MODE_ID_STR)) {
    _phy_common_params.phy_mode_id = value;
#endif
    // Not found
  } else {
    return SL_STATUS_FAIL;
  }

  return SL_STATUS_OK;
}

#if defined(SL_CATALOG_WISUN_MODE_SWITCH_PRESENT)
/* App CLI getting mode switch counters */
static sl_status_t _app_ms_get_counters(char *value_str,
                                        const char *key_str,
                                        const app_cli_entry_t *entry)
{
  sl_status_t res = SL_STATUS_FAIL;
  sl_wisun_statistics_t stat;
  (void)key_str;

  if ((value_str == NULL) || (entry == NULL) || (entry->key == NULL)) {
    return res;
  }

  // gets the statsitic that contains the mode switch information
  res = sl_wisun_get_statistics(SL_WISUN_STATISTICS_TYPE_MAC, &stat);

  if (res != SL_STATUS_OK) {
    return res;
  }
  // mode switch TX-RX counters (inc. failed)
  if (strstr(entry->key, "mode_switch_tx_counter")) {
    snprintf(value_str, APP_CLI_STR_VALUE_LENGTH, "%lu", stat.mac.tx_ms_count);
  } else if (strstr(entry->key, "mode_switch_tx_failed_counter")) {
    snprintf(value_str, APP_CLI_STR_VALUE_LENGTH, "%lu", stat.mac.tx_ms_failed_count);
  } else if (strstr(entry->key, "mode_switch_rx_counter")) {
    snprintf(value_str, APP_CLI_STR_VALUE_LENGTH, "%lu", stat.mac.rx_ms_count);
  } else if (strstr(entry->key, "mode_switch_rx_failed_counter")) {
    snprintf(value_str, APP_CLI_STR_VALUE_LENGTH, "%lu", stat.mac.rx_ms_failed_count);
  } else {
    // nothing to do
  }

  return SL_STATUS_OK;
}
#endif

#if defined(SL_CATALOG_WISUN_APP_CORE_PRESENT)
static sl_status_t _app_set_regulation(const char *value_str,
                                       const char *key_str,
                                       const app_settings_entry_t *entry)
{
  sl_status_t res = SL_STATUS_FAIL;
  uint32_t value = 0U;
  sl_wisun_app_core_reg_thresholds_t thresholds = { 0U };
  sl_wisun_join_state_t join_state = SL_WISUN_JOIN_STATE_DISCONNECTED;
  (void)key_str;
  (void)entry;

  if ((value_str == NULL) || (entry == NULL) || (entry->key == NULL)) {
    return SL_STATUS_FAIL;
  }

  // checking if the device connected or not
  res = sl_wisun_get_join_state(&join_state);
  if (res != SL_STATUS_OK) {
    return res;
  } else {
    if (join_state != SL_WISUN_JOIN_STATE_DISCONNECTED) {
      printf("[Regulation related settings unavailable, disconnection is needed]\n");
      return SL_STATUS_FAIL;
    }
  }

  res = app_util_get_integer(&value,
                             value_str,
                             entry->input_enum_list,
                             entry->input & APP_CLI_INPUT_FLAG_SIGNED);

  if (res != SL_STATUS_OK) {
    printf("[Failed: Get value error: %lu]\n", res);
    return res;
  }

  if (strstr(entry->key, "regulation")) {
    // sets the thresholds
    (void)sl_wisun_app_core_get_regulation_thresholds(&thresholds);
    res = sl_wisun_set_regulation_tx_thresholds(thresholds.warning_threshold,
                                                thresholds.alert_threshold);
    if (res != SL_STATUS_OK) {
      printf("[Failed: unable to set regulation TX thresholds: %lu]\n", res);
      return res;
    }

    // sets regulation
    res = sl_wisun_set_regulation((sl_wisun_regulation_t)value);
    if (res != SL_STATUS_OK) {
      printf("[Regulation not valid]\n");
      return res;
    } else {
      _app_regulation = (sl_wisun_regulation_t)value;
    }

    // sets status of regulation
    if ((sl_wisun_regulation_t)value == SL_WISUN_APP_CORE_REGULATION_NONE) {
      sl_wisun_app_core_set_regulation_active(false);
    } else {
      sl_wisun_app_core_set_regulation_active(true);
    }
  }

  return SL_STATUS_OK;
}

static sl_status_t _app_set_regulation_warning_threshold(const char *value_str,
                                                         const char *key_str,
                                                         const app_settings_entry_t *entry)
{
  sl_status_t res = SL_STATUS_FAIL;
  uint32_t value = 0U;
  sl_wisun_app_core_reg_thresholds_t thresholds = { 0U };
  sl_wisun_join_state_t join_state = SL_WISUN_JOIN_STATE_DISCONNECTED;
  (void)key_str;

  if ((value_str == NULL) || (entry == NULL) || (entry->key == NULL)) {
    return SL_STATUS_FAIL;
  }

  // checking if the device connected or not
  res = sl_wisun_get_join_state(&join_state);
  if (res != SL_STATUS_OK) {
    return res;
  } else {
    if (join_state != SL_WISUN_JOIN_STATE_DISCONNECTED) {
      printf("[Regulation related settings unavailable, disconnection is needed]\n");
      return SL_STATUS_FAIL;
    }
  }

  res = app_util_get_integer(&value,
                             value_str,
                             entry->input_enum_list,
                             entry->input & APP_CLI_INPUT_FLAG_SIGNED);

  if (res != SL_STATUS_OK) {
    printf("[Failed: Get value error: %lu]\n", res);
    return res;
  }

  if (strstr(entry->key, "regulation_warning_threshold")) {
    (void)sl_wisun_app_core_get_regulation_thresholds(&thresholds);
    res = sl_wisun_set_regulation_tx_thresholds((int8_t)value, thresholds.alert_threshold);
    if (res != SL_STATUS_OK) {
      printf("[Failed: unable to set regulation TX warning threshold: %lu]\n", res);
      return res;
    } else {
      sl_wisun_app_core_set_regulation_thresholds((int8_t)value, thresholds.alert_threshold);
    }
  }

  return SL_STATUS_OK;
}

static sl_status_t _app_set_regulation_alert_threshold(const char *value_str,
                                                       const char *key_str,
                                                       const app_settings_entry_t *entry)
{
  sl_status_t res = SL_STATUS_FAIL;
  uint32_t value = 0U;
  sl_wisun_app_core_reg_thresholds_t thresholds = { 0U };
  sl_wisun_join_state_t join_state = SL_WISUN_JOIN_STATE_DISCONNECTED;
  (void)key_str;
  (void)entry;

  if ((value_str == NULL) || (entry == NULL) || (entry->key == NULL)) {
    return SL_STATUS_FAIL;
  }

  // checking if the device connected or not
  res = sl_wisun_get_join_state(&join_state);
  if (res != SL_STATUS_OK) {
    return res;
  } else {
    if (join_state != SL_WISUN_JOIN_STATE_DISCONNECTED) {
      printf("[Regulation related settings unavailable, disconnection is needed]\n");
      return SL_STATUS_FAIL;
    }
  }

  res = app_util_get_integer(&value,
                             value_str,
                             entry->input_enum_list,
                             entry->input & APP_CLI_INPUT_FLAG_SIGNED);

  if (res != SL_STATUS_OK) {
    printf("[Failed: Get value error: %lu]\n", res);
    return res;
  }

  if (strstr(entry->key, "regulation_alert_threshold")) {
    (void)sl_wisun_app_core_get_regulation_thresholds(&thresholds);
    res = sl_wisun_set_regulation_tx_thresholds(thresholds.warning_threshold, (int8_t)value);
    if (res != SL_STATUS_OK) {
      printf("[Failed: unable to set regulation TX alert threshold: %lu]\n", res);
      return res;
    } else {
      sl_wisun_app_core_set_regulation_thresholds(thresholds.warning_threshold, (int8_t)value);
    }
  }

  return SL_STATUS_OK;
}

static sl_status_t _app_get_regulation(char *value_str,
                                       const char *key_str,
                                       const app_cli_entry_t *entry)
{
  const app_enum_t *value_enum = NULL;
  (void)key_str;

  if ((value_str == NULL) || (entry == NULL) || (entry->key == NULL)) {
    return SL_STATUS_FAIL;
  }

  if (!strstr(entry->key, "regulation")) {
    return SL_STATUS_FAIL;
  }

  // finds the proper string for the value for regulation
  value_enum = entry->output_enum_list;
  if (value_enum->value_str == NULL) {
    return SL_STATUS_FAIL;
  }

  while (value_enum) {
    if (value_enum->value == _app_regulation) {
      // Matching enumeration found
      break;
    }
    value_enum++;
  }

  snprintf(value_str, APP_CLI_STR_VALUE_LENGTH, "%s (%d)",
           value_enum->value_str, (uint8_t)value_enum->value);

  return SL_STATUS_OK;
}

static sl_status_t _app_get_regulation_warning_threshold(char *value_str,
                                                         const char *key_str,
                                                         const app_cli_entry_t *entry)
{
  sl_wisun_app_core_reg_thresholds_t thresholds = { 0U };
  (void)key_str;

  if ((value_str == NULL) || (entry == NULL) || (entry->key == NULL)) {
    return SL_STATUS_FAIL;
  }

  if (strstr(entry->key, "regulation_warning_threshold")) {
    (void)sl_wisun_app_core_get_regulation_thresholds(&thresholds);
    snprintf(value_str, APP_CLI_STR_VALUE_LENGTH, "%d",
             thresholds.warning_threshold);
  }

  return SL_STATUS_OK;
}

static sl_status_t _app_get_regulation_alert_threshold(char *value_str,
                                                       const char *key_str,
                                                       const app_cli_entry_t *entry)
{
  sl_wisun_app_core_reg_thresholds_t thresholds = { 0U };
  (void)key_str;

  if ((value_str == NULL) || (entry == NULL) || (entry->key == NULL)) {
    return SL_STATUS_FAIL;
  }

  if (strstr(entry->key, "regulation_alert_threshold")) {
    (void)sl_wisun_app_core_get_regulation_thresholds(&thresholds);
    snprintf(value_str, APP_CLI_STR_VALUE_LENGTH, "%d",
             thresholds.alert_threshold);
  }

  return SL_STATUS_OK;
}

static sl_status_t _app_settings_set_mac_address(const char *value_str,
                                                 const char *key_str,
                                                 const app_settings_entry_t *entry)
{
  sl_status_t ret;
  sl_wisun_mac_address_t address;
  (void)key_str;
  (void)entry;

  ret = app_util_get_mac_address(&address, value_str);
  if (ret == SL_STATUS_OK) {
    ret = sl_wisun_set_mac_address(&address);
  }

  return ret;
}

static sl_status_t _app_settings_get_mac_address(char *value_str,
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

#endif

#if defined(SL_CATALOG_WISUN_LFN_DEVICE_SUPPORT_PRESENT)
static sl_status_t _app_get_device_type(char *value_str,
                                        const char *key_str,
                                        const app_cli_entry_t *entry)
{
  const char *dev_type_str = NULL;
  sl_wisun_device_type_t dev_type = SL_WISUN_ROUTER;
  (void) entry;
  (void) key_str;

  if (value_str == NULL) {
    return SL_STATUS_FAIL;
  }

  dev_type =  sl_wisun_app_core_get_device_type();
  dev_type_str = app_wisun_trace_util_device_type_to_str((uint32_t) dev_type);

  if (dev_type_str == NULL) {
    return SL_STATUS_FAIL;
  }

  (void) snprintf(value_str, APP_CLI_STR_VALUE_LENGTH, "%s (%lu)", dev_type_str, (uint32_t) dev_type);
  return SL_STATUS_OK;
}

static sl_status_t _app_get_lfn_profile(char *value_str,
                                        const char *key_str,
                                        const app_cli_entry_t *entry)
{
  const char *lfn_profile_str = NULL;
  sl_wisun_lfn_profile_t lfn_profile = SL_WISUN_LFN_PROFILE_TEST;
  (void) entry;
  (void) key_str;

  if (value_str == NULL) {
    return SL_STATUS_FAIL;
  }

  lfn_profile =  sl_wisun_app_core_get_lfn_profile();
  lfn_profile_str = app_wisun_trace_util_lfn_profile_to_str((uint32_t) lfn_profile);

  if (lfn_profile_str == NULL) {
    return SL_STATUS_FAIL;
  }

  (void) snprintf(value_str, APP_CLI_STR_VALUE_LENGTH, "%s (%lu)", lfn_profile_str, (uint32_t) lfn_profile);
  return SL_STATUS_OK;
}
#endif

static void _cli_handler_iterator(const sl_cli_command_arg_t *args,
                                  const size_t size,
                                  const sl_wisun_cli_handler_property_t *props)
{
  for (size_t i = 0U; i < size - 1U; ++i) {
    if (((sl_strcasecmp(args->argv[0], props[i].cli_group) == 0) || (sl_strcasecmp(args->argv[0], props[i].cli_group_shortcut) == 0))
        && ((sl_strcasecmp(args->argv[1], props[i].cli_command) == 0) || (sl_strcasecmp(args->argv[1], props[i].cli_command_shortcut) == 0))
        && (props[i].cli_handler_fnc != NULL)) {
      props[i].cli_handler_fnc(args);
      return;
    }
  }
}

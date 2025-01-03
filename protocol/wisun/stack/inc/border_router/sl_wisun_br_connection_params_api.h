/*
 * Copyright (c) 2023 Silicon Laboratories Inc. (www.silabs.com)
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this software
 * is governed by the terms of the Silicon Labs Master Software License Agreement (MSLA)
 * available at www.silabs.com/about-us/legal/master-software-license-agreement.
 * This software is distributed to you in Object Code format and/or Source Code format and
 * is governed by the sections of the MSLA applicable to Object Code, Source Code and
 * Modified Open Source Code. By using this software, you agree to the terms of the MSLA.
 *
 * This software is a modified version of the ARM/Pelion Wi-SUN FAN software stack which is
 * licensed under Apache 2.0 (see below). Modifications to the ARM/Pelion Wi-SUN software stack
 * within this software are subject to the above copyright notice and licensed pursuant to the MSLA.
 *
 * The original ARM/Pelion Wi-SUN FAN software stack is subject to the following copyright notice.
 *
 * Copyright (c) 2014-2018, Pelion and affiliates.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef SL_WISUN_BR_CONNECTION_PARAMS_API_H
#define SL_WISUN_BR_CONNECTION_PARAMS_API_H

#include <stdint.h>
#include "sl_status.h"
#include "sl_wisun_connection_params_api.h"

/// API version used to check compatibility (do not edit when using this header)
#define SL_WISUN_BR_PARAMS_API_VERSION 4

/**************************************************************************//**
 * @addtogroup SL_WISUN_TYPES
 * @{
 *****************************************************************************/

/// PAN discovery parameter set
SL_PACK_START(1)
typedef struct {
  /// PAN advertisement trickle timer
  sl_wisun_trickle_params_t trickle_pa;
} SL_ATTRIBUTE_PACKED sl_wisun_br_params_discovery;
SL_PACK_END()

/// PAN configuration parameter set
SL_PACK_START(1)
typedef struct {
  /// PAN configuration trickle timer
  sl_wisun_trickle_params_t trickle_pc;
} SL_ATTRIBUTE_PACKED sl_wisun_br_params_configuration;
SL_PACK_END()

/// Authentication parameter set
SL_PACK_START(1)
typedef struct {
  /// Security key lifetimes
  sl_wisun_key_lifetimes_params_t key_lifetimes;
  /// Security protocol trickle timer
  sl_wisun_trickle_params_t sec_prot_trickle; // k is ignored/forced
  /// Temporary neighbor link minimum timeout (seconds)
  uint16_t temp_min_timeout_s;
  /// Security protocol trickle max expirations
  uint8_t sec_prot_trickle_expirations;
  /// Reserved, set to zero
  uint8_t reserved;
} SL_ATTRIBUTE_PACKED sl_wisun_br_params_eapol;
SL_PACK_END()

/// DHCP server parameter set
SL_PACK_START(1)
typedef struct {
  /// Lease time (seconds)
  uint32_t dhcp_address_lifetime_s;
} SL_ATTRIBUTE_PACKED sl_wisun_br_params_dhcp;
SL_PACK_END()


/// RPL parameter set
SL_PACK_START(1)
typedef struct {
  /// RPL MinHopRankIncrease RFC6550 3.5.1
  uint16_t rpl_min_hop_rank_increase;
  /// RPL allowable increase in rank in support of local repair RFC 6550 6.7.6
  uint16_t rpl_dag_max_rank_increase;
  /// RPL default lifetime unit RFC6550 6.7.6
  uint16_t rpl_default_lifetime_unit;
  /// RPL trickle Imin equals (2^rpl_dio_interval_min) ms
  uint8_t rpl_dio_interval_min;
  /// RPL trickle Imax (DIOIntervalDoublings)
  uint8_t rpl_dio_interval_doublings;
  /// RPL trickle k DIORedundancyConstant
  uint8_t rpl_dio_redundancy_constant;
  /// RPL default lifetime for routes (lifetime units) RFC6550 6.7.6
  uint8_t rpl_default_lifetime;
  /// Reserved, set to zero
  uint8_t reserved[2];
} SL_ATTRIBUTE_PACKED sl_wisun_br_params_rpl;
SL_PACK_END()

/// MPL parameter set
SL_PACK_START(1)
typedef struct {
  /// Warning: MPL parameters may be overwritten for border router
  /// MPL trickle timer
  sl_wisun_trickle_params_t trickle;
  /// MPL seed set entry lifetime
  uint16_t seed_set_entry_lifetime_s;
  /// MPL trickle timer expirations
  uint8_t trickle_expirations;
  /// Reserved, set to zero
  uint8_t reserved;
} SL_ATTRIBUTE_PACKED sl_wisun_br_params_mpl;
SL_PACK_END()

/// LFN parent parameter set
SL_PACK_START(1)
typedef struct {
  /// Number of broadcast LFN Pan Config retries when LFN Version is incremented
  /// Referred to as LFN_MAINTAIN_PARENT_TIME in FAN TPS 1.1
  uint8_t lfn_lpc_retry_count;
  /// Reserved, set to zero
  uint8_t reserved[3];
} SL_ATTRIBUTE_PACKED sl_wisun_br_params_lfn_parent;
SL_PACK_END()

/// Misc parameter set
SL_PACK_START(1)
typedef struct {
  /// Temporary neighbor link minimum timeout
  uint16_t temp_link_min_timeout_s;
  /// Reserved, set to zero
  uint16_t reserved;
} SL_ATTRIBUTE_PACKED sl_wisun_br_params_misc;
SL_PACK_END()

/// BR parameter set
SL_PACK_START(1)
typedef struct {
  /**
   * Version of this API.
   *
   * This field allows to store the parameters in an NVM and check on reload
   * that they are compatible with the stack if there was an update.
   */
  uint32_t version;
  /// PAN discovery parameter set
  sl_wisun_br_params_discovery discovery;
  /// PAN configuration parameter set
  sl_wisun_br_params_configuration configuration;
  /// Authentication parameter set
  sl_wisun_br_params_eapol eapol;
  /// DHCP server parameter set
  sl_wisun_br_params_dhcp dhcp;
  /// RPL parameter set
  sl_wisun_br_params_rpl rpl;
  /// MPL parameter set
  sl_wisun_br_params_mpl mpl;
  /// LFN parent parameter set
  sl_wisun_br_params_lfn_parent lfn_parent;
  /// Misc parameter set
  sl_wisun_br_params_misc misc;
  /// Maximum number of nodes supported by border router at once
  uint32_t pan_capacity;
  /// Enable FAN 1.0 router node support PAN-wide
  uint8_t enable_ffn10;
} SL_ATTRIBUTE_PACKED sl_wisun_br_connection_params_t;
SL_PACK_END()

/***************************************************************************//**
 * @addtogroup SL_WISUN_BR_PARAMETER_SETS Predefined BR parameter sets
 *
 * Predefined BR parameter sets for sl_wisun_br_set_connection_parameters().
 *
 * These parameter sets can be used either as-is to replicate the behavior of
 * sl_wisun_br_set_network_size(), or used as an initialization value for an
 * application-specific parameter set.
 *
 * @{
 ******************************************************************************/

/// Profile for development (shorter connection time)
static const sl_wisun_br_connection_params_t SL_WISUN_BR_PARAMS_PROFILE_TEST = {
  .version = SL_WISUN_BR_PARAMS_API_VERSION,
  .discovery = {
    .trickle_pa = {
      .imin_s = 5,
      .imax_s = 60,
      .k = 0
    }
  },
  .configuration = {
    .trickle_pc = {
      .imin_s = 5,
      .imax_s = 60,
      .k = 0
    }
  },
  .eapol = {
    .key_lifetimes = {
      .pmk_lifetime_m = 172800,
      .lpmk_lifetime_m = 788400,
      .ptk_lifetime_m = 86400,
      .lptk_lifetime_m = 525600,
      .gtk_expire_offset_m = 43200,
      .lgtk_expire_offset_m = 129600,
      .gtk_new_activation_time = 720,
      .lgtk_new_activation_time = 180,
      .gtk_new_install_required = 80,
      .lgtk_new_install_required = 90,
      .ffn_revocation_lifetime_reduction = 30,
      .lfn_revocation_lifetime_reduction = 30,
    },
    .sec_prot_trickle = {
      .imin_s = 60,
      .imax_s = 120,
      .k = 0
    },
    .temp_min_timeout_s = 330,
    .sec_prot_trickle_expirations = 4,
  },
  .dhcp = {
    .dhcp_address_lifetime_s = HOUR_TO_SEC(12)
  },
  .rpl = {
    .rpl_min_hop_rank_increase = 128,
    .rpl_dag_max_rank_increase = 0,
    .rpl_default_lifetime_unit = 1200,
    .rpl_dio_interval_min = 11,
    .rpl_dio_interval_doublings = 6,
    .rpl_dio_redundancy_constant = 0,
    .rpl_default_lifetime = 6,
  },
  .mpl = {
    .trickle = {
      .imin_s = 6,
      .imax_s = 48,
      .k = 3,
    },
    .seed_set_entry_lifetime_s = 1800,
    .trickle_expirations = 3,
  },
  .lfn_parent = {
    .lfn_lpc_retry_count = 5,
  },
  .misc = {
    .temp_link_min_timeout_s = 260,
  },
  .pan_capacity = 100,
  .enable_ffn10 = false
};

/// Profile for a small network
static const sl_wisun_br_connection_params_t SL_WISUN_BR_PARAMS_PROFILE_SMALL = {
  .version = SL_WISUN_BR_PARAMS_API_VERSION,
  .discovery = {
    .trickle_pa = {
      .imin_s = 15,
      .imax_s = 60,
      .k = 0
    }
  },
  .configuration = {
    .trickle_pc = {
      .imin_s = 15,
      .imax_s = 60,
      .k = 0
    }
  },
  .eapol = {
    .key_lifetimes = {
      .pmk_lifetime_m = 172800,
      .lpmk_lifetime_m = 788400,
      .ptk_lifetime_m = 86400,
      .lptk_lifetime_m = 525600,
      .gtk_expire_offset_m = 43200,
      .lgtk_expire_offset_m = 129600,
      .gtk_new_activation_time = 720,
      .lgtk_new_activation_time = 180,
      .gtk_new_install_required = 80,
      .lgtk_new_install_required = 90,
      .ffn_revocation_lifetime_reduction = 30,
      .lfn_revocation_lifetime_reduction = 30,
    },
    .sec_prot_trickle = {
      .imin_s = 60,
      .imax_s = 120,
      .k = 0
    },
    .temp_min_timeout_s = 330,
    .sec_prot_trickle_expirations = 4,
  },
  .dhcp = {
    .dhcp_address_lifetime_s = HOUR_TO_SEC(12)
  },
  .rpl = {
    .rpl_min_hop_rank_increase = 128,
    .rpl_dag_max_rank_increase = 0,
    .rpl_default_lifetime_unit = 1200,
    .rpl_dio_interval_min = 15,
    .rpl_dio_interval_doublings = 2,
    .rpl_dio_redundancy_constant = 0,
    .rpl_default_lifetime = 6,
  },
  .mpl = {
    .trickle = {
      .imin_s = 1,
      .imax_s = 10,
      .k = 8
    },
    .seed_set_entry_lifetime_s = 180,
    .trickle_expirations = 2,
  },
  .lfn_parent = {
    .lfn_lpc_retry_count = 5,
  },
  .misc = {
    .temp_link_min_timeout_s = 260,
  },
  .pan_capacity = 100,
  .enable_ffn10 = false
};

/// Profile for a medium network
static const sl_wisun_br_connection_params_t SL_WISUN_BR_PARAMS_PROFILE_MEDIUM = {
  .version = SL_WISUN_BR_PARAMS_API_VERSION,
  .discovery = {
    .trickle_pa = {
      .imin_s = 60,
      .imax_s = 960,
      .k = 0
    }
  },
  .configuration = {
    .trickle_pc = {
      .imin_s = 60,
      .imax_s = 960,
      .k = 0
    }
  },
  .eapol = {
    .key_lifetimes = {
      .pmk_lifetime_m = 172800,
      .lpmk_lifetime_m = 788400,
      .ptk_lifetime_m = 86400,
      .lptk_lifetime_m = 525600,
      .gtk_expire_offset_m = 43200,
      .lgtk_expire_offset_m = 129600,
      .gtk_new_activation_time = 720,
      .lgtk_new_activation_time = 180,
      .gtk_new_install_required = 80,
      .lgtk_new_install_required = 90,
      .ffn_revocation_lifetime_reduction = 30,
      .lfn_revocation_lifetime_reduction = 30,
    },
    .sec_prot_trickle = {
      .imin_s = 60,
      .imax_s = 120,
      .k = 0
    },
    .temp_min_timeout_s = 330,
    .sec_prot_trickle_expirations = 4,
  },
  .dhcp = {
    .dhcp_address_lifetime_s = DAY_TO_SEC(7)
  },
  .rpl = {
    .rpl_min_hop_rank_increase = 128,
    .rpl_dag_max_rank_increase = 0,
    .rpl_default_lifetime_unit = 1200,
    .rpl_dio_interval_min = 17,
    .rpl_dio_interval_doublings = 3,
    .rpl_dio_redundancy_constant = 10,
    .rpl_default_lifetime = 12,
  },
  .mpl = {
    .trickle = {
      .imin_s = 1,
      .imax_s = 32,
      .k = 8
    },
    .seed_set_entry_lifetime_s = 576,
    .trickle_expirations = 2,
  },
  .lfn_parent = {
    .lfn_lpc_retry_count = 20,
  },
  .misc = {
    .temp_link_min_timeout_s = 260,
  },
  .pan_capacity = 1000,
  .enable_ffn10 = false
};

/// Profile for a large network
static const sl_wisun_br_connection_params_t SL_WISUN_BR_PARAMS_PROFILE_LARGE = {
  .version = SL_WISUN_BR_PARAMS_API_VERSION,
  .discovery = {
    .trickle_pa = {
      .imin_s = 120,
      .imax_s = 1536,
      .k = 0
    }
  },
  .configuration = {
    .trickle_pc = {
      .imin_s = 120,
      .imax_s = 1536,
      .k = 0
    }
  },
  .eapol = {
    .key_lifetimes = {
      .pmk_lifetime_m = 172800,
      .lpmk_lifetime_m = 788400,
      .ptk_lifetime_m = 86400,
      .lptk_lifetime_m = 525600,
      .gtk_expire_offset_m = 43200,
      .lgtk_expire_offset_m = 129600,
      .gtk_new_activation_time = 720,
      .lgtk_new_activation_time = 180,
      .gtk_new_install_required = 80,
      .lgtk_new_install_required = 90,
      .ffn_revocation_lifetime_reduction = 30,
      .lfn_revocation_lifetime_reduction = 30,
    },
    .sec_prot_trickle = {
      .imin_s = 60,
      .imax_s = 240,
      .k = 0
    },
    .temp_min_timeout_s = 750,
    .sec_prot_trickle_expirations = 4,
  },
  .dhcp = {
    .dhcp_address_lifetime_s = DAY_TO_SEC(30)
  },
  .rpl = {
    .rpl_min_hop_rank_increase = 128,
    .rpl_dag_max_rank_increase = 0,
    .rpl_default_lifetime_unit = 1200,
    .rpl_dio_interval_min = 18,
    .rpl_dio_interval_doublings = 3,
    .rpl_dio_redundancy_constant = 10,
    .rpl_default_lifetime = 24,
  },
  .mpl = {
    .trickle = {
      .imin_s = 5,
      .imax_s = 40,
      .k = 8
    },
    .seed_set_entry_lifetime_s = 720,
    .trickle_expirations = 2,
  },
  .lfn_parent = {
    .lfn_lpc_retry_count = 60,
  },
  .misc = {
    .temp_link_min_timeout_s = 520,
  },
  .pan_capacity = 10000,
  .enable_ffn10 = false
};

/** @} */

/** @} (end SL_WISUN_TYPES) */

#endif

/***************************************************************************//**
 * @file
 * @brief GP utilities common to SOC, Host, and NCP
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

#include PLATFORM_HEADER
#include "stack/include/sl_zigbee_types.h"
// I am commenting out this #include because we do release this source file,
// but we don't release this sl_zigbee_stack.h header file.
//#include "stack/core/sl_zigbee_stack.h"
#include "stack/include/gp-types.h"
//#include "gp.h"

bool sli_zigbee_gp_address_match(const sl_zigbee_gp_address_t *a1, const sl_zigbee_gp_address_t *a2)
{
  if (a1->applicationId == SL_ZIGBEE_GP_APPLICATION_SOURCE_ID
      && a2->applicationId == SL_ZIGBEE_GP_APPLICATION_SOURCE_ID) {
    if (a1->id.sourceId == a2->id.sourceId) {
      return true;
    }
  } else if (a1->applicationId == SL_ZIGBEE_GP_APPLICATION_IEEE_ADDRESS
             && a2->applicationId == SL_ZIGBEE_GP_APPLICATION_IEEE_ADDRESS) {
    if (!memcmp(a1->id.gpdIeeeAddress, a2->id.gpdIeeeAddress, EUI64_SIZE)) {
      if (a1->endpoint == a2->endpoint
          || a1->endpoint == 0xff
          || a2->endpoint == 0xff
          || a1->endpoint == 0x00
          || a2->endpoint == 0x00) { //TODO handle endpoints 0 and FF fully correctly
        return true;
      }
    }
  }
  return false;
}

sl_802154_short_addr_t sli_zigbee_gpd_alias(sl_zigbee_gp_address_t *addr)
{
  sl_802154_short_addr_t alias;
  //The alias algorithm is the same for both sourceID and IEEE GPDS
  //Take advantage of the fact that they're stored in a union
  //to reuse the same code

  alias = addr->id.sourceId & 0xFFFF;
  if (alias == 0x0000 || ((alias & 0xFFF8) == 0xFFF8)) {
    alias = alias ^ (addr->id.sourceId >> 16);
    if (alias == 0x0000 || ((alias & 0xFFF8) == 0xFFF8)) {
      if ((addr->id.sourceId & 0xFFFF) == 0x0000) {
        alias = 0x0007;
      } else {
        alias -= 0x0008;
      }
    }
  }
  return alias;
}

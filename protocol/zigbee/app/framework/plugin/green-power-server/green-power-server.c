/***************************************************************************//**
 * @file
 * @brief Routines for the Green Power Server plugin.
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
#ifdef SL_COMPONENT_CATALOG_PRESENT
#include "sl_component_catalog.h"
#endif

#ifdef SL_CATALOG_ZIGBEE_ZCL_FRAMEWORK_CORE_PRESENT
#include "app/framework/include/af.h"
#include "app/framework/util/af-main.h"
#include "app/framework/util/common.h"
#include "zap-cluster-command-parser.h"
#else // !SL_CATALOG_ZIGBEE_ZCL_FRAMEWORK_CORE_PRESENT
#include "green-power-adapter.h"
#endif //SL_CATALOG_ZIGBEE_ZCL_FRAMEWORK_CORE_PRESENT
#include "green-power-server-config.h"
#include "green-power-common.h"

//keys are loaded into core in this file to avoid
//passing them down to functions (design centered around
//Zigbee security manager's functionality).
//For host apps that don't support the security manager,
//declare sli_util_load_key_into_core so this file can still load keys.
#include "stack/include/zigbee-security-manager.h"

#ifdef SL_CATALOG_ZIGBEE_GREEN_POWER_TRANSLATION_TABLE_PRESENT
#include "green-power-translation-table.h"
#endif //SL_CATALOG_ZIGBEE_GREEN_POWER_TRANSLATION_TABLE_PRESENT

#include "stack/include/gp-types.h"
#include "stack/include/gp-proxy-table.h"
#include "stack/include/gp-sink-table.h"
#include "green-power-server.h"

#define GP_NON_MANUFACTURER_ZCL_HEADER_LENGTH           (3)
#ifndef SL_ZIGBEE_AF_PLUGIN_GREEN_POWER_SERVER_COMMISSIONING_GPD_INSTANCES
#define SL_ZIGBEE_AF_PLUGIN_GREEN_POWER_SERVER_COMMISSIONING_GPD_INSTANCES 1
#endif

#ifdef EZSP_HOST
#ifdef SL_ZIGBEE_GP_SINK_TABLE_SIZE
#warn "SL_ZIGBEE_GP_SINK_TABLE_SIZE must be configured on the NCP using the zigbee_gp component. The user-supplied SL_ZIGBEE_GP_SINK_TABLE_SIZE value is being ignored."
#endif // SL_ZIGBEE_GP_SINK_TABLE_SIZE
static uint8_t gpSinkTableSize = 0;
  #define SL_ZIGBEE_GP_SINK_TABLE_SIZE gpSinkTableSize
#endif

#if defined(SL_ZIGBEE_AF_NCP) && defined(SL_CATALOG_ZIGBEE_AF_SUPPORT_PRESENT)
#ifdef MAX_ENDPOINT_COUNT
#undef MAX_ENDPOINT_COUNT
// MAX_ENDPOINT_COUNT is defined as ZCL_FIXED_ENDPOINT_COUNT on Host and Soc,
// Since GP plugins can be supported on NCP then there is no way we can know
// what is the ZCL_FIXED_ENDPOINT_COUNT configured on HOST to pre-allocate static array on NCP.
// Therefore we are assuming the MAX_ENDPOINT_COUNT should be fixed value
#define MAX_ENDPOINT_COUNT 10
#endif // MAX_ENDPOINT_COUNT

// Based the AF NCP support the endpoint count and the index endpoint number
// is retrieved on NCP.
#define sl_zigbee_get_endpoint_count() sl_zigbee_get_endpoint_count()
#define sl_zigbee_get_endpoint(i)     sl_zigbee_get_endpoint(i)
#else // defined(SL_ZIGBEE_AF_NCP) && defined(SL_CATALOG_ZIGBEE_AF_SUPPORT_PRESENT)

#define sl_zigbee_get_endpoint_count() (ZCL_FIXED_ENDPOINT_COUNT)
#define sl_zigbee_get_endpoint(i)     (sli_zigbee_af_endpoints[i].endpoint)
#endif // defined(SL_ZIGBEE_AF_NCP) && defined(SL_CATALOG_ZIGBEE_AF_SUPPORT_PRESENT)

typedef struct {
  uint8_t gpdCommand;
  sli_zigbee_zigbee_cluster_t cluster;
  uint8_t endpoints[MAX_ENDPOINT_COUNT];
  uint8_t numberOfEndpoints;
} sli_zigbee_supported_gpd_command_cluster_endpoint_map_t;

sl_zigbee_af_status_t defaultResponseFailureStatus;
sl_zigbee_af_event_t sl_zigbee_af_green_power_server_generic_switch_commissioning_timeout_event;
void sl_zigbee_af_green_power_server_generic_switch_commissioning_timeout_event_handler(sl_zigbee_af_event_t * event);
sl_zigbee_af_event_t sl_zigbee_af_green_power_server_multi_sensor_commissioning_timeout_event;
void sl_zigbee_af_green_power_server_multi_sensor_commissioning_timeout_event_handler(sl_zigbee_af_event_t * event);
sl_zigbee_af_event_t sl_zigbee_af_green_power_server_commissioning_window_timeout_event;
void sl_zigbee_af_green_power_server_commissioning_window_timeout_event_handler(sl_zigbee_af_event_t * event);
#define genericSwitchCommissioningTimeout (&sl_zigbee_af_green_power_server_generic_switch_commissioning_timeout_event)
#define multiSensorCommissioningTimeout (&sl_zigbee_af_green_power_server_multi_sensor_commissioning_timeout_event)
#define commissioningWindowTimeout (&sl_zigbee_af_green_power_server_commissioning_window_timeout_event)

static void import_load_zb_sec_man_internal_key(sl_zigbee_sec_man_key_t* key);

static sli_zigbee_gp_comm_data_saved_t gpdCommDataSaved[SL_ZIGBEE_AF_PLUGIN_GREEN_POWER_SERVER_COMMISSIONING_GPD_INSTANCES] = { 0 };
static uint8_t zeroLengthZclOctateString[] = { 0 };
static uint8_t commissionedEndPoints[MAX_ENDPOINT_COUNT] = { 0 };
static uint8_t noOfCommissionedEndpoints = 0;
static sl_zigbee_af_green_power_server_commissioning_state_t commissioningState = { 0 };
static bool greenPowerServerInitialised = false;

//shortcut wrapper for loading in the specified key data into the
//Zigbee security manager core.
static void import_load_zb_sec_man_internal_key(sl_zigbee_sec_man_key_t* key)
{
  sl_zigbee_sec_man_context_t context;
  sl_zigbee_sec_man_init_context(&context);
  context.core_key_type = SL_ZB_SEC_MAN_KEY_TYPE_INTERNAL;
  sl_zigbee_sec_man_import_key(&context, key);
  sl_zigbee_sec_man_load_key_context(&context);
}

static bool gpdAddrZero(sl_zigbee_gp_address_t *gpdAddr)
{
  sl_zigbee_gp_address_t gpdAddrZero = { 0 };
  gpdAddrZero.applicationId = gpdAddr->applicationId;
  gpdAddrZero.endpoint = gpdAddr->endpoint;
  return sl_zigbee_af_green_power_common_gp_addr_compare(&gpdAddrZero, gpdAddr);
}

static uint8_t gpNumberOfFreeSinkEntries(void)
{
  return (SL_ZIGBEE_GP_SINK_TABLE_SIZE - sl_zigbee_gp_sink_table_get_number_of_active_entries());
}

static uint8_t gpNumberOfTempAllocatedCommisioningInstances(void)
{
  uint8_t allocatedEntries = 0;
  for (int i = 0; i < SL_ZIGBEE_AF_PLUGIN_GREEN_POWER_SERVER_COMMISSIONING_GPD_INSTANCES; i++) {
    if (!gpdAddrZero(&gpdCommDataSaved[i].addr)) {
      allocatedEntries++;
    }
  }
  return allocatedEntries;
}

static sli_zigbee_gp_comm_data_saved_t *allocateCommissioningGpdInstance(sl_zigbee_gp_address_t *gpdAddr)
{
  // Check if there are any more sink tables availalable to allow participation of this new GPD
  if (!(gpNumberOfFreeSinkEntries() > gpNumberOfTempAllocatedCommisioningInstances())) {
    // No more allocation, because the current temporary allocations need an entry confirmed as one
    return NULL;
  }
  if (gpdAddr != NULL) {
    for (int i = 0; i < SL_ZIGBEE_AF_PLUGIN_GREEN_POWER_SERVER_COMMISSIONING_GPD_INSTANCES; i++) {
      if (gpdAddrZero(&gpdCommDataSaved[i].addr)) {
        // gpd address 0 means free
        return (&gpdCommDataSaved[i]);
      }
    }
  }
  // allocation failed
  return NULL;
}

static sli_zigbee_gp_comm_data_saved_t *findOrAllocateCommissioningGpdInstance(sl_zigbee_gp_address_t *gpdAddr)
{
  if (gpdAddr == NULL) {
    return NULL;
  }
  if (gpdAddrZero(gpdAddr)) {
    return NULL;
  }
  sli_zigbee_gp_comm_data_saved_t *gpd = sl_zigbee_af_green_power_server_find_commissioning_gpd_instance(gpdAddr);
  if (gpd != NULL) {
    return gpd;
  }
  // not found hence allocate one
  return allocateCommissioningGpdInstance(gpdAddr);
}

// Writes the TCInvolved bit - this should only be called on first joining of the node.
static void writeInvolveTCBit(void)
{
  // Proceed with processing the involveTC
  uint8_t gpsSecurityLevelAttribute = 0;
  uint8_t type;
  sl_zigbee_af_status_t secLevelStatus = sl_zigbee_af_read_attribute(SL_ZIGBEE_GP_ENDPOINT,
                                                                     ZCL_GREEN_POWER_CLUSTER_ID,
                                                                     ZCL_GP_SERVER_GPS_SECURITY_LEVEL_ATTRIBUTE_ID,
                                                                     (CLUSTER_MASK_SERVER),
                                                                     (uint8_t*)&gpsSecurityLevelAttribute,
                                                                     sizeof(uint8_t),
                                                                     &type);
  if (secLevelStatus != SL_ZIGBEE_ZCL_STATUS_SUCCESS) {
    return; //  No security Level attribute ? Don't proceed
  }
  sl_zigbee_af_green_power_cluster_println("");
  sl_zigbee_af_green_power_cluster_print("GPS in ");
  // Find the security state of the node to examine the network security.
  sl_zigbee_current_security_state_t securityState;
  // Distributed network - return with out any change to InvolveTc bit
  if (sl_zigbee_get_current_security_state(&securityState) == SL_STATUS_OK
      && (securityState.bitmask & SL_ZIGBEE_DISTRIBUTED_TRUST_CENTER_MODE)) {
    sl_zigbee_af_green_power_cluster_print("Distributed Network");
    // reset the InvolveTC bit
    gpsSecurityLevelAttribute &= ~(GREEN_POWER_SERVER_GPS_SECURITY_LEVEL_ATTRIBUTE_FIELD_INVOLVE_TC);
  } else {
    // If centralised - checkif default TC link key is used
    const sl_zigbee_key_data_t linkKey = { GP_DEFAULT_LINK_KEY };
    sl_zigbee_sec_man_context_t context;

    sl_zigbee_sec_man_init_context(&context);
    context.core_key_type = SL_ZB_SEC_MAN_KEY_TYPE_TC_LINK;

    sl_status_t keyReadStatus = sl_zigbee_sec_man_check_key_context(&context);
    if (keyReadStatus == SL_STATUS_OK
        && !sl_zigbee_sec_man_compare_key_to_value(&context, (sl_zigbee_sec_man_key_t*)&linkKey.contents)) {
      sl_zigbee_af_green_power_cluster_print("Centralised Network : Non Default TC Key used - Set InvoveTc bit");
      // Set the InvolveTC bit
      gpsSecurityLevelAttribute |= GREEN_POWER_SERVER_GPS_SECURITY_LEVEL_ATTRIBUTE_FIELD_INVOLVE_TC;
    }
  }
  sl_zigbee_af_green_power_cluster_println("");
  secLevelStatus = sl_zigbee_af_write_attribute(SL_ZIGBEE_GP_ENDPOINT,
                                                ZCL_GREEN_POWER_CLUSTER_ID,
                                                ZCL_GP_SERVER_GPS_SECURITY_LEVEL_ATTRIBUTE_ID,
                                                (CLUSTER_MASK_SERVER),
                                                (uint8_t*)&gpsSecurityLevelAttribute,
                                                type);
  sl_zigbee_af_green_power_cluster_println("Security Level writen = %d, Status = %d",
                                           gpsSecurityLevelAttribute,
                                           secLevelStatus);
}

// Update the GPS Node state in non volatile token
static void clearGpsStateInToken(void)
{
  #if (SL_ZIGBEE_AF_PLUGIN_GREEN_POWER_SERVER_USE_TOKENS == 1) && !defined(EZSP_HOST)
  sli_zigbee_gps_network_state_t gpsNodeState = GREEN_POWER_SERVER_GPS_NODE_STATE_NOT_IN_NETWORK;
  halCommonSetToken(TOKEN_GPS_NETWORK_STATE, &gpsNodeState);
  #endif
}

// Checks if the node is just joined checking from the tokens
static bool updateInvolveTCNeeded(void)
{
#if (SL_ZIGBEE_AF_PLUGIN_GREEN_POWER_SERVER_USE_TOKENS == 1) && !defined(EZSP_HOST)
  sli_zigbee_gps_network_state_t gpsNodeState;
  halCommonGetToken(&gpsNodeState, TOKEN_GPS_NETWORK_STATE);
  if (gpsNodeState == GREEN_POWER_SERVER_GPS_NODE_STATE_IN_NETWORK) {
    return false;
  } else {
    gpsNodeState = GREEN_POWER_SERVER_GPS_NODE_STATE_IN_NETWORK;
    halCommonSetToken(TOKEN_GPS_NETWORK_STATE, &gpsNodeState);
  }
  return true;
#else
  return false;
#endif
}

// Process the update based on statck state
static void updateInvolveTC(sl_status_t status)
{
  // Pre process network status to decide if a InvolveTC is needed
  if (status == SL_STATUS_NETWORK_DOWN
      && sl_zigbee_is_performing_rejoin() == FALSE) {
    clearGpsStateInToken();
    return;
  } else if (status == SL_STATUS_NETWORK_UP) {
    if (!updateInvolveTCNeeded()) {
      return;
    }
    writeInvolveTCBit();
  }
}

// Internal functions used to maintain the group table within the context
// of the binding table.
//
// In the binding:
// The first two bytes of the identifier is set to the groupId
// The local endpoint is set to the endpoint that is mapped to this group
static uint8_t findGroupInBindingTable(uint8_t endpoint, uint16_t groupId)
{
  for (uint8_t i = 0; i < SL_ZIGBEE_BINDING_TABLE_SIZE; i++) {
    sl_zigbee_binding_table_entry_t binding;
    if (sl_zigbee_get_binding(i, &binding) == SL_STATUS_OK
        && binding.type == SL_ZIGBEE_MULTICAST_BINDING
        && binding.identifier[0] == LOW_BYTE(groupId)
        && binding.identifier[1] == HIGH_BYTE(groupId)
        && binding.local == endpoint) {
      return i;
    }
  }
  return 0xFF;
}

static sl_zigbee_af_status_t removeFromApsGroup(uint8_t endpoint, uint16_t groupId)
{
  uint8_t index = findGroupInBindingTable(endpoint, groupId);
  if (index == 0xFF) {
    return SL_ZIGBEE_ZCL_STATUS_NOT_FOUND;
  }
  // search returned a valid index, so delete that
  sl_status_t status = sl_zigbee_delete_binding(index);
  return ((status == SL_STATUS_OK) ? SL_ZIGBEE_ZCL_STATUS_SUCCESS : SL_ZIGBEE_ZCL_STATUS_FAILURE);
}

static bool isValidAppEndpoint(uint8_t endpoint)
{
  if (endpoint == GREEN_POWER_SERVER_ALL_SINK_ENDPOINTS
      || endpoint == GREEN_POWER_SERVER_SINK_DERIVES_ENDPOINTS) {
    return true;
  }
#if defined(SL_ZIGBEE_AF_NCP) && defined(SL_CATALOG_ZIGBEE_AF_SUPPORT_PRESENT)
  sl_zigbee_endpoint_description_t endptDesc;
  // on NCP we don't want to add a new function to check for the exsitence of endpoint,
  // to reduce code size on NCP, so we reuse sl_zigbee_get_endpoint_description().
  return sl_zigbee_get_endpoint_description(endpoint, &endptDesc);
#else // !(defined(SL_ZIGBEE_AF_NCP) && defined(SL_CATALOG_ZIGBEE_AF_SUPPORT_PRESENT))
  for (uint8_t i = 0; i < ZCL_FIXED_ENDPOINT_COUNT; i++) {
    if (sli_zigbee_af_endpoints[i].endpoint == endpoint) {
      return true;
    }
  }
#endif // defined(SL_ZIGBEE_AF_NCP) && defined(SL_CATALOG_ZIGBEE_AF_SUPPORT_PRESENT)
  return false;
}

bool isCommissioningAppEndpoint(uint8_t endpoint)
{
  // Gp Controlable end points are all valid enabled endpoints
  if (endpoint >= GREEN_POWER_SERVER_MIN_VALID_APP_ENDPOINT
      && endpoint <= GREEN_POWER_SERVER_MAX_VALID_APP_ENDPOINT
      && (endpoint == commissioningState.endpoint
          || commissioningState.endpoint == GREEN_POWER_SERVER_ALL_SINK_ENDPOINTS      // All Endpoints - Sink derives
          || commissioningState.endpoint == GREEN_POWER_SERVER_SINK_DERIVES_ENDPOINTS  // Sink Derives the end points
          || commissioningState.endpoint == GREEN_POWER_SERVER_RESERVED_ENDPOINTS      // No Endpoint specified
          || commissioningState.endpoint == GREEN_POWER_SERVER_NO_PAIRED_ENDPOINTS)) { // No endpoint specified
    return true;
  }

  sl_zigbee_af_green_power_cluster_println("Endpoint %d Not in Commissioning Endpoint", endpoint);
  return false;
}

static uint8_t getGpCommissioningEndpoint(uint8_t *endpoints)
{
  uint8_t numberOfAppEndpoints = 0;
  if (commissioningState.endpoint == GREEN_POWER_SERVER_ALL_SINK_ENDPOINTS         // All Endpoints - Sink derives
      || commissioningState.endpoint == GREEN_POWER_SERVER_SINK_DERIVES_ENDPOINTS  // Sink Derives the end points
      || commissioningState.endpoint == GREEN_POWER_SERVER_RESERVED_ENDPOINTS      // No Endpoint specified
      || commissioningState.endpoint == GREEN_POWER_SERVER_NO_PAIRED_ENDPOINTS) {  // No endpoint specified
#if defined(SL_ZIGBEE_AF_NCP) && defined(SL_CATALOG_ZIGBEE_AF_SUPPORT_PRESENT)
    uint8_t endptCount = sl_zigbee_get_endpoint_count();
    for (uint8_t i = 0; (i < endptCount) && (i < MAX_ENDPOINT_COUNT); i++) {
      uint8_t endpt = sl_zigbee_get_endpoint(i);
      // Check only valid application endpoint ranges does not include GP EP
      if (endpt >= GREEN_POWER_SERVER_MIN_VALID_APP_ENDPOINT
          && endpt <= GREEN_POWER_SERVER_MAX_VALID_APP_ENDPOINT) {
        endpoints[numberOfAppEndpoints++] = endpt;
      }
    }
#else // !(defined(SL_ZIGBEE_AF_NCP) && defined(SL_CATALOG_ZIGBEE_AF_SUPPORT_PRESENT))
    for (uint8_t i = 0; i < ZCL_FIXED_ENDPOINT_COUNT; i++) {
      // Check only valid application endpoint ranges does not include GP EP
      if (sli_zigbee_af_endpoints[i].endpoint >= GREEN_POWER_SERVER_MIN_VALID_APP_ENDPOINT
          && sli_zigbee_af_endpoints[i].endpoint <= GREEN_POWER_SERVER_MAX_VALID_APP_ENDPOINT) {
        endpoints[numberOfAppEndpoints++] = sli_zigbee_af_endpoints[i].endpoint;
      }
    }
#endif //defined(SL_ZIGBEE_AF_NCP) && defined(SL_CATALOG_ZIGBEE_AF_SUPPORT_PRESENT)
  } else {
    // else just one endpoint in commissioning
    endpoints[0] = commissioningState.endpoint;
    numberOfAppEndpoints = 1;
  }
  return numberOfAppEndpoints;
}

static uint8_t getAllSinkEndpoints(uint8_t *endpoints)
{
  return getGpCommissioningEndpoint(endpoints);
}

static void resetOfMultisensorDataSaved(bool completeReset,
                                        sli_zigbee_gp_comm_data_saved_t *gpd)
{
  // The reset could be complete or partial - back up the switch information
  sl_zigbee_gp_switch_information_t tempSwitchInformationStruct = gpd->switchInformationStruct;
  if (completeReset) {
    // reset all data belonging to this Gpd
    memset(gpd, 0, sizeof(sli_zigbee_gp_comm_data_saved_t));
    // if the switch timer was active, just copy it back
    if (sl_zigbee_af_event_is_scheduled(genericSwitchCommissioningTimeout)) {
      gpd->switchInformationStruct = tempSwitchInformationStruct;
    }
    return;
  }
  // minimal reset (partial part):
  memset(gpd->reportsStorage, 0, SIZE_OF_REPORT_STORAGE);
  gpd->lastIndex = 0;
  gpd->numberOfReports = 0;
  gpd->totalNbOfReport = 0;
}

static bool sinkFunctionalitySupported(uint32_t mask)
{
  uint32_t gpsFuntionnalityAttribut = 0;
  uint32_t gpsActiveFunctionnalityAttribut = 0;
  sl_zigbee_af_attribute_type_t type;
  sl_zigbee_af_read_attribute(SL_ZIGBEE_GP_ENDPOINT,
                              ZCL_GREEN_POWER_CLUSTER_ID,
                              ZCL_GP_SERVER_GPS_FUNCTIONALITY_ATTRIBUTE_ID,
                              (CLUSTER_MASK_SERVER),
                              (uint8_t*)&gpsFuntionnalityAttribut,
                              sizeof(uint32_t),
                              &type);
  sl_zigbee_af_read_attribute(SL_ZIGBEE_GP_ENDPOINT,
                              ZCL_GREEN_POWER_CLUSTER_ID,
                              ZCL_GP_SERVER_GPS_ACTIVE_FUNCTIONALITY_ATTRIBUTE_ID,
                              (CLUSTER_MASK_SERVER),
                              (uint8_t*)&gpsActiveFunctionnalityAttribut,
                              sizeof(uint32_t),
                              &type);
  if (mask & (gpsFuntionnalityAttribut & gpsActiveFunctionnalityAttribut)) {
    return true;
  }
  return false;
}

static sl_status_t sendGpPairingMessage(sl_zigbee_outgoing_message_type_t type,
                                        uint16_t indexOrDestination,
                                        uint32_t options,
                                        sl_zigbee_gp_address_t *gpdAddr,
                                        uint16_t sinkGroupId,
                                        uint8_t deviceId,
                                        uint32_t gpdSecurityFrameCounter,
                                        uint8_t *gpdKey,
                                        uint16_t assignedAlias,
                                        uint8_t groupcastRadius,
                                        bool sendGpPairing)
{
  sl_status_t status = SL_STATUS_FAIL;
  sl_802154_long_addr_t ourEUI;
  sl_zigbee_af_get_eui64(ourEUI);
  if (sendGpPairing) {
    if (sl_zigbee_af_fill_command_green_power_cluster_gp_pairing_smart(options,
                                                                       gpdAddr->id.sourceId,
                                                                       gpdAddr->id.gpdIeeeAddress,
                                                                       gpdAddr->endpoint,
                                                                       ourEUI,
                                                                       sl_zigbee_get_node_id(),
                                                                       sinkGroupId,
                                                                       deviceId,
                                                                       gpdSecurityFrameCounter,
                                                                       gpdKey,
                                                                       assignedAlias,
                                                                       groupcastRadius) == 0) {
      return status;
    }

    sl_zigbee_aps_frame_t *apsFrame;
    apsFrame = sl_zigbee_af_get_command_aps_frame();
    apsFrame->sourceEndpoint = SL_ZIGBEE_GP_ENDPOINT;
    apsFrame->destinationEndpoint = SL_ZIGBEE_GP_ENDPOINT;

    if (type == SL_ZIGBEE_OUTGOING_BROADCAST) {
      status = sl_zigbee_af_send_command_broadcast(SL_ZIGBEE_RX_ON_WHEN_IDLE_BROADCAST_ADDRESS, SL_ZIGBEE_NULL_NODE_ID, 0);
    } else if (type == SL_ZIGBEE_OUTGOING_DIRECT) {
      status = sl_zigbee_af_send_command_unicast(SL_ZIGBEE_OUTGOING_DIRECT, indexOrDestination);
    }
  }
  #if (defined SL_CATALOG_ZIGBEE_GREEN_POWER_CLIENT_PRESENT && !defined EZSP_HOST)
  if (type == SL_ZIGBEE_OUTGOING_DIRECT
      && indexOrDestination != sl_zigbee_get_node_id()) {
    // Not a BCAST or Not addressed to this proxy
    return status;
  }
  sl_zigbee_gp_proxy_table_process_gp_pairing(options,
                                              gpdAddr,
                                              ((options & SL_ZIGBEE_AF_GP_PAIRING_OPTION_COMMUNICATION_MODE) \
                                               >> SL_ZIGBEE_AF_GP_PAIRING_OPTION_COMMUNICATION_MODE_OFFSET),
                                              sl_zigbee_get_node_id(),
                                              sinkGroupId,
                                              assignedAlias,
                                              ourEUI,
                                              (sl_zigbee_key_data_t *) gpdKey,
                                              gpdSecurityFrameCounter,
                                              groupcastRadius);

  #endif // SL_CATALOG_ZIGBEE_GREEN_POWER_CLIENT_PRESENT && !ESZP_HOST
  return status;
}

void sl_zigbee_af_green_power_server_remove_sink_entry(sl_zigbee_gp_address_t *gpdAddr)
{
  if (gpdAddr == NULL) {
    return;
  }
  bool removeAll = (gpdAddr->applicationId == SL_ZIGBEE_GP_APPLICATION_IEEE_ADDRESS
                    && gpdAddr->endpoint == 0xFF);
  uint8_t sinkEntryIndex;
  do {
    sinkEntryIndex = sl_zigbee_gp_sink_table_lookup(gpdAddr);
    if (sinkEntryIndex != 0xFF) {
      sl_zigbee_gp_sink_table_entry_t entry = { 0 };
      if (sl_zigbee_gp_sink_table_get_entry(sinkEntryIndex, &entry) != SL_STATUS_OK) {
        return;
      }
      // Delet the entry in TT
      #ifdef SL_CATALOG_ZIGBEE_GREEN_POWER_TRANSLATION_TABLE_PRESENT
      // Delete custom Translation table entry for this GPD
      (void) sli_zigbee_af_gp_trans_table_delete_paired_devicefrom_translation_table_entry(&(entry.gpd));
      #endif // SL_CATALOG_ZIGBEE_GREEN_POWER_TRANSLATION_TABLE_PRESENT
      if (sl_zigbee_af_green_power_server_sink_table_access_notification_cb(gpdAddr,
                                                                            GREEN_POWER_SERVER_SINK_TABLE_ACCESS_TYPE_REMOVE_GPD)) {
        sl_zigbee_gp_sink_table_remove_entry(sinkEntryIndex);
      }
    }
  } while ((sinkEntryIndex != 0xff) && (removeAll));
}

// decommission a gpd
static void decommissionGpd(uint8_t secLvl,
                            uint8_t keyType,
                            sl_zigbee_gp_address_t *gpdAddr,
                            bool setRemoveGpdflag,
                            bool sendGpPairing)
{
  uint8_t sinkEntryIndex = sl_zigbee_gp_sink_table_lookup(gpdAddr);
  if (sinkEntryIndex != 0xFF) {
    sl_zigbee_gp_sink_table_entry_t entry = { 0 };
    if (sl_zigbee_gp_sink_table_get_entry(sinkEntryIndex, &entry) != SL_STATUS_OK) {
      return;
    }

    uint8_t gpsCommunicationMode = (entry.options & SL_ZIGBEE_AF_GP_SINK_TABLE_ENTRY_OPTIONS_COMMUNICATION_MODE)
                                   >> SL_ZIGBEE_AF_GP_SINK_TABLE_ENTRY_OPTIONS_COMMUNICATION_MODE_OFFSET;
    uint16_t dGroupId = 0xFFFF;
    uint32_t pairingOptions = (gpdAddr->applicationId & SL_ZIGBEE_AF_GP_PAIRING_OPTION_APPLICATION_ID)
                              | (setRemoveGpdflag << SL_ZIGBEE_AF_GP_PAIRING_OPTION_REMOVE_GPD_OFFSET)
                              | (gpsCommunicationMode << SL_ZIGBEE_AF_GP_PAIRING_OPTION_COMMUNICATION_MODE_OFFSET)
                              | (secLvl << SL_ZIGBEE_AF_GP_PAIRING_OPTION_SECURITY_LEVEL_OFFSET)
                              | (keyType << SL_ZIGBEE_AF_GP_PAIRING_OPTION_SECURITY_KEY_TYPE_OFFSET);

    if (gpsCommunicationMode == SL_ZIGBEE_GP_SINK_TYPE_D_GROUPCAST) {
      dGroupId = sli_zigbee_gpd_alias(gpdAddr);
    }
    sl_status_t retval = sendGpPairingMessage(SL_ZIGBEE_OUTGOING_BROADCAST,
                                              SL_ZIGBEE_RX_ON_WHEN_IDLE_BROADCAST_ADDRESS,
                                              pairingOptions,
                                              gpdAddr,
                                              dGroupId,
                                              0xFF,
                                              0xFFFFFFFF,
                                              NULL,
                                              0xFFFF,
                                              0xFF,
                                              sendGpPairing);
    sl_zigbee_af_green_power_cluster_println("Gp Pairing for Decommissing send returned %d", retval);

    // In case of Sink was supporting groupcast for the GPD, then send a Gp Pairing config.
    if (gpsCommunicationMode == SL_ZIGBEE_GP_SINK_TYPE_GROUPCAST
        && (sinkFunctionalitySupported(SL_ZIGBEE_AF_GP_GPS_FUNCTIONALITY_SINK_TABLE_BASED_GROUPCAST_FORWARDING)
            || sinkFunctionalitySupported(SL_ZIGBEE_AF_GP_GPS_FUNCTIONALITY_PRE_COMMISSIONED_GROUPCAST_COMMUNICATION))) {
      if (sl_zigbee_af_fill_command_green_power_cluster_gp_pairing_configuration_smart(SL_ZIGBEE_ZCL_GP_PAIRING_CONFIGURATION_ACTION_REMOVE_GPD,
                                                                                       gpdAddr->applicationId,
                                                                                       gpdAddr->id.sourceId,
                                                                                       gpdAddr->id.gpdIeeeAddress,
                                                                                       gpdAddr->endpoint,
                                                                                       entry.deviceId,
                                                                                       0,
                                                                                       NULL,
                                                                                       0,
                                                                                       0,
                                                                                       0,
                                                                                       0,
                                                                                       NULL,
                                                                                       0xFE,
                                                                                       NULL,
                                                                                       0,
                                                                                       0,
                                                                                       0,
                                                                                       0,
                                                                                       NULL,
                                                                                       0,
                                                                                       NULL,
                                                                                       NULL,
                                                                                       0,
                                                                                       0,
                                                                                       0,
                                                                                       0,
                                                                                       0,
                                                                                       NULL) == 0) {
        return;
      }

      sl_zigbee_aps_frame_t *apsFrame = sl_zigbee_af_get_command_aps_frame();
      apsFrame->sourceEndpoint = SL_ZIGBEE_GP_ENDPOINT;
      apsFrame->destinationEndpoint = SL_ZIGBEE_GP_ENDPOINT;
      uint8_t retval = sl_zigbee_af_send_command_broadcast(SL_ZIGBEE_RX_ON_WHEN_IDLE_BROADCAST_ADDRESS, SL_ZIGBEE_NULL_NODE_ID, 0);
      sl_zigbee_af_green_power_cluster_println("Gp Pairing Config for Decommissing send returned %d", retval);
    }
    removeFromApsGroup(SL_ZIGBEE_GP_ENDPOINT, dGroupId);
    sl_zigbee_af_green_power_server_remove_sink_entry(gpdAddr);
  }
}

static void gppGpdLinkUnpack(uint8_t gppLink,
                             int8_t *rssi,
                             uint8_t *linkQuality)
{
  *rssi = (gppLink & SL_ZIGBEE_AF_GP_GPP_GPD_LINK_RSSI);
  // since gp 1.0b-14-
  // "The RSSI sub-field of the GPP-GPD link field encodes the RSSI from the range <+8 ; -109> [dBm],
  // with 2dBm granularity.", then
  // 0 represent -109dbm
  // 1 represent -107dbm
  // 2 represent -105dbm
  // ...
  // 54 represent -1dbm
  // 55 represent  0dbm // "110 is add to capped RSSI value, to obtain a non-negative value" gp1.0b 14-
  // 56 represent  2dbm
  // 57 represent  4dbm
  // 58 represent  6dbm
  // 59 represent  8dbm
  // 60 is value not used
  if ( *rssi < 55 ) {
    *rssi = -109 + 2 * (*rssi);
  } else {
    *rssi = ((*rssi) - 55) * 2;
  }
  *linkQuality = (gppLink & SL_ZIGBEE_AF_GP_GPP_GPD_LINK_LINK_QUALITY)
                 >> SL_ZIGBEE_AF_GP_GPP_GPD_LINK_LINK_QUALITY_OFFSET;
}

static void sendGpPairingConfigBasedOnSinkCommunicationMode(sli_zigbee_gp_comm_data_saved_t *commissioningGpd)
{
  sl_zigbee_af_green_power_cluster_println("Communication mode to send GpPairingConfig %d", commissioningGpd->communicationMode);
  if (commissioningGpd->communicationMode != SL_ZIGBEE_GP_SINK_TYPE_GROUPCAST) {
    return;
  }
  // Send the Gp Configuration if the sink supports
  // - sink table-based forwarding
  if (sinkFunctionalitySupported(SL_ZIGBEE_AF_GP_GPS_FUNCTIONALITY_SINK_TABLE_BASED_GROUPCAST_FORWARDING)
      || sinkFunctionalitySupported(SL_ZIGBEE_AF_GP_GPS_FUNCTIONALITY_PRE_COMMISSIONED_GROUPCAST_COMMUNICATION) ) {
    // Send GP Pairing Config with
    // SL_ZIGBEE_AF_GP_PAIRING_CONFIGURATION_OPTION_SECURITY_USE
    // | SL_ZIGBEE_AF_GP_PAIRING_CONFIGURATION_OPTION_SEQUENCE_NUMBER_CAPABILITIES
    // | (communication mode = GroupCast Forwarding (2))
    uint16_t pairigConfigOptions = GP_PAIRING_CONFIGURATION_FIXED_FLAG;
    pairigConfigOptions |= ((commissioningGpd->applicationInfo.applInfoBitmap)
                            ? SL_ZIGBEE_AF_GP_PAIRING_CONFIGURATION_OPTION_APPLICATION_INFORMATION_PRESENT : 0);
    uint8_t securityOptions = commissioningGpd->securityLevel
                              | (commissioningGpd->securityKeyType << SL_ZIGBEE_AF_GP_SINK_TABLE_ENTRY_SECURITY_OPTIONS_SECURITY_KEY_TYPE_OFFSET);

    if (sl_zigbee_af_fill_command_green_power_cluster_gp_pairing_configuration_smart(SL_ZIGBEE_ZCL_GP_PAIRING_CONFIGURATION_ACTION_EXTEND_SINK_TABLE_ENTRY,
                                                                                     pairigConfigOptions,
                                                                                     commissioningGpd->addr.id.sourceId,
                                                                                     commissioningGpd->addr.id.gpdIeeeAddress,
                                                                                     commissioningGpd->addr.endpoint,
                                                                                     commissioningGpd->applicationInfo.deviceId,
                                                                                     commissioningGpd->groupList[0],
                                                                                     &(commissioningGpd->groupList[1]),
                                                                                     0,
                                                                                     0,
                                                                                     securityOptions,
                                                                                     commissioningGpd->outgoingFrameCounter,
                                                                                     commissioningGpd->key.contents,
                                                                                     0xFE,
                                                                                     NULL,
                                                                                     commissioningGpd->applicationInfo.applInfoBitmap,
                                                                                     0,
                                                                                     0,
                                                                                     0,
                                                                                     NULL,
                                                                                     0,
                                                                                     NULL,
                                                                                     NULL,
                                                                                     commissioningGpd->switchInformationStruct.switchInfoLength,
                                                                                     (commissioningGpd->switchInformationStruct.nbOfContacts  \
                                                                                      + (commissioningGpd->switchInformationStruct.switchType \
                                                                                         << SL_ZIGBEE_AF_GP_APPLICATION_INFORMATION_SWITCH_INFORMATION_CONFIGURATION_SWITCH_TYPE_OFFSET)),
                                                                                     commissioningGpd->switchInformationStruct.currentContact,
                                                                                     0,
                                                                                     0,
                                                                                     NULL) == 0) {
      return;
    }

    sl_zigbee_aps_frame_t *apsFrame;
    apsFrame = sl_zigbee_af_get_command_aps_frame();
    apsFrame->sourceEndpoint = SL_ZIGBEE_GP_ENDPOINT;
    apsFrame->destinationEndpoint = SL_ZIGBEE_GP_ENDPOINT;

    uint8_t retval = sl_zigbee_af_send_command_broadcast(SL_ZIGBEE_RX_ON_WHEN_IDLE_BROADCAST_ADDRESS, SL_ZIGBEE_NULL_NODE_ID, 0);
    sl_zigbee_af_green_power_cluster_println("Gp Pairing Config Extend Sink send returned %d", retval);
    if (commissioningGpd->applicationInfo.applInfoBitmap
        & SL_ZIGBEE_AF_GP_APPLICATION_INFORMATION_APPLICATION_DESCRIPTION_PRESENT) {
      if (sl_zigbee_af_fill_command_green_power_cluster_gp_pairing_configuration_smart(SL_ZIGBEE_ZCL_GP_PAIRING_CONFIGURATION_ACTION_APPLICATION_DESCRIPTION,
                                                                                       0,
                                                                                       commissioningGpd->addr.id.sourceId,
                                                                                       commissioningGpd->addr.id.gpdIeeeAddress,
                                                                                       commissioningGpd->addr.endpoint,
                                                                                       commissioningGpd->applicationInfo.deviceId,
                                                                                       0,
                                                                                       NULL,
                                                                                       0,
                                                                                       0,
                                                                                       0,
                                                                                       0,
                                                                                       NULL,
                                                                                       0xFE,
                                                                                       NULL,
                                                                                       0,
                                                                                       0,
                                                                                       0,
                                                                                       0,
                                                                                       NULL,
                                                                                       0,
                                                                                       NULL,
                                                                                       NULL,
                                                                                       0,
                                                                                       0,
                                                                                       0,
                                                                                       commissioningGpd->totalNbOfReport,
                                                                                       commissioningGpd->numberOfReports,
                                                                                       commissioningGpd->reportsStorage) == 0) {
        return;
      }

      apsFrame = sl_zigbee_af_get_command_aps_frame();
      apsFrame->sourceEndpoint = SL_ZIGBEE_GP_ENDPOINT;
      apsFrame->destinationEndpoint = SL_ZIGBEE_GP_ENDPOINT;
      retval = sl_zigbee_af_send_command_broadcast(SL_ZIGBEE_RX_ON_WHEN_IDLE_BROADCAST_ADDRESS, SL_ZIGBEE_NULL_NODE_ID, 0);
      sl_zigbee_af_green_power_cluster_println("Gp Pairing Config App Description send returned %d", retval);
    }
  }
}

static uint8_t reportLength(const uint8_t *inReport)
{
  uint8_t length = 0; // 0 - position of reportId
  length += 1;  // 1 - position of reportOption
  if (inReport[length]
      & SL_ZIGBEE_AF_GP_GPD_APPLICATION_DESCRIPTION_COMMAND_REPORT_OPTIONS_TIMEOUT_PERIOD_PRESENT) {
    length += 3; // 2,3 - position of timeout and 4 position of length
  } else {
    length += 1; // 2 position of length excluding itself
  }
  length += inReport[length]; // sum up length of report descriptor remaining length
  length += 1; // add the length byte itself
  return length;
}

static bool saveReportDescriptor(uint8_t totalNumberOfReports,
                                 uint8_t numberOfReports,
                                 uint8_t *reports,
                                 sli_zigbee_gp_comm_data_saved_t *commissioningGpd)
{
  if ((totalNumberOfReports == 0)
      || ((commissioningGpd->numberOfReports == totalNumberOfReports)
          && (commissioningGpd->numberOfReports == commissioningGpd->totalNbOfReport))) {
    return true; // the report was already collected
  }
  uint8_t *temp = reports;
  uint8_t length = 0; // 0 - position of reportId
  for (uint8_t noOfReport = 0; noOfReport < numberOfReports; noOfReport++) {
    length = reportLength(temp);
    // Negative test - drop the packet if the record is of bad length - 4.4.1.12
    // Minimum length for 1 report that has 1 option record in that 1 attribute record =
    // reportId(1) + option(1) + length(1) of option records
    // Option Record = option (1) + clusterid (2)
    // Attribute record = attrId (2) + datatype (1) + att option(1)
    if (length < GREEN_POWER_SERVER_MIN_REPORT_LENGTH ) {
      // Error, so return without saving
      return false;
    }
    // The report is not already collected
    if (NULL ==  sli_zigbee_af_gp_find_report_id(*temp,
                                                 commissioningGpd->numberOfReports,
                                                 commissioningGpd->reportsStorage)) {
      // now save the entire payload, appending to last received reports if any
      memcpy(&(commissioningGpd->reportsStorage[commissioningGpd->lastIndex]),
             temp,
             length);
      commissioningGpd->numberOfReports++;
      commissioningGpd->lastIndex += length;
      commissioningGpd->totalNbOfReport = totalNumberOfReports;
    } else {
      sl_zigbee_af_green_power_cluster_println("Duplicate ReportId %d - Not Saved", *temp);
    }
    temp = temp + length;
  }
  return true;
}

static const uint8_t *findDataPointDescriptorInReport(uint8_t dataPointDescriptorId,
                                                      const uint8_t *report)
{
  uint8_t remainingLength;
  const uint8_t *finger;
  if (report[1]) {
    remainingLength = report[4]; // with timeout
    finger = &report[5];
  } else {
    remainingLength = report[2]; // without timeout
    finger = &report[3];
  }
  uint8_t tempDatapointDescrId = 0;
  for (int index = 0; index < remainingLength - 1; index++) {
    // travel through each data point descriptor from here until the input is
    // found
    if (tempDatapointDescrId == dataPointDescriptorId) {
      return &finger[index];
    }
    uint8_t datapointOptions = finger[index];
    index += 3 + ((datapointOptions & 0x10) ? 2 : 0); //for ManufactureId
    // traverse the attribute records in each data point
    for (int i = 0; i < ((datapointOptions & 0x07) + 1); i++) {
      index += 3;
      uint8_t attributeOptions = finger[index];
      index += ((attributeOptions & 0x0f) + 1);
    }
    tempDatapointDescrId++;
  }
  return NULL;
}

//function return true if the param_in communication mode is supported by the sink
static bool sli_zigbee_af_gp_check_communication_mode_support(uint8_t communicationModeToCheck)
{
  uint32_t gpsFuntionnalityAttribut = 0;
  uint32_t gpsActiveFunctionnalityAttribut = 0;
  sl_zigbee_af_attribute_type_t type;
  sl_zigbee_af_read_attribute(SL_ZIGBEE_GP_ENDPOINT,
                              ZCL_GREEN_POWER_CLUSTER_ID,
                              ZCL_GP_SERVER_GPS_FUNCTIONALITY_ATTRIBUTE_ID,
                              CLUSTER_MASK_SERVER,
                              (uint8_t*)&gpsFuntionnalityAttribut,
                              sizeof(uint32_t),
                              &type);
  sl_zigbee_af_read_attribute(SL_ZIGBEE_GP_ENDPOINT,
                              ZCL_GREEN_POWER_CLUSTER_ID,
                              ZCL_GP_SERVER_GPS_ACTIVE_FUNCTIONALITY_ATTRIBUTE_ID,
                              (CLUSTER_MASK_SERVER),
                              (uint8_t*)&gpsActiveFunctionnalityAttribut,
                              sizeof(uint32_t),
                              &type);
  uint32_t currentFunctionnalities = (gpsFuntionnalityAttribut & gpsActiveFunctionnalityAttribut);
  if ((communicationModeToCheck == SL_ZIGBEE_GP_SINK_TYPE_FULL_UNICAST
       && (currentFunctionnalities & SL_ZIGBEE_AF_GP_GPS_FUNCTIONALITY_FULL_UNICAST_COMMUNICATION))
      || (communicationModeToCheck == SL_ZIGBEE_GP_SINK_TYPE_D_GROUPCAST
          && (currentFunctionnalities & SL_ZIGBEE_AF_GP_GPS_FUNCTIONALITY_DERIVED_GROUPCAST_COMMUNICATION))
      || (communicationModeToCheck == SL_ZIGBEE_GP_SINK_TYPE_GROUPCAST
          && (currentFunctionnalities & SL_ZIGBEE_AF_GP_GPS_FUNCTIONALITY_PRE_COMMISSIONED_GROUPCAST_COMMUNICATION))
      || (communicationModeToCheck == SL_ZIGBEE_GP_SINK_TYPE_LW_UNICAST
          && (currentFunctionnalities & SL_ZIGBEE_AF_GP_GPS_FUNCTIONALITY_LIGHTWEIGHT_UNICAST_COMMUNICATION))) {
    return true;
  }
  return false;
}

static void sendGpPairingFromSinkEntry(sl_zigbee_gp_sink_table_entry_t *sinkEntry,
                                       uint16_t gppShortAddress,
                                       bool sendGpPairing)
{
  uint8_t appId = (sinkEntry->options & SL_ZIGBEE_AF_GP_SINK_TABLE_ENTRY_OPTIONS_APPLICATION_ID);
  uint8_t sinkCommunicationMode = (sinkEntry->options & SL_ZIGBEE_AF_GP_SINK_TABLE_ENTRY_OPTIONS_COMMUNICATION_MODE)
                                  >> SL_ZIGBEE_AF_GP_SINK_TABLE_ENTRY_OPTIONS_COMMUNICATION_MODE_OFFSET;
  bool gpdFixed = (sinkEntry->options & SL_ZIGBEE_AF_GP_SINK_TABLE_ENTRY_OPTIONS_FIXED_LOCATION)
                  ? true : false;
  bool gpdMacCapabilities = (sinkEntry->options & SL_ZIGBEE_AF_GP_SINK_TABLE_ENTRY_OPTIONS_SEQUENCE_NUM_CAPABILITIES)
                            ? true : false;
  bool sinkTableAssignedAliasNeeded = (sinkEntry->options & SL_ZIGBEE_AF_GP_SINK_TABLE_ENTRY_OPTIONS_ASSIGNED_ALIAS)
                                      ? true : false;
  uint8_t securityLevel = (sinkEntry->securityOptions & SL_ZIGBEE_AF_GP_SINK_TABLE_ENTRY_SECURITY_OPTIONS_SECURITY_LEVEL);
  uint8_t securityKeyType = ((sinkEntry->securityOptions & SL_ZIGBEE_AF_GP_SINK_TABLE_ENTRY_SECURITY_OPTIONS_SECURITY_KEY_TYPE)
                             >> SL_ZIGBEE_AF_GP_SINK_TABLE_ENTRY_SECURITY_OPTIONS_SECURITY_KEY_TYPE_OFFSET);
  uint32_t pairingOptions = 0;
  pairingOptions = SL_ZIGBEE_AF_GP_PAIRING_OPTION_ADD_SINK
                   | (appId & SL_ZIGBEE_AF_GP_PAIRING_OPTION_APPLICATION_ID)
                   | (sinkCommunicationMode << SL_ZIGBEE_AF_GP_PAIRING_OPTION_COMMUNICATION_MODE_OFFSET)
                   | (gpdFixed << SL_ZIGBEE_AF_GP_PAIRING_OPTION_GPD_FIXED_OFFSET)
                   | (gpdMacCapabilities << SL_ZIGBEE_AF_GP_PAIRING_OPTION_GPD_MAC_SEQUENCE_NUMBER_CAPABILITIES_OFFSET)
                   | (securityLevel << SL_ZIGBEE_AF_GP_PAIRING_OPTION_SECURITY_LEVEL_OFFSET)
                   | (securityKeyType << SL_ZIGBEE_AF_GP_PAIRING_OPTION_SECURITY_KEY_TYPE_OFFSET)
                   | ((gpdMacCapabilities || securityLevel) ? SL_ZIGBEE_AF_GP_PAIRING_OPTION_GPD_SECURITY_FRAME_COUNTER_PRESENT : 0)
                   | ((securityLevel && securityKeyType) ? SL_ZIGBEE_AF_GP_PAIRING_OPTION_GPD_SECURITY_KEY_PRESENT : 0)
                   | (sinkTableAssignedAliasNeeded << SL_ZIGBEE_AF_GP_PAIRING_OPTION_ASSIGNED_ALIAS_PRESENT_OFFSET)
                   | ((sinkCommunicationMode == SL_ZIGBEE_GP_SINK_TYPE_D_GROUPCAST
                       || sinkCommunicationMode == SL_ZIGBEE_GP_SINK_TYPE_GROUPCAST)
                      ? SL_ZIGBEE_AF_GP_PAIRING_OPTION_GROUPCAST_RADIUS_PRESENT : 0);
  uint16_t groupId = 0xFFFF;
  sl_zigbee_outgoing_message_type_t type = SL_ZIGBEE_OUTGOING_BROADCAST;
  uint16_t indexOrDestination = SL_ZIGBEE_RX_ON_WHEN_IDLE_BROADCAST_ADDRESS;
  if (sinkCommunicationMode == SL_ZIGBEE_GP_SINK_TYPE_D_GROUPCAST) {
    groupId = sli_zigbee_gpd_alias(&(sinkEntry->gpd));
  } else if (sinkCommunicationMode == SL_ZIGBEE_GP_SINK_TYPE_GROUPCAST) {
    groupId = sinkEntry->sinkList[0].target.groupcast.groupID;
  } else if (sinkCommunicationMode == SL_ZIGBEE_GP_SINK_TYPE_LW_UNICAST
             && commissioningState.sendGpPairingInUnicastMode) {
    type = SL_ZIGBEE_OUTGOING_DIRECT;
    indexOrDestination = gppShortAddress;
  }
  sl_status_t retval = sendGpPairingMessage(type,
                                            indexOrDestination,
                                            pairingOptions,
                                            &(sinkEntry->gpd),
                                            groupId,
                                            sinkEntry->deviceId,
                                            sinkEntry->gpdSecurityFrameCounter,
                                            sinkEntry->gpdKey.contents,
                                            sinkEntry->assignedAlias,
                                            sinkEntry->groupcastRadius,
                                            sendGpPairing);
  sl_zigbee_af_green_power_cluster_println("pairing send returned %d", retval);
  // If a pairing was added, the sink SHALL send a Device_annce command
  // for the alias (with the exception of lightweight unicast communication mode).
  if (sendGpPairing
      && (sinkCommunicationMode != SL_ZIGBEE_GP_SINK_TYPE_LW_UNICAST)) {
    sl_802154_long_addr_t eui64 = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
    sli_zigbee_af_gp_spoof_device_annce(sinkEntry->assignedAlias, // Short Id
                                        eui64, // Eui64
                                        0); // capabilities
  }
}

static void sendGpPairingLookingUpAddressInSinkEntry(sl_zigbee_gp_address_t *gpdAddr,
                                                     bool sendGpPairing)
{
  if (gpdAddr == NULL) {
    return;
  }
  uint8_t sinkEntryIndex = sl_zigbee_gp_sink_table_lookup(gpdAddr);
  if (sinkEntryIndex == 0xFF) {
    return;
  }
  // Start updating sink table
  sl_zigbee_gp_sink_table_entry_t sinkEntry = { 0 };
  if (sl_zigbee_gp_sink_table_get_entry(sinkEntryIndex, &sinkEntry) != SL_STATUS_OK) {
    return;
  }
  sendGpPairingFromSinkEntry(&sinkEntry,
                             SL_ZIGBEE_RX_ON_WHEN_IDLE_BROADCAST_ADDRESS,
                             sendGpPairing);
}

// Builds a list of all the gpd commands from incoming commissioning req
static uint8_t formGpdCommandListFromIncommingCommReq(uint8_t gpdDeviceId,
                                                      uint8_t *inGpdCommandList,
                                                      uint8_t inCommandListLength,
                                                      uint8_t *gpdCommandList)
{
  uint8_t gpdCommandListLength = 0;
  // if there is already a command list present in incoming payload copy that
  // Realign A0-A3 as AF if TT compacting is enabled and filter out duplicate AF
  bool foundAlready = false;
  for (int i = 0; i < inCommandListLength; i++) {
    if (inGpdCommandList[i] == SL_ZIGBEE_ZCL_GP_GPDF_ATTRIBUTE_REPORTING
        || inGpdCommandList[i] == SL_ZIGBEE_ZCL_GP_GPDF_MULTI_CLUSTER_RPTG
        || inGpdCommandList[i] == SL_ZIGBEE_ZCL_GP_GPDF_MFR_SP_ATTR_RPTG
        || inGpdCommandList[i] == SL_ZIGBEE_ZCL_GP_GPDF_MFR_SP_MULTI_CLUSTER_RPTG) {
      if (!foundAlready) {
        foundAlready = true;
        gpdCommandList[gpdCommandListLength++] = SL_ZIGBEE_ZCL_GP_GPDF_ANY_GPD_SENSOR_CMD;
      }
    } else {
      gpdCommandList[gpdCommandListLength++] = inGpdCommandList[i];
    }
  }
  //Now extend the command list for the device id, take out the duplicate if any
  if (gpdDeviceId != 0xFE) {
    // look up the device Id derive the commands if there is no commands supplied
    if (gpdCommandListLength == 0) {
      gpdCommandListLength += sli_zigbee_af_get_command_list_from_device_id_lookup(gpdDeviceId,
                                                                                   gpdCommandList);
    }
  }
  return gpdCommandListLength;
}

// resolves and relates endpoint - cluster from the given the number of paired endpoints
// cluster list (present in application info and appl description)
static uint8_t buildSinkSupportedClusterEndPointListForGpdCommands(uint8_t numberOfPairedEndpoints,
                                                                   uint8_t *pairedEndpoints,
                                                                   uint8_t numberOfClusters,
                                                                   sli_zigbee_zigbee_cluster_t *clusterList,
                                                                   sli_zigbee_supported_gpd_command_cluster_endpoint_map_t *gpdCommandClusterEpMap)
{
  //for (int i = 0; i < numberOfClusters; i++) {
  //  sl_zigbee_af_green_power_cluster_print("clusterList[%d] = %x ", i, clusterList[i]);
  //}
  //sl_zigbee_af_green_power_cluster_println("");
  uint8_t tempEndpointsOnSink[MAX_ENDPOINT_COUNT] = { 0 };
  uint8_t tempSupportedEndpointCount = 0;
  if (numberOfPairedEndpoints == GREEN_POWER_SERVER_NO_PAIRED_ENDPOINTS
      || numberOfPairedEndpoints == GREEN_POWER_SERVER_RESERVED_ENDPOINTS) {
    // The target endpoint for validation is the Current commissioning Ep
    tempSupportedEndpointCount = getGpCommissioningEndpoint(tempEndpointsOnSink);
  } else if (numberOfPairedEndpoints == GREEN_POWER_SERVER_SINK_DERIVES_ENDPOINTS
             || numberOfPairedEndpoints == GREEN_POWER_SERVER_ALL_SINK_ENDPOINTS) {
    tempSupportedEndpointCount = getAllSinkEndpoints(tempEndpointsOnSink);
  } else {
#if defined(SL_ZIGBEE_AF_NCP) && defined(SL_CATALOG_ZIGBEE_AF_SUPPORT_PRESENT)
    uint8_t endptCount = sl_zigbee_get_endpoint_count();
    for (uint8_t i = 0; (i < endptCount) && (i < MAX_ENDPOINT_COUNT); i++) {
      uint8_t endpt = sl_zigbee_get_endpoint(i);
      if (endpt != 0xFF) {
        for (int j = 0; j < numberOfPairedEndpoints; j++) {
          sl_zigbee_af_green_power_cluster_println("Checking PairedEp[%d]=%x with SinkEp = %d ",
                                                   j, pairedEndpoints[j], endpt);
          if (pairedEndpoints[j] == endpt) {
            tempEndpointsOnSink[tempSupportedEndpointCount] = endpt;
            tempSupportedEndpointCount++;
            // Found, so break inner loop
            // - this is ensure the temp endpoint list is sorted without duplicate
            break;
          }
        }
      }
    }
#else // !(defined(SL_ZIGBEE_AF_NCP) && defined(SL_CATALOG_ZIGBEE_AF_SUPPORT_PRESENT))
    for (int i = 0; i < MAX_ENDPOINT_COUNT; i++) {
      for (int j = 0; j < numberOfPairedEndpoints; j++) {
        sl_zigbee_af_green_power_cluster_println("Checking PairedEp[%d]=%x with SinkEp = %d ",
                                                 j, pairedEndpoints[j], sli_zigbee_af_endpoints[i].endpoint);
        if (pairedEndpoints[j] == sli_zigbee_af_endpoints[i].endpoint) {
          tempEndpointsOnSink[tempSupportedEndpointCount] = sli_zigbee_af_endpoints[i].endpoint;
          tempSupportedEndpointCount++;
          // Found, so break inner loop
          // - this is ensure the temp endpoint list is sorted without duplicate
          break;
        }
      }
    }
#endif //defined(SL_ZIGBEE_AF_NCP) && defined(SL_CATALOG_ZIGBEE_AF_SUPPORT_PRESENT)
  }
  // Here, the endpoints are filtered, those, only are supported out of input paired
  // endpoint list - run through all the clusters for each end point to filter
  // (endpoint, cluster) pair that is supported.
  uint8_t count = 0;
  for (int i = 0; i < tempSupportedEndpointCount; i++) {
    for (int j = 0; j < numberOfClusters; j++) {
      if (0xFFFF != clusterList[j].clusterId) {
        if (sli_zigbee_af_gp_endpoint_and_cluster_id_validation(tempEndpointsOnSink[i],
                                                                clusterList[j].serverClient,
                                                                clusterList[j].clusterId)) {
          gpdCommandClusterEpMap->endpoints[count] = tempEndpointsOnSink[i];
          count++;
          break;
        }
      }
    }
  }
  noOfCommissionedEndpoints = 0;
  gpdCommandClusterEpMap->numberOfEndpoints = 0;
#if defined(SL_ZIGBEE_AF_NCP) && defined(SL_CATALOG_ZIGBEE_AF_SUPPORT_PRESENT)
  uint8_t endptCount = sl_zigbee_get_endpoint_count();
  if ((count <= endptCount) && (count <= MAX_ENDPOINT_COUNT))
#else // !(defined(SL_ZIGBEE_AF_NCP) && defined(SL_CATALOG_ZIGBEE_AF_SUPPORT_PRESENT))
  if (count <= MAX_ENDPOINT_COUNT)
#endif // defined(SL_ZIGBEE_AF_NCP) && defined(SL_CATALOG_ZIGBEE_AF_SUPPORT_PRESENT)
  {
    gpdCommandClusterEpMap->numberOfEndpoints = count;
    noOfCommissionedEndpoints = count;
    memcpy(commissionedEndPoints, gpdCommandClusterEpMap->endpoints, count);
  }
  return count;
}

static uint8_t appendClusterSupportForGenericSwitch(sli_zigbee_zigbee_cluster_t *clusterPtr,
                                                    sl_zigbee_gp_switch_information_t *switchInformationStruct)
{
  clusterPtr[0].clusterId = ZCL_ON_OFF_CLUSTER_ID;
  clusterPtr[0].serverClient = 1;
  return 1;
}

static bool validateSwitchCommand(uint8_t *validatedCommands,
                                  uint8_t commandIndex,
                                  uint8_t commandId,
                                  sl_zigbee_gp_application_info_t *applicationInfo,
                                  sli_zigbee_supported_gpd_command_cluster_endpoint_map_t *gpdCommandClusterEpMap,
                                  sli_zigbee_gp_comm_data_saved_t *commissioningGpd)
{
  uint8_t noOfClusters = 0;
  sli_zigbee_zigbee_cluster_t cluster[30];
  // handle switch validation
  // from user get a set of clusters that generic switch wants to use
  if (commandId == SL_ZIGBEE_ZCL_GP_GPDF_8BITS_VECTOR_PRESS
      || commandId == SL_ZIGBEE_ZCL_GP_GPDF_8BITS_VECTOR_RELEASE) {
    gpdCommandClusterEpMap[commandIndex].gpdCommand = commandId;
    sli_zigbee_zigbee_cluster_t *clusterPtr = &cluster[noOfClusters];
    noOfClusters += appendClusterSupportForGenericSwitch(clusterPtr,
                                                         &(commissioningGpd->switchInformationStruct));
    uint8_t endpointCount = 0;
    endpointCount = buildSinkSupportedClusterEndPointListForGpdCommands(applicationInfo->numberOfPairedEndpoints,
                                                                        applicationInfo->pairedEndpoints,
                                                                        noOfClusters,
                                                                        cluster,
                                                                        &gpdCommandClusterEpMap[commandIndex]);
    if (endpointCount) {
      *validatedCommands += 1;
    }
    return true;
  }
  return false;
}

static uint8_t appendClustersFromApplicationDescription(sli_zigbee_zigbee_cluster_t *clusterPtr,
                                                        bool reverseDirection,
                                                        sli_zigbee_gp_comm_data_saved_t *commissioningGpd)
{
  uint8_t count = 0;
  for (int index = 0; index < commissioningGpd->numberOfReports; index++) {
    const uint8_t *report = sli_zigbee_af_gp_find_report_id(index,
                                                            commissioningGpd->numberOfReports,
                                                            commissioningGpd->reportsStorage);
    if (report) {
      for (int j = 0;; j++) {
        const uint8_t *datapointOption = findDataPointDescriptorInReport(j,
                                                                         report);
        if (datapointOption) {
          clusterPtr[count].serverClient = (datapointOption[0] & 0x08) >> 3;
          if (reverseDirection) {
            clusterPtr[count].serverClient = !(clusterPtr[count].serverClient);
          }
          clusterPtr[count].clusterId = datapointOption[1] + (((uint16_t)datapointOption[2]) << 8);
          count++;
        } else {
          break; // break the inner for loop if datapointOption is null
        }
      }
    }
  }
  return count;
}

static bool validateCARCommand(uint8_t *validatedCommands,
                               uint8_t commandIndex,
                               uint8_t commandId,
                               sl_zigbee_gp_application_info_t *applicationInfo,
                               sli_zigbee_supported_gpd_command_cluster_endpoint_map_t *gpdCommandClusterEpMap,
                               sli_zigbee_gp_comm_data_saved_t *commissioningGpd)
{
  uint8_t noOfClusters = 0;
  sli_zigbee_zigbee_cluster_t cluster[30];
  // Handle Compact Attribute Reporting command validation, check the clusterts present in the
  // stored report descriptor and validate those cluster to see if sink has it on the eintended
  // endpoint on sink.
  if (commandId == SL_ZIGBEE_ZCL_GP_GPDF_COMPACT_ATTRIBUTE_REPORTING) {
    gpdCommandClusterEpMap[commandIndex].gpdCommand = commandId;
    sli_zigbee_zigbee_cluster_t *clusterPtr = &cluster[noOfClusters];
    noOfClusters += appendClustersFromApplicationDescription(clusterPtr,
                                                             true,
                                                             commissioningGpd);
    uint8_t endpointCount = 0;
    endpointCount = buildSinkSupportedClusterEndPointListForGpdCommands(applicationInfo->numberOfPairedEndpoints,
                                                                        applicationInfo->pairedEndpoints,
                                                                        noOfClusters,
                                                                        cluster,
                                                                        &gpdCommandClusterEpMap[commandIndex]);
    if (endpointCount) {
      *validatedCommands += 1;
    }
    return true;
  }
  return false;
}

// final list of mapping of gpdCommand-Cluster-EndpointList that is supported
static uint8_t validateGpdCommandSupportOnSink(uint8_t *gpdCommandList,
                                               uint8_t gpdCommandListLength,
                                               sl_zigbee_gp_application_info_t *applicationInfo,
                                               sli_zigbee_supported_gpd_command_cluster_endpoint_map_t *gpdCommandClusterEpMap,
                                               sli_zigbee_gp_comm_data_saved_t *commissioningGpd)
{
  uint8_t validatedCommands = 0;
  for (int commandIndex = 0; commandIndex < gpdCommandListLength; commandIndex++) {
    // First see if it is switch command - validate it else do rest of
    // the command validation
    if (validateSwitchCommand(&validatedCommands,
                              commandIndex,
                              gpdCommandList[commandIndex],
                              applicationInfo,
                              gpdCommandClusterEpMap,
                              commissioningGpd)) {
    } else if (validateCARCommand(&validatedCommands,
                                  commandIndex,
                                  gpdCommandList[commandIndex],
                                  applicationInfo,
                                  gpdCommandClusterEpMap,
                                  commissioningGpd)) {
    } else {
      // default sub translation table has the (gpdCommnd<->ClusterId) mapping,
      // use the look up to decide when the mapped clusterId = 0xFFFF, then it
      // need to use the cluster list from the incoming message payload.
      // pick up clusters to validate against the supplied pair of end points
      // and available endpoints in sink;
      gpdCommandClusterEpMap[commandIndex].gpdCommand = gpdCommandList[commandIndex];
      // Prepare a cluster list that will be input for next level
      uint8_t noOfClusters = 0;
      sli_zigbee_zigbee_cluster_t cluster[30] = { 0 };
      if ((applicationInfo->numberOfGpdClientCluster != 0)
          || (applicationInfo->numberOfGpdServerCluster != 0)) {
        // Now a generic cluster - read, write, reporting etc
        // many clusters are in for the command
        // prepare a list of all the clusters in the incoming message
        // pick from appInfo
        for (int i = 0; i < applicationInfo->numberOfGpdClientCluster; i++) {
          cluster[noOfClusters].clusterId = applicationInfo->clientClusters[i];
          cluster[noOfClusters].serverClient = 1; // reverse for the match
          noOfClusters++;
        }
        for (int i = 0; i < applicationInfo->numberOfGpdServerCluster; i++) {
          cluster[noOfClusters].clusterId = applicationInfo->serverClusters[i];
          cluster[noOfClusters].serverClient = 0; // reverse for the match
          noOfClusters++;
        }
      } else {
        sli_zigbee_zigbee_cluster_t gpdCluster;
        if (!sli_zigbee_af_get_cluster_list_from_cmd_id_lookup(gpdCommandList[commandIndex],
                                                               &gpdCluster)) {
          continue;
        }

        // Apend the cluster found in the TT (default or customised) if available
        if (0xFFFF != gpdCluster.clusterId) {
          cluster[noOfClusters].clusterId = gpdCluster.clusterId;
          cluster[noOfClusters].serverClient = gpdCluster.serverClient;
          noOfClusters += 1;
        } else {
          // Now, get the list of clusters from the device Id if the cluster associated with this
          // command in the translation table is reserved.
          noOfClusters += sli_zigbee_af_get_cluster_list_from_device_id_lookup(applicationInfo->deviceId,
                                                                               &cluster[noOfClusters]);
        }
      }
      uint8_t endpointCount = 0;
      endpointCount = buildSinkSupportedClusterEndPointListForGpdCommands(applicationInfo->numberOfPairedEndpoints,
                                                                          applicationInfo->pairedEndpoints,
                                                                          noOfClusters,
                                                                          cluster,
                                                                          &gpdCommandClusterEpMap[commandIndex]);
      if (endpointCount) {
        validatedCommands += 1;
      }
    }
  }
  return validatedCommands;
}

// Checks if GP can support some or all of the commands and clusters
static uint8_t getMatchedSinkFunctionality(sli_zigbee_supported_gpd_command_cluster_endpoint_map_t *gpdCommandClusterEpMap,
                                           sli_zigbee_gp_comm_data_saved_t *commissioningGpd)
{
  sl_zigbee_gp_application_info_t *applicationInfo = &(commissioningGpd->applicationInfo);
  // to hold the temporary list of gpd commands derived from deviceId and added
  // from the gpdCommandList from app info
  uint8_t gpdCommandList[50] = { 0 };
  // Build a list of all the commands - present and derived from GPD ID look up
  uint8_t gpdCommandListLength = formGpdCommandListFromIncommingCommReq(applicationInfo->deviceId,
                                                                        applicationInfo->gpdCommands,
                                                                        applicationInfo->numberOfGpdCommands,
                                                                        gpdCommandList);
  // Simulate the commands as per the spec from other information in absence of
  // command list and device Id
  if (gpdCommandListLength == 0
      && 0xFE == applicationInfo->deviceId) {
    uint8_t commandIndex = 0;
    if (applicationInfo->applInfoBitmap & SL_ZIGBEE_AF_GP_APPLICATION_INFORMATION_APPLICATION_DESCRIPTION_PRESENT) {
      // this command is for a CAR
      gpdCommandList[commandIndex++] = SL_ZIGBEE_ZCL_GP_GPDF_COMPACT_ATTRIBUTE_REPORTING;
    }
    if (applicationInfo->applInfoBitmap & SL_ZIGBEE_AF_GP_APPLICATION_INFORMATION_CLUSTER_LIST_PRESENT) {
      // Add the AF to compact the Translation table A0/A1
      gpdCommandList[commandIndex++] = SL_ZIGBEE_ZCL_GP_GPDF_ANY_GPD_SENSOR_CMD;
      // Add the Tunneling as an implicit to adding reporting A0/A1
      gpdCommandList[commandIndex++] = SL_ZIGBEE_ZCL_GP_GPDF_ZCL_TUNNELING_WITH_PAYLOAD;
    }
    gpdCommandListLength = commandIndex;
  }

  uint8_t validatedCommands = 0;
  validatedCommands = validateGpdCommandSupportOnSink(gpdCommandList,
                                                      gpdCommandListLength,
                                                      applicationInfo,
                                                      gpdCommandClusterEpMap,
                                                      commissioningGpd);
  return validatedCommands;
}

static bool handleGpdMatchingSinkFunctionality(sli_zigbee_gp_comm_data_saved_t *commissioningGpd)
{
  // Let user to take decission first
  //1. User handles completely validates the matching functionality( plugin does not process anything) - this is the case of a Gateway Combo
  //     a. Found matching functionality, so that a commissioning state changes (like a commissioning reply queued)
  //     b. Not Found
  //2. Plugin completely handles the functionality matching - Most of the cases of an SoC Combo.
  //     In some cases - user would like to just manipulated the appInfo and lets the plugin to handle rest.

  bool matchFound = true;
  if (sl_zigbee_af_green_power_server_gpd_commissioning_cb(&(commissioningGpd->applicationInfo), &matchFound)) {
    // return based on user handling
    return matchFound;
  }

  // Relation between GP deviceID and Translation table content creation
  // for example : GPD_DeviceID 0x02 (on/off)
  // means TT entries ZbProfile ZHA, ZbCluster 0x0006, ZbCmd ON+OFF+TOGGLE,
  // for Gps endpoint which match this cluster On/Off 0x0006
  uint8_t gpdMatch = 0;
  sli_zigbee_supported_gpd_command_cluster_endpoint_map_t gpdCommandClusterEpMap[50] = { 0 };
  gpdMatch = getMatchedSinkFunctionality(gpdCommandClusterEpMap,
                                         commissioningGpd);
  return (gpdMatch ? true : false);
}

static void sendCommissioningReply(sli_zigbee_gp_comm_data_saved_t *commissioningGpd)
{
  uint8_t gpsSecurityKeyTypeAtrribute = 0;
  sl_zigbee_key_data_t gpsKeyAttribute = { { 0 } };
  sl_zigbee_af_status_t status = sl_zigbee_af_green_power_server_derive_shared_key_from_sink_attribute(&gpsSecurityKeyTypeAtrribute,
                                                                                                       &gpsKeyAttribute,
                                                                                                       &(commissioningGpd->addr));

  uint8_t incomingGpdKeyType = (((commissioningGpd->gpdfExtendedOptions) & SL_ZIGBEE_AF_GP_GPD_COMMISSIONING_EXTENDED_OPTIONS_KEY_TYPE)
                                >> SL_ZIGBEE_AF_GP_GPD_COMMISSIONING_EXTENDED_OPTIONS_KEY_TYPE_OFFSET);
  bool incomingGpdKeyEncryption = (((commissioningGpd->gpdfExtendedOptions) & SL_ZIGBEE_AF_GP_GPD_COMMISSIONING_EXTENDED_OPTIONS_GPD_KEY_ENCRYPTION)
                                   ? true : false);
  uint8_t sendSecurityKeyType = 0;
  bool sendKeyinReply = false;
  if ((gpsSecurityKeyTypeAtrribute == 0
       && ((commissioningGpd->gpdfOptions) & SL_ZIGBEE_AF_GP_GPD_COMMISSIONING_OPTIONS_GP_SECURITY_KEY_REQUEST))
      || !((commissioningGpd->gpdfOptions) & SL_ZIGBEE_AF_GP_GPD_COMMISSIONING_OPTIONS_GP_SECURITY_KEY_REQUEST)
      || (incomingGpdKeyType == (gpsSecurityKeyTypeAtrribute & GPS_ATTRIBUTE_KEY_TYPE_MASK)
          && 0 == memcmp(commissioningGpd->key.contents,
                         gpsKeyAttribute.contents,
                         SL_ZIGBEE_ENCRYPTION_KEY_SIZE)
          && gpsSecurityKeyTypeAtrribute != 0)) {
    // Use the proposed key, that has come in the commissioning gfdf and saved in commissioningGpd
    sendKeyinReply = false;
    sendSecurityKeyType = incomingGpdKeyType;
  }
  sl_zigbee_key_data_t sendKey = { { 0 } };
  bool sendKeyEncryption = false;
  if (status == SL_ZIGBEE_ZCL_STATUS_SUCCESS
      && ((commissioningGpd->gpdfOptions) & SL_ZIGBEE_AF_GP_GPD_COMMISSIONING_OPTIONS_GP_SECURITY_KEY_REQUEST)
      && gpsSecurityKeyTypeAtrribute != incomingGpdKeyType
      && gpsSecurityKeyTypeAtrribute != 0) {
    sendSecurityKeyType = gpsSecurityKeyTypeAtrribute;
    memcpy(sendKey.contents, gpsKeyAttribute.contents, SL_ZIGBEE_ENCRYPTION_KEY_SIZE);
    sendKeyEncryption = incomingGpdKeyEncryption;
    sendKeyinReply = true;
    // Set up same key and key type for the Gp Pairing
    commissioningGpd->securityKeyType = sendSecurityKeyType;
    memcpy(commissioningGpd->key.contents, sendKey.contents, SL_ZIGBEE_ENCRYPTION_KEY_SIZE);
  }

  uint8_t commReplyOptions = 0;
  sl_zigbee_af_green_power_cluster_println("Added securityLevel %d", commissioningGpd->securityLevel);
  commReplyOptions |= (commissioningGpd->securityLevel << SL_ZIGBEE_AF_GP_GPD_COMMISSIONING_REPLY_OPTIONS_SECURITY_LEVEL_OFFSET);
  if ((commissioningGpd->gpdfOptions) & SL_ZIGBEE_AF_GP_GPD_COMMISSIONING_OPTIONS_PAN_ID_REQUEST) {
    commReplyOptions |= SL_ZIGBEE_AF_GP_GPD_COMMISSIONING_REPLY_OPTIONS_PAN_ID_PRESENT;
    sl_zigbee_af_green_power_cluster_println("Added PanId");
  }
  sl_zigbee_af_green_power_cluster_println("Added Key Type = %d", sendSecurityKeyType);
  commReplyOptions |= (sendSecurityKeyType
                       << SL_ZIGBEE_AF_GP_GPD_COMMISSIONING_REPLY_OPTIONS_KEY_TYPE_OFFSET);

  if (sendKeyinReply) {
    sl_zigbee_af_green_power_cluster_println("Added Key Present");
    commReplyOptions |= SL_ZIGBEE_AF_GP_GPD_COMMISSIONING_REPLY_OPTIONS_GPD_SECURITY_KEY_PRESENT;
    if (sendKeyEncryption) { // GPDF GPKeyEncryption Subfeild
      sl_zigbee_af_green_power_cluster_println("Added Key Encrypted");
      commReplyOptions |= SL_ZIGBEE_AF_GP_GPD_COMMISSIONING_REPLY_OPTIONS_GPDKEY_ENCRYPTION;
    }
  }
  // Prepare the payload
  uint8_t commReplyPayload[COMM_REPLY_PAYLOAD_SIZE] = { 0 };
  commReplyPayload[0] = commReplyOptions;
  uint8_t index = 1;
  if (commReplyOptions & SL_ZIGBEE_AF_GP_GPD_COMMISSIONING_REPLY_OPTIONS_PAN_ID_PRESENT) {
    sl_util_store_low_high_int16u(commReplyPayload + index, sl_zigbee_af_get_pan_id());
    index += 2;
  }

  if (commReplyOptions & SL_ZIGBEE_AF_GP_GPD_COMMISSIONING_REPLY_OPTIONS_GPD_SECURITY_KEY_PRESENT) {
    if (commReplyOptions & SL_ZIGBEE_AF_GP_GPD_COMMISSIONING_REPLY_OPTIONS_GPDKEY_ENCRYPTION) {
      uint8_t mic[4] = { 0 };
      uint32_t fc = commissioningGpd->outgoingFrameCounter + 1;
      sl_zigbee_af_green_power_cluster_println("Key :");
      for (int i = 0; i < SL_ZIGBEE_ENCRYPTION_KEY_SIZE; i++) {
        sl_zigbee_af_green_power_cluster_print("%x", sendKey.contents[i]);
      }
      sl_zigbee_af_green_power_cluster_println("");
      // Encrypt the Commissioning Reply (outgoing) Key
      const sl_zigbee_key_data_t linkKey[] = { GP_DEFAULT_LINK_KEY };
      import_load_zb_sec_man_internal_key((sl_zigbee_sec_man_key_t *)&linkKey);
      sli_zigbee_af_gp_key_tc_lk_derivation(&(commissioningGpd->addr),
                                            fc,
                                            mic,
                                            &sendKey,
                                            false);
      memcpy(commReplyPayload + index, sendKey.contents, SL_ZIGBEE_ENCRYPTION_KEY_SIZE);
      index += SL_ZIGBEE_ENCRYPTION_KEY_SIZE;
      memcpy(commReplyPayload + index, mic, 4);
      index += 4;
      memcpy(commReplyPayload + index, &fc, 4);
      index += 4;
    } else {
      memcpy(commReplyPayload + index, sendKey.contents, SL_ZIGBEE_ENCRYPTION_KEY_SIZE);
      index += SL_ZIGBEE_ENCRYPTION_KEY_SIZE;
    }
  }
  sl_zigbee_af_green_power_cluster_println("GpResponse : Sending Commissioning Reply");
  if (sl_zigbee_af_fill_command_green_power_cluster_gp_response_smart(commissioningGpd->addr.applicationId,
                                                                      commissioningGpd->gppShortAddress,
                                                                      (sl_zigbee_af_get_radio_channel() - 11),
                                                                      commissioningGpd->addr.id.sourceId,
                                                                      commissioningGpd->addr.id.gpdIeeeAddress,
                                                                      commissioningGpd->addr.endpoint,
                                                                      SL_ZIGBEE_ZCL_GP_GPDF_COMMISSIONING_REPLY,
                                                                      index,
                                                                      commReplyPayload) == 0) {
    return;
  }

  sl_zigbee_aps_frame_t *apsFrame;
  apsFrame = sl_zigbee_af_get_command_aps_frame();
  apsFrame->sourceEndpoint = SL_ZIGBEE_GP_ENDPOINT;
  apsFrame->destinationEndpoint = SL_ZIGBEE_GP_ENDPOINT;
  sl_status_t UNUSED retval;
  if (commissioningState.unicastCommunication) { //  based on the commission mode as decided by sink
    retval = sl_zigbee_af_send_command_unicast(SL_ZIGBEE_OUTGOING_DIRECT, commissioningGpd->gppShortAddress);
  } else {
    retval = sl_zigbee_af_send_command_broadcast(SL_ZIGBEE_RX_ON_WHEN_IDLE_BROADCAST_ADDRESS, SL_ZIGBEE_NULL_NODE_ID, 0);
  }
  sl_zigbee_af_green_power_cluster_println("GpResponse send returned %d", retval);
}

static bool handleAddingTranstionTableEntry(sli_zigbee_gp_comm_data_saved_t *commissioningGpd)
{
  if (!sinkFunctionalitySupported(SL_ZIGBEE_AF_GP_GPS_FUNCTIONALITY_TRANSLATION_TABLE)) {
    return true;
  }
  uint8_t gpdMatch = 0;

  sli_zigbee_supported_gpd_command_cluster_endpoint_map_t gpdCommandClusterEpMap[50] = { 0 };
  gpdMatch = getMatchedSinkFunctionality(gpdCommandClusterEpMap,
                                         commissioningGpd);

  // In case the sink supports the gpd parameters, create an TT entry for each gpd command
  for (uint8_t cmdIndex = 0; cmdIndex < gpdMatch; cmdIndex++) {
    for (uint8_t i = 0; i < gpdCommandClusterEpMap[cmdIndex].numberOfEndpoints; i++) {
      sl_zigbee_af_green_power_cluster_println("gpdCommandClusterEpMap[%d].gpdCommand = %d gpdCommandClusterEpMap[%d].endpoints[%d] = %d",
                                               cmdIndex,
                                               gpdCommandClusterEpMap[cmdIndex].gpdCommand,
                                               cmdIndex,
                                               i,
                                               gpdCommandClusterEpMap[cmdIndex].endpoints[i]);
      #ifdef SL_CATALOG_ZIGBEE_GREEN_POWER_TRANSLATION_TABLE_PRESENT
      sli_zigbee_af_gp_pairing_done_thus_set_customized_translation_table(&(commissioningGpd->addr),
                                                                          gpdCommandClusterEpMap[cmdIndex].gpdCommand,
                                                                          gpdCommandClusterEpMap[cmdIndex].endpoints[i]);
      #endif // SL_CATALOG_ZIGBEE_GREEN_POWER_TRANSLATION_TABLE_PRESENT
    }
  }
  return (gpdMatch ? true : false);
}

static sl_zigbee_sink_pairing_status_t handleSinkEntryAndPairing(sli_zigbee_gp_comm_data_saved_t *commissioningGpd)
{
  sl_zigbee_gp_address_t *gpdAddr = &(commissioningGpd->addr);

  uint8_t sinkEntryIndex = sl_zigbee_gp_sink_table_find_or_allocate_entry(gpdAddr);
  // must have a valid sinkEntryIndex by now, else indicates issue with sink table such as sink table full.
  if (sinkEntryIndex == 0xFF) {
    return SINK_PAIRING_STATUS_FAIL_NO_SPACE_IN_SINK_TABLE;
  }
  // Start updating sink table
  sl_zigbee_gp_sink_table_entry_t sinkEntry;
  if (sl_zigbee_gp_sink_table_get_entry(sinkEntryIndex, &sinkEntry) != SL_STATUS_OK) {
    return SINK_PAIRING_STATUS_FAIL_ENTRY_CORRUPTED;
  }
  sl_zigbee_af_green_power_server_pre_sink_pairing_cb(commissioningGpd);
  commissioningGpd->preSinkCbSource = GP_PRE_SINK_PAIRING_CALLBACK_SOURCE_UNKNOWN;
  bool gpdFixed = ((commissioningGpd->gpdfOptions) & SL_ZIGBEE_AF_GP_GPD_COMMISSIONING_OPTIONS_FIXED_LOCATION) \
                  ? true : false;
  bool gpdMacCapabilities = ((commissioningGpd->gpdfOptions) & SL_ZIGBEE_AF_GP_GPD_COMMISSIONING_OPTIONS_MAC_SEQ_NUM_CAP) \
                            ? true : false;

  uint32_t newSinkTableOptions = (gpdAddr->applicationId & SL_ZIGBEE_AF_GP_SINK_TABLE_ENTRY_OPTIONS_APPLICATION_ID)
                                 | (commissioningGpd->communicationMode << SL_ZIGBEE_AF_GP_SINK_TABLE_ENTRY_OPTIONS_COMMUNICATION_MODE_OFFSET)
                                 | (gpdMacCapabilities << SL_ZIGBEE_AF_GP_SINK_TABLE_ENTRY_OPTIONS_SEQUENCE_NUM_CAPABILITIES_OFFSET)
                                 | ((((commissioningGpd->gpdfOptions) & SL_ZIGBEE_AF_GP_GPD_COMMISSIONING_OPTIONS_RX_ON_CAP) >> SL_ZIGBEE_AF_GP_GPD_COMMISSIONING_OPTIONS_RX_ON_CAP_OFFSET) << SL_ZIGBEE_AF_GP_SINK_TABLE_ENTRY_OPTIONS_RX_ON_CAPABILITY_OFFSET)
                                 | (gpdFixed << SL_ZIGBEE_AF_GP_SINK_TABLE_ENTRY_OPTIONS_FIXED_LOCATION_OFFSET)
                                 | (commissioningGpd->useGivenAssignedAlias << SL_ZIGBEE_AF_GP_SINK_TABLE_ENTRY_OPTIONS_ASSIGNED_ALIAS_OFFSET)
                                 | ((((commissioningGpd->gpdfOptions) & 0x80) \
                                     && (commissioningGpd->gpdfExtendedOptions)) ? SL_ZIGBEE_AF_GP_SINK_TABLE_ENTRY_OPTIONS_SECURITY_USE : 0);
  // SecurityUse bit always set to 0b1, so securityOptions field always present
  // also to indicate level 0 or key NONE, cause it will be extremly rare that a GPD is SecLvl=0,
  uint8_t newSinkTableSecurityOptions = commissioningGpd->securityLevel
                                        | (commissioningGpd->securityKeyType << SL_ZIGBEE_AF_GP_SINK_TABLE_ENTRY_SECURITY_OPTIONS_SECURITY_KEY_TYPE_OFFSET);
  sinkEntry.options = newSinkTableOptions;
  sinkEntry.deviceId = commissioningGpd->applicationInfo.deviceId;
  sinkEntry.assignedAlias = sli_zigbee_gpd_alias(gpdAddr);
  if (commissioningGpd->useGivenAssignedAlias) {
    sinkEntry.assignedAlias = commissioningGpd->givenAlias;
  }
  if (sinkEntry.groupcastRadius == 0xff
      || sinkEntry.groupcastRadius < commissioningGpd->groupcastRadius) {
    sinkEntry.groupcastRadius = commissioningGpd->groupcastRadius;
  }
  // Add the Group
  uint16_t groupId = 0xFFFF;
  sl_zigbee_af_status_t status = SL_ZIGBEE_ZCL_STATUS_SUCCESS;
  if (commissioningGpd->communicationMode == SL_ZIGBEE_GP_SINK_TYPE_D_GROUPCAST) {
    groupId = sli_zigbee_gpd_alias(gpdAddr);
    status = sli_zigbee_af_gp_add_to_aps_group(SL_ZIGBEE_GP_ENDPOINT, groupId);
    sl_zigbee_af_core_println("Added to Group %d Status = %d", groupId, status);
  } else if (commissioningGpd->communicationMode == SL_ZIGBEE_GP_SINK_TYPE_GROUPCAST) {
    for (uint8_t index = 0, pos = 1;
         index < GP_SINK_LIST_ENTRIES && pos < ((commissioningGpd->groupList[0] * sizeof(sl_zigbee_gp_sink_group_t)) + 1);
         index++) {
      sinkEntry.sinkList[index].type = SL_ZIGBEE_GP_SINK_TYPE_GROUPCAST;
      memcpy(&(sinkEntry.sinkList[index].target.groupcast), &(commissioningGpd->groupList[pos]), sizeof(sl_zigbee_gp_sink_group_t));
      pos += sizeof(sl_zigbee_gp_sink_group_t);
      groupId = sinkEntry.sinkList[index].target.groupcast.groupID;
      status = sli_zigbee_af_gp_add_to_aps_group(SL_ZIGBEE_GP_ENDPOINT, groupId);
    }
  }

  if (status != SL_ZIGBEE_ZCL_STATUS_SUCCESS) {
    return SINK_PAIRING_STATUS_FAILURE;
  }

  sinkEntry.securityOptions = newSinkTableSecurityOptions;
  // carefull, take the gpd outgoing FC (not the framecounter of the commissioning frame "gpdSecurityFrameCounter")
  sinkEntry.gpdSecurityFrameCounter = commissioningGpd->outgoingFrameCounter;
  memcpy(sinkEntry.gpdKey.contents, commissioningGpd->key.contents, SL_ZIGBEE_ENCRYPTION_KEY_SIZE);
  if (sl_zigbee_af_green_power_server_sink_table_access_notification_cb(gpdAddr,
                                                                        GREEN_POWER_SERVER_SINK_TABLE_ACCESS_TYPE_ADD_GPD)) {
    sl_zigbee_gp_sink_table_set_entry(sinkEntryIndex, &sinkEntry);
  }
  // End of Sink Table Update

  sendGpPairingFromSinkEntry(&sinkEntry,
                             commissioningGpd->gppShortAddress,
                             !(commissioningGpd->doNotSendGpPairing));

  if (commissioningGpd->doNotSendGpPairing) {
    commissioningGpd->doNotSendGpPairing = false;
    return SINK_PAIRING_STATUS_SUCCESS;
  }

  // Send GpPairing Configuration Based on Sink Communication mode
  sendGpPairingConfigBasedOnSinkCommunicationMode(commissioningGpd);

  return SINK_PAIRING_STATUS_SUCCESS;
}

static void handleClosingCommissioningSessionOnFirstPairing(void)
{
  uint8_t gpsCommissioningExitMode;
  sl_zigbee_af_status_t statusExitMode;
  sl_zigbee_af_attribute_type_t type;
  statusExitMode = sl_zigbee_af_read_attribute(SL_ZIGBEE_GP_ENDPOINT,
                                               ZCL_GREEN_POWER_CLUSTER_ID,
                                               ZCL_GP_SERVER_GPS_COMMISSIONING_EXIT_MODE_ATTRIBUTE_ID,
                                               (CLUSTER_MASK_SERVER),
                                               (uint8_t*)&gpsCommissioningExitMode,
                                               sizeof(uint8_t),
                                               &type);
  if (statusExitMode == SL_ZIGBEE_ZCL_STATUS_SUCCESS
      && (gpsCommissioningExitMode & SL_ZIGBEE_AF_GP_SINK_COMMISSIONING_MODE_EXIT_MODE_ON_FIRST_PAIRING_SUCCESS)) {
    // if commissioning mode is ON and received frame set it to OFF, exit comissioning mode
    commissioningState.inCommissioningMode = false;

    if (commissioningState.proxiesInvolved) {
      uint8_t UNUSED retval;
      uint8_t proxyOptions = 0;
      if (commissioningState.unicastCommunication) { //  based on the commission mode as decided by sink
        proxyOptions |= 0x20;
      }
      if (sl_zigbee_af_fill_command_green_power_cluster_gp_proxy_commissioning_mode_smart(proxyOptions,
                                                                                          0,
                                                                                          0) == 0) {
        return;
      }

      sl_zigbee_aps_frame_t *apsFrame;
      apsFrame = sl_zigbee_af_get_command_aps_frame();
      apsFrame->sourceEndpoint = SL_ZIGBEE_GP_ENDPOINT;
      apsFrame->destinationEndpoint = SL_ZIGBEE_GP_ENDPOINT;
      retval = sl_zigbee_af_send_command_broadcast(SL_ZIGBEE_RX_ON_WHEN_IDLE_BROADCAST_ADDRESS, SL_ZIGBEE_NULL_NODE_ID, 0);
      sl_zigbee_af_green_power_cluster_println("SinkCommissioningModeCallback(Exit) send returned %d", retval);
      // add a call for user application to let it know that the commissioning session ends
      // for application perspective this should be as same as a commissioning windows tiemout expiration
      // then the following user callback is used:
      sl_zigbee_af_green_power_server_commissioning_timeout_cb(COMMISSIONING_TIMEOUT_TYPE_COMMISSIONING_WINDOW_TIMEOUT,
                                                               noOfCommissionedEndpoints,
                                                               commissionedEndPoints);
    }
  }
}

static void finalisePairing(sli_zigbee_gp_comm_data_saved_t *commissioningGpd)
{
  sl_zigbee_sink_pairing_status_t status = SINK_PAIRING_STATUS_IN_PROGRESS;
  // Check if the Sink can support the functionality of GPD
  if (handleGpdMatchingSinkFunctionality(commissioningGpd)) {
    // If bidirectional flag is set, queue a Commissioning Reply
    // with Gp Response.
    if (commissioningGpd->commissionState == GP_SINK_COMM_STATE_SEND_COMM_REPLY) {
      sl_zigbee_af_green_power_cluster_println("Sink Supports GPD - Calling GP Response");
      // Finalize the sink entry upon reception of success
      sendCommissioningReply(commissioningGpd);
      commissioningGpd->commissionState = GP_SINK_COMM_STATE_WAIT_FOR_SUCCESS;
    } else if (commissioningGpd->commissionState == GP_SINK_COMM_STATE_FINALISE_PAIRING) {
      sl_zigbee_af_green_power_cluster_println("Calling Adding TT and Sink ");
      if (handleAddingTranstionTableEntry(commissioningGpd)) {
        status = handleSinkEntryAndPairing(commissioningGpd);
        if (status == SINK_PAIRING_STATUS_SUCCESS) {
          handleClosingCommissioningSessionOnFirstPairing();
        }
        commissioningGpd->commissionState = GP_SINK_COMM_STATE_PAIRING_DONE;
        // Call the user to inform the pairing is finalised.
        sl_zigbee_af_green_power_server_pairing_complete_cb(noOfCommissionedEndpoints,
                                                            commissionedEndPoints);
        sl_zigbee_af_green_power_server_pairing_status_cb(status,
                                                          commissioningGpd);
        sl_zigbee_af_green_power_server_delete_commissioning_gpd_instance(&(commissioningGpd->addr));
        // commissioning session ended here,clean up stored information
        resetOfMultisensorDataSaved(true, commissioningGpd);
        return;
      } else {
        status = SINK_PAIRING_STATUS_FAIL_ADDING_TRANSLATION;
        sl_zigbee_af_green_power_cluster_println("Finalize Pairing : Translation Table Addition Failed.");
      }
    }
  } else {
    status = SINK_PAIRING_STATUS_FAIL_NO_MATCHING_FUNCTIONALITY;
    sl_zigbee_af_green_power_cluster_println("Finalize Pairing : Sink Functionality Does Not Support the GPD");
  }
  sl_zigbee_af_green_power_server_pairing_status_cb(status,
                                                    commissioningGpd);
}

static bool gpCommissioningNotificationCommissioningGpdf(uint16_t commNotificationOptions,
                                                         sl_zigbee_gp_address_t *gpdAddr,
                                                         uint32_t gpdSecurityFrameCounter,
                                                         uint8_t gpdCommandId,
                                                         uint8_t *gpdCommandPayload,
                                                         uint16_t gppShortAddress,
                                                         sli_zigbee_gp_comm_data_saved_t *commissioningGpd)
{
  sl_zigbee_af_green_power_cluster_println("Process GP CN");
  uint8_t gpsSecurityLevelAttribute = 0;
  sl_zigbee_af_status_t status;
  sl_zigbee_af_attribute_type_t type;
  status = sl_zigbee_af_read_attribute(SL_ZIGBEE_GP_ENDPOINT,
                                       ZCL_GREEN_POWER_CLUSTER_ID,
                                       ZCL_GP_SERVER_GPS_SECURITY_LEVEL_ATTRIBUTE_ID,
                                       (CLUSTER_MASK_SERVER),
                                       (uint8_t*)&gpsSecurityLevelAttribute,
                                       sizeof(uint8_t),
                                       &type);
  // Reject the req if InvolveTC is set in the attribute
  if (gpsSecurityLevelAttribute & 0x08) { // Involve TC is set in the Sink
    sl_zigbee_af_green_power_cluster_println("DROP - GP CN : Involve TC bit is set in gpsSecurityLevelAttribute!");
    return true;
  }

  sl_zigbee_gp_application_info_t appInfoStruct = { 0 };
  uint8_t index = 1; // gpdCommandPayload [0] is length, hence set to 1
  appInfoStruct.deviceId = gpdCommandPayload[index++];
  uint8_t gpdfOptions = gpdCommandPayload[index++];
  // parse option and ext option from gpdCommandPayload of the commissioning command
  uint8_t gpdfExtendedOptions = 0;
  if (gpdfOptions & SL_ZIGBEE_AF_GP_GPD_COMMISSIONING_OPTIONS_EXTENDED_OPTIONS_FIELD) {
    gpdfExtendedOptions = gpdCommandPayload[index++];
  }
  // Parse security key type and level
  uint8_t securityLevel = (gpdfExtendedOptions & SL_ZIGBEE_AF_GP_GPD_COMMISSIONING_EXTENDED_OPTIONS_SECURITY_LEVEL_CAPABILITIES);

  if (securityLevel == SL_ZIGBEE_GP_SECURITY_LEVEL_RESERVED
      || (status == SL_ZIGBEE_ZCL_STATUS_SUCCESS
          && (((gpsSecurityLevelAttribute & 0x03) > securityLevel) // GPD Security Level is lower than Sink supports
              || ((gpsSecurityLevelAttribute & 0x04) // Sink requires the GPD to support key encryption
                  && (gpdfExtendedOptions & SL_ZIGBEE_AF_GP_GPD_COMMISSIONING_EXTENDED_OPTIONS_GPD_KEY_PRESENT) // Key Present but not encrypted
                  && !(gpdfExtendedOptions & SL_ZIGBEE_AF_GP_GPD_COMMISSIONING_EXTENDED_OPTIONS_GPD_KEY_ENCRYPTION))))) {
    sl_zigbee_af_green_power_cluster_println("DROP - GP CN : Sec Level < gpsSecurityLevelAttribute!");
    return true;
  }
  uint8_t securityKeyType = ((gpdfExtendedOptions & SL_ZIGBEE_AF_GP_GPD_COMMISSIONING_EXTENDED_OPTIONS_KEY_TYPE)
                             >> SL_ZIGBEE_AF_GP_GPD_COMMISSIONING_EXTENDED_OPTIONS_KEY_TYPE_OFFSET);
  // If security Level is none 0 and Gpd is in commissioning process currently
  // It has following combinitions for key negotiation
  // Unidirectional (rxAfterTx = 0) : shall send a key in one of the commissioning frames (current or last one)
  //                                  if it does not have then drop it.
  // Bidirectional  (rxAfterTx = 1) : may send a key and/or request one
  if (securityLevel != 0
      && !(commissioningGpd->gpdfExtendedOptions & SL_ZIGBEE_AF_GP_GPD_COMMISSIONING_EXTENDED_OPTIONS_GPD_KEY_PRESENT)  // has it received key from GPD in past, for first commissioning frame - this will be false
      && !(gpdfExtendedOptions & SL_ZIGBEE_AF_GP_GPD_COMMISSIONING_EXTENDED_OPTIONS_GPD_KEY_PRESENT)) { // No Key in this frame
    sl_zigbee_af_green_power_cluster_println("Sec Level > 0, No Key Received Yet");
    if (!(commNotificationOptions & SL_ZIGBEE_AF_GP_COMMISSIONING_NOTIFICATION_OPTION_RX_AFTER_TX)) {
      sl_zigbee_af_green_power_cluster_println("DROP - GP CN : No Key Present, Unidirectional RxAfterTx = 0");
      return true;
    } else {
      // RxAfterTx is set but no request key, then Drop
      if (!(gpdfOptions & SL_ZIGBEE_AF_GP_GPD_COMMISSIONING_OPTIONS_GP_SECURITY_KEY_REQUEST)) {
        sl_zigbee_af_green_power_cluster_println("DROP - GP CN : No Key Request, Bidirectional RxAfterTx = 1");
        return true;
      }
    }
  }
  sl_zigbee_key_data_t key = { 0 };
  if (gpdfExtendedOptions & SL_ZIGBEE_AF_GP_GPD_COMMISSIONING_EXTENDED_OPTIONS_GPD_KEY_PRESENT) {
    memmove(key.contents, (gpdCommandPayload + index), SL_ZIGBEE_ENCRYPTION_KEY_SIZE);
    index += SL_ZIGBEE_ENCRYPTION_KEY_SIZE;
    // If the incoming key is encrypted, check its authenticity
    if (gpdfExtendedOptions & SL_ZIGBEE_AF_GP_GPD_COMMISSIONING_EXTENDED_OPTIONS_GPD_KEY_ENCRYPTION) {
      sl_zigbee_key_data_t linkKey = { GP_DEFAULT_LINK_KEY };
      import_load_zb_sec_man_internal_key((sl_zigbee_sec_man_key_t *)&linkKey);
      uint8_t mic[4] = { 0 };
      // Read in reverse for comparison with generated calculated mic array
      uint32_t keyMic = sl_util_fetch_low_high_int32u(gpdCommandPayload + index);
      index += 4;
      // Decrypt and derive the incomming Key
      sli_zigbee_af_gp_key_tc_lk_derivation(gpdAddr,
                                            0,
                                            mic,
                                            &key,
                                            true);
      sl_zigbee_af_green_power_cluster_println("rx MIC %4x", keyMic);
      sl_zigbee_af_green_power_cluster_print("Calculated MIC :");
      sl_zigbee_af_green_power_cluster_print_buffer(mic, 4, 0);
      sl_zigbee_af_green_power_cluster_print("\nKey : ");
      sl_zigbee_af_green_power_cluster_print_buffer(key.contents, 16, 0);
      sl_zigbee_af_green_power_cluster_println("");
      if (0 != memcmp(mic, (uint8_t*)(&keyMic), 4)) {
        sl_zigbee_af_green_power_cluster_println("DROP - GP CN : Supplied Key MIC mismatch");
        return true;
      }
    }
  }
  // Initialize framecounter with gpdSecurityFrameCounter,
  // then if gpdf has security info pick it from gpd command payload.
  uint32_t outgoingFrameCounter = gpdSecurityFrameCounter;
  if (securityLevel >= SL_ZIGBEE_GP_SECURITY_LEVEL_FC_MIC
      && (gpdfExtendedOptions & SL_ZIGBEE_AF_GP_GPD_COMMISSIONING_EXTENDED_OPTIONS_GPD_OUTGOING_COUNTER_PRESENT)) {
    outgoingFrameCounter = sl_util_fetch_low_high_int32u(gpdCommandPayload + index);
    index += 4;
  }

  if (gpdfOptions & SL_ZIGBEE_AF_GP_GPD_COMMISSIONING_OPTIONS_APPLICATION_INFORMATION_PRESENT) {
    appInfoStruct.applInfoBitmap = gpdCommandPayload[index++];
  }

  if (appInfoStruct.applInfoBitmap & SL_ZIGBEE_AF_GP_APPLICATION_INFORMATION_MANUFACTURE_ID_PRESENT) {
    appInfoStruct.manufacturerId = sl_util_fetch_low_high_int16u(gpdCommandPayload + index);
    index += 2;
  }
  if (appInfoStruct.applInfoBitmap & SL_ZIGBEE_AF_GP_APPLICATION_INFORMATION_MODEL_ID_PRESENT) {
    appInfoStruct.modelId = sl_util_fetch_low_high_int16u(gpdCommandPayload + index);
    index += 2;
  }
  // Parse the Commands and flag the generic command
  bool genericCmdPresent = false;
  if (appInfoStruct.applInfoBitmap & SL_ZIGBEE_AF_GP_APPLICATION_INFORMATION_GPD_COMMANDS_PRESENT) {
    appInfoStruct.numberOfGpdCommands = gpdCommandPayload[index++];
    for (int i = 0; i < appInfoStruct.numberOfGpdCommands; i++) {
      appInfoStruct.gpdCommands[i] = gpdCommandPayload[index];
      if ((appInfoStruct.gpdCommands[i] == SL_ZIGBEE_ZCL_GP_GPDF_8BITS_VECTOR_PRESS)
          || (appInfoStruct.gpdCommands[i] == SL_ZIGBEE_ZCL_GP_GPDF_8BITS_VECTOR_RELEASE)) {
        genericCmdPresent = true;
      }
      index++;
    }
  }
  // Parse the cluster list
  if (appInfoStruct.applInfoBitmap & SL_ZIGBEE_AF_GP_APPLICATION_INFORMATION_CLUSTER_LIST_PRESENT) {
    uint8_t numClusters = gpdCommandPayload[index++];
    appInfoStruct.numberOfGpdServerCluster = numClusters & 0x0F;
    for (int i = 0; i < appInfoStruct.numberOfGpdServerCluster; i++) {
      appInfoStruct.serverClusters[i] = sl_util_fetch_low_high_int16u(gpdCommandPayload + index);
      index += 2;
    }
    appInfoStruct.numberOfGpdClientCluster = numClusters >> 4;
    for (int i = 0; i < appInfoStruct.numberOfGpdClientCluster; i++) {
      appInfoStruct.clientClusters[i] = sl_util_fetch_low_high_int16u(gpdCommandPayload + index);
      index += 2;
    }
  }

  sl_zigbee_gp_switch_information_t switchInformationStruct = { 0 };
  if (appInfoStruct.applInfoBitmap & SL_ZIGBEE_AF_GP_APPLICATION_INFORMATION_SWITCH_INFORMATION_PRESENT) {
    switchInformationStruct.switchInfoLength = gpdCommandPayload[index++];
    switchInformationStruct.nbOfContacts = (gpdCommandPayload[index]
                                            & SL_ZIGBEE_AF_GP_APPLICATION_INFORMATION_SWITCH_INFORMATION_CONFIGURATION_NB_OF_CONTACT);
    switchInformationStruct.switchType = ((gpdCommandPayload[index++]
                                           & SL_ZIGBEE_AF_GP_APPLICATION_INFORMATION_SWITCH_INFORMATION_CONFIGURATION_SWITCH_TYPE)
                                          >> SL_ZIGBEE_AF_GP_APPLICATION_INFORMATION_SWITCH_INFORMATION_CONFIGURATION_SWITCH_TYPE_OFFSET);
    switchInformationStruct.currentContact = gpdCommandPayload[index++];
  }

  if ((appInfoStruct.applInfoBitmap & SL_ZIGBEE_AF_GP_APPLICATION_INFORMATION_GPD_APPLICATION_DESCRIPTION_COMMAND_FOLLOWS)
      && (commNotificationOptions & SL_ZIGBEE_AF_GP_COMMISSIONING_NOTIFICATION_OPTION_RX_AFTER_TX)) {
    sl_zigbee_af_green_power_cluster_println("DROP - GP CN : rxAfterTx and Application Description follows both are set");
    return true;
  }

  if (commissioningGpd->commissionState == GP_SINK_COMM_STATE_COLLECT_REPORTS
      && (commNotificationOptions & SL_ZIGBEE_AF_GP_COMMISSIONING_NOTIFICATION_OPTION_RX_AFTER_TX)) {
    sl_zigbee_af_green_power_cluster_println("DROP - GP CN : rxAfterTx while report collection in progress");
    return true;
  }

  if (index > gpdCommandPayload[0] + 1) {
    sl_zigbee_af_green_power_cluster_println("DROP - GP CN : Short payload");
    //we ran off the end of the payload
    return true;
  }

  sl_zigbee_af_green_power_cluster_println("GP CN : Saving the GPD");
  memcpy(&(commissioningGpd->addr), gpdAddr, sizeof(sl_zigbee_gp_address_t));
  commissioningGpd->applicationInfo.deviceId = appInfoStruct.deviceId;
  commissioningGpd->gpdfOptions = gpdfOptions;
  if (gpdfOptions & SL_ZIGBEE_AF_GP_GPD_COMMISSIONING_OPTIONS_EXTENDED_OPTIONS_FIELD) {
    commissioningGpd->gpdfExtendedOptions = gpdfExtendedOptions;
    commissioningGpd->securityKeyType = securityKeyType;
    commissioningGpd->securityLevel = securityLevel;
    if (gpdfExtendedOptions & SL_ZIGBEE_AF_GP_GPD_COMMISSIONING_EXTENDED_OPTIONS_GPD_KEY_PRESENT) {
      memcpy(commissioningGpd->key.contents, key.contents, SL_ZIGBEE_ENCRYPTION_KEY_SIZE);
    }
  }
  if (gpdfOptions & SL_ZIGBEE_AF_GP_GPD_COMMISSIONING_OPTIONS_APPLICATION_INFORMATION_PRESENT) {
    memcpy(&(commissioningGpd->applicationInfo), &appInfoStruct, sizeof(sl_zigbee_gp_application_info_t));
    if (appInfoStruct.applInfoBitmap & SL_ZIGBEE_AF_GP_APPLICATION_INFORMATION_SWITCH_INFORMATION_PRESENT) {
      memcpy(&(commissioningGpd->switchInformationStruct), &switchInformationStruct, sizeof(sl_zigbee_gp_switch_information_t));
    }
  }
  commissioningGpd->outgoingFrameCounter = outgoingFrameCounter;

  // Use the sinkDefault communication mode as the Gpdf has no option to provide one
  sl_zigbee_af_read_attribute(SL_ZIGBEE_GP_ENDPOINT,
                              ZCL_GREEN_POWER_CLUSTER_ID,
                              ZCL_GP_SERVER_GPS_COMMUNICATION_MODE_ATTRIBUTE_ID,
                              (CLUSTER_MASK_SERVER),
                              (uint8_t*)&(commissioningGpd->communicationMode),
                              sizeof(uint8_t),
                              &type);

  // No assigned alias as the Gpdf has no option to provide one, then call the user to get one.
  commissioningGpd->useGivenAssignedAlias =
    sl_zigbee_af_green_power_server_update_alias_cb(gpdAddr,
                                                    &(commissioningGpd->givenAlias));

  if ((appInfoStruct.applInfoBitmap & SL_ZIGBEE_AF_GP_APPLICATION_INFORMATION_SWITCH_INFORMATION_PRESENT)
      && (appInfoStruct.deviceId == SL_ZIGBEE_GP_DEVICE_ID_GPD_GENERIC_SWITCH
          || genericCmdPresent == true)) {
    // on each reception of a Generic Switch commissioning command start a 60s
    // timeout to collect the PRESS and RELEASE order frames to extend
    // GenericSwitch Translation Table created for this GPD
    sl_zigbee_af_green_power_cluster_println("GP CN : Start Switch Commissioning Timeout");
    uint32_t delay = SL_ZIGBEE_AF_PLUGIN_GREEN_POWER_SERVER_GENERIC_SWITCH_COMMISSIONING_TIMEOUT_IN_SEC;
    sl_zigbee_af_event_set_delay_ms(genericSwitchCommissioningTimeout,
                                    delay * MILLISECOND_TICKS_PER_SECOND);
  }

  if (appInfoStruct.applInfoBitmap & SL_ZIGBEE_AF_GP_APPLICATION_INFORMATION_GPD_APPLICATION_DESCRIPTION_COMMAND_FOLLOWS) {
    if (!sl_zigbee_af_event_is_scheduled(multiSensorCommissioningTimeout)) {
      sl_zigbee_af_green_power_cluster_println("GP CN : Reseting Partial data - Only Report Data");
      resetOfMultisensorDataSaved(false, commissioningGpd);
    }
    sl_zigbee_af_green_power_cluster_println("GP CN : Start MS/CAR Commissioning Timeout and waiting for ADCF");
    uint32_t delay = SL_ZIGBEE_AF_PLUGIN_GREEN_POWER_SERVER_MULTI_SENSOR_COMMISSIONING_TIMEOUT_IN_SEC;
    sl_zigbee_af_event_set_delay_ms(multiSensorCommissioningTimeout,
                                    delay * MILLISECOND_TICKS_PER_SECOND);
    // All set to collect the report descriptors
    commissioningGpd->commissionState = GP_SINK_COMM_STATE_COLLECT_REPORTS;
    return true;
  }
  // Check if bidirectional commissioning, flag the state to finalize the pairing.
  if (commNotificationOptions & SL_ZIGBEE_AF_GP_COMMISSIONING_NOTIFICATION_OPTION_RX_AFTER_TX) {
    // Send Commissioning reply and finalize the sink entry upon reception of success
    commissioningGpd->gppShortAddress = gppShortAddress;
    commissioningGpd->commissionState = GP_SINK_COMM_STATE_SEND_COMM_REPLY;
  } else {
    // No reports to collect AND no commReply to be sent, then finalise pairing
    commissioningGpd->commissionState = GP_SINK_COMM_STATE_FINALISE_PAIRING;
    commissioningGpd->preSinkCbSource = GP_PRE_SINK_PAIRING_CALLBACK_COMMISSONING_FINALIZE;
  }
  // Based on the states the following function either sends a commReply or finalises pairing
  finalisePairing(commissioningGpd);
  return true;
}

static bool gpCommissioningNotificationApplicationDescriptionGpdf(uint16_t commNotificationOptions,
                                                                  uint32_t gpdSecurityFrameCounter,
                                                                  uint8_t *gpdCommandPayload,
                                                                  uint16_t gppShortAddress,
                                                                  sli_zigbee_gp_comm_data_saved_t *commissioningGpd)
{
  if (gpdCommandPayload == NULL) {
    return true;
  }
  if (gpdCommandPayload[0] < 2) { // ADCF payload has minimum 2 entries
    return true;
  }
  static bool lastReportIdHasRxAfterTx = false;
  sl_zigbee_af_green_power_cluster_println("GP CN : Process ADCF");
  if (commissioningGpd->commissionState != GP_SINK_COMM_STATE_COLLECT_REPORTS
      && !lastReportIdHasRxAfterTx) {
    sl_zigbee_af_green_power_cluster_println("ADCF DROP : Not in report collection state (only allow bidirectional frames)");
    return true;
  }
  // Process this command if the MS timer is still running that was started in a commissioning gpdf
  // else reject command or the node has collected the reports just allow the bidirectional frames to pull a
  // commissioning reply out
  if (!sl_zigbee_af_event_is_scheduled(multiSensorCommissioningTimeout)
      && !lastReportIdHasRxAfterTx) {
    sl_zigbee_af_green_power_cluster_println("ADCF DROP : MS timer has expired");
    return true;
  }
  // The first Application description frame received,start the timeout
  if (commissioningGpd->totalNbOfReport == 0) {
    lastReportIdHasRxAfterTx = false;
    // start a timeout to collect the ReportDescriptors thanks to the application description frames
    uint32_t delay =  SL_ZIGBEE_AF_PLUGIN_GREEN_POWER_SERVER_MULTI_SENSOR_COMMISSIONING_TIMEOUT_IN_SEC;
    sl_zigbee_af_event_set_delay_ms(multiSensorCommissioningTimeout,
                                    delay * MILLISECOND_TICKS_PER_SECOND);
  }
  // Collect the report descriptors in RAM with short payload error check
  if (!saveReportDescriptor(gpdCommandPayload[1],    // Total Number of Reports
                            gpdCommandPayload[2],    // Number of reports in this frame
                            &gpdCommandPayload[3],
                            commissioningGpd)) { // Start of report data
    // Error in Application Description payload
    sl_zigbee_af_green_power_cluster_println("ADCF DROP : Error in Report Desc.");
    return true;
  }
  // just remeber if the lastfarme during the report collection has indicated a rx capability when out of order
  if ((commNotificationOptions & SL_ZIGBEE_AF_GP_COMMISSIONING_NOTIFICATION_OPTION_RX_AFTER_TX)
      && sli_zigbee_af_gp_find_report_id((gpdCommandPayload[1] - 1),
                                         gpdCommandPayload[2],
                                         &gpdCommandPayload[3]) != NULL) {
    lastReportIdHasRxAfterTx = true;
  }
  // Check if all the reports received else keep collecting
  if (commissioningGpd->numberOfReports != commissioningGpd->totalNbOfReport) {
    sl_zigbee_af_green_power_cluster_println("ADCF : Wait for % d more reports",
                                             (commissioningGpd->totalNbOfReport - commissioningGpd->totalNbOfReport));
    return true;
  }
  sl_zigbee_af_green_power_cluster_println("ADCF : Collected All the reports -- looking for last reportId");
  // Find the last reportId (= total reports - 1) in this frame it it was decided that it was a bidirectional
  // with rx capability
  if (lastReportIdHasRxAfterTx
      && sli_zigbee_af_gp_find_report_id((gpdCommandPayload[1] - 1),
                                         gpdCommandPayload[2],
                                         &gpdCommandPayload[3]) == NULL) {
    return true;
  }
  sl_zigbee_af_green_power_cluster_println("ADCF : Last reportId found in this frame");
  sl_zigbee_af_event_set_inactive(multiSensorCommissioningTimeout);
  // If this frame with last reportId has the rxAfterTx set send a commissioning reply and the pairing to be finalized
  // in Success
  if (commNotificationOptions & SL_ZIGBEE_AF_GP_COMMISSIONING_NOTIFICATION_OPTION_RX_AFTER_TX) {
    // Send Commissioning reply and finalize the sink entry upon reception of success
    commissioningGpd->gppShortAddress = gppShortAddress;
    commissioningGpd->commissionState = GP_SINK_COMM_STATE_SEND_COMM_REPLY;
  } else {
    commissioningGpd->commissionState = GP_SINK_COMM_STATE_FINALISE_PAIRING;
  }

  finalisePairing(commissioningGpd);
  return true;
}

static bool gpCommissioningNotificationSuccessGpdf(uint16_t commNotificationOptions,
                                                   uint32_t gpdSecurityFrameCounter,
                                                   sli_zigbee_gp_comm_data_saved_t *commissioningGpd)
{
  uint8_t securityLevel = ((commNotificationOptions & SL_ZIGBEE_AF_GP_COMMISSIONING_NOTIFICATION_OPTION_SECURITY_LEVEL)
                           >> SL_ZIGBEE_AF_GP_COMMISSIONING_NOTIFICATION_OPTION_SECURITY_LEVEL_OFFSET);

  if (securityLevel != commissioningGpd->securityLevel) {
    sl_zigbee_af_green_power_cluster_println("DROP - GP CN Success: Sec Level mismatch");
    return true;
  }
  // security level has passed so check the FC
  if (commissioningGpd->outgoingFrameCounter > gpdSecurityFrameCounter) {
    sl_zigbee_af_green_power_cluster_println("DROP - GP CN Success: Frame Counter Too Low");
    return true;
  }
  // Save FC
  commissioningGpd->outgoingFrameCounter = gpdSecurityFrameCounter;
  sl_zigbee_af_green_power_cluster_print("GP CN : Process Success - Finalise Pairing");
  commissioningGpd->commissionState = GP_SINK_COMM_STATE_FINALISE_PAIRING;
  finalisePairing(commissioningGpd);
  return true;
}

static bool gpCommissioningNotificationDecommissioningGpdf(uint16_t commNotificationOptions,
                                                           sl_zigbee_gp_address_t *gpdAddr)
{
  sl_zigbee_af_green_power_cluster_println("Processing Decommissioning");
  uint8_t secLvl = ((commNotificationOptions & SL_ZIGBEE_AF_GP_COMMISSIONING_NOTIFICATION_OPTION_SECURITY_LEVEL)
                    >> SL_ZIGBEE_AF_GP_COMMISSIONING_NOTIFICATION_OPTION_SECURITY_LEVEL_OFFSET);
  uint8_t keyType = ((commNotificationOptions & SL_ZIGBEE_AF_GP_COMMISSIONING_NOTIFICATION_OPTION_SECURITY_KEY_TYPE)
                     >> SL_ZIGBEE_AF_GP_COMMISSIONING_NOTIFICATION_OPTION_SECURITY_KEY_TYPE_OFFSET);
  decommissionGpd(secLvl, keyType, gpdAddr, true, true);
  return true;
}

static void handleChannelRequest(uint16_t options,
                                 uint16_t gppShortAddress,
                                 bool rxAfterTx,
                                 uint8_t *gpdCommandPayload)
{
  if (gpdCommandPayload == NULL) {
    return; // Channel request has always one byte payload
  }
  if (!rxAfterTx) {
    // DROP as the rxAfterTx is not set of this notification
    sl_zigbee_af_green_power_cluster_println("DROP : Channel Request NO - rxAfterTx");
    return;
  }
  //Basic sink just runs with the first one, doesn't select the best
  uint8_t responseOption = options & SL_ZIGBEE_AF_GP_NOTIFICATION_OPTION_APPLICATION_ID; // Application Id
  uint8_t nextChannel = (gpdCommandPayload[1]
                         & SL_ZIGBEE_AF_GP_GPD_CHANNEL_REQUEST_CHANNEL_TOGGLING_BEHAVIOR_RX_CHANNEL_NEXT_ATTEMPT);
  uint8_t channelConfigPayload;
  channelConfigPayload = (sl_zigbee_af_get_radio_channel() - 11) & SL_ZIGBEE_AF_GP_GPD_CHANNEL_CONFIGURATION_CHANNEL_OPERATIONAL_CHANNEL;
  channelConfigPayload |= (1 << SL_ZIGBEE_AF_GP_GPD_CHANNEL_CONFIGURATION_CHANNEL_BASIC_OFFSET); // BASIC
  if (sl_zigbee_af_fill_command_green_power_cluster_gp_response_smart(responseOption,
                                                                      gppShortAddress,
                                                                      nextChannel,
                                                                      0,
                                                                      NULL,
                                                                      0,
                                                                      SL_ZIGBEE_ZCL_GP_GPDF_CHANNEL_CONFIGURATION,
                                                                      sizeof(uint8_t),
                                                                      &channelConfigPayload) == 0) {
    return;
  }

  sl_zigbee_aps_frame_t *apsFrame;
  apsFrame = sl_zigbee_af_get_command_aps_frame();
  apsFrame->sourceEndpoint = SL_ZIGBEE_GP_ENDPOINT;
  apsFrame->destinationEndpoint = SL_ZIGBEE_GP_ENDPOINT;
  if (commissioningState.unicastCommunication) { //  based on the commission mode as decided by sink
    sl_zigbee_af_send_command_unicast(SL_ZIGBEE_OUTGOING_DIRECT, gppShortAddress);
  } else {
    sl_zigbee_af_send_command_broadcast(SL_ZIGBEE_RX_ON_WHEN_IDLE_BROADCAST_ADDRESS, SL_ZIGBEE_NULL_NODE_ID, 0);
  }
}

static uint16_t storeSinkTableEntryInBuffer(sl_zigbee_gp_sink_table_entry_t *entry,
                                            uint8_t *buffer)
{
  uint8_t *finger = buffer;
  uint8_t securityLevel = (entry->securityOptions & SL_ZIGBEE_AF_GP_SINK_TABLE_ENTRY_SECURITY_OPTIONS_SECURITY_LEVEL);

  sl_zigbee_af_green_power_cluster_println("GP SERVER - STORE SINK TABLE ENTRY into a buffer");
  // copy options field
  sl_zigbee_af_copy_int16u(finger, 0, (uint16_t)(entry->options & SL_ZIGBEE_AF_GP_SINK_TABLE_ENTRY_OPTIONS_MASK));
  finger += sizeof(uint16_t);
  // copy 32bit or 64bit address field
  if (entry->gpd.applicationId == SL_ZIGBEE_GP_APPLICATION_SOURCE_ID) {
    sl_zigbee_af_copy_int32u(finger, 0, entry->gpd.id.sourceId);
    finger += sizeof(uint32_t);
  } else if (entry->gpd.applicationId == SL_ZIGBEE_GP_APPLICATION_IEEE_ADDRESS) {
    memmove(finger, entry->gpd.id.gpdIeeeAddress, EUI64_SIZE);
    finger += EUI64_SIZE;
  }
  // copy ieee endpoint field
  if (entry->gpd.applicationId == SL_ZIGBEE_GP_APPLICATION_IEEE_ADDRESS) {
    sl_zigbee_af_copy_int8u(finger, 0, entry->gpd.endpoint);
    finger += sizeof(uint8_t);
  }
  // DeviceID field
  sl_zigbee_af_green_power_cluster_println("GPD deviceId %x", entry->deviceId);
  sl_zigbee_af_copy_int8u(finger, 0, entry->deviceId);
  finger += sizeof(uint8_t);

  // copy Group List field if present
  sl_zigbee_gp_sink_type_t sinkCommunicationMode = (sl_zigbee_gp_sink_type_t)((entry->options
                                                                               & SL_ZIGBEE_AF_GP_SINK_TABLE_ENTRY_OPTIONS_COMMUNICATION_MODE)
                                                                              >> SL_ZIGBEE_AF_GP_SINK_TABLE_ENTRY_OPTIONS_COMMUNICATION_MODE_OFFSET);
  if (sinkCommunicationMode == SL_ZIGBEE_GP_SINK_TYPE_GROUPCAST) {
    // let's count
    uint8_t index = 0;
    uint8_t *entryCount = finger;
    sl_zigbee_af_copy_int8u(finger, 0, 0x00);
    finger += sizeof(uint8_t);
    for (index = 0; index < GP_SINK_LIST_ENTRIES; index++) {
      // table is available
      sl_zigbee_gp_sink_list_entry_t *sinkEntry = &entry->sinkList[index];
      if (sinkEntry->type == SL_ZIGBEE_GP_SINK_TYPE_GROUPCAST) {
        sl_zigbee_af_copy_int16u(finger, 0, sinkEntry->target.groupcast.groupID);
        finger += sizeof(uint16_t);
        sl_zigbee_af_copy_int16u(finger, 0, sinkEntry->target.groupcast.alias);
        finger += sizeof(uint16_t);
        (*entryCount)++;
      } else {
        continue;
      }
    }
  }
  // copy GPD assigned alias field
  if (entry->options & SL_ZIGBEE_AF_GP_SINK_TABLE_ENTRY_OPTIONS_ASSIGNED_ALIAS) {
    sl_zigbee_af_green_power_cluster_println("assigned alias %2x", entry->assignedAlias);
    sl_zigbee_af_copy_int16u(finger, 0, entry->assignedAlias);
    finger += sizeof(uint16_t);
  }
  // copy Groupcast radius field
  sl_zigbee_af_copy_int8u(finger, 0, entry->groupcastRadius);
  finger += sizeof(uint8_t);

  // copy Security Options field
  if (entry->options & SL_ZIGBEE_AF_GP_SINK_TABLE_ENTRY_OPTIONS_SECURITY_USE) {
    sl_zigbee_af_green_power_cluster_println("security options %1x", entry->securityOptions);
    sl_zigbee_af_copy_int8u(finger, 0, entry->securityOptions);
    finger += sizeof(uint8_t);

    // copy GPD Sec Frame Counter (if lvl>0 or if SeqNumCapability=0b1)
    // Lvl>0 is check cause it is allow to set SecurityUse=0b1 and tell SecLvl=0
    if ( (securityLevel > SL_ZIGBEE_GP_SECURITY_LEVEL_NONE)
         || (entry->options & SL_ZIGBEE_AF_GP_SINK_TABLE_ENTRY_OPTIONS_SEQUENCE_NUM_CAPABILITIES) ) {
      sl_zigbee_af_green_power_cluster_println("security frame counter %4x", entry->gpdSecurityFrameCounter);
      sl_zigbee_af_copy_int32u(finger, 0, entry->gpdSecurityFrameCounter);
      finger += sizeof(uint32_t);
      if ( !(securityLevel == SL_ZIGBEE_GP_SECURITY_LEVEL_NONE) ) {
        memmove(finger, entry->gpdKey.contents, SL_ZIGBEE_ENCRYPTION_KEY_SIZE);
        finger += SL_ZIGBEE_ENCRYPTION_KEY_SIZE;
      }
    }
  } else if (entry->options & SL_ZIGBEE_AF_GP_SINK_TABLE_ENTRY_OPTIONS_SEQUENCE_NUM_CAPABILITIES) {
    // gpdSecurityFrameCounter is mandatory if Security use = 0b1 (treated above)
    //      or Sequence number capabilities = 0b1 and Security use = 0b0
    sl_zigbee_af_green_power_cluster_println("security frame counter %4x", entry->gpdSecurityFrameCounter);
    sl_zigbee_af_copy_int32u(finger, 0, entry->gpdSecurityFrameCounter);
    finger += sizeof(uint32_t);
  } else {
    // if no security and no seq number capability, nothing more to copy into the response
  }
  return (uint16_t)(finger - buffer);
}

static bool processCommNotificationsWithSecurityProcessingFailedFlag(uint16_t commNotificationOptions,
                                                                     sl_zigbee_gp_address_t *gpdAddr,
                                                                     uint32_t gpdSecurityFrameCounter,
                                                                     uint8_t *gpdCommandId,
                                                                     uint8_t *gpdCommandPayload,
                                                                     uint32_t commissioningNotificationMic,
                                                                     uint8_t securityKeyType)
{
  sl_zigbee_af_green_power_cluster_println("\nGP CN Security Processing Failed is set");
  // MIC is only present if security processing failed is set, so validate MIC
  uint8_t mic[4] = { 0 };
  uint8_t secLevel = (commNotificationOptions & SL_ZIGBEE_AF_GP_COMMISSIONING_NOTIFICATION_OPTION_SECURITY_LEVEL)
                     >> SL_ZIGBEE_AF_GP_COMMISSIONING_NOTIFICATION_OPTION_SECURITY_LEVEL_OFFSET;
  bool securityProcessing = sli_zigbee_af_gp_calculate_incoming_command_mic(gpdAddr,
                                                                            ((commNotificationOptions & SL_ZIGBEE_AF_GP_COMMISSIONING_NOTIFICATION_OPTION_RX_AFTER_TX) ? true : false),
                                                                            ((securityKeyType == SL_ZIGBEE_ZCL_GP_SECURITY_KEY_TYPE_INDIVIDIGUAL_GPD_KEY
                                                                              || securityKeyType == SL_ZIGBEE_ZCL_GP_SECURITY_KEY_TYPE_DERIVED_INDIVIDUAL_GPD_KEY) \
                                                                             ? SL_ZIGBEE_AF_GREEN_POWER_GP_INDIVIDUAL_KEY : SL_ZIGBEE_AF_GREEN_POWER_GP_SHARED_KEY),
                                                                            secLevel,
                                                                            gpdSecurityFrameCounter,
                                                                            *gpdCommandId,
                                                                            gpdCommandPayload,
                                                                            ((secLevel > SL_ZIGBEE_GP_SECURITY_LEVEL_FC_MIC) ? true : false),
                                                                            mic);
  sl_zigbee_af_green_power_cluster_print("\n GP CN Sec Level = %d, App Id = %d, MIC Validation : %p"
                                         "\n GP CN Calculated Mic : %4x"
                                         "\n GP CN Received Mic   : %4x\n",
                                         secLevel,
                                         gpdAddr->applicationId,
                                         (securityProcessing ? "SUCCESS" : "FAILED"),
                                         (*(uint32_t*)mic),
                                         commissioningNotificationMic);
  if (!securityProcessing
      || ((*(uint32_t*)mic) != commissioningNotificationMic)) {
    sl_zigbee_af_green_power_cluster_println("DROP - GP CN MIC Mismatch");
    sl_zigbee_af_green_power_server_gpd_security_failure_cb(gpdAddr);
    return false;
  }

  if (SL_ZIGBEE_GP_SECURITY_LEVEL_FC_MIC_ENCRYPTED == secLevel) {
    // Temp Payload to be decrypted without payloadLengthe in between in the form of : {commandId,{command payload}}
    uint8_t payload[GP_COMMISSIONING_MAX_BYTES + 1] = { 0 };
    payload[0] = *gpdCommandId;
    uint8_t length = 1;
    if ((gpdCommandPayload != NULL)
        && (gpdCommandPayload[0] < GP_COMMISSIONING_MAX_BYTES)) {
      memcpy((payload + 1), gpdCommandPayload + 1, gpdCommandPayload[0]);
      length += gpdCommandPayload[0];
    }
    sli_zigbee_af_gp_calculate_incoming_command_decrypt(gpdAddr,
                                                        gpdSecurityFrameCounter,
                                                        length,
                                                        payload);
    // Copy back the decrypted command with octate string payload format.
    *gpdCommandId = payload[0];
    if ((gpdCommandPayload != NULL)
        && (gpdCommandPayload[0] < GP_COMMISSIONING_MAX_BYTES)) {
      memcpy((gpdCommandPayload + 1), (payload + 1), gpdCommandPayload[0]);
    }
  }
  return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Global Functions
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
const uint8_t *sli_zigbee_af_gp_find_report_id(uint8_t reportId,
                                               uint8_t numberOfReports,
                                               const uint8_t *reports)
{
  const uint8_t *temp = reports;
  uint8_t length = 0; // 0 - position of reportId
  for (uint8_t noOfReport = 0; noOfReport < numberOfReports; noOfReport++) {
    // calculate the length of each report in the payload
    if (*temp == reportId) {
      return temp;
    }
    length = reportLength(temp);
    temp = temp + length;
  }
  return NULL;
}

// Add the endpoint to APS group with GroupId
sl_zigbee_af_status_t sli_zigbee_af_gp_add_to_aps_group(uint8_t endpoint, uint16_t groupId)
{
  if (0xFF != findGroupInBindingTable(endpoint, groupId)) {
    // search returned an valid index, so duplicate exists
    return SL_ZIGBEE_ZCL_STATUS_SUCCESS; // per ZCL8; was DUPLICATE_EXISTS
  }
  // No duplicate entry, try adding
  // Look for an empty binding slot.
  for (uint8_t i = 0; i < SL_ZIGBEE_BINDING_TABLE_SIZE; i++) {
    sl_zigbee_binding_table_entry_t binding;
    if (sl_zigbee_get_binding(i, &binding) == SL_STATUS_OK
        && binding.type == SL_ZIGBEE_UNUSED_BINDING) {
      binding.type = SL_ZIGBEE_MULTICAST_BINDING;
      binding.identifier[0] = LOW_BYTE(groupId);
      binding.identifier[1] = HIGH_BYTE(groupId);
      binding.local = endpoint;

      sl_status_t status = sl_zigbee_set_binding(i, &binding);
      if (status == SL_STATUS_OK) {
        return SL_ZIGBEE_ZCL_STATUS_SUCCESS;
      } else {
        sl_zigbee_af_core_println("ERR: Failed to create binding (0x%x)", status);
        return SL_ZIGBEE_ZCL_STATUS_FAILURE;
      }
    }
  }
  sl_zigbee_af_core_println("ERR: Binding table is full");
  return SL_ZIGBEE_ZCL_STATUS_INSUFFICIENT_SPACE;
}

// Allow the sink to decided if it match with the GP frame it received
bool sli_zigbee_af_gp_endpoint_and_cluster_id_validation(uint8_t endpoint,
                                                         bool server,
                                                         sl_zigbee_af_cluster_id_t clusterId)
{
  if (isCommissioningAppEndpoint(endpoint)
      && ((server && sl_zigbee_af_contains_server(endpoint, clusterId))
          || (!server && sl_zigbee_af_contains_client(endpoint, clusterId)))) {
    return true;
  }
  return false;
}

bool sli_zigbee_af_green_power_server_retrieve_attribute_and_craft_response(uint8_t endpoint,
                                                                            sl_zigbee_af_cluster_id_t clusterId,
                                                                            sl_zigbee_af_attribute_id_t attrId,
                                                                            uint8_t mask,
                                                                            uint16_t manufacturerCode,
                                                                            uint16_t readLength)
{
  uint8_t sinkTableEntryAppResponseData[SL_ZIGBEE_AF_RESPONSE_BUFFER_LEN];
  uint8_t zclStatus = SL_ZIGBEE_ZCL_STATUS_SUCCESS;
  uint16_t stringDataOffsetStart = 0;
  uint16_t stringLength = 0;
  bool    status = false;

  if (endpoint != SL_ZIGBEE_GP_ENDPOINT
      || clusterId != ZCL_GREEN_POWER_CLUSTER_ID
      || attrId != ZCL_GP_SERVER_SINK_TABLE_ATTRIBUTE_ID
      || mask != CLUSTER_MASK_SERVER
      || manufacturerCode != SL_ZIGBEE_AF_NULL_MANUFACTURER_CODE) {
    // do nothing but return false
  } else if (readLength < 6) {
    sl_zigbee_af_green_power_cluster_println("ERROR, Buffer length supplied %d is too small", readLength);
    // Can't fit the ZCL header in available length so exit with error
    // do nothing but return true as we are processing the correct type of packet
  } else {
    (void) sl_zigbee_af_put_int16u_in_resp(attrId);
    // The sink table attribute is a long string ZCL attribute type, which means
    // it is encoded starting with a 2-byte length. We fill in the real length
    // after we have encoded the whole sink table.
    // Four bytes extra = 2byte length + 1 byte ZCl Status + 1 byte Attribute Type
    uint16_t stringDataOffset =  appResponseLength + 4;
    stringDataOffsetStart = stringDataOffset;
    // Search the sink table and respond with entries
    for (uint8_t i = 0; i < SL_ZIGBEE_GP_SINK_TABLE_SIZE; i++) {
      sl_zigbee_gp_sink_table_entry_t entry;
      if (sl_zigbee_gp_sink_table_get_entry(i, &entry) == SL_STATUS_OK) {
        sl_zigbee_af_green_power_cluster_println("Craft Response - Encode Sink Table %d", i);
        // Have a valid entry so encode response in temp buffer and add if it fits
        uint16_t sinkTableEntryLength = storeSinkTableEntryInBuffer(&entry, sinkTableEntryAppResponseData);
        if ((sinkTableEntryLength + stringDataOffset) > readLength) {
          // String is too big so
          zclStatus = SL_ZIGBEE_ZCL_STATUS_INSUFFICIENT_SPACE;
          sl_zigbee_af_green_power_cluster_println("Sink Table Attribute read INSUFFICIENT SPACE");
          break;
        } else {
          sl_zigbee_af_green_power_cluster_println("SAVE ENTRY %d", i);
          memmove(&appResponseData[stringDataOffset], sinkTableEntryAppResponseData, sinkTableEntryLength);
          stringDataOffset += sinkTableEntryLength;
        }
      }
    }
    // calculate string length
    stringLength = stringDataOffset - stringDataOffsetStart;
    if (zclStatus == SL_ZIGBEE_ZCL_STATUS_SUCCESS) {
      (void) sl_zigbee_af_put_int8u_in_resp(zclStatus);
      (void) sl_zigbee_af_put_int8u_in_resp(ZCL_LONG_OCTET_STRING_ATTRIBUTE_TYPE);
      (void) sl_zigbee_af_put_int16u_in_resp(stringLength);
      sl_zigbee_af_green_power_cluster_println(" calculated string length = %d", (stringDataOffsetStart - stringDataOffset));
    } else {
      (void) sl_zigbee_af_put_int8u_in_resp(zclStatus);
    }
    appResponseLength += stringLength;
    status = true;
  }
  sl_zigbee_af_green_power_cluster_println(" String length = %d ", stringLength);
  return status;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Green Power Server Commissioning Event Handlers.
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Switch commissioning timeout handler
void sl_zigbee_af_green_power_server_generic_switch_commissioning_timeout_event_handler(sl_zigbee_af_event_t * event)
{
  // stop the delay
  sl_zigbee_af_event_set_inactive(genericSwitchCommissioningTimeout);
  sl_zigbee_af_green_power_server_commissioning_timeout_cb(COMMISSIONING_TIMEOUT_TYPE_GENERIC_SWITCH,
                                                           noOfCommissionedEndpoints,
                                                           commissionedEndPoints);
  resetOfMultisensorDataSaved(true, &gpdCommDataSaved[0]);
}

// MS commissioning timeout handler
void sl_zigbee_af_green_power_server_multi_sensor_commissioning_timeout_event_handler(sl_zigbee_af_event_t * event)
{
  // stop the delay
  sl_zigbee_af_event_set_inactive(multiSensorCommissioningTimeout);
  sl_zigbee_af_green_power_server_commissioning_timeout_cb(COMMISSIONING_TIMEOUT_TYPE_MULTI_SENSOR,
                                                           noOfCommissionedEndpoints,
                                                           commissionedEndPoints);
  // reset RAM data saved concerning multisensor
  resetOfMultisensorDataSaved(true, &gpdCommDataSaved[0]);
}

// Green Power Server Commissioning Window
void sl_zigbee_af_green_power_server_commissioning_window_timeout_event_handler(sl_zigbee_af_event_t * event)
{
  sl_zigbee_af_event_set_inactive(commissioningWindowTimeout);
  sl_zigbee_af_green_power_server_commissioning_timeout_cb(COMMISSIONING_TIMEOUT_TYPE_COMMISSIONING_WINDOW_TIMEOUT,
                                                           noOfCommissionedEndpoints,
                                                           commissionedEndPoints);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Green Power Server Initialisation and stack status callbacks.
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

void sl_zigbee_af_green_power_server_sink_table_init(void)
{
  sl_zigbee_af_green_power_cluster_println("SinkTable Init..");
  sl_zigbee_gp_sink_table_init();
  greenPowerServerInitialised = true;
}

void sl_zigbee_af_green_power_server_init_cb(uint8_t init_level)
{
  switch (init_level) {
    case SL_ZIGBEE_INIT_LEVEL_EVENT:
    {
      sl_zigbee_af_event_init(genericSwitchCommissioningTimeout,
                              sl_zigbee_af_green_power_server_generic_switch_commissioning_timeout_event_handler);
      sl_zigbee_af_event_init(multiSensorCommissioningTimeout,
                              sl_zigbee_af_green_power_server_multi_sensor_commissioning_timeout_event_handler);
      sl_zigbee_af_event_init(commissioningWindowTimeout,
                              sl_zigbee_af_green_power_server_commissioning_window_timeout_event_handler);
      break;
    }

    case SL_ZIGBEE_INIT_LEVEL_DONE:
    {
      // Init GP Sink Table
#ifdef EZSP_HOST
      if (SL_ZIGBEE_EZSP_SUCCESS != sl_zigbee_ezsp_get_configuration_value(SL_ZIGBEE_EZSP_CONFIG_GP_SINK_TABLE_SIZE, (uint16_t*)&gpSinkTableSize)) {
        sl_zigbee_af_green_power_cluster_println("ERR: Cannot get the sink table size from GP stack.");
      }
#endif //EZSP_HOST
      sl_zigbee_af_green_power_server_sink_table_init();
      break;
    }
  }
}

WEAK(void sl_zigbee_af_green_power_server_stack_status_cb(sl_status_t status))
{
  sl_zigbee_af_green_power_cluster_println("Green Power Server Stack Status Callback status = %x", status);
  if (status == SL_STATUS_NETWORK_DOWN
      && sl_zigbee_is_performing_rejoin() == FALSE) {
    sl_zigbee_gp_sink_table_clear_all();
    greenPowerServerInitialised = false;
  }
  if (!sl_zigbee_af_green_power_server_update_involve_t_c_cb(status)) {
    updateInvolveTC(status);
  }
  if (status == SL_STATUS_NETWORK_UP) {
    sl_zigbee_af_green_power_server_sink_table_init();
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Following Section has all the command callback implementation.
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// The callbacks are called fromt he call-command-handler.c, which is an autogenerated module baased on the
// device configuration.
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool sli_zigbee_af_green_power_server_gp_sink_commissioning_mode_command_handler(uint8_t options,
                                                                                 uint16_t gpmAddrForSecurity,
                                                                                 uint16_t gpmAddrForPairing,
                                                                                 uint8_t sinkEndpoint)
{
  // Test 4..4.6 - not a sink ep or bcast ep - drop
  if (!isValidAppEndpoint(sinkEndpoint)) {
    sl_zigbee_af_green_power_cluster_println("DROP - Comm Mode Callback: Sink EP not supported");
    // 3.3.4.8.2
    defaultResponseFailureStatus = SL_ZIGBEE_ZCL_STATUS_NOT_FOUND;
    return false;
  }
  if ((options & SL_ZIGBEE_AF_GP_SINK_COMMISSIONING_MODE_OPTIONS_INVOLVE_GPM_IN_SECURITY)
      || (options & SL_ZIGBEE_AF_GP_SINK_COMMISSIONING_MODE_OPTIONS_INVOLVE_GPM_IN_PAIRING)) {
    return false;
  }
  sl_zigbee_af_attribute_type_t type;
  uint8_t gpsSecurityLevelAttribute = 0;
  sl_zigbee_af_status_t secLevelStatus = sl_zigbee_af_read_attribute(SL_ZIGBEE_GP_ENDPOINT,
                                                                     ZCL_GREEN_POWER_CLUSTER_ID,
                                                                     ZCL_GP_SERVER_GPS_SECURITY_LEVEL_ATTRIBUTE_ID,
                                                                     (CLUSTER_MASK_SERVER),
                                                                     (uint8_t*)&gpsSecurityLevelAttribute,
                                                                     sizeof(uint8_t),
                                                                     &type);
  // Reject the req if InvolveTC is et in the attribute
  if (secLevelStatus == SL_ZIGBEE_ZCL_STATUS_SUCCESS
      && (gpsSecurityLevelAttribute & 0x08)) {
    return false;
  }
  uint16_t commissioningWindow = 0;
  uint8_t proxyOptions = 0;

  if (options & SL_ZIGBEE_AF_GP_SINK_COMMISSIONING_MODE_OPTIONS_ACTION) {
    commissioningState.inCommissioningMode = true;
    //enter commissioning mode
    // 3.3.4.8.2
    // If the GPM address for security or pairing is different from 0xffff
    // or if any involve GPM in pairing bit is set
    // the default response status should be set to INVALID_VALUE
    if ( gpmAddrForSecurity != 0xffff
         || gpmAddrForPairing  != 0xffff
         || (options
             & (SL_ZIGBEE_AF_GP_SINK_COMMISSIONING_MODE_OPTIONS_INVOLVE_GPM_IN_SECURITY
                | SL_ZIGBEE_AF_GP_SINK_COMMISSIONING_MODE_OPTIONS_INVOLVE_GPM_IN_PAIRING))) {
      //these SHALL be 0 for now
      //TODO also check involve-TC
      defaultResponseFailureStatus = SL_ZIGBEE_ZCL_STATUS_INVALID_VALUE;
      commissioningState.inCommissioningMode = false;
      return false;
    }
    // default 180s of GP specification
    commissioningWindow = SL_ZIGBEE_AF_ZCL_CLUSTER_GP_GPS_COMMISSIONING_WINDOWS_DEFAULT_TIME_S;

    uint8_t gpsCommissioningExitMode;
    uint16_t gpsCommissioningWindows;
    sl_zigbee_af_status_t statusExitMode = sl_zigbee_af_read_attribute(SL_ZIGBEE_GP_ENDPOINT,
                                                                       ZCL_GREEN_POWER_CLUSTER_ID,
                                                                       ZCL_GP_SERVER_GPS_COMMISSIONING_EXIT_MODE_ATTRIBUTE_ID,
                                                                       CLUSTER_MASK_SERVER,
                                                                       (uint8_t*)&gpsCommissioningExitMode,
                                                                       sizeof(uint8_t),
                                                                       &type);
    sl_zigbee_af_status_t statusCommWindow = sl_zigbee_af_read_attribute(SL_ZIGBEE_GP_ENDPOINT,
                                                                         ZCL_GREEN_POWER_CLUSTER_ID,
                                                                         ZCL_GP_SERVER_GPS_COMMISSIONING_WINDOW_ATTRIBUTE_ID,
                                                                         CLUSTER_MASK_SERVER,
                                                                         (uint8_t*)&gpsCommissioningWindows,
                                                                         sizeof(uint16_t),
                                                                         &type);
    proxyOptions = SL_ZIGBEE_AF_GP_PROXY_COMMISSIONING_MODE_OPTION_ACTION;

    // set commissioningWindow based on "gpsCommissioningWindows" attribute if different from
    // commissioningWindow will be part of the frame if it is different from default 180s
    // in any case there is a timeout running into the GPPs to exit,
    // the default one (180s) not include or the one include in the ProxyCommMode(Enter) frame
    if (statusCommWindow == SL_ZIGBEE_ZCL_STATUS_SUCCESS
        && (gpsCommissioningExitMode
            & SL_ZIGBEE_AF_GP_SINK_COMMISSIONING_MODE_EXIT_MODE_ON_COMMISSIONING_WINDOW_EXPIRATION)) {
      commissioningWindow = gpsCommissioningWindows;
      proxyOptions |= SL_ZIGBEE_AF_GP_PROXY_COMMISSIONING_MODE_EXIT_MODE_ON_COMMISSIONING_WINDOW_EXPIRATION;
    }
    sl_zigbee_af_event_set_delay_ms(commissioningWindowTimeout,
                                    commissioningWindow * MILLISECOND_TICKS_PER_SECOND);
    // Only one of the 2 next flag shall be set at the same time
    if (statusExitMode == SL_ZIGBEE_ZCL_STATUS_SUCCESS
        && (gpsCommissioningExitMode
            & SL_ZIGBEE_AF_GP_SINK_COMMISSIONING_MODE_EXIT_MODE_ON_FIRST_PAIRING_SUCCESS)
        && !(gpsCommissioningExitMode
             & SL_ZIGBEE_AF_GP_SINK_COMMISSIONING_MODE_EXIT_MODE_ON_GP_PROXY_COMMISSIONING_MODE_EXIT)) {
      proxyOptions |= SL_ZIGBEE_AF_GP_PROXY_COMMISSIONING_MODE_EXIT_MODE_ON_FIRST_PAIRING_SUCCESS;
    } else if ((statusExitMode == SL_ZIGBEE_ZCL_STATUS_SUCCESS)
               && (gpsCommissioningExitMode
                   & SL_ZIGBEE_AF_GP_SINK_COMMISSIONING_MODE_EXIT_MODE_ON_GP_PROXY_COMMISSIONING_MODE_EXIT)
               && !(gpsCommissioningExitMode
                    & SL_ZIGBEE_AF_GP_SINK_COMMISSIONING_MODE_EXIT_MODE_ON_FIRST_PAIRING_SUCCESS)) {
      proxyOptions |= SL_ZIGBEE_AF_GP_PROXY_COMMISSIONING_MODE_EXIT_MODE_ON_GP_PROXY_COMMISSIONING_MODE_EXIT;
    } else {
      // In case both are cleared or both are set, nothing more to do
    }
  } else if (commissioningState.inCommissioningMode) {
    // If commissioning mode is ON and received frame set it to OFF, exit commissioning mode
    commissioningState.inCommissioningMode = false;
    // Deactive the timer
    sl_zigbee_af_event_set_inactive(commissioningWindowTimeout);
    if (commissioningState.proxiesInvolved) {
      //involve proxies
      proxyOptions = 0;
    }
  }

  if (options & SL_ZIGBEE_AF_GP_SINK_COMMISSIONING_MODE_OPTIONS_INVOLVE_PROXIES) {
    commissioningState.proxiesInvolved = true;
  } else {
    commissioningState.proxiesInvolved = false;
  }
  commissioningState.endpoint = sinkEndpoint; // The commissioning endpoint can be 0xFF as well

  // On commissioning entry or exist, reset the number of report config received
  for (int i = 0; i < SL_ZIGBEE_AF_PLUGIN_GREEN_POWER_SERVER_COMMISSIONING_GPD_INSTANCES; i++) {
    resetOfMultisensorDataSaved(true, &gpdCommDataSaved[i]);
  }
  if (commissioningState.unicastCommunication) { //  based on the commission mode as decided by sink
    proxyOptions |= 0x20; // Flag unicast communication
  }
  if (sl_zigbee_af_fill_command_green_power_cluster_gp_proxy_commissioning_mode_smart(proxyOptions,
                                                                                      commissioningWindow,
                                                                                      0) == 0) {
    defaultResponseFailureStatus = SL_ZIGBEE_ZCL_STATUS_INSUFFICIENT_SPACE;
    return false;
  }

  sl_zigbee_aps_frame_t *apsFrame;
  apsFrame = sl_zigbee_af_get_command_aps_frame();
  apsFrame->sourceEndpoint = SL_ZIGBEE_GP_ENDPOINT;
  apsFrame->destinationEndpoint = SL_ZIGBEE_GP_ENDPOINT;
  if (commissioningState.proxiesInvolved) {
    sl_status_t status = sl_zigbee_af_send_command_broadcast(SL_ZIGBEE_RX_ON_WHEN_IDLE_BROADCAST_ADDRESS, SL_ZIGBEE_NULL_NODE_ID, 0);
    // Callback to inform the status of message submission to network
    sl_zigbee_af_green_power_cluster_commissioning_message_status_notification_cb(&commissioningState,
                                                                                  apsFrame,
                                                                                  SL_ZIGBEE_OUTGOING_BROADCAST,
                                                                                  SL_ZIGBEE_RX_ON_WHEN_IDLE_BROADCAST_ADDRESS,
                                                                                  status);
  } else {
    #ifdef SL_CATALOG_ZIGBEE_GREEN_POWER_CLIENT_PRESENT
    // Put the proxy instance on this node commissioning mode so that it can accept a pairing from itself.
    // This is to ensure the node will be able to handle gpdf commands after pairig.
    sl_status_t status = sl_zigbee_af_send_command_unicast(SL_ZIGBEE_OUTGOING_DIRECT, sl_zigbee_af_get_node_id());
    // Callback to inform the status of message submission to network
    sl_zigbee_af_green_power_cluster_commissioning_message_status_notification_cb(&commissioningState,
                                                                                  apsFrame,
                                                                                  SL_ZIGBEE_OUTGOING_DIRECT,
                                                                                  sl_zigbee_af_get_node_id(),
                                                                                  status);
    #endif // SL_CATALOG_ZIGBEE_GREEN_POWER_CLIENT_PRESENT
  }
  return true;
}

//Green Power Cluster Gp Notification
bool sl_zigbee_af_green_power_cluster_gp_notification_cb(sl_zigbee_af_cluster_command_t *cmd)
{
  sl_zcl_green_power_cluster_gp_notification_command_t cmd_data;

  if (zcl_decode_green_power_cluster_gp_notification_command(cmd, &cmd_data)
      != SL_ZIGBEE_ZCL_STATUS_SUCCESS) {
    return false;
  }

  // If the green power server initialisation has not been called,
  // do not process a Gp Notification.
  if (!greenPowerServerInitialised) {
    return true;
  }

  if (cmd_data.gpdCommandPayload == NULL) {
    // add a 0 length ZCL OCTATE string type
    cmd_data.gpdCommandPayload = zeroLengthZclOctateString;
  }

  sl_zigbee_gp_address_t gpdAddr;
  if (!sli_zigbee_af_gp_make_addr(&gpdAddr,
                                  (cmd_data.options & SL_ZIGBEE_AF_GP_NOTIFICATION_OPTION_APPLICATION_ID),
                                  cmd_data.gpdSrcId,
                                  cmd_data.gpdIeee,
                                  cmd_data.gpdEndpoint)) {
    return true; // Handled, but dropped because of the bad GPD addressing
  }
  if ((gpdAddrZero(&gpdAddr)
       && (cmd_data.gpdCommandId != SL_ZIGBEE_ZCL_GP_GPDF_CHANNEL_REQUEST))
      || (!gpdAddrZero(&gpdAddr)
          && (cmd_data.gpdCommandId == SL_ZIGBEE_ZCL_GP_GPDF_CHANNEL_REQUEST))) {
    // Address 0 only for channel request command.
    return true;
  }

  if (cmd_data.gpdCommandId == SL_ZIGBEE_ZCL_GP_GPDF_CHANNEL_REQUEST) {
    handleChannelRequest(cmd_data.options,
                         cmd_data.gppShortAddress,
                         ((cmd_data.options & SL_ZIGBEE_AF_GP_NOTIFICATION_OPTION_RX_AFTER_TX) ? true : false),
                         cmd_data.gpdCommandPayload);
    return true;
  }
  sl_zigbee_af_green_power_cluster_println("command %d", cmd_data.gpdCommandId);
  //if (cmd_data.gpdCommandPayload != NULL) { // Ensure gpdCommandPayload is not NULL to print the payload
  //  sl_zigbee_af_green_power_cluster_print("payload: ");
  //  sl_zigbee_af_green_power_cluster_print_buffer(&cmd_data.gpdCommandPayload[1],
  //                                      cmd_data.gpdCommandPayload[0],
  //                                      true);
  //  sl_zigbee_af_green_power_cluster_println("");
  //}

  // Find the sink entry and update the security frame counter from gpd
  uint8_t sinkIndex = sl_zigbee_gp_sink_table_lookup(&gpdAddr);
  if (sinkIndex != 0xFF) {
    sl_zigbee_gp_sink_table_entry_t entry = { 0 };
    sl_status_t status = sl_zigbee_gp_sink_table_get_entry(sinkIndex, &entry);
    // GPD Security validation, if fails drop!
    if (status == SL_STATUS_OK) {
      uint8_t receivedSecLevel = ((cmd_data.options & SL_ZIGBEE_AF_GP_NOTIFICATION_OPTION_SECURITY_LEVEL)
                                  >> SL_ZIGBEE_AF_GP_NOTIFICATION_OPTION_SECURITY_LEVEL_OFFSET);
      uint8_t receivedKeyType = ((cmd_data.options & SL_ZIGBEE_AF_GP_NOTIFICATION_OPTION_SECURITY_KEY_TYPE)
                                 >> SL_ZIGBEE_AF_GP_NOTIFICATION_OPTION_SECURITY_KEY_TYPE_OFFSET);
      uint8_t sinkSecLevel = (entry.securityOptions & SL_ZIGBEE_AF_GP_SINK_TABLE_ENTRY_SECURITY_OPTIONS_SECURITY_LEVEL);
      uint8_t sinkKeyType = ((entry.securityOptions & SL_ZIGBEE_AF_GP_SINK_TABLE_ENTRY_SECURITY_OPTIONS_SECURITY_KEY_TYPE)
                             >> SL_ZIGBEE_AF_GP_SINK_TABLE_ENTRY_SECURITY_OPTIONS_SECURITY_KEY_TYPE_OFFSET);
      if (sinkSecLevel > 0) {
        if (sinkSecLevel > receivedSecLevel
            || sinkKeyType != receivedKeyType
            || entry.gpdSecurityFrameCounter >= cmd_data.gpdSecurityFrameCounter) {
          // DROP
          sl_zigbee_af_green_power_cluster_println("Gp Notif : DROP - SecLevel, Key type or framecounter mismatch");
          return true;
        }
      }
    }
    sl_zigbee_gp_sink_table_set_security_frame_counter(sinkIndex, cmd_data.gpdSecurityFrameCounter);
  }

  if (cmd_data.gpdCommandId == SL_ZIGBEE_ZCL_GP_GPDF_COMMISSIONING) {
    return true; // Drop Commissioning Command - Test 4.4.2.8 Step 5
  }

  if (cmd_data.gpdCommandId == SL_ZIGBEE_ZCL_GP_GPDF_DECOMMISSIONING) {
    decommissionGpd(((cmd_data.options & SL_ZIGBEE_AF_GP_NOTIFICATION_OPTION_SECURITY_LEVEL) >> SL_ZIGBEE_AF_GP_NOTIFICATION_OPTION_SECURITY_LEVEL_OFFSET),
                    ((cmd_data.options & SL_ZIGBEE_AF_GP_NOTIFICATION_OPTION_SECURITY_KEY_TYPE) >> SL_ZIGBEE_AF_GP_NOTIFICATION_OPTION_SECURITY_KEY_TYPE_OFFSET),
                    &gpdAddr,
                    true,
                    true);
    return true;
  }

  // Call user first to give a chance to handle the notification.
  if (sl_zigbee_af_green_power_cluster_gp_notification_forward_cb(cmd_data.options,
                                                                  &gpdAddr,
                                                                  cmd_data.gpdSecurityFrameCounter,
                                                                  cmd_data.gpdCommandId,
                                                                  cmd_data.gpdCommandPayload,
                                                                  cmd_data.gppShortAddress,
                                                                  cmd_data.gppDistance)) {
    return true;
  }
  // Check if sink has the translation table support in the gpsFunctionality attribute?
  if (sinkFunctionalitySupported(SL_ZIGBEE_AF_GP_GPS_FUNCTIONALITY_TRANSLATION_TABLE)) {
    #ifdef SL_CATALOG_ZIGBEE_GREEN_POWER_TRANSLATION_TABLE_PRESENT
    sli_zigbee_af_gp_forward_gpd_command_based_on_translation_table(&gpdAddr,
                                                                    cmd_data.gpdCommandId,
                                                                    cmd_data.gpdCommandPayload);
    #else
    sli_zigbee_af_gp_forward_gpd_command_default(&gpdAddr,
                                                 cmd_data.gpdCommandId,
                                                 cmd_data.gpdCommandPayload);
    #endif // SL_CATALOG_ZIGBEE_GREEN_POWER_TRANSLATION_TABLE_PRESENT
  }
  return true;
}

//Green Power Cluster Gp Commissioning Notification
bool sl_zigbee_af_green_power_cluster_gp_commissioning_notification_cb(sl_zigbee_af_cluster_command_t *cmd)
{
  sl_zcl_green_power_cluster_gp_commissioning_notification_command_t cmd_data;

  if (zcl_decode_green_power_cluster_gp_commissioning_notification_command(cmd, &cmd_data)
      != SL_ZIGBEE_ZCL_STATUS_SUCCESS) {
    return false;
  }

  if (!(commissioningState.inCommissioningMode)) {
    sl_zigbee_af_green_power_cluster_println("DROP - GP CN : Sink not in commissioning!");
    return true;
  }

  if (cmd_data.gpdCommandPayload == NULL) {
    // add a 0 length ZCL OCTATE string type. 1st byte holds the length of rest of the octate string.
    cmd_data.gpdCommandPayload = zeroLengthZclOctateString;
  }

  // Bad address with respect to application type - return back
  sl_zigbee_gp_address_t gpdAddr;
  if (!sli_zigbee_af_gp_make_addr(&gpdAddr,
                                  (cmd_data.options & SL_ZIGBEE_AF_GP_COMMISSIONING_NOTIFICATION_OPTION_APPLICATION_ID),
                                  cmd_data.gpdSrcId,
                                  cmd_data.gpdIeee,
                                  cmd_data.endpoint)) {
    return true;
  }

  if ((gpdAddrZero(&gpdAddr)
       && (cmd_data.gpdCommandId != SL_ZIGBEE_ZCL_GP_GPDF_CHANNEL_REQUEST))
      || (!gpdAddrZero(&gpdAddr)
          && (cmd_data.gpdCommandId == SL_ZIGBEE_ZCL_GP_GPDF_CHANNEL_REQUEST))) {
    // Address 0 only for channel request command.
    return true;
  }
  sli_zigbee_gp_comm_data_saved_t *commissioningGpd = sl_zigbee_af_green_power_server_find_commissioning_gpd_instance(&gpdAddr);
  // When the security processing failed sub-field is set, try validating. It can only be validated if there are
  // keys sent by GPD in a previous commissioning message which will be temporary collected in the commissioningGpd
  // instance.
  if (cmd_data.options & SL_ZIGBEE_AF_GP_COMMISSIONING_NOTIFICATION_OPTION_SECURITY_PROCESSING_FAILED) {
    if (commissioningGpd == NULL) {
      // In this case, the commissioning notification can not be processed because:
      // It is a protected message without earlier information of its security keys.
      return true;
    }
    if (commissioningGpd != NULL) {
      import_load_zb_sec_man_internal_key((sl_zigbee_sec_man_key_t*)&commissioningGpd->key);
      // Attempt to process the secure GPDF using its earlier credentials
      if (!processCommNotificationsWithSecurityProcessingFailedFlag(cmd_data.options,
                                                                    &gpdAddr,
                                                                    cmd_data.gpdSecurityFrameCounter,
                                                                    &cmd_data.gpdCommandId,
                                                                    cmd_data.gpdCommandPayload,
                                                                    cmd_data.mic,
                                                                    commissioningGpd->securityKeyType)) {
        return true;
      }
    }
  }

  int8_t rssi = 0;
  uint8_t linkQuality = 0;
  uint8_t gppDistance = 0;
  if (cmd_data.options & SL_ZIGBEE_AF_GP_COMMISSIONING_NOTIFICATION_OPTION_RX_AFTER_TX ) {
    gppDistance = cmd_data.gppLink;
  } else {
    gppGpdLinkUnpack(cmd_data.gppLink, &rssi, &linkQuality);
  }

  if (sl_zigbee_af_green_power_server_gpd_commissioning_notification_cb(cmd_data.gpdCommandId,
                                                                        cmd_data.options,
                                                                        &gpdAddr,
                                                                        cmd_data.gpdSecurityFrameCounter,
                                                                        cmd_data.gpdCommandId,
                                                                        cmd_data.gpdCommandPayload,
                                                                        cmd_data.gppShortAddress,
                                                                        rssi,
                                                                        linkQuality,
                                                                        gppDistance,
                                                                        cmd_data.mic)) {
    // User application handled
    return true;
  }
  // If the Application did not handle the notification through the above callback
  // The plugin handles it from here.
  if (cmd_data.gpdCommandId == SL_ZIGBEE_ZCL_GP_GPDF_DECOMMISSIONING) {
    gpCommissioningNotificationDecommissioningGpdf(cmd_data.options,
                                                   &gpdAddr);
  } else if (cmd_data.gpdCommandId == SL_ZIGBEE_ZCL_GP_GPDF_CHANNEL_REQUEST) {
    handleChannelRequest(cmd_data.options,
                         cmd_data.gppShortAddress,
                         ((cmd_data.options & SL_ZIGBEE_AF_GP_COMMISSIONING_NOTIFICATION_OPTION_RX_AFTER_TX) ? true : false),
                         cmd_data.gpdCommandPayload);
  } else if (cmd_data.gpdCommandId == SL_ZIGBEE_ZCL_GP_GPDF_COMMISSIONING) {
    if (commissioningGpd == NULL) { // there was no previous temporary transaction, its a new GPD.
      commissioningGpd = allocateCommissioningGpdInstance(&gpdAddr); // Allocate one instance if available.
      if (commissioningGpd == NULL) {
        return true;
      }
    }
    gpCommissioningNotificationCommissioningGpdf(cmd_data.options,
                                                 &gpdAddr,
                                                 cmd_data.gpdSecurityFrameCounter,
                                                 cmd_data.gpdCommandId,
                                                 cmd_data.gpdCommandPayload,
                                                 cmd_data.gppShortAddress,
                                                 commissioningGpd);
  } else {
    // All other commands can only be processed if they have an instance else not.
    if (commissioningGpd == NULL) {
      return true;
    }
    if (cmd_data.gpdCommandId == SL_ZIGBEE_ZCL_GP_GPDF_APPLICATION_DESCRIPTION) {
      gpCommissioningNotificationApplicationDescriptionGpdf(cmd_data.options,
                                                            cmd_data.gpdSecurityFrameCounter,
                                                            cmd_data.gpdCommandPayload,
                                                            cmd_data.gppShortAddress,
                                                            commissioningGpd);
    } else {
      // Success or any other GPDF needs a GPD instance to be already present to proceed.
      // Any valid command that can be processed should behave as success
      gpCommissioningNotificationSuccessGpdf(cmd_data.options,
                                             cmd_data.gpdSecurityFrameCounter,
                                             commissioningGpd);
    }
  }
  return true;
}

// Commissioning Mode callback
bool sl_zigbee_af_green_power_cluster_gp_sink_commissioning_mode_cb(sl_zigbee_af_cluster_command_t *cmd)
{
  sl_zcl_green_power_cluster_gp_sink_commissioning_mode_command_t cmd_data;

  if (zcl_decode_green_power_cluster_gp_sink_commissioning_mode_command(cmd, &cmd_data)
      != SL_ZIGBEE_ZCL_STATUS_SUCCESS) {
    return false;
  }

  return sli_zigbee_af_green_power_server_gp_sink_commissioning_mode_command_handler(cmd_data.options,
                                                                                     cmd_data.gpmAddrForSecurity,
                                                                                     cmd_data.gpmAddrForPairing,
                                                                                     cmd_data.sinkEndpoint);
}

#ifdef SL_CATALOG_ZIGBEE_GREEN_POWER_CLIENT_PRESENT
bool sli_zigbee_af_green_power_client_gp_proxy_commissioning_mode_command_handler(uint8_t options,
                                                                                  uint16_t commissioningWindow,
                                                                                  uint8_t channel,
                                                                                  bool localCommandLoopback);
#endif // SL_CATALOG_ZIGBEE_GREEN_POWER_CLIENT_PRESENT

bool sl_zigbee_af_green_power_cluster_gp_pairing_configuration_cb(sl_zigbee_af_cluster_command_t *cmd)
{
  sl_zcl_green_power_cluster_gp_pairing_configuration_command_t cmd_data;

  if (zcl_decode_green_power_cluster_gp_pairing_configuration_command(cmd, &cmd_data)
      != SL_ZIGBEE_ZCL_STATUS_SUCCESS) {
    return false;
  }

  if (cmd->source == sl_zigbee_get_node_id()) {
    // Silent Drop : Loopback message - return true to ensure no action from the framework.
    return true;
  }

  uint8_t gpdAppId = (cmd_data.options & SL_ZIGBEE_AF_GP_PAIRING_CONFIGURATION_OPTION_APPLICATION_ID);
  sl_zigbee_gp_address_t gpdAddr;
  if (!sli_zigbee_af_gp_make_addr(&gpdAddr,
                                  gpdAppId,
                                  cmd_data.gpdSrcId,
                                  cmd_data.gpdIeee,
                                  cmd_data.endpoint)) {
    return true; // Silent Drop : Address not valid.
  }
  // Silent Drop for the gpd addr = 0
  if (gpdAddrZero(&gpdAddr)) {
    sl_zigbee_af_green_power_cluster_println("DROP - GP Pairing Config : GPD Address is 0!");
    return true;
  }
  if ((cmd_data.options & SL_ZIGBEE_AF_GP_PAIRING_CONFIGURATION_OPTION_SECURITY_USE)
      && ((cmd_data.securityOptions & SL_ZIGBEE_AF_GP_SINK_TABLE_ENTRY_SECURITY_OPTIONS_SECURITY_LEVEL) == SL_ZIGBEE_GP_SECURITY_LEVEL_RESERVED)) {
    sl_zigbee_af_green_power_cluster_println("DROP - GP Pairing Config : Security Level reserved !");
    return true;
  }

  uint8_t gpConfigAtion = (cmd_data.actions & SL_ZIGBEE_AF_GP_PAIRING_CONFIGURATION_ACTIONS_ACTION);

  // Action 0b000 (No Action)
  // Input(s) - SendGpPairingBit , Gpd Address
  if (gpConfigAtion == SL_ZIGBEE_ZCL_GP_PAIRING_CONFIGURATION_ACTION_NO_ACTION) {
    sendGpPairingLookingUpAddressInSinkEntry(&gpdAddr,
                                             (cmd_data.actions & SL_ZIGBEE_AF_GP_PAIRING_CONFIGURATION_ACTIONS_SEND_GP_PAIRING));
    return true;
  }
  // Action = 0b100 (Remove GPD)
  // Input(s) - Gpd Address
  if (gpConfigAtion == SL_ZIGBEE_ZCL_GP_PAIRING_CONFIGURATION_ACTION_REMOVE_GPD) {
    decommissionGpd(0, 0, &gpdAddr, true, cmd_data.actions & SL_ZIGBEE_AF_GP_PAIRING_CONFIGURATION_ACTIONS_SEND_GP_PAIRING);
    return true;
  }
  // Action = 0b011 (Remove Pairing)
  // Input(s) - Gpd Address, Communication Mode, GroupList, Number Of Paired Eps , Paired Ep List
  //            Application Information, Switch Information, Additional Block Information
  uint8_t gpPairingConfigCommunicationMode = (cmd_data.options
                                              & SL_ZIGBEE_AF_GP_PAIRING_CONFIGURATION_OPTION_COMMUNICATION_MODE)
                                             >> SL_ZIGBEE_AF_GP_PAIRING_CONFIGURATION_OPTION_COMMUNICATION_MODE_OFFSET;
  if (gpConfigAtion == SL_ZIGBEE_ZCL_GP_PAIRING_CONFIGURATION_ACTION_REMOVE_A_PAIRING) {
    if (((gpdAddr.applicationId == SL_ZIGBEE_GP_APPLICATION_SOURCE_ID)
         && gpdAddr.id.sourceId == GP_ADDR_SRC_ID_WILDCARD)
        || ((gpdAddr.applicationId == SL_ZIGBEE_GP_APPLICATION_IEEE_ADDRESS)
            && sl_zigbee_af_memory_byte_compare(gpdAddr.id.gpdIeeeAddress, EUI64_SIZE, 0xFF))) {
      // TODO: apply action to all GPD with this particular applicationID (SrcId or IEEE)
      return true;
    }

    uint8_t sinkEntryIndex = sl_zigbee_gp_sink_table_lookup(&gpdAddr);
    if (sinkEntryIndex != 0xFF) {
      sl_zigbee_gp_sink_table_entry_t entry = { 0 };
      if (sl_zigbee_gp_sink_table_get_entry(sinkEntryIndex, &entry) != SL_STATUS_OK) {
        // return if entry not found
        return true;
      } else {
        uint8_t gpsCommunicationMode = (entry.options
                                        & SL_ZIGBEE_AF_GP_PAIRING_CONFIGURATION_OPTION_COMMUNICATION_MODE)
                                       >> SL_ZIGBEE_AF_GP_PAIRING_CONFIGURATION_OPTION_COMMUNICATION_MODE_OFFSET;
        // return if communication mode mismatch
        if (gpPairingConfigCommunicationMode != gpsCommunicationMode) {
          return true;
        }

        // return if communication mode is matched as group cast
        // but the group ID mismatch
        if (gpPairingConfigCommunicationMode == SL_ZIGBEE_GP_SINK_TYPE_GROUPCAST) {
          if (cmd_data.groupListCount == 0 || cmd_data.groupList == NULL) {
            return true;
          }
          for (uint8_t index = 0, pos = 1;
               index < GP_SINK_LIST_ENTRIES && pos < ((cmd_data.groupList[0] * sizeof(sl_zigbee_gp_sink_group_t)) + 1);
               index++) {
            sl_zigbee_gp_sink_group_t gpPairingConfigGroupID = { 0 };
            memcpy(&gpPairingConfigGroupID, &(cmd_data.groupList[pos]), sizeof(sl_zigbee_gp_sink_group_t));
            pos += sizeof(sl_zigbee_gp_sink_group_t);
            if (entry.sinkList[index].target.groupcast.groupID != gpPairingConfigGroupID.groupID) {
              return true;
            }
          }
        }
      }
      decommissionGpd(0, 0, &gpdAddr, false, cmd_data.actions & SL_ZIGBEE_AF_GP_PAIRING_CONFIGURATION_ACTIONS_SEND_GP_PAIRING);
    }
    return true;
  }
  // All other command actions would need a temporary storage
  sli_zigbee_gp_comm_data_saved_t *commissioningGpd = findOrAllocateCommissioningGpdInstance(&gpdAddr);
  if (commissioningGpd == NULL) {
    return true;
  }
  memcpy(&(commissioningGpd->addr), &gpdAddr, sizeof(sl_zigbee_gp_address_t));

  // Save the grouplist for group cast communication mode, that eventually gets saved into the sink table upon succesfull pairing.
  if (gpPairingConfigCommunicationMode == SL_ZIGBEE_GP_SINK_TYPE_GROUPCAST
      && cmd_data.groupListCount
      && cmd_data.groupList) {
    commissioningGpd->groupList[0] = cmd_data.groupListCount > GP_SINK_LIST_ENTRIES ? GP_SINK_LIST_ENTRIES : cmd_data.groupListCount;
    memcpy(&commissioningGpd->groupList[1], cmd_data.groupList, commissioningGpd->groupList[0] * sizeof(sl_zigbee_gp_sink_group_t));
  }

  // Save the grouplist from the existing entry, in case there is availability
  uint8_t sinkEntryIndex = sl_zigbee_gp_sink_table_lookup(&gpdAddr);
  if (sinkEntryIndex != 0xFF) {
    sl_zigbee_gp_sink_table_entry_t entry = { 0 };
    if (sl_zigbee_gp_sink_table_get_entry(sinkEntryIndex, &entry) == SL_STATUS_OK) {
      if (cmd_data.groupListCount < GP_SINK_LIST_ENTRIES) {
        for (uint8_t index = 0; commissioningGpd->groupList[0] < GP_SINK_LIST_ENTRIES; index++) {
          uint8_t pos = commissioningGpd->groupList[0] * sizeof(sl_zigbee_gp_sink_group_t) + 1;
          if (entry.sinkList[index].type == SL_ZIGBEE_GP_SINK_TYPE_GROUPCAST) {
            memcpy(&commissioningGpd->groupList[pos], &entry.sinkList[index].target.groupcast, sizeof(sl_zigbee_gp_sink_group_t));
            commissioningGpd->groupList[0]++;
          }
        }
      }
    }
  }

  bool appInformationPresent = (cmd_data.options & SL_ZIGBEE_AF_GP_PAIRING_CONFIGURATION_OPTION_APPLICATION_INFORMATION_PRESENT)
                               ? true : false;
  if (appInformationPresent) { // application info present
    commissioningGpd->applicationInfo.applInfoBitmap = cmd_data.applicationInformation;
    if (cmd_data.applicationInformation & SL_ZIGBEE_AF_GP_APPLICATION_INFORMATION_MANUFACTURE_ID_PRESENT) { // ManufactureId Present
      commissioningGpd->applicationInfo.manufacturerId = cmd_data.manufacturerId;
    }
    if (cmd_data.applicationInformation & SL_ZIGBEE_AF_GP_APPLICATION_INFORMATION_MODEL_ID_PRESENT) { // ModelId Present
      commissioningGpd->applicationInfo.modelId = cmd_data.modeId;
    }
    if (cmd_data.applicationInformation & SL_ZIGBEE_AF_GP_APPLICATION_INFORMATION_GPD_COMMANDS_PRESENT) {   // gpd Commands present
      commissioningGpd->applicationInfo.numberOfGpdCommands = cmd_data.numberOfGpdCommands;
      memcpy(commissioningGpd->applicationInfo.gpdCommands,
             cmd_data.gpdCommandIdList,
             cmd_data.numberOfGpdCommands);
    }
    if (cmd_data.applicationInformation & SL_ZIGBEE_AF_GP_APPLICATION_INFORMATION_CLUSTER_LIST_PRESENT) { // Cluster List present
      commissioningGpd->applicationInfo.numberOfGpdClientCluster = cmd_data.clusterIdListCount >> 4;
      memcpy(commissioningGpd->applicationInfo.clientClusters,
             cmd_data.clusterListClient,
             (commissioningGpd->applicationInfo.numberOfGpdClientCluster * 2));
      commissioningGpd->applicationInfo.numberOfGpdServerCluster = cmd_data.clusterIdListCount & 0x0f;
      memcpy(commissioningGpd->applicationInfo.serverClusters,
             cmd_data.clusterListServer,
             (commissioningGpd->applicationInfo.numberOfGpdServerCluster * 2));
    }
    if (cmd_data.applicationInformation & SL_ZIGBEE_AF_GP_APPLICATION_INFORMATION_SWITCH_INFORMATION_PRESENT) { // Switch Information
      commissioningGpd->switchInformationStruct.switchInfoLength = cmd_data.switchInformationLength;
      commissioningGpd->switchInformationStruct.nbOfContacts = (cmd_data.switchConfiguration
                                                                & SL_ZIGBEE_AF_GP_APPLICATION_INFORMATION_SWITCH_INFORMATION_CONFIGURATION_NB_OF_CONTACT);
      commissioningGpd->switchInformationStruct.switchType = (cmd_data.switchConfiguration
                                                              & SL_ZIGBEE_AF_GP_APPLICATION_INFORMATION_SWITCH_INFORMATION_CONFIGURATION_SWITCH_TYPE)
                                                             >> SL_ZIGBEE_AF_GP_APPLICATION_INFORMATION_SWITCH_INFORMATION_CONFIGURATION_SWITCH_TYPE_OFFSET;
      commissioningGpd->switchInformationStruct.currentContact = cmd_data.currentContactStatus;

      // Start the generic switch timeout
      uint32_t delay = SL_ZIGBEE_AF_PLUGIN_GREEN_POWER_SERVER_GENERIC_SWITCH_COMMISSIONING_TIMEOUT_IN_SEC;
      sl_zigbee_af_event_set_delay_ms(genericSwitchCommissioningTimeout,
                                      delay * MILLISECOND_TICKS_PER_SECOND);
    }
  }

  bool assignedAllias = (cmd_data.options & SL_ZIGBEE_AF_GP_PAIRING_CONFIGURATION_OPTION_ASSIGNED_ALIAS) ? true : false;
  if (assignedAllias) {
    commissioningGpd->useGivenAssignedAlias = true;
    commissioningGpd->givenAlias = cmd_data.gpdAssignedAlias;
  }

  // Always present in the GpPairingConfig payload
  commissioningGpd->applicationInfo.deviceId = cmd_data.deviceId;
  commissioningGpd->applicationInfo.numberOfPairedEndpoints = cmd_data.numberOfPairedEndpoints;
  if (cmd_data.numberOfPairedEndpoints < 0xFD) {
    memcpy(commissioningGpd->applicationInfo.pairedEndpoints,
           cmd_data.pairedEndpoints,
           cmd_data.numberOfPairedEndpoints);
  }

  bool securityUse = (cmd_data.options & SL_ZIGBEE_AF_GP_PAIRING_CONFIGURATION_OPTION_SECURITY_USE) ? true : false;
  if (securityUse) {
    uint8_t gpdfExtendedOptions = 0;
    commissioningGpd->securityLevel = (cmd_data.securityOptions & SL_ZIGBEE_AF_GP_SINK_TABLE_ENTRY_SECURITY_OPTIONS_SECURITY_LEVEL);
    commissioningGpd->securityKeyType = (cmd_data.securityOptions & SL_ZIGBEE_AF_GP_SINK_TABLE_ENTRY_SECURITY_OPTIONS_SECURITY_KEY_TYPE)
                                        >> SL_ZIGBEE_AF_GP_SINK_TABLE_ENTRY_SECURITY_OPTIONS_SECURITY_KEY_TYPE_OFFSET;
    gpdfExtendedOptions = commissioningGpd->securityLevel
                          | (commissioningGpd->securityKeyType << SL_ZIGBEE_AF_GP_GPD_COMMISSIONING_EXTENDED_OPTIONS_KEY_TYPE_OFFSET);
    gpdfExtendedOptions |= SL_ZIGBEE_AF_GP_GPD_COMMISSIONING_EXTENDED_OPTIONS_GPD_KEY_PRESENT;
    gpdfExtendedOptions |= ((commissioningGpd->securityLevel == 3)
                            ? SL_ZIGBEE_AF_GP_GPD_COMMISSIONING_EXTENDED_OPTIONS_GPD_KEY_ENCRYPTION : 0);
    memcpy(commissioningGpd->key.contents, cmd_data.gpdSecurityKey, SL_ZIGBEE_ENCRYPTION_KEY_SIZE);
    commissioningGpd->gpdfExtendedOptions |= gpdfExtendedOptions;
  }

  bool seqNumberCapability = (cmd_data.options & SL_ZIGBEE_AF_GP_PAIRING_CONFIGURATION_OPTION_SEQUENCE_NUMBER_CAPABILITIES) ? true : false;
  if (securityUse
      || (!securityUse
          && seqNumberCapability)) {
    commissioningGpd->outgoingFrameCounter = cmd_data.gpdSecurityFrameCounter; // save/update the frame counter
  }

  bool rxOnCapability = (cmd_data.options & SL_ZIGBEE_AF_GP_PAIRING_CONFIGURATION_OPTION_RX_ON_CAPABILITY) ? true : false;
  bool fixedLocation = (cmd_data.options & SL_ZIGBEE_AF_GP_PAIRING_CONFIGURATION_OPTION_FIXED_LOCATION) ? true : false;
  uint8_t gpdfOptions = (seqNumberCapability ? SL_ZIGBEE_AF_GP_GPD_COMMISSIONING_OPTIONS_MAC_SEQ_NUM_CAP : 0)
                        | (rxOnCapability ? SL_ZIGBEE_AF_GP_GPD_COMMISSIONING_OPTIONS_RX_ON_CAP : 0)
                        | (appInformationPresent ? SL_ZIGBEE_AF_GP_GPD_COMMISSIONING_OPTIONS_APPLICATION_INFORMATION_PRESENT : 0)
                        | (fixedLocation ? SL_ZIGBEE_AF_GP_GPD_COMMISSIONING_OPTIONS_FIXED_LOCATION : 0)
                        | (securityUse ? SL_ZIGBEE_AF_GP_GPD_COMMISSIONING_OPTIONS_EXTENDED_OPTIONS_FIELD : 0);
  commissioningGpd->gpdfOptions |= gpdfOptions;
  commissioningGpd->groupcastRadius = cmd_data.groupcastRadius;
  uint8_t gpPairingConfigSecurityLevel = (cmd_data.securityOptions
                                          & SL_ZIGBEE_AF_GP_SINK_TABLE_ENTRY_SECURITY_OPTIONS_SECURITY_LEVEL);

  uint8_t gpsSecurityLevelAttribute = 0;
  sl_zigbee_af_status_t UNUSED status;
  sl_zigbee_af_attribute_type_t type;
  status = sl_zigbee_af_read_attribute(SL_ZIGBEE_GP_ENDPOINT,
                                       ZCL_GREEN_POWER_CLUSTER_ID,
                                       ZCL_GP_SERVER_GPS_SECURITY_LEVEL_ATTRIBUTE_ID,
                                       (CLUSTER_MASK_SERVER),
                                       (uint8_t*)&gpsSecurityLevelAttribute,
                                       sizeof(uint8_t),
                                       &type);

  gpsSecurityLevelAttribute = gpsSecurityLevelAttribute & 0x03; // mask the encryption part
  if ((cmd_data.groupListCount == 0xFF || cmd_data.groupList == NULL)
      || (sinkFunctionalitySupported(SL_ZIGBEE_AF_GP_GPS_FUNCTIONALITY_SINK_TABLE_BASED_GROUPCAST_FORWARDING)
          || sinkFunctionalitySupported(SL_ZIGBEE_AF_GP_GPS_FUNCTIONALITY_PRE_COMMISSIONED_GROUPCAST_COMMUNICATION))) {
    if (cmd->type != SL_ZIGBEE_INCOMING_BROADCAST
        && gpPairingConfigSecurityLevel == SL_ZIGBEE_GP_SECURITY_LEVEL_RESERVED) {
      sl_zigbee_af_send_immediate_default_response(SL_ZIGBEE_ZCL_STATUS_FAILURE);
      return true;
    }
    // Action = 0b001
    // Action = 0b010
    if ((gpConfigAtion == SL_ZIGBEE_ZCL_GP_PAIRING_CONFIGURATION_ACTION_EXTEND_SINK_TABLE_ENTRY
         || gpConfigAtion == SL_ZIGBEE_ZCL_GP_PAIRING_CONFIGURATION_ACTION_REPLACE_SINK_TABLE_ENTRY)
        && (gpPairingConfigSecurityLevel != SL_ZIGBEE_GP_SECURITY_LEVEL_RESERVED
            && (gpPairingConfigSecurityLevel >= gpsSecurityLevelAttribute
                && sli_zigbee_af_gp_check_communication_mode_support(gpPairingConfigCommunicationMode)))) {
      sl_zigbee_af_green_power_cluster_println("Replace or Extend Sink");
      sl_zigbee_af_green_power_server_remove_sink_entry(&gpdAddr);
      commissioningGpd->communicationMode = gpPairingConfigCommunicationMode;
      // Set/reset the doNotSendGpPairing flag to be used when finaliing paring, Note : the negation.
      commissioningGpd->doNotSendGpPairing = ((cmd_data.actions & SL_ZIGBEE_AF_GP_PAIRING_CONFIGURATION_ACTIONS_SEND_GP_PAIRING)
                                              ? false : true);
      if ((commissioningGpd->applicationInfo.applInfoBitmap & SL_ZIGBEE_AF_GP_APPLICATION_INFORMATION_GPD_APPLICATION_DESCRIPTION_COMMAND_FOLLOWS)) {
        if (!sl_zigbee_af_event_is_scheduled(multiSensorCommissioningTimeout)) {
          sl_zigbee_af_green_power_cluster_println("Gp PairingConfig : Reseting Partial data - Only Report Data");
          resetOfMultisensorDataSaved(false, commissioningGpd);
        }
        sl_zigbee_af_green_power_cluster_println("Gp PairingConfig : Start MS/CAR Commissioning Timeout and waiting for ADCF");
        uint32_t delay = SL_ZIGBEE_AF_PLUGIN_GREEN_POWER_SERVER_MULTI_SENSOR_COMMISSIONING_TIMEOUT_IN_SEC;
        sl_zigbee_af_event_set_delay_ms(multiSensorCommissioningTimeout,
                                        delay * MILLISECOND_TICKS_PER_SECOND);
        // All set to collect the report descriptors
        commissioningGpd->commissionState = GP_SINK_COMM_STATE_COLLECT_REPORTS;
        return true;
      }
      // Replace or Extend Sink with new application information, then create TT entries as well - if the application description is not following
      // Makes proxy on this node enter commissioning so that it can creat a proxy entry when Gp Pairing is heard
      if (!(commissioningState.inCommissioningMode)) {
        // The sink is not in commissioning mode for the CT based commissioning.
        // The endpoints are supplied by the CT, hence the sink need to match all the endpoints on it.
        commissioningState.endpoint = GREEN_POWER_SERVER_ALL_SINK_ENDPOINTS;
      #ifdef SL_CATALOG_ZIGBEE_GREEN_POWER_CLIENT_PRESENT
        sli_zigbee_af_green_power_client_gp_proxy_commissioning_mode_command_handler((SL_ZIGBEE_AF_GP_PROXY_COMMISSIONING_MODE_OPTION_ACTION // Enter Commissioning
                                                                                      | SL_ZIGBEE_AF_GP_PROXY_COMMISSIONING_MODE_EXIT_MODE_ON_FIRST_PAIRING_SUCCESS),        // Exit on First pairing
                                                                                     0,        // No Commissioning window
                                                                                     0,        // No channel present
                                                                                     true);
      #endif // SL_CATALOG_ZIGBEE_GREEN_POWER_CLIENT_PRESENT
      }
      commissioningGpd->commissionState = GP_SINK_COMM_STATE_FINALISE_PAIRING;
      commissioningGpd->preSinkCbSource = GP_PRE_SINK_PAIRING_CALLBACK_PAIRING_CONFIGURATION;
      finalisePairing(commissioningGpd);
      return true;
    }
    // Action = 0b101
    if (gpConfigAtion == SL_ZIGBEE_ZCL_GP_PAIRING_CONFIGURATION_ACTION_APPLICATION_DESCRIPTION) {
      // if total number of report is 0 then it is the first gp pairing
      if (commissioningGpd->totalNbOfReport == 0) {
        // first sensor to follow, reset structure for the report descriptors
        resetOfMultisensorDataSaved(false, commissioningGpd);
        memcpy(&commissioningGpd->addr, &gpdAddr, sizeof(sl_zigbee_gp_address_t));
        commissioningGpd->totalNbOfReport = cmd_data.totalNumberOfReports;
        commissioningGpd->commissionState = GP_SINK_COMM_STATE_COLLECT_REPORTS;
      }
      // if the timer is running then process else just drop
      if (!sl_zigbee_af_event_is_scheduled(multiSensorCommissioningTimeout)) {
        sl_zigbee_af_green_power_cluster_println("Gp PairingConfig App Description - DROP : MS timer has expired");
        return false;
      }
      if (commissioningGpd->commissionState != GP_SINK_COMM_STATE_COLLECT_REPORTS) {
        // not in right state
        return false;
      }
      if (!saveReportDescriptor(cmd_data.totalNumberOfReports,
                                cmd_data.numberOfReports,
                                cmd_data.reportDescriptor,
                                commissioningGpd)) {
        // error in the application desc payload
        return false;
      }
      if (commissioningGpd->numberOfReports != commissioningGpd->totalNbOfReport) {
        // still to collect more reports
        return true;
      }
      // Report collection is over - finalise the pairing
      commissioningGpd->commissionState = GP_SINK_COMM_STATE_FINALISE_PAIRING;
      commissioningGpd->preSinkCbSource = GP_PRE_SINK_PAIRING_CALLBACK_PAIRING_CONFIGURATION;
      finalisePairing(commissioningGpd);
      return true;
    }
  }
  return true;
}

bool sl_zigbee_af_green_power_cluster_gp_sink_table_request_cb(sl_zigbee_af_cluster_command_t *cmd)
{
  sl_zcl_green_power_cluster_gp_sink_table_request_command_t cmd_data;

  if (zcl_decode_green_power_cluster_gp_sink_table_request_command(cmd, &cmd_data)
      != SL_ZIGBEE_ZCL_STATUS_SUCCESS) {
    return false;
  }

  uint8_t entryIndex = 0;
  uint8_t appId = (cmd_data.options & SL_ZIGBEE_AF_GP_SINK_TABLE_REQUEST_OPTIONS_APPLICATION_ID);
  uint8_t requestType = (cmd_data.options & SL_ZIGBEE_AF_GP_SINK_TABLE_REQUEST_OPTIONS_REQUEST_TYPE)
                        >> SL_ZIGBEE_AF_GP_SINK_TABLE_REQUEST_OPTIONS_REQUEST_TYPE_OFFSET;
  sl_zigbee_gp_sink_table_entry_t entry;

  sl_zigbee_af_green_power_cluster_println("Got sink table request with options %1x and index %1x", cmd_data.options, cmd_data.index);
  // only respond to unicast messages.
  if (cmd->type != SL_ZIGBEE_INCOMING_UNICAST) {
    sl_zigbee_af_green_power_cluster_println("Not unicast");
    goto kickout;
  }

  // the device SHALL check if it implements a Sink Table.
  // If not, it SHALL generate a ZCL Default Response command,
  // with the Status code field carrying UNSUP_COMMAND, subject to the rules as specified in sec. 2.4.12 of [3]
  if (SL_ZIGBEE_GP_SINK_TABLE_SIZE == 0) {
    sl_zigbee_af_green_power_cluster_println("Unsup cluster command");
    sl_zigbee_af_send_immediate_default_response(SL_ZIGBEE_ZCL_STATUS_UNSUP_COMMAND);
    return true;
  }

  if (sl_zigbee_af_current_endpoint() != SL_ZIGBEE_GP_ENDPOINT) {
    sl_zigbee_af_green_power_cluster_println("Drop frame due to unknown endpoint: %X", sl_zigbee_af_current_endpoint());
    return false;
  }

  uint8_t validEntriesCount = sl_zigbee_gp_sink_table_get_number_of_active_entries();

  // If its Sink Table is empty, and the triggering GP Sink Table Request was received in unicast,
  // then the GP Sink Table Response SHALL be sent with Status NOT_FOUND,
  // Total number of non-empty Sink Table entries carrying 0x00,
  // Start index carrying 0xFF (in case of request by GPD ID) or
  // the Index value from the triggering GP Sink Table Request (in case of request by index),
  // Entries count field set to 0x00, and any Sink Table entry fields absent
  if (validEntriesCount == 0) {
    // "index" is already 0xFF if search by ID
    // or already set to the value from "SinkTableRequest" triggered frame in case it is search by INDEX
    sl_zigbee_af_fill_command_green_power_cluster_gp_sink_table_response_smart(SL_ZIGBEE_ZCL_GP_SINK_TABLE_RESPONSE_STATUS_NOT_FOUND,
                                                                               0,
                                                                               cmd_data.index,
                                                                               0);
    sli_zigbee_af_gp_send_response_unicast();
    return true;
  } else {
    // Valid Entries are present!
    if (requestType == SL_ZIGBEE_ZCL_GP_SINK_TABLE_REQUEST_OPTIONS_REQUEST_TABLE_ENTRIES_BY_GPD_ID) {
      sl_zigbee_gp_address_t gpdAddr;
      if (!sli_zigbee_af_gp_make_addr(&gpdAddr,
                                      appId,
                                      cmd_data.gpdSrcId,
                                      cmd_data.gpdIeee,
                                      cmd_data.endpoint)) {
        sl_zigbee_af_fill_command_green_power_cluster_gp_sink_table_response_smart(SL_ZIGBEE_ZCL_GP_SINK_TABLE_RESPONSE_STATUS_NOT_FOUND,
                                                                                   validEntriesCount,
                                                                                   0xFF,
                                                                                   0);
        sli_zigbee_af_gp_send_response_unicast();
        return true;
      }
      // Check for gpd addr = 0
      if (gpdAddrZero(&gpdAddr)) {
        sl_zigbee_af_fill_command_green_power_cluster_gp_sink_table_response_smart(SL_ZIGBEE_ZCL_GP_SINK_TABLE_RESPONSE_STATUS_NOT_FOUND,
                                                                                   validEntriesCount,
                                                                                   0xFF,
                                                                                   0);
        sli_zigbee_af_gp_send_response_unicast();
        return true;
      }
      entryIndex = sl_zigbee_gp_sink_table_lookup(&gpdAddr);
      if (entryIndex == 0xFF) {
        // Valid entries present but none for this gpdAddr - Send NOT FOUND sesponse.
        sl_zigbee_af_fill_command_green_power_cluster_gp_sink_table_response_smart(SL_ZIGBEE_ZCL_GP_SINK_TABLE_RESPONSE_STATUS_NOT_FOUND,
                                                                                   validEntriesCount,
                                                                                   entryIndex,
                                                                                   0);
        sli_zigbee_af_gp_send_response_unicast();
        goto kickout;
      } else {
        // A valid entry with the ID is present
        if (sl_zigbee_gp_sink_table_get_entry(entryIndex, &entry) == SL_STATUS_OK) {
          // If the triggering GP Sink Table Request command contained a GPD ID field, the device SHALL check
          // if it has a Sink Table entry for this GPD ID (and Endpoint, if ApplicationID = 0b010). If yes, the device
          // SHALL create a GP Sink Table Response with Status SUCCESS, Total number of non-empty Sink Table
          // entries carrying the total number of non-empty Sink Table entries on this device, Start index set to
          // 0xff, Entries count field set to 0x01, and one Sink Table entry field for the requested GPD ID (and
          // Endpoint, if ApplicationID = 0b010), formatted as specified in sec. A.3.3.2.2.1, present.
          sl_zigbee_af_fill_command_green_power_cluster_gp_sink_table_response_smart(SL_ZIGBEE_ZCL_GP_SINK_TABLE_RESPONSE_STATUS_SUCCESS,
                                                                                     validEntriesCount,
                                                                                     0xff,
                                                                                     1);
          appResponseLength += storeSinkTableEntryInBuffer(&entry, (appResponseData + appResponseLength));
          sli_zigbee_af_gp_send_response_unicast();
        } else {
          // Not found status to go out.
        }
        goto kickout;
      }
    } else if (requestType == SL_ZIGBEE_ZCL_GP_SINK_TABLE_REQUEST_OPTIONS_REQUEST_TABLE_ENTRIES_BY_INDEX) {
      if (cmd_data.index >= validEntriesCount) {
        sl_zigbee_af_fill_command_green_power_cluster_gp_sink_table_response_smart(SL_ZIGBEE_ZCL_GP_SINK_TABLE_RESPONSE_STATUS_NOT_FOUND,
                                                                                   validEntriesCount,
                                                                                   cmd_data.index,
                                                                                   0);
        sli_zigbee_af_gp_send_response_unicast();
        return true;
      } else {
        // return the sink table entry content into the reponse payload from indicated
        // index and nexts until these are consistant (adress type, etc) and
        // as long as it feet into one message
        sl_zigbee_af_fill_command_green_power_cluster_gp_sink_table_response_smart(SL_ZIGBEE_ZCL_GP_SINK_TABLE_RESPONSE_STATUS_SUCCESS,
                                                                                   validEntriesCount,
                                                                                   cmd_data.index,
                                                                                   0xff);
        validEntriesCount = 0;
        uint16_t entriesCount = 0;
        for (entryIndex = 0; entryIndex < SL_ZIGBEE_GP_SINK_TABLE_SIZE; entryIndex++) {
          if (sl_zigbee_gp_sink_table_get_entry(entryIndex, &entry) != SL_STATUS_OK) {
            continue;
          }

          uint8_t  tempDatabuffer[SL_ZIGBEE_AF_RESPONSE_BUFFER_LEN];
          uint16_t tempDataLength = 0;

          if (entry.status != SL_ZIGBEE_GP_SINK_TABLE_ENTRY_STATUS_UNUSED
              && entry.status != 0) {
            validEntriesCount++;
            if (validEntriesCount > cmd_data.index) {
              // Copy to a temp buffer and add if there is space
              tempDataLength = storeSinkTableEntryInBuffer(&entry, tempDatabuffer);
              // If space add to buffer
              if (sizeof(appResponseData) > (appResponseLength + tempDataLength)) {
                memmove(&appResponseData[appResponseLength], tempDatabuffer, tempDataLength);
                appResponseLength +=  tempDataLength;
                entriesCount++;
              } else {
                break;
              }
            }
          }
        }
        //Insert the number of entries actually included
        appResponseData[GP_SINK_TABLE_RESPONSE_ENTRIES_OFFSET + GP_NON_MANUFACTURER_ZCL_HEADER_LENGTH] = entriesCount;
        sl_status_t status = sli_zigbee_af_gp_send_response_unicast();
        if (status == SL_STATUS_MESSAGE_TOO_LONG) {
          sl_zigbee_af_fill_command_green_power_cluster_gp_sink_table_response_smart(SL_ZIGBEE_ZCL_GP_SINK_TABLE_RESPONSE_STATUS_SUCCESS,
                                                                                     validEntriesCount,
                                                                                     cmd_data.index,
                                                                                     0);
          sli_zigbee_af_gp_send_response_unicast();
        }
        goto kickout;
      }
    } else {
      // nothing, other value of requestType are reserved
    }
  }
  kickout: return true;
}

sli_zigbee_gp_comm_data_saved_t *sl_zigbee_af_green_power_server_find_commissioning_gpd_instance(sl_zigbee_gp_address_t *gpdAddr)
{
  if (gpdAddr != NULL) {
    for (int i = 0; i < SL_ZIGBEE_AF_PLUGIN_GREEN_POWER_SERVER_COMMISSIONING_GPD_INSTANCES; i++) {
      if (sl_zigbee_af_green_power_common_gp_addr_compare(gpdAddr, &gpdCommDataSaved[i].addr)) {
        return &gpdCommDataSaved[i];
      }
    }
  }
  return NULL;
}

void sl_zigbee_af_green_power_server_delete_commissioning_gpd_instance(sl_zigbee_gp_address_t *gpdAddr)
{
  if (gpdAddr != NULL) {
    sli_zigbee_gp_comm_data_saved_t *gpd = sl_zigbee_af_green_power_server_find_commissioning_gpd_instance(gpdAddr);
    if (gpd != NULL) {
      memset(gpd, 0, sizeof(sli_zigbee_gp_comm_data_saved_t));
    }
  }
}

sl_zigbee_af_status_t sl_zigbee_af_green_power_server_derive_shared_key_from_sink_attribute(uint8_t *gpsSecurityKeyTypeAtrribute,
                                                                                            sl_zigbee_key_data_t *gpSharedKeyAttribute,
                                                                                            sl_zigbee_gp_address_t *gpdAddr)
{
  sl_zigbee_af_attribute_type_t type;
  sl_zigbee_af_status_t status = sl_zigbee_af_read_attribute(SL_ZIGBEE_GP_ENDPOINT,
                                                             ZCL_GREEN_POWER_CLUSTER_ID,
                                                             ZCL_GP_SERVER_GP_SHARED_SECURITY_KEY_TYPE_ATTRIBUTE_ID,
                                                             CLUSTER_MASK_SERVER,
                                                             gpsSecurityKeyTypeAtrribute,
                                                             sizeof(uint8_t),
                                                             &type);

  if (status == SL_ZIGBEE_ZCL_STATUS_SUCCESS) {
    if ((*gpsSecurityKeyTypeAtrribute == SL_ZIGBEE_ZCL_GP_SECURITY_KEY_TYPE_ZIGBEE_NETWORK_KEY)
        || (*gpsSecurityKeyTypeAtrribute == SL_ZIGBEE_ZCL_GP_SECURITY_KEY_TYPE_NETWORK_DERIVED_GROUP_KEY)) {
      sl_zigbee_sec_man_key_t nwkKey;
      sl_zigbee_sec_man_context_t context;
      sl_zigbee_sec_man_init_context(&context);
      context.core_key_type = SL_ZB_SEC_MAN_KEY_TYPE_NETWORK;
      // Get NWK Key from NIB
      if (SL_STATUS_OK == sl_zigbee_sec_man_export_key(&context, &nwkKey)) {
        if (*gpsSecurityKeyTypeAtrribute == SL_ZIGBEE_ZCL_GP_SECURITY_KEY_TYPE_NETWORK_DERIVED_GROUP_KEY) {
          // if derived hash it with 'ZGP'
          uint8_t result[SL_ZIGBEE_ENCRYPTION_KEY_SIZE] = { 0 };
          sl_zigbee_sec_man_load_key_context(&context);
          sl_zigbee_sec_man_hmac_aes_mmo((uint8_t *)"ZGP",
                                         3,
                                         result);
          memcpy(gpSharedKeyAttribute->contents, result, SL_ZIGBEE_ENCRYPTION_KEY_SIZE);
        } else {
          memcpy(sl_zigbee_key_contents(gpSharedKeyAttribute), sl_zigbee_key_contents((sl_zigbee_key_data_t*)&nwkKey.key), sizeof(sl_zigbee_key_data_t));
        }
        return SL_ZIGBEE_ZCL_STATUS_SUCCESS;
      }
    } else if ((*gpsSecurityKeyTypeAtrribute == SL_ZIGBEE_ZCL_GP_SECURITY_KEY_TYPE_GPD_GROUP_KEY)
               || (*gpsSecurityKeyTypeAtrribute == SL_ZIGBEE_ZCL_GP_SECURITY_KEY_TYPE_DERIVED_INDIVIDUAL_GPD_KEY)) {
      // Get the Group Key from gps shared key attribute
      status = sl_zigbee_af_read_attribute(SL_ZIGBEE_GP_ENDPOINT,
                                           ZCL_GREEN_POWER_CLUSTER_ID,
                                           ZCL_GP_SERVER_GP_SHARED_SECURITY_KEY_ATTRIBUTE_ID,
                                           CLUSTER_MASK_SERVER,
                                           gpSharedKeyAttribute->contents,
                                           SL_ZIGBEE_ENCRYPTION_KEY_SIZE,
                                           &type);
      if (status == SL_ZIGBEE_ZCL_STATUS_SUCCESS) {
        // If derived, use the HMAC based on GPD address.
        if (*gpsSecurityKeyTypeAtrribute == SL_ZIGBEE_ZCL_GP_SECURITY_KEY_TYPE_DERIVED_INDIVIDUAL_GPD_KEY) {
          uint8_t result[SL_ZIGBEE_ENCRYPTION_KEY_SIZE] = { 0 };
          // The size is always 20 because the header is of 4 bytes and the Key is 16 bytes
          uint8_t data[8] = { 0 };
          if (gpdAddr->applicationId == SL_ZIGBEE_GP_APPLICATION_IEEE_ADDRESS) {
            memmove(data, gpdAddr->id.gpdIeeeAddress, 8);
          } else {
            sl_util_store_low_high_int32u(data, gpdAddr->id.sourceId);
          }
          import_load_zb_sec_man_internal_key((sl_zigbee_sec_man_key_t*)&gpSharedKeyAttribute->contents);
          sl_zigbee_sec_man_hmac_aes_mmo(data,
                                         ((gpdAddr->applicationId == SL_ZIGBEE_GP_APPLICATION_IEEE_ADDRESS) ? 8 : 4),
                                         result);
          memcpy(gpSharedKeyAttribute->contents, result, SL_ZIGBEE_ENCRYPTION_KEY_SIZE);
        }
        return SL_ZIGBEE_ZCL_STATUS_SUCCESS;
      }
    } else {
      // Any other key type is not valid that the shared key type attribute can hold,
      // so just read and return the contents in such case.
      return sl_zigbee_af_read_attribute(SL_ZIGBEE_GP_ENDPOINT,
                                         ZCL_GREEN_POWER_CLUSTER_ID,
                                         ZCL_GP_SERVER_GP_SHARED_SECURITY_KEY_ATTRIBUTE_ID,
                                         CLUSTER_MASK_SERVER,
                                         gpSharedKeyAttribute->contents,
                                         SL_ZIGBEE_ENCRYPTION_KEY_SIZE,
                                         &type);
    }
  }
  return SL_ZIGBEE_ZCL_STATUS_FAILURE;
}

void sl_zigbee_af_green_power_cluster_gp_sink_commissioning_window_extend(uint16_t commissioningWindow)
{
  if (!(commissioningState.inCommissioningMode)) {
    return; // If not in commissioning mode no action.
  }
  sl_zigbee_af_event_set_delay_ms(commissioningWindowTimeout,
                                  commissioningWindow * MILLISECOND_TICKS_PER_SECOND);

  uint8_t proxyOptions = SL_ZIGBEE_AF_GP_PROXY_COMMISSIONING_MODE_OPTION_ACTION \
                         | SL_ZIGBEE_AF_GP_PROXY_COMMISSIONING_MODE_EXIT_MODE_ON_COMMISSIONING_WINDOW_EXPIRATION;

  if (commissioningState.unicastCommunication) { //  based on the commission mode as decided by sink
    proxyOptions |= SL_ZIGBEE_AF_GP_PROXY_COMMISSIONING_MODE_OPTION_UNICAST_COMMUNICATION;
  }
  if (sl_zigbee_af_fill_command_green_power_cluster_gp_proxy_commissioning_mode_smart(proxyOptions,
                                                                                      commissioningWindow,
                                                                                      0) == 0) {
    return;
  }

  sl_zigbee_aps_frame_t *apsFrame;
  apsFrame = sl_zigbee_af_get_command_aps_frame();
  apsFrame->sourceEndpoint = SL_ZIGBEE_GP_ENDPOINT;
  apsFrame->destinationEndpoint = SL_ZIGBEE_GP_ENDPOINT;
  if (commissioningState.proxiesInvolved) {
    sl_status_t status = sl_zigbee_af_send_command_broadcast(SL_ZIGBEE_RX_ON_WHEN_IDLE_BROADCAST_ADDRESS, SL_ZIGBEE_NULL_NODE_ID, 0);
    // Callback to inform the status of message submission to network
    sl_zigbee_af_green_power_cluster_commissioning_message_status_notification_cb(&commissioningState,
                                                                                  apsFrame,
                                                                                  SL_ZIGBEE_OUTGOING_BROADCAST,
                                                                                  SL_ZIGBEE_RX_ON_WHEN_IDLE_BROADCAST_ADDRESS,
                                                                                  status);
    sl_zigbee_af_green_power_cluster_println("Extended Commissioning Status = %d", status);
  } else {
    #ifdef SL_CATALOG_ZIGBEE_GREEN_POWER_CLIENT_PRESENT
    // Put the proxy instance on this node commissioning mode so that it can accept a pairing from itself.
    // This is to ensure the node will be able to handle gpdf commands after pairig.
    sl_status_t status = sl_zigbee_af_send_command_unicast(SL_ZIGBEE_OUTGOING_DIRECT, sl_zigbee_af_get_node_id());
    // Callback to inform the status of message submission to network
    sl_zigbee_af_green_power_cluster_commissioning_message_status_notification_cb(&commissioningState,
                                                                                  apsFrame,
                                                                                  SL_ZIGBEE_OUTGOING_DIRECT,
                                                                                  sl_zigbee_af_get_node_id(),
                                                                                  status);
    sl_zigbee_af_green_power_cluster_println("Extended Commissioning Status = %d", status);
    #endif // SL_CATALOG_ZIGBEE_GREEN_POWER_CLIENT_PRESENT
  }
}

// Returns the commissioning state of the sink.
sl_zigbee_af_green_power_server_commissioning_state_t *sl_zigbee_af_green_power_cluster_get_server_commissioning_sate(void)
{
  return (&commissioningState);
}

#ifdef SL_CATALOG_ZIGBEE_GREEN_POWER_TRANSLATION_TABLE_PRESENT
bool sl_zigbee_af_green_power_cluster_gp_translation_table_update_cb(sl_zigbee_af_cluster_command_t *cmd);
bool sl_zigbee_af_green_power_cluster_gp_translation_table_request_cb(sl_zigbee_af_cluster_command_t *cmd);
#endif

uint32_t sl_zigbee_af_green_power_cluster_server_command_parse(sl_service_opcode_t opcode,
                                                               sl_service_function_context_t *context)
{
  (void)opcode;
  defaultResponseFailureStatus = SL_ZIGBEE_ZCL_STATUS_UNSUP_COMMAND;
  sl_zigbee_af_cluster_command_t *cmd = (sl_zigbee_af_cluster_command_t *)context->data;
  bool wasHandled = false;

  if (!cmd->mfgSpecific) {
    switch (cmd->commandId) {
      case ZCL_GP_NOTIFICATION_COMMAND_ID:
      {
        wasHandled = sl_zigbee_af_green_power_cluster_gp_notification_cb(cmd);
        break;
      }
      case ZCL_GP_COMMISSIONING_NOTIFICATION_COMMAND_ID:
      {
        wasHandled = sl_zigbee_af_green_power_cluster_gp_commissioning_notification_cb(cmd);
        break;
      }
      case ZCL_GP_SINK_COMMISSIONING_MODE_COMMAND_ID:
      {
        wasHandled = sl_zigbee_af_green_power_cluster_gp_sink_commissioning_mode_cb(cmd);
        break;
      }
#ifdef SL_CATALOG_ZIGBEE_GREEN_POWER_TRANSLATION_TABLE_PRESENT
      // When the translation table component is supported process the commands related to
      // translation table.
      case ZCL_GP_TRANSLATION_TABLE_UPDATE_COMMAND_ID:
      {
        wasHandled = sl_zigbee_af_green_power_cluster_gp_translation_table_update_cb(cmd);
        break;
      }
      case ZCL_GP_TRANSLATION_TABLE_REQUEST_COMMAND_ID:
      {
        wasHandled = sl_zigbee_af_green_power_cluster_gp_translation_table_request_cb(cmd);
        break;
      }
#endif // SL_CATALOG_ZIGBEE_GREEN_POWER_TRANSLATION_TABLE_PRESENT
      case ZCL_GP_PAIRING_CONFIGURATION_COMMAND_ID:
      {
        wasHandled = sl_zigbee_af_green_power_cluster_gp_pairing_configuration_cb(cmd);
        break;
      }
      case ZCL_GP_SINK_TABLE_REQUEST_COMMAND_ID:
      {
        wasHandled = sl_zigbee_af_green_power_cluster_gp_sink_table_request_cb(cmd);
        break;
      }
    }
  }

  return ((wasHandled)
          ? SL_ZIGBEE_ZCL_STATUS_SUCCESS
          : defaultResponseFailureStatus);
}

// This callback is application-specific
// to get the group id for the groupcast communication mode.
WEAK(void sl_zigbee_af_green_power_server_pre_sink_pairing_cb(sli_zigbee_gp_comm_data_saved_t *commissioningGpd))
{
}

void sl_zigbee_af_green_power_client_gpdf_sink_table_based_forward_cb(sl_zigbee_gp_address_t *addr,
                                                                      sl_zigbee_gp_sink_list_entry_t *sinkList,
                                                                      uint8_t maxNumberEntries)
{
  uint8_t sinkEntryIndex = sl_zigbee_gp_sink_table_lookup(addr);
  if (sinkEntryIndex != 0xFF) {
    sl_zigbee_gp_sink_table_entry_t sinkEntry;
    if (sl_zigbee_gp_sink_table_get_entry(sinkEntryIndex, &sinkEntry) == SL_STATUS_OK) {
      for (uint8_t idx = 0; idx < maxNumberEntries; idx++) {
        if (sinkEntry.sinkList[idx].type == SL_ZIGBEE_GP_SINK_TYPE_GROUPCAST) {
          memcpy(&sinkList[idx], &sinkEntry.sinkList[idx], sizeof(sl_zigbee_gp_sink_list_entry_t));
        }
      }
    }
  }
}

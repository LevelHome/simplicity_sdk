/***************************************************************************//**
 * @file
 * @brief Signature verification APIs for the OTA Client plugin.
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

// Hashing a file can be expensive for small processors. The maxHashCalculations
// determines how many iterations are made before returning back to the caller.
// A value of 0 indicates to completely calculate the digest before returning.
// A value greater than 0 means that a number of hashes will be performed and
// then the routine will return SL_ZIGBEE_AF_IMAGE_VERIFY_IN_PROGRESS.  In that
// case it is expected that this function must be called repeatedly until it
// returns another status code.
// When SL_ZIGBEE_AF_IMAGE_VERIFY_WAIT is returned then no further calls
// are necessary.  The verify code will fire the callback
// sli_zigbee_af_ota_verify_stored_data_finish() when it is has a result.
sl_zigbee_af_image_verify_status_t sli_zigbee_af_ota_image_signature_verify(uint16_t maxHashCalculations,
                                                                            const sl_zigbee_af_ota_image_id_t* id,
                                                                            bool newVerification);

// This is the maximum number of digest calculations we perform per call to
// sli_zigbee_af_ota_image_signature_verify().  Arbitrary chosen value to limit
// time spent in this routine.  A value of 0 means we will NOT return
// until we are completely done with our digest calculations.
// Empirically waiting until digest calculations are complete can
// take quite a while for EZSP hosts (~40 seconds for a UART connected host).
// So we want to make sure that other parts of the framework can run during
// this time.  On SOC systems a similar problem occurs.  If we set this to 0
// then sl_zigbee_tick() will not fire and therefore the watchdog timer will not be
// serviced.
#define MAX_DIGEST_CALCULATIONS_PER_CALL 5

void sli_zigbee_af_ota_verify_stored_data_finish(sl_zigbee_af_image_verify_status_t status);

void sli_zigbee_af_ota_client_signature_verify_print_signers(void);

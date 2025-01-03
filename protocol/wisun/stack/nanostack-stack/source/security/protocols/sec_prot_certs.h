/*
 * Copyright (c) 2016-2020, Pelion and affiliates.
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

#ifndef SEC_PROT_CERTS_H_
#define SEC_PROT_CERTS_H_

#include "sli_wisun_certificate.h"

/*
 * Security protocols certificate interface. This is used by security protocols to
 * access certificate information.
 *
 * Own certificate chain contains the certificate chain that is sent on TLS handshake
 * to remote end. Typically this is one certificate long, and the certificate chains
 * to root CA certificate or to intermediate certificate known to other end. It is
 * also possible to send chain longer than one certificate.
 *
 * Key on own certificate chain must be the private key of the certificate used on
 * TLS handshake.
 *
 * Trusted certificate chains contains the root CA certificates and intermediate
 * certificates chains that are used to validate remote certificates.
 *
 */

typedef struct sec_prot_certs_s {
    const sli_wisun_certificate_t *own_cert;            /**< Own certificate */
    uint32_t own_private_key;                           /**< Own private key */
    const sli_wisun_certificate_chain_t *trusted_cert;  /**< Trusted certificate chain */
} sec_prot_certs_t;

/**
 * sec_prot_certs_init initialize certificate information
 *
 * \param certs certificate information
 *
 * \return < 0 failure
 * \return >= 0 success
 */
int8_t sec_prot_certs_init(sec_prot_certs_t *certs);

/**
 * sec_prot_certs_delete delete certificate information
 *
 * \param certs certificate information
 *
 */
void sec_prot_certs_delete(sec_prot_certs_t *certs);


#endif /* SEC_PROT_CERTS_H_ */

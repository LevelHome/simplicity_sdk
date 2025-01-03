/*
 *    Copyright (c) 2020, The OpenThread Authors.
 *    All rights reserved.
 *
 *    Redistribution and use in source and binary forms, with or without
 *    modification, are permitted provided that the following conditions are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *    3. Neither the name of the copyright holder nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 *    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *    ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *    LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *    INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *    CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *    POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 * This file defines bswap_* on macOS.
 */

#ifndef OTBR_COMMON_BYTESWAP_HPP_
#define OTBR_COMMON_BYTESWAP_HPP_

#include "platform-si91x.h"
#include "openthread-br/config.h"

#define bswap_16 __REV16
#define bswap_32 __REV

#define bswap_64(x)                                                               \
    ((((x) << 56) & 0xff00000000000000UL) | (((x) << 40) & 0x00ff000000000000UL)  \
     | (((x) << 24) & 0x0000ff0000000000UL) | (((x) << 8) & 0x000000ff00000000UL) \
     | (((x) >> 8) & 0x00000000ff000000UL) | (((x) >> 24) & 0x0000000000ff0000UL) \
     | (((x) >> 40) & 0x000000000000ff00UL) | (((x) >> 56) & 0x00000000000000ffUL))

#define htobe16(x) bswap_16(x)
#define htole16(x) (uint16_t)(x)
#define be16toh(x) bswap_16(x)
#define le16toh(x) (uint16_t)(x)

#define htobe32(x) bswap_32(x)
#define htole32(x) (uint32_t)(x)
#define be32toh(x) bswap_32(x)
#define le32toh(x) (uint32_t)(x)

#define htobe64(x) bswap_64(x)
#define htole64(x) (uint64_t)(x)
#define be64toh(x) bswap_64(x)
#define le64toh(x) (uint64_t)(x)

#endif // OTBR_COMMON_BYTESWAP_HPP_

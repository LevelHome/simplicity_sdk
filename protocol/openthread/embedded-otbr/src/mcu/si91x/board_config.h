/*
 *  Copyright (c) 2023, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 *   This file includes compile-time configuration constants for si91x.
 *
 */

#ifndef __BOARD_CONFIG_H__
#define __BOARD_CONFIG_H__

#ifndef RADIO_CONFIG_DEBUG_COUNTERS_SUPPORT
#define RADIO_CONFIG_DEBUG_COUNTERS_SUPPORT 0 /// Set to 1 to enable debug counters in radio.c
#endif

#ifndef RADIO_CONFIG_ENABLE_CUSTOM_EUI_SUPPORT
#define RADIO_CONFIG_ENABLE_CUSTOM_EUI_SUPPORT 1 /// Set to 1 to enable custom EUI support (enabled by default)
#endif

#ifndef RADIO_CONFIG_DMP_SUPPORT
#define RADIO_CONFIG_DMP_SUPPORT 0 /// Set to 1 to enable Dynamic Multi-Protocol support in radio.c
#endif

#include <string.h>

#ifndef strnlen
#define strnlen(s, n) (((n) < (strlen(s))) ? (n) : strlen((s)))
#endif /* strnlen */

#define SL_PLATFORM_UART_PERIPHERAL USART_0
#define SL_LOG_UART_PERIPHERAL ULPUART

/**
 * @Note
 *
 *   BRD4338A, BRD4342A  |  BRD4002A | BRD8045A  |
 *  UART1 TX  GPIO_30    |    P35    |  P103 D3  |
 *  UART1 RX  GPIO_29    |    P33    |  P103 D2  |
 *
 */

#endif // __BOARD_CONFIG_H__

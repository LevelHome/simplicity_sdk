/*
 *  Copyright (c) 2021, The OpenThread Authors.
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

#if defined SL_CATALOG_OPENTHREAD_CLI_PRESENT

#include <openthread/config.h>
#include <openthread/openthread-system.h>

#include <ctype.h>
#include <error.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <openthread/cli.h>
#include <openthread/platform/debug_uart.h>
#include "cli/cli_config.h"
#include "common/code_utils.hpp"

#include "openthread-core-config.h"
#include "platform-posix.h"

extern "C" void otSysEventSignalPending(void);

static char     buffer[OPENTHREAD_CONFIG_CLI_MAX_LINE_LENGTH] = {0};
static uint32_t count                                         = 0;

namespace {

int OutputCallback(void *aContext, const char *aFormat, va_list aArguments)
{
    OT_UNUSED_VARIABLE(aContext);

    otPlatDebugUart_vprintf(aFormat, aArguments);
    return 0;
}
} // namespace

extern "C" void otAppCliInit(otInstance *aInstance)
{
    otCliInit(aInstance, OutputCallback, nullptr);
}

extern "C" void otAppCliDeinit(void)
{
    /* NOP */
}

extern "C" void otAppCliUpdate(otSysMainloopContext *aMainloop)
{
    OT_UNUSED_VARIABLE(aMainloop);
}

extern "C" void otAppCliProcess(const otSysMainloopContext *aMainloop)
{
    OT_UNUSED_VARIABLE(aMainloop);
    int read_char;

    do
    {
        read_char = otPlatDebugUart_getc();

        char input_char = (char)read_char;
        if (input_char == 127u || input_char == '\b') /* Backspace */
        {
            if (count > 0)
            {
                count--;
                // Erase last char
                otPlatDebugUart_putchar_raw('\b');
                otPlatDebugUart_putchar_raw(' ');
                otPlatDebugUart_putchar_raw('\b');
            }
        }
        else if (input_char == '\n' || input_char == '\r') /* Enter */
        {
            buffer[count++] = input_char;
            otPlatDebugUart_putchar_raw('\n');
            otPlatDebugUart_putchar_raw('\r');

            otCliInputLine(buffer);
            /* Clear "Enter" character in buffer, otCliInputLine() uses this
             * character to find the string terminator. */
            memset(buffer, 0, sizeof(char) * OPENTHREAD_CONFIG_CLI_MAX_LINE_LENGTH);
            count = 0;
            otSysEventSignalPending();
        }
        else if (isprint(input_char))
        {
            buffer[count++] = input_char;
            otPlatDebugUart_putchar_raw(input_char);
        }
    } while (read_char != -1);
}

#endif // SL_CATALOG_OPENTHREAD_CLI_PRESENT

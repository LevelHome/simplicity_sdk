/*******************************************************************************
 * @file
 * @brief syscalls
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

/*
 *
 *    Atollic TrueSTUDIO Minimal System calls file
 *
 *    For more information about which c-functions
 *    need which of these lowlevel functions
 *    please consult the Newlib libc-manual
 *
 * */
#include "syscalls.h"
#include "errno.h"
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/times.h>
#include <time.h>

#include "sl_component_catalog.h"
#include <openthread/platform/debug_uart.h>
#if defined(SL_CATALOG_KERNEL_PRESENT)
#include "cmsis_os2.h"
#endif
#define IO_MAXLINE 20U // maximun read length
typedef int (*PUTCHAR_FUNC)(int a);
char *stack_ptr __asm("sp");

extern char __HeapBase[];
extern char __HeapLimit[];

/*! @brief Specification modifier flags for scanf. */
enum _debugconsole_scanf_flag
{
    kSCANF_Suppress   = 0x2U,  /*!< Suppress Flag. */
    kSCANF_DestMask   = 0x7cU, /*!< Destination Mask. */
    kSCANF_DestChar   = 0x4U,  /*!< Destination Char Flag. */
    kSCANF_DestString = 0x8U,  /*!< Destination String FLag. */
    kSCANF_DestSet    = 0x10U, /*!< Destination Set Flag. */
    kSCANF_DestInt    = 0x20U, /*!< Destination Int Flag. */
    kSCANF_DestFloat  = 0x30U, /*!< Destination Float Flag. */
    kSCANF_LengthMask = 0x1f00U,
    /*!< Length Mask Flag. */    /*PRINTF_FLOAT_ENABLE */
    kSCANF_TypeSinged = 0x2000U, /*!< TypeSinged Flag. */
};
void initialise_monitor_handles()
{
}

int _getpid(void)
{
    return 1;
}

int _kill(int pid, int sig)
{
    (void)pid;
    (void)sig;
    errno = EINVAL;
    return -1;
}

void _exit(int status)
{
    _kill(status, -1);
    while (1)
    {
    } /* Make sure we hang here */
}

int _write(int file, char *ptr, int len)
{
    (void)file;
    (void)ptr;
    (void)len;

#if (OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_DEBUG_UART)
    otPlatDebugUart_write_bytes((const uint8_t *)ptr, len);
    return len;
#else

    return 0;
#endif
}

#ifndef SL_WIFI_COMPONENT_INCLUDED
void *_sbrk(int incr)
{
    static char *heap_end = __HeapBase;
    char        *prev_heap_end;

    if ((heap_end + incr) > __HeapLimit)
    {
        // Not enough heap
        return (void *)-1;
    }

    prev_heap_end = heap_end;
    heap_end += incr;

    return prev_heap_end;
}
#endif

int _close(int file)
{
    (void)file;
    return -1;
}

int _fstat(int file, struct stat *st)
{
    (void)file;
    st->st_mode = S_IFCHR;
    return 0;
}

int _isatty(int file)
{
    (void)file;
    return 1;
}

int _lseek(int file, int ptr, int dir)
{
    (void)file;
    (void)ptr;
    (void)dir;
    return 0;
}

int _read(char *fmt_ptr, ...)
{
    (void)fmt_ptr;

    return -1;
}

int _open(char *path, int flags, ...)
{
    (void)path;
    (void)flags;
    /* Pretend like we always fail */
    return -1;
}

int _wait(int *status)
{
    (void)status;
    errno = ECHILD;
    return -1;
}

int _unlink(char *name)
{
    (void)name;
    errno = ENOENT;
    return -1;
}

int _times(struct tms *buff)
{
    (void)buff;
    return -1;
}

int _stat(char *file, struct stat *st)
{
    (void)file;
    st->st_mode = S_IFCHR;
    return 0;
}

int _link(char *old_link, char *new_link)
{
    (void)old_link; // This statement is added only to resolve compilation warning, value is unchanged
    (void)new_link; // This statement is added only to resolve compilation warning, value is unchanged
    errno = EMLINK;
    return -1;
}

int _fork(void)
{
    errno = EAGAIN;
    return -1;
}

int _execve(char *name, char **argv, char **env)
{
    (void)name;
    (void)argv;
    (void)env;

    errno = ENOMEM;

    return -1;
}

SL_WEAK void _putchar(char character)
{
    (void)character;
}

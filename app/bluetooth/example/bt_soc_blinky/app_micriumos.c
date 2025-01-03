/***************************************************************************//**
 * @file
 * @brief Core application logic for Micirum OS.
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
#include <stdint.h>
#include <stdbool.h>
#include "os.h"
#include "rtos_err.h"
#include "app.h"
#include "app_assert.h"
#include "sl_memory_manager.h"

#define APP_TASK_NAME          "app_task"
#define APP_TASK_STACK_SIZE    1024
#define APP_TASK_PRIO          31u

// Application task function
static void app_task(void *p_arg);
// Task stack
static CPU_STK *app_task_stack;
// Task handle
static OS_TCB  app_task_handle;
// Semaphore for button state
static OS_SEM  button_semaphore;

/******************************************************************************
 * Application Runtime Init.
 *****************************************************************************/
void app_init_runtime(void)
{
  RTOS_ERR err;
  // Allocate stack for the task
  size_t stack_size = APP_TASK_STACK_SIZE;
  stack_size -= (stack_size % CPU_CFG_STK_ALIGN_BYTES);
  app_task_stack = (CPU_STK *)sl_malloc(stack_size);
  app_assert(app_task_stack != NULL,
             "Application task stack allocation failed.");

  // Create the task for app_process_action
  OSTaskCreate(&app_task_handle,
               APP_TASK_NAME,
               app_task,
               0u,
               APP_TASK_PRIO,
               &app_task_stack[0u],
               0u,
               stack_size / sizeof(CPU_STK),
               0u,
               0u,
               0u,
               (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
               &err);
  app_assert(err.Code == RTOS_ERR_NONE,
             "Application task creation failed.");

  // Create a semaphore for button state
  OSSemCreate(&button_semaphore, "Button semaphore", 0, &err);
  app_assert(err.Code == RTOS_ERR_NONE,
             "Application semaphore creation failed.");
}

/******************************************************************************
 * Set button state.
 *****************************************************************************/
void app_set_button_state(void)
{
  RTOS_ERR err;
  OSSemPost(&button_semaphore, OS_OPT_POST_1, &err);
  app_assert(err.Code == RTOS_ERR_NONE,
             "Semaphore post failed!");
}

/******************************************************************************
 * Get button state.
 * @return button state (true if pressed)
 *****************************************************************************/
bool app_get_button_state(void)
{
  RTOS_ERR err;
  OSSemPend(&button_semaphore,
            (OS_TICK)0,
            OS_OPT_PEND_BLOCKING,
            DEF_NULL,
            &err);
  return (err.Code == RTOS_ERR_NONE);
}

/******************************************************************************
 * Application task
 *****************************************************************************/
static void app_task(void *p_arg)
{
  (void)p_arg;
  for (;; ) {
    app_process_action();
  }
}

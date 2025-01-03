/***************************************************************************//**
 * @file
 * @brief Core application logic for FreeRTOS.
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
#include "app.h"
#include "app_assert.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "sl_component_catalog.h"
#include "task.h"

#ifdef SL_CATALOG_APP_LOG_PRESENT
#include "app_log.h"
#endif // SL_CATALOG_APP_LOG_PRESENT

#define APP_TASK_STACK_SIZE    512u
#define APP_TASK_PRIO          24u

// Application task function
static void app_task(void *p_arg);

// Task handle
static TaskHandle_t      app_task_handle  = NULL;
// Semaphore handle
static SemaphoreHandle_t app_semaphore_handle = NULL;

// Application Runtime Init.
void app_init_runtime(void)
{
  BaseType_t ret;
  // Create the task for app_process_action
  ret = xTaskCreate(app_task,
                    "app_task",
                    APP_TASK_STACK_SIZE,
                    NULL,
                    APP_TASK_PRIO,
                    &app_task_handle);
  app_assert(ret == pdPASS, "Application task creation failed.");
  // Create the semaphore
  app_semaphore_handle = xSemaphoreCreateCounting(UINT16_MAX, 0);
  app_assert(app_semaphore_handle != NULL, "Semaphore creation failed.");
}

/******************************************************************************
 * Application task.
 *****************************************************************************/
static void app_task(void *p_arg)
{
  (void)p_arg;
  app_log("Bt Mesh Sensor Client initialized" APP_LOG_NL);
  // Ensure right init order in case of shared pin for enabling buttons
  app_change_buttons_to_leds();
  // Change LEDs to buttons in case of shared pin
  app_change_leds_to_buttons();
  app_handle_reset_conditions();
  while (1) {
    app_process_action();
  }
}

// Proceed with execution.
void app_proceed(void)
{
  if (xPortIsInsideInterrupt()) {
    // Interrupt context
    BaseType_t woken = pdFALSE;
    (void)xSemaphoreGiveFromISR(app_semaphore_handle, &woken);
    portYIELD_FROM_ISR(woken);
  } else {
    // Non-interrupt context
    (void)xSemaphoreGive(app_semaphore_handle);
  }
}

// Check if it is required to process with execution.
bool app_is_process_required(void)
{
  BaseType_t ret = xSemaphoreTake(app_semaphore_handle, portMAX_DELAY);
  return (ret == pdTRUE);
}

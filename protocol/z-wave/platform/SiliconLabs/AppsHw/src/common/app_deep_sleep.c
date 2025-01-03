/**
 * @file
 * Platform abstraction for all sleeping reporting applications
 *
 * @copyright 2021 Silicon Laboratories Inc.
 */
#include "app_hw.h"
#include "sl_component_catalog.h"

void app_hw_deep_sleep_wakeup_handler(void)
{
#ifdef SL_CATALOG_SIMPLE_BUTTON_PRESENT
  app_button_press_stay_awake_until_release();
#endif
}

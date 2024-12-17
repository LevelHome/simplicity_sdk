/***************************************************************************//**
 * @file    iot_gpio_drv.c
 * @brief   GPIO driver for Silicon Labs boards.
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
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

/*******************************************************************************
 *                               INCLUDES
 ******************************************************************************/

/* Simplicity SDK emlib layer */
#include "sl_core.h"
#include "em_device.h"
#include "sl_device_gpio.h"

/* GPIO driver layer */
#include "iot_adc_pinout_drv.h"

/*******************************************************************************
 *                              slGetPort()
 ******************************************************************************/
sl_gpio_port_t slGetPort(slPin pin)
{
  sl_gpio_port_t ret = (sl_gpio_port_t) -1;

  switch (pin & 0xF0) {
    case 0x00:
      ret = (sl_gpio_port_t) -1;
      break;

    case 0x10:
#if (GPIO_PI_COUNT > 0)
      ret = SL_GPIO_PORT_I;
#else
      ret = (sl_gpio_port_t) -1;
#endif
      break;

    case 0x20:
      ret = (sl_gpio_port_t) -1;
      break;

    case 0x30:
      ret = (sl_gpio_port_t) -1;
      break;

    case 0x40:
      ret = (sl_gpio_port_t) -1;
      break;

    case 0x50:
      ret = (sl_gpio_port_t) -1;
      break;

    case 0x60:
#if (GPIO_PG_COUNT > 0)
      ret = SL_GPIO_PORT_G;
#else
      ret = (sl_gpio_port_t) -1;
#endif
      break;

    case 0x70:
#if (GPIO_PJ_COUNT > 0)
      ret = SL_GPIO_PORT_J;
#else
      ret = (sl_gpio_port_t) -1;
#endif
      break;

    case 0x80:
#if (GPIO_PH_COUNT > 0)
      ret = SL_GPIO_PORT_H;
#else
      ret = (sl_gpio_port_t) -1;
#endif
      break;

    case 0x90:
#if (GPIO_PK_COUNT > 0)
      ret = SL_GPIO_PORT_K;
#else
      ret = (sl_gpio_port_t) -1;
#endif
      break;

    case 0xA0:
#if (GPIO_PA_COUNT > 0)
      ret = SL_GPIO_PORT_A;
#else
      ret = (sl_gpio_port_t) -1;
#endif
      break;

    case 0xB0:
#if (GPIO_PB_COUNT > 0)
      ret = SL_GPIO_PORT_B;
#else
      ret = (sl_gpio_port_t) -1;
#endif
      break;

    case 0xC0:
#if (GPIO_PC_COUNT > 0)
      ret = SL_GPIO_PORT_C;
#else
      ret = (sl_gpio_port_t) -1;
#endif
      break;

    case 0xD0:
#if (GPIO_PD_COUNT > 0)
      ret = SL_GPIO_PORT_D;
#else
      ret = (sl_gpio_port_t) -1;
#endif
      break;

    case 0xE0:
#if (GPIO_PE_COUNT > 0)
      ret = SL_GPIO_PORT_E;
#else
      ret = (sl_gpio_port_t) -1;
#endif
      break;

    case 0xF0:
#if (GPIO_PF_COUNT > 0)
      ret = SL_GPIO_PORT_F;
#else
      ret = (sl_gpio_port_t) -1;
#endif
      break;

    default:
      ret = (sl_gpio_port_t) -1;
      break;
  }
  return ret;
}

/*******************************************************************************
 *                              slGetPin()
 ******************************************************************************/
uint32_t slGetPin(slPin pin)
{
  return ((uint32_t) pin) & 0x0F;
}

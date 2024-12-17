/***************************************************************************//**
 * @file
 * @brief app_menu.h
 *******************************************************************************
 * # License
 * <b>Copyright 2018 Silicon Laboratories Inc. www.silabs.com</b>
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

#ifndef APP_MENU_H
#define APP_MENU_H

// -----------------------------------------------------------------------------
//                                   Includes
// -----------------------------------------------------------------------------
#include <stdbool.h>
#include <stdint.h>

#include "sl_component_catalog.h"
#if defined(SL_CATALOG_RADIO_CONFIG_SIMPLE_RAIL_SINGLEPHY_PRESENT)
#include "rail_config.h"
#endif

// -----------------------------------------------------------------------------
//                              Macros and Typedefs
// -----------------------------------------------------------------------------
/// Enum to use to tell which menu items are visible in which Range Test modes.
typedef enum {
  NO_HIDE,                          ///> Menu item visible in all modes.
  RX_HIDE,                          ///> Item hidden in RX mode.
  TX_HIDE,                          ///> Item hidden in TX mode.
  TRX_HIDE,                         ///> Unused.
} menu_hide_t;

/// Displayable Icon types in an enum.
typedef enum {
  ICON_DOWN,                        ///> Down arrow for menu navigation.
  ICON_PLUS,                        ///> Plus sign if increment is allowed.
  ICON_GO,                          ///> Unused.
  ICON_SHOW,                        ///> 'SHOW' is printed.
  ICON_START,                       ///> 'START' is printed.
  ICON_STOP,                        ///> 'STOP' is printed.
  ICON_BACK,                        ///> 'BACK' is printed.
  ICON_NONE,                        ///> Nothing is printed or drawn.
} menu_item_icon_t;

typedef struct {
  menu_hide_t   menu_hidden;     ///> The mode in which this menu item is hidden.
  bool (*action)(bool);      ///> Handle triggers
  menu_item_icon_t (*display)(char **);  ///> Display function
  bool is_last;
} menu_item_t;

/// Default payload length value

#define RANGETEST_PAYLOAD_LEN_MIN        (7U)
#define RANGETEST_PAYLOAD_LEN_MAX        (64U)

// -----------------------------------------------------------------------------
//                                Global Variables
// -----------------------------------------------------------------------------
extern uint8_t number_of_phys;

// -----------------------------------------------------------------------------
//                          Public Function Declarations
// -----------------------------------------------------------------------------
/*******************************************************************************
 * @brief Initialization of the menu on the display by going through the actions
 *         of all menu items.
 ******************************************************************************/
void menu_init(void);

/*******************************************************************************
 * @brief Returns the index of the currently selected menu item.
 *
 * @return The index of the currently selected menu item.
 ******************************************************************************/
uint8_t menu_get_actual_id(void);

/*******************************************************************************
 * @brief Returns the index of the currently selected menu item offset by
 *         the hidden menus
 *
 * @return The index of the currently selected menu item.
 ******************************************************************************/
uint8_t menu_get_id_with_hidden(uint8_t current_id);

/*******************************************************************************
 * @brief Returns the menu index that is the top visible menu line.
 *
 * @return Pointer to (global) string buffer.
 ******************************************************************************/
uint8_t menu_get_display_start_id(void);

/*******************************************************************************
 * @brief Checks isf the given item is the last element.
 *
 * @param[in] index: Menu item which is to be checked if last or not.
 *
 * @return True if this is the last item.
 ******************************************************************************/
bool menu_is_last_item(uint8_t index);

/*******************************************************************************
 * @brief Triggers the display function of the selected item but it just
 *         determines the type of action icon used for this menu.
 *
 * @param[in] item: The current menu item index.
 *
 * @return The icon of the chosen item.
 ******************************************************************************/
uint8_t menu_item_icon(uint8_t item);

/*******************************************************************************
 * @brief Handles the list display for menu states that have string values.
 *
 * @param[in] item: Item string.
 *
 * @return Pointer to (global) string buffer.
 ******************************************************************************/
char *menu_item_str(uint8_t item);

/*******************************************************************************
 * @brief Tell if menu is hidden in this mode
 *
 * @param[in] index: which menu item
 *
 * @return bool: true if it is hidden
 ******************************************************************************/
bool menu_item_is_hidden(uint8_t index);

/*******************************************************************************
 * @brief Handles the list display for menu states that have string values.
 *         Extra alignment is done to pad out the value to  given
 *
 * @param[in] item: Item string.
 * @param[in] value: Value string.
 * @param[in] col: column to align the value to.
 *
 * @return Pointer to (global) string buffer.
 ******************************************************************************/
char* menu_print_aligned(char* item, char* value, uint8_t col);

/*******************************************************************************
 * @brief Iterates to the next menu item
 ******************************************************************************/
void menu_next_item(void);

/*******************************************************************************
 * @brief Executes current menu item action
 ******************************************************************************/
bool menu_item_action(void);

/*******************************************************************************
 * @brief Return the number of the phy used by the program
 *
 * @return number_of_phys
 ******************************************************************************/
uint8_t get_number_of_phys(void);

/*******************************************************************************
 * @brief Helper function to apply all settings for the current phy
 *
 * @param[in] init: Init is needed or not
 ******************************************************************************/
void apply_new_phy(bool init);

/*******************************************************************************
 * @brief This function is defined in app_measurement_standard.c
 *         If that file is not present then it is an only custom phy range test
 *
 * @param[in] init: Init is needed or not
 ******************************************************************************/
void menu_set_std_phy(bool init);

#endif // APP_MENU_H

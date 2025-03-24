/*
 * comm_menu_system.c
 *
 *  Created on: Feb 2, 2025
 *      Author: ericv
 */

/* Private includes ----------------------------------------------------------*/

#include "stm32h7xx_hal.h"
#include "cmsis_os.h"
#include "comm_menu_system.h"
#include <stdbool.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/



/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/

static MenuNode_t* registered_menus[MENU_ID_COUNT] = {NULL};

/* Private function prototypes -----------------------------------------------*/



/* Exported function definitions ---------------------------------------------*/

bool registerMenu(const MenuNode_t* menu)
{
  if (menu->id < MENU_ID_COUNT) {
    if (registered_menus[menu->id] == NULL) {
      registered_menus[menu->id] = (MenuNode_t*) menu;
      
      return true;
    }
  }
  return false;
}

MenuNode_t* getMenu(MenuID_t id)
{
  if (id < MENU_ID_COUNT) {
    return registered_menus[id];
  }
  return NULL;
}

/* Private function definitions ----------------------------------------------*/

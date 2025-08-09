/*
 * comm_main_menu.c
 *
 *  Created on: Feb 2, 2025
 *      Author: ericv
 */

/* Private includes ----------------------------------------------------------*/

#include "comm_menu_registration.h"
#include "comm_menu_system.h"
#include <stdbool.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/



/* Private macro -------------------------------------------------------------*/



/* Private function prototypes -----------------------------------------------*/



/* Private variables ---------------------------------------------------------*/

static MenuID_t mainMenuChildren[] = {
  MENU_ID_CFG, MENU_ID_DBG, MENU_ID_HIST, MENU_ID_TXRX, MENU_ID_JANUS, MENU_ID_EVAL
};
static const MenuNode_t mainMenu = {
  .id = MENU_ID_MAIN,
  .description = "Main Menu",
  .handler = NULL,
  .parent_id = MENU_ID_MAIN,
  .children_ids = mainMenuChildren,
  .num_children = sizeof(mainMenuChildren) / sizeof(mainMenuChildren[0]),
  .access_level = 0,
  .parameters = NULL
};

/* Exported function definitions ---------------------------------------------*/

bool COMM_RegisterMainMenu(void)
{
  bool ret = registerMenu(&mainMenu);
  return ret;
}

/* Private function definitions ----------------------------------------------*/

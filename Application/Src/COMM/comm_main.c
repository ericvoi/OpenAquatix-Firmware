/*
 * comm_main.c
 *
 *  Created on: Feb 2, 2025
 *      Author: ericv
 * 
 * Copyright (c) 2025 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

/* Private includes ----------------------------------------------------------*/

#include "stm32h7xx_hal.h"
#include "cmsis_os.h"
#include "arm_math.h"

#include "hmi_usb.h"
#include "dau_card-driver.h"
#include "error_manager.h"

#include "comm_menu_registration.h"
#include "comm_main.h"
#include "comm_menu_system.h"
#include "comm_print.h"

#include "mess_main.h"

#include "cfg_main.h"
#include "cfg_parameters.h"
#include "cfg_defaults.h"

#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* Private typedef -----------------------------------------------------------*/

typedef struct {
  MenuNode_t* current_menu;
  CommInterface_t interface;
} MenuContext_t;

typedef struct {
  uint32_t flag;
  const char* message;
} HmiNotification_t;

/* Private define ------------------------------------------------------------*/

#define MAX_MENU_NUMBER_LENGTH    2

#define ANSI_ESCAPE               "\r\033[K"
#define ANSI_ESCAPE_LEN           (strlen(ANSI_ESCAPE))

/* Private macro -------------------------------------------------------------*/

#define MIN(a, b)                 (((a) < (b)) ? (a) : (b))
#define MAX(a, b)                 (((a) > (b)) ? (a) : (b))

/* Private variables ---------------------------------------------------------*/

static MenuContext_t menu_context;
static uint8_t out_buffer[MAX_COMM_OUT_BUFFER_SIZE];

static uint8_t msg_buffer[MAX_COMM_IN_BUFFER_SIZE];
static uint16_t msg_buf_len = 0;
static uint8_t test_msg[] = "Welcome to the OpenAquatix HMI!\r\n";

static uint16_t pending_input_len = 0;

static const HmiNotification_t hmi_notifications[] = {
  { MESS_DROPPED_PACKET_PREAMBLE,         "RX DECODE FAILED: Dropped a packet with an invalid preamble\r\n" },
  { MESS_DROPPED_PACKET_CARGO,            "RX DECODE FAILED: Dropped a packet with an invalid cargo\r\n" },
  { MESS_MAC_LOST_MESSAGE,                "TX REQUEST FAILED: Lost message in MAC; message not sent\r\n" },
  { MESS_MAC_TX_SPACE,                    "TX REQUEST FAILED: No space in TX queue for message\r\n" },
  { MESS_MAC_DROPPED_MESSAGE,             "TX REQUEST FAILED: Channel did not free in time\r\n" },
  { MESS_FAILED_RANGING_REQUEST,          "RANGE REQUEST FAILED: Failed to send ranging request\r\n" },
  { MESS_FAILED_RANGING_RESPONSE,         "RANGE REQUEST FAILED: Received ranging request, but could not respond\r\n" },
  { MESS_RECEIVED_RANGING_RESPONSE_BAD,   "RANGE REQUEST FAILED: Received valid ranging response, but could not add to queue\r\n" },
  { MESS_FBK_TEST_DISABLED,               "Cannot complete feedback tests as subsystem is disabled\r\n" },
};

/* Private function prototypes -----------------------------------------------*/

static void registerMenus(void);
static RxState_t getHmiInput(CommInterface_t* interface);

static void handleHmiWithdraw(void);
static void handleHmiNavigation(void);
static void handleHmiFunction(void);

static void displaySubMenus(void);
static bool isNumber(uint8_t* buf, uint16_t len);
static bool checkMenuNumberInput(uint8_t* buf, uint16_t len, uint16_t* number);
static void refreshInputLine(uint8_t* buffer, uint16_t len);
static void clearInputLine(void);

static bool printNotifications(void);

static void registerCommParams(void);

static void resetTask(void);

/* Exported function definitions ---------------------------------------------*/

void COMM_StartTask(void *argument)
{
  (void)(argument);
  menu_context.interface = COMM_BOTH;

  Error_RegisterTask("COMM");
  registerCommParams();
  Error_ParameterRegistrationComplete();
  CFG_WaitLoadComplete();

  osDelay(1000);

  COMM_TransmitData(test_msg, sizeof(test_msg) - 1, menu_context.interface);

  registerMenus();

  menu_context.current_menu = MenuSystem_GetMenu(MENU_ID_MAIN);
  displaySubMenus();
  resetTask();
  for(;;) {
    Message_t rx_msg;
    if (MESS_GetMessageFromRxQ(&rx_msg) == true) {
      Print_DisplayReceivedMessage(&rx_msg, out_buffer, menu_context.interface);
    }

    RxState_t state = getHmiInput(&menu_context.interface);
    bool notified = printNotifications();

    switch (state) {
      case DATA_READY:
        pending_input_len = 0;
        if (msg_buffer[0] == WITHDRAW_CHAR && msg_buf_len > 0) {
          handleHmiWithdraw();
        } else {
          COMM_TransmitData("\r\n", 2, menu_context.interface);
          handleHmiNavigation();
          handleHmiFunction();
        }
        break;
      case NEW_CONTENT:
        refreshInputLine(msg_buffer, msg_buf_len);
        pending_input_len = msg_buf_len;
        break;
      case NO_CHANGE:
        if (notified && pending_input_len > 0) {
          // Reprint partial input that was cleared by notifications
          refreshInputLine(msg_buffer, pending_input_len);
        }
        break;
      default:
        break;
    }

    if (Error_CheckModuleReset() == TASK_RESET) {
      resetTask();
    }
    Error_ResetAbortFlag();
    osDelay(10);
  }
}


void COMM_TransmitData(const void *data, uint32_t data_len, CommInterface_t interface)
{
  if (data_len == CALC_LEN) {
    data_len = strlen((char*) data);
  }
  switch (interface) {
    case COMM_USB:
      USB_TransmitData((uint8_t*) data, (uint16_t) data_len);
      break;
    case COMM_UART:
      DAU_TransmitData((uint8_t*) data, (uint16_t) data_len);
      break;
    case COMM_BOTH:
      USB_TransmitData((uint8_t*) data, (uint16_t) data_len);
      DAU_TransmitData((uint8_t*) data, (uint16_t) data_len);
      break;
    default:
      break;
  }
}

/* Private function definitions ----------------------------------------------*/

void registerMenus(void)
{
  COMM_RegisterMainMenu();
  COMM_RegisterConfigurationMenu();
  COMM_RegisterDebugMenu();
  COMM_RegisterHistoryMenu();
  COMM_RegisterTxRxMenu();
  COMM_RegisterEvalMenu();
  COMM_RegisterJanusMenu();
}

RxState_t getHmiInput(CommInterface_t* interface)
{
  // Check for HMI input on USB. If it exists, update state and set interface to USB
  RxState_t state = USB_GetHmiInput(msg_buffer, &msg_buf_len);
  if (state != NO_CHANGE) {
    *interface = COMM_USB;
    return state;
  }

  // Check for HMI input on UART. If it exists, update state and set interface to UART
  state = DAU_GetHmiInput(msg_buffer, &msg_buf_len);
  if (state != NO_CHANGE) {
    *interface = COMM_UART;
    return state;
  }
  return state; // NO_CHANGE
}

void handleHmiWithdraw(void)
{
  menu_context.current_menu->parameters->state = PARAM_STATE_0;
  menu_context.current_menu = MenuSystem_GetMenu(menu_context.current_menu->parent_id);
  displaySubMenus();
}

void handleHmiNavigation(void)
{
  // Cannot navigate without children
  if (menu_context.current_menu->num_children == 0) return;

  uint16_t menu_number;
  if (checkMenuNumberInput(msg_buffer, msg_buf_len, &menu_number) == true) {
    // Valid menu option
    menu_context.current_menu = MenuSystem_GetMenu(menu_context.current_menu->children_ids[menu_number - 1]);
    menu_context.current_menu->parameters->state = PARAM_STATE_0;
  }
  else {
    COMM_TransmitData("\r\nInvalid option!\r\n", CALC_LEN, menu_context.interface);
  }
  displaySubMenus();
}

void handleHmiFunction(void)
{
  if (menu_context.current_menu->num_children != 0) return;

  // no children so handle function
  // Prepare function argument
  FunctionContext_t context = {
      .state = menu_context.current_menu->parameters,
      .input_len = msg_buf_len,
      .output_buffer = msg_buffer,
      .comm_interface = menu_context.interface
  };
  strncpy(context.input, (char*) msg_buffer, MAX_COMM_IN_BUFFER_SIZE);

  (*menu_context.current_menu->handler)(&context);

  if (menu_context.current_menu->parameters->state == PARAM_STATE_COMPLETE) {
    menu_context.current_menu->parameters->state = PARAM_STATE_0;
    menu_context.current_menu = MenuSystem_GetMenu(menu_context.current_menu->parent_id);
    displaySubMenus();
  }
}

void displaySubMenus(void)
{
  if (menu_context.current_menu->num_children == 0) return;
  COMM_TransmitData("\r\n", 2, menu_context.interface);
  COMM_TransmitData(menu_context.current_menu->description, CALC_LEN, menu_context.interface);
  COMM_TransmitData("\r\n", 2, menu_context.interface);
  for (int i = 0; i < menu_context.current_menu->num_children; i++) {
    uint16_t child_id = menu_context.current_menu->children_ids[i];
    // TODO: add error checking
    MenuNode_t* child_menu = MenuSystem_GetMenu(child_id);
    sprintf((char*) out_buffer, "%d: %s\r\n", i + 1, child_menu->description);
    COMM_TransmitData(out_buffer, CALC_LEN, menu_context.interface);
  }
}

bool isNumber(uint8_t* buf, uint16_t len)
{
  for (int i = 0; i < len; i++) {
    if (! isdigit(buf[i])) return false;
  }
  return true;
}

bool checkMenuNumberInput(uint8_t* buf, uint16_t len, uint16_t* number)
{
  if (! isNumber(buf, len)) return false;
  if (len > MAX_MENU_NUMBER_LENGTH) return false;

  *number = (uint16_t) atoi((char*)buf);
  uint16_t num_children = menu_context.current_menu->num_children;

  if (*number > num_children) return false;
  return true;
}

// Clears current line content on the terminal and reprints the buffer
void refreshInputLine(uint8_t* buffer, uint16_t len)
{
  COMM_TransmitData(ANSI_ESCAPE, ANSI_ESCAPE_LEN, menu_context.interface);
  if (len > 0) {
    COMM_TransmitData(buffer, len, menu_context.interface);
  }
}

// Clears the input line without reprinting (used before printing other output)
void clearInputLine(void)
{
  if (pending_input_len > 0) {
    COMM_TransmitData(ANSI_ESCAPE, ANSI_ESCAPE_LEN, menu_context.interface);
  }
}

bool printNotifications(void)
{
  if (print_event_handle == NULL) return false;

  uint32_t flags = osEventFlagsGet(print_event_handle);
  if (flags == 0) return false;

  bool printed = false;

  for (uint32_t i = 0; i < sizeof(hmi_notifications) / sizeof(hmi_notifications[0]); i++) {
    if (flags & hmi_notifications[i].flag) {
      if (printed == false) {
        clearInputLine();  // clear partial input before first notification
        printed = true;
      }
      COMM_TransmitData(hmi_notifications[i].message, CALC_LEN, menu_context.interface);
      osEventFlagsClear(print_event_handle, hmi_notifications[i].flag);
    }
  }

  return printed;
}

void registerCommParams(void)
{
  Print_RegisterParams();
}

void resetTask(void)
{
  USB_Init();
  DAU_Init();
}

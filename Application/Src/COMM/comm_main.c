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
#include <stdio.h>
#include <stdlib.h>

/* Private typedef -----------------------------------------------------------*/

typedef struct {
  MenuNode_t* current_menu;
  CommInterface_t interface;
} MenuContext_t;

/* Private define ------------------------------------------------------------*/

#define ECHO_USB
// #define ECHO_UART

#define MAX_MENU_NUMBER_LENGTH    2
#define BUFFER_BACK_TRACK_AMOUNT  5

/* Private macro -------------------------------------------------------------*/

#define MIN(a, b)                 (((a) < (b)) ? (a) : (b))
#define MAX(a, b)                 (((a) > (b)) ? (a) : (b))

/* Private variables ---------------------------------------------------------*/

static MenuContext_t menu_context;
static uint8_t out_buffer[MAX_COMM_OUT_BUFFER_SIZE];

static uint8_t msg_buffer[MAX_COMM_IN_BUFFER_SIZE];
static uint16_t msg_buf_len = 0;
static uint8_t test_msg[] = "Welcome to the UAM HMI!\r\n";

static uint16_t last_echo_len = 0;

/* Private function prototypes -----------------------------------------------*/

static void registerMenus(void);
static RxState_t getHmiInput(CommInterface_t* interface);

static void handleHmiWithdraw(void);
static void handleHmiNavigation(void);
static void handleHmiFunction(void);

static void echoInput(void);

static void displaySubMenus(void);
static bool isNumber(uint8_t* buf, uint16_t len);
static bool checkMenuNumberInput(uint8_t* buf, uint16_t len, uint16_t* number);
static void updateInputEcho(uint8_t* msg_buffer, uint16_t len);
static void resetInputEcho(void);

static void printNotifications(void);

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

    printNotifications();

    switch (state) {
      case DATA_READY:
        if (msg_buffer[0] == WITHDRAW_CHAR && msg_buf_len > 0) {
          handleHmiWithdraw();
          break;
        }

        handleHmiNavigation();
        handleHmiFunction();
        break;
      case NEW_CONTENT:
        echoInput();
        break;
      case NO_CHANGE:
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
  updateInputEcho(msg_buffer, msg_buf_len);
  osDelay(1);
  resetInputEcho();
  FunctionContext_t context = {
      .state = menu_context.current_menu->parameters,
      .input_len = msg_buf_len,
      .output_buffer = msg_buffer,
      .comm_interface = menu_context.interface
  };
  strncpy(context.input, (char*) msg_buffer, MAX_COMM_IN_BUFFER_SIZE);

  (*menu_context.current_menu->handler)(&context);
  resetInputEcho();

  if (menu_context.current_menu->parameters->state == PARAM_STATE_COMPLETE) {
    menu_context.current_menu->parameters->state = PARAM_STATE_0;
    menu_context.current_menu = MenuSystem_GetMenu(menu_context.current_menu->parent_id);
    displaySubMenus();
  }
}

void echoInput(void)
{
  #ifdef ECHO_USB
    if (menu_context.interface == COMM_USB) {
      updateInputEcho(msg_buffer, msg_buf_len);
    }
  #endif
  #ifdef ECHO_UART
    if (menu_context.interface == COMM_UART) {
      updateInputEcho(msg_buffer, msg_buf_len);
    }
  #endif
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

void updateInputEcho(uint8_t* msg_buffer, uint16_t len)
{
  int16_t len_difference = (int16_t) len - (int16_t) last_echo_len;
  uint16_t start_index;
  if (len_difference > BUFFER_BACK_TRACK_AMOUNT) {
    start_index = last_echo_len;
  }
  else {
    start_index = (len > BUFFER_BACK_TRACK_AMOUNT) ? (len - BUFFER_BACK_TRACK_AMOUNT) : 0;
  }
  uint16_t back_amount = (uint16_t) MIN((int16_t) last_echo_len,
      (int16_t) (BUFFER_BACK_TRACK_AMOUNT - MIN(len_difference, BUFFER_BACK_TRACK_AMOUNT)));
  uint16_t out_buffer_len = back_amount;
  if (back_amount > 0) {
    memset(out_buffer, '\b', back_amount);
  }
  uint16_t new_data_len = MIN(len, MAX(BUFFER_BACK_TRACK_AMOUNT, len_difference));
  for (int i = 0; i < new_data_len; i++) {
    out_buffer[back_amount + i] = msg_buffer[start_index + i];
    out_buffer_len++;
  }
  if (last_echo_len > len) {
    for (int i = 0; i < last_echo_len - len; i++) {
      out_buffer[out_buffer_len++] = ' ';
    }
    for (int i = 0; i < last_echo_len - len; i++) {
      out_buffer[out_buffer_len++] = '\b';
    }
  }

  COMM_TransmitData(out_buffer, MAX(out_buffer_len, len_difference), menu_context.interface);

  last_echo_len = len;
}

void resetInputEcho(void)
{
  last_echo_len = 0;
}

void printNotifications(void)
{
  if (print_event_handle == NULL) return;

  uint32_t flags = osEventFlagsGet(print_event_handle);
  if (flags & MESS_DROPPED_PACKET_PREAMBLE) {
    COMM_TransmitData("Dropped a packet with an invalid preamble\r\n", CALC_LEN, menu_context.interface);
    osEventFlagsClear(print_event_handle, MESS_DROPPED_PACKET_PREAMBLE);
  }
  if (flags & MESS_DROPPED_PACKET_CARGO) {
    COMM_TransmitData("Dropped a packet with an invalid cargo\r\n", CALC_LEN, menu_context.interface);
    osEventFlagsClear(print_event_handle, MESS_DROPPED_PACKET_CARGO);
  }
  if (flags & MESS_MAC_LOST_MESSAGE) {
    COMM_TransmitData("TX REQUEST FAILED: Lost message in MAC; message not sent\r\n", CALC_LEN, menu_context.interface);
    osEventFlagsClear(print_event_handle, MESS_MAC_LOST_MESSAGE);
  }
  if (flags & MESS_MAC_TX_SPACE) {
    COMM_TransmitData("TX REQUEST FAILED: No space in TX queue for message\r\n", CALC_LEN, menu_context.interface);
    osEventFlagsClear(print_event_handle, MESS_MAC_TX_SPACE);
  }
  if (flags & MESS_MAC_DROPPED_MESSAGE) {
    COMM_TransmitData("TX REQUEST FAILED: Channel did not free in time\r\n", CALC_LEN, menu_context.interface);
    osEventFlagsClear(print_event_handle, MESS_MAC_DROPPED_MESSAGE);
  }
  if (flags & MESS_FAILED_RANGING_REQUEST) {
    COMM_TransmitData("Failed to send ranging request\r\n", CALC_LEN, menu_context.interface);
    osEventFlagsClear(print_event_handle, MESS_FAILED_RANGING_REQUEST);
  }
  if (flags & MESS_FAILED_RANGING_RESPONSE) {
    COMM_TransmitData("Received ranging request, but could not respond\r\n", CALC_LEN, menu_context.interface);
    osEventFlagsClear(print_event_handle, MESS_FAILED_RANGING_RESPONSE);
  }
  if (flags & MESS_RECEIVED_RANGING_RESPONSE_BAD) {
    COMM_TransmitData("Received valid ranging response, but could not add to queue\r\n", CALC_LEN, menu_context.interface);
    osEventFlagsClear(print_event_handle, MESS_RECEIVED_RANGING_RESPONSE_BAD);
  }
  if (flags & MESS_FBK_TEST_DISABLED) {
    COMM_TransmitData("Cannot complete feedback tests as subsytem is disabled\r\n", CALC_LEN, menu_context.interface);
    osEventFlagsClear(print_event_handle, MESS_FBK_TEST_DISABLED);
  }
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

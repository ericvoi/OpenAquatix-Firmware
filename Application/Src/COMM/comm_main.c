/*
 * comm_main.c
 *
 *  Created on: Feb 2, 2025
 *      Author: ericv
 */

/* Private includes ----------------------------------------------------------*/

#include "stm32h7xx_hal.h"
#include "cmsis_os.h"
#include "arm_math.h"

#include "usb_comm.h"
#include "dau_card-driver.h"

#include "comm_menu_registration.h"
#include "comm_main.h"
#include "comm_menu_system.h"

#include "mess_main.h"
#include "mess_evaluate.h"

#include "sys_error.h"

#include "cfg_main.h"
#include "cfg_parameters.h"
#include "cfg_defaults.h"

#include <string.h>
#include <ctype.h>

/* Private typedef -----------------------------------------------------------*/

typedef struct {
  MenuNode_t* current_menu;
  CommInterface_t interface;
} MenuContext_t;

/* Private define ------------------------------------------------------------*/

#define MAX_MENU_NUMBER_LENGTH    2
#define BUFFER_BACK_TRACK_AMOUNT  5
#define LEN_RESET                 32451

/* Private macro -------------------------------------------------------------*/

#define MIN(a, b)                 (((a) < (b)) ? (a) : (b))
#define MAX(a, b)                 (((a) > (b)) ? (a) : (b))

/* Private variables ---------------------------------------------------------*/

static MenuContext_t menu_context;
uint8_t out_buffer[MAX_COMM_OUT_BUFFER_SIZE];

static bool print_received_messages = DEFAULT_PRINT_ENABLED;

/* Private function prototypes -----------------------------------------------*/

static void displaySubMenus(void);
static bool isNumber(uint8_t* buf, uint16_t len);
static bool checkMenuNumberInput(uint8_t* buf, uint16_t len, uint16_t* number);
static void updateInputEcho(uint8_t* msg_buffer, uint16_t len);
static void resetInputEcho(void);

static void printReceivedMessage(Message_t* msg);
static void printCustomHeader(Message_t* msg);
static void printCustomData(Message_t* msg);
static void printJanusHeader(Message_t* msg);
static void printJanusData(Message_t* msg);
static void printJanusHeaderParameter(PreambleValue_t parameter);

static void printSenderId(Message_t* msg);
static void printDestinationId(Message_t* msg);
static void printCoding(Message_t* msg);
static void printEncryption(Message_t* msg);

static void printStringData(Message_t* msg);
static void printBitsData(Message_t* msg);
static void printInteger(Message_t* msg);
static void printFloat(Message_t* msg);
static void printEvalMessage(Message_t* msg);

static bool registerCommParams(void);

/* Exported function definitions ---------------------------------------------*/

void COMM_StartTask(void *argument)
{
  (void)(argument);
  USB_Init();
  DAU_Init();
  uint8_t msg_buffer[MAX_COMM_IN_BUFFER_SIZE];
  uint16_t msg_buf_len = 0;
  uint16_t menu_number = 0;
  uint8_t test_msg[] = "Welcome to the UAM HMI!\r\n";
  menu_context.interface = COMM_BOTH;


  if (Param_RegisterTask(COMM_TASK, "COMM") == false) {
    Error_Routine(ERROR_COMM_INIT);
  }

  if (registerCommParams() == false) {
    Error_Routine(ERROR_COMM_INIT);
  }

  if (Param_TaskRegistrationComplete(COMM_TASK) == false) {
    Error_Routine(ERROR_COMM_INIT);
  }

  CFG_WaitLoadComplete();

  osDelay(1000);

  COMM_TransmitData(test_msg, sizeof(test_msg) - 1, menu_context.interface);
  if (COMM_RegisterMainMenu() == false) {
    Error_Routine(ERROR_COMM_INIT);
  }
  if (COMM_RegisterConfigurationMenu() == false) {
    Error_Routine(ERROR_COMM_INIT);
  }
  if (COMM_RegisterDebugMenu() == false) {
    Error_Routine(ERROR_COMM_INIT);
  }
  if (COMM_RegisterHistoryMenu() == false) {
    Error_Routine(ERROR_COMM_INIT);
  }
  if (COMM_RegisterTxRxMenu() == false) {
    Error_Routine(ERROR_COMM_INIT);
  }
  if (COMM_RegisterEvalMenu() == false) {
    Error_Routine(ERROR_COMM_INIT);
  }
  if (COMM_RegisterJanusMenu() == false) {
    Error_Routine(ERROR_COMM_INIT);
  }

  menu_context.current_menu = getMenu(MENU_ID_MAIN);
  displaySubMenus();
  // Main task loop - processes messages and handles menu navigation
  for(;;) {
    Message_t rx_msg;
    if (MESS_GetMessageFromRxQ(&rx_msg) == pdPASS) {
      printReceivedMessage(&rx_msg);
    }

    RxState_t state = USB_GetMessage(msg_buffer, &msg_buf_len);
    if (state == NO_CHANGE) {
      state = DAU_GetMessage(msg_buffer, &msg_buf_len);
      if (state != NO_CHANGE) {
        menu_context.interface = COMM_UART;
      }
    }
    else {
      menu_context.interface = COMM_USB;
    }
    switch (state) {
      case DATA_READY:
        if (msg_buf_len > 0) {
          if (msg_buffer[0] == '\e') {
            menu_context.current_menu->parameters->state = PARAM_STATE_0;
            menu_context.current_menu = getMenu(menu_context.current_menu->parent_id);
            displaySubMenus();
            break;
          }
        }

        if (menu_context.current_menu->num_children != 0) {
          // change menu
          if (checkMenuNumberInput(msg_buffer, msg_buf_len, &menu_number) == true) {
            // Valid menu option
            menu_context.current_menu = getMenu(menu_context.current_menu->children_ids[menu_number - 1]);
            menu_context.current_menu->parameters->state = PARAM_STATE_0;
            displaySubMenus();
          }
          else {
            COMM_TransmitData("\r\nInvalid option!\r\n", CALC_LEN, menu_context.interface);
            displaySubMenus();
          }
        }

        if (menu_context.current_menu->num_children == 0) {
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
            menu_context.current_menu = getMenu(menu_context.current_menu->parent_id);
            displaySubMenus();
          }
        }
        break;
      case NEW_CONTENT:
        if (menu_context.interface == COMM_USB) {
          updateInputEcho(msg_buffer, msg_buf_len);
        }
        else if (menu_context.interface == COMM_UART) {
          updateInputEcho(msg_buffer, msg_buf_len);
        }
        break;
      case NO_CHANGE:
        break;
      default:
        break;
    }
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

void displaySubMenus(void)
{
  if (menu_context.current_menu->num_children == 0) return;
  COMM_TransmitData("\r\n", 2, menu_context.interface);
  COMM_TransmitData(menu_context.current_menu->description, CALC_LEN, menu_context.interface);
  COMM_TransmitData("\r\n", 2, menu_context.interface);
  for (int i = 0; i < menu_context.current_menu->num_children; i++) {
    uint16_t child_id = menu_context.current_menu->children_ids[i];
    // TODO: add error checking
    MenuNode_t* child_menu = getMenu(child_id);
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
  static uint16_t old_len = 0;

  if (len == LEN_RESET) {
    old_len = 0;

    return;
  }

  int16_t len_difference = (int16_t) len - (int16_t) old_len;
  uint16_t start_index;
  if (len_difference > BUFFER_BACK_TRACK_AMOUNT) {
    start_index = old_len;
  }
  else {
    start_index = (len > BUFFER_BACK_TRACK_AMOUNT) ? (len - BUFFER_BACK_TRACK_AMOUNT) : 0;
  }
  uint16_t back_amount = (uint16_t) MIN((int16_t) old_len,
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
  if (old_len > len) {
    for (int i = 0; i < old_len - len; i++) {
      out_buffer[out_buffer_len++] = ' ';
    }
    for (int i = 0; i < old_len - len; i++) {
      out_buffer[out_buffer_len++] = '\b';
    }
  }

  COMM_TransmitData(out_buffer, MAX(out_buffer_len, len_difference), menu_context.interface);

  old_len = len;
}

void resetInputEcho(void)
{
  updateInputEcho(NULL, LEN_RESET);
}

void printReceivedMessage(Message_t* msg)
{
  if (print_received_messages == false) {
    return;
  }

  sprintf((char*) out_buffer, "\r\nReceived a new message at %ds\r\n", (int) msg->timestamp / 1000);
  COMM_TransmitData(out_buffer, CALC_LEN, menu_context.interface);

  switch (msg->protocol) {
    case PROTOCOL_CUSTOM:
      printCustomHeader(msg);
      printCustomData(msg);
      break;
    case PROTOCOL_JANUS:
      printJanusHeader(msg);
      printJanusData(msg);
      break;
    default:
      COMM_TransmitData("Internal error when printing message\r\n", CALC_LEN, menu_context.interface);
  }
  COMM_TransmitData("\r\n\r\n", 4, menu_context.interface);
}

void printCustomHeader(Message_t* msg)
{
  sprintf((char*) out_buffer, "Errors Present: %s\r\n", msg->error_detected ? "Yes" : "No");
  COMM_TransmitData(out_buffer, CALC_LEN, menu_context.interface);

  sprintf((char*) out_buffer, "Sender id: %u\r\n", msg->preamble.modem_id.value);
  COMM_TransmitData(out_buffer, CALC_LEN, menu_context.interface);

  sprintf((char*) out_buffer, "\r\nMessage Length (bits): %u\r\n", msg->length_bits);
  COMM_TransmitData(out_buffer, CALC_LEN, menu_context.interface);

  sprintf((char*) out_buffer, "\r\nMobile sender: %s\r\n", msg->preamble.is_mobile.value ? "Yes" : "No");
  COMM_TransmitData(out_buffer, CALC_LEN, menu_context.interface);
}

void printCustomData(Message_t* msg)
{
  switch (msg->preamble.message_type.value) {
    case STRING:
      sprintf((char*) out_buffer, "String: ");
      COMM_TransmitData(out_buffer, CALC_LEN, menu_context.interface);
      printStringData(msg);
      break;
    case BITS:
      sprintf((char*) out_buffer, "Bits: ");
      COMM_TransmitData(out_buffer, CALC_LEN, menu_context.interface);
      printBitsData(msg);
      break;
    case INTEGER:
      sprintf((char*) out_buffer, "Integer: ");
      COMM_TransmitData(out_buffer, CALC_LEN, menu_context.interface);
      printInteger(msg);
      break;
    case FLOAT:
      sprintf((char*) out_buffer, "Float: ");
      COMM_TransmitData(out_buffer, CALC_LEN, menu_context.interface);
      printFloat(msg);
      break;
    case EVAL:
      printEvalMessage(msg);
      return;
    default:
      COMM_TransmitData("Unknown data type: N/A", CALC_LEN, menu_context.interface);
      break;
  }
}

void printJanusHeader(Message_t* msg)
{
  COMM_TransmitData("Mobility flag: ", CALC_LEN, menu_context.interface);
  printJanusHeaderParameter(msg->preamble.is_mobile);

  COMM_TransmitData("Schedule flag: ", CALC_LEN, menu_context.interface);
  printJanusHeaderParameter(msg->preamble.schedule_flag);

  COMM_TransmitData("Tx/Rx flag: ", CALC_LEN, menu_context.interface);
  printJanusHeaderParameter(msg->preamble.tx_rx_capable);

  COMM_TransmitData("Forwarding capability: ", CALC_LEN, menu_context.interface);
  printJanusHeaderParameter(msg->preamble.can_forward);

  COMM_TransmitData("Class user i.d.: ", CALC_LEN, menu_context.interface);
  printJanusHeaderParameter(msg->preamble.class_user_id);

  COMM_TransmitData("Application type: ", CALC_LEN, menu_context.interface);
  printJanusHeaderParameter(msg->preamble.application_type);

  COMM_TransmitData("Message length (bits): ", CALC_LEN, menu_context.interface);
  sprintf((char*) out_buffer, "%u\r\n", msg->length_bits);
  COMM_TransmitData(out_buffer, CALC_LEN, menu_context.interface);

  switch (msg->janus_data_type) {
    case JANUS_011_01_SMS:
      printSenderId(msg);
      printDestinationId(msg);
      printCoding(msg);
      printEncryption(msg);
      break;
    default:
      COMM_TransmitData("Unknown JANUS message!\r\n", CALC_LEN, menu_context.interface);
  }
}

void printJanusData(Message_t* msg)
{
  switch (msg->janus_data_type) {
    case JANUS_011_01_SMS:
      COMM_TransmitData("SMS: ", CALC_LEN, menu_context.interface);
      printStringData(msg);
      break;
    default:
  }
}

void printJanusHeaderParameter(PreambleValue_t parameter)
{
  if (parameter.valid == true) {
    sprintf((char*) out_buffer, "%u\r\n", parameter.value);
    COMM_TransmitData(out_buffer, CALC_LEN, menu_context.interface);
  }
  else {
    COMM_TransmitData("Parameter not set in preamble!\r\n", CALC_LEN, menu_context.interface);
  }
}

void printSenderId(Message_t* msg)
{
  COMM_TransmitData("Sender i.d.: ", CALC_LEN, menu_context.interface);
  printJanusHeaderParameter(msg->preamble.modem_id);
}

void printDestinationId(Message_t* msg)
{
  COMM_TransmitData("Destination i.d.: ", CALC_LEN, menu_context.interface);
  printJanusHeaderParameter(msg->preamble.destination_id);
}

void printCoding(Message_t* msg)
{
  COMM_TransmitData("Coding: ", CALC_LEN, menu_context.interface);
  printJanusHeaderParameter(msg->preamble.coding);
}

void printEncryption(Message_t* msg)
{
  COMM_TransmitData("Encryption: ", CALC_LEN, menu_context.interface);
  printJanusHeaderParameter(msg->preamble.encryption);
}

void printStringData(Message_t* msg)
{
  uint16_t num_characters = msg->uncoded_data_len / 8;
  if (num_characters > PACKET_DATA_MAX_LENGTH_BYTES) {
    sprintf((char*) out_buffer, "Clipped %u : ", num_characters);
    COMM_TransmitData(out_buffer, CALC_LEN, menu_context.interface);
    num_characters = PACKET_DATA_MAX_LENGTH_BYTES;
  }

  // Printing ASCII values above 127 can cause terminal emulators to become
  // confused and start outputting garbage
  for (uint16_t i = 0; i < num_characters; i++) {
    if (msg->data[i] > 127) {
      msg->data[i] = ' ';
    }
  }

  COMM_TransmitData(msg->data, num_characters, menu_context.interface);
}

void printBitsData(Message_t* msg)
{
  uint16_t length_bits = msg->length_bits;

  if (length_bits > PACKET_DATA_MAX_LENGTH_BYTES * 8) {
    sprintf((char*) out_buffer, "Clipped %u : ", length_bits);
    COMM_TransmitData(out_buffer, CALC_LEN, menu_context.interface);
    length_bits = PACKET_DATA_MAX_LENGTH_BYTES * 8;
  }

  uint16_t remainder_bits = length_bits % 8;
  uint16_t byte_index = 0;

  uint8_t byte = msg->data[byte_index] >> remainder_bits;
  sprintf((char*) out_buffer, "%X ", byte);

  while (length_bits != 0) {
    byte = (msg->data[byte_index] << remainder_bits) | (msg->data[byte_index] >> (7 - remainder_bits));
    sprintf((char*) out_buffer, "%X ", byte);
    COMM_TransmitData(out_buffer, CALC_LEN, menu_context.interface);
    length_bits -= 8;
  }
}

void printInteger(Message_t* msg)
{
  if (msg->length_bits != sizeof(unsigned int) * 8) {
    sprintf((char*) out_buffer, "Forcing %u to %u: ", msg->length_bits, sizeof(unsigned int) * 8);
    COMM_TransmitData(out_buffer, CALC_LEN, menu_context.interface);
  }
  sprintf((char*) out_buffer, "%u", *((unsigned int*) &msg->data[0]));
  COMM_TransmitData(out_buffer, CALC_LEN, menu_context.interface);
}

void printFloat(Message_t* msg)
{
  float temp_float;
  memcpy(&temp_float, &msg->data[0], sizeof(float));
  sprintf((char*) out_buffer, "%f", temp_float);
  COMM_TransmitData(out_buffer, CALC_LEN, menu_context.interface);
}

void printEvalMessage(Message_t* msg)
{
  EvalMessageInfo_t eval_info = msg->eval_info;
  float uncoded_ber = 100.0f * ((float) eval_info.uncoded_errors) / ((float) eval_info.uncoded_bits);
  float coded_ber = 100.0f * ((float) eval_info.coded_errors) / ((float) eval_info.coded_bits);

  COMM_TransmitData("\r\nEvaluation Message:", CALC_LEN, menu_context.interface);

  sprintf((char*) out_buffer, "\r\nUncoded BER: %hu/%hu, %.3f%%", 
          eval_info.uncoded_errors, eval_info.uncoded_bits, uncoded_ber);
  COMM_TransmitData(out_buffer, CALC_LEN, menu_context.interface);

  sprintf((char*) out_buffer, "\r\nCoded BER: %hu/%hu, %.3f%%\r\n",
          eval_info.coded_errors, eval_info.coded_bits, coded_ber);
  COMM_TransmitData(out_buffer, CALC_LEN, menu_context.interface);
}

bool registerCommParams(void)
{
  uint32_t min_u32 = (uint32_t) MIN_PRINT_ENABLED;
  uint32_t max_u32 = (uint32_t) MAX_PRINT_ENABLED;
  if (Param_Register(PARAM_PRINT_ENABLED, "printing received messages", PARAM_TYPE_UINT8,
                     &print_received_messages, sizeof(bool), &min_u32, &max_u32, NULL) == false) {
    return false;
  }

  return true;
}

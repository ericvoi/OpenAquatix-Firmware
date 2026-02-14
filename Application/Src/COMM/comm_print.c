/*
 * comm_print.c
 *
 *  Created on: Feb 12, 2026
 *      Author: ericv
 * Copyright (c) 2025 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

/* Private includes ----------------------------------------------------------*/

#include "comm_print.h"
#include "comm_main.h"
#include "mess_main.h"
#include "mess_sync.h"
#include "cfg_parameters.h"
#include "cfg_defaults.h"
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/



/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/

static bool print_received_messages = DEFAULT_PRINT_ENABLED;
static CargoErrorBehavior_t cargo_error_behavior = DEFAULT_CARGO_ERROR_BEHAVIOR;

DEFINE_DESC_TABLE(CARGO_ERROR_BEHAVIOR_TABLE, cargo_error_behavior_descriptors)

/* Private function prototypes -----------------------------------------------*/

static void printCustomHeader(Message_t* msg, uint8_t* out_buffer, CommInterface_t interface);
static void printCustomData(Message_t* msg, uint8_t* out_buffer, CommInterface_t interface);
static void printJanusHeader(Message_t* msg, uint8_t* out_buffer, CommInterface_t interface);
static void printJanusData(Message_t* msg, uint8_t* out_buffer, CommInterface_t interface);
static void printJanusHeaderParameter(PreambleValue_t parameter, uint8_t* out_buffer, CommInterface_t interface);

static void printSenderId(Message_t* msg, uint8_t* out_buffer, CommInterface_t interface);
static void printDestinationId(Message_t* msg, uint8_t* out_buffer, CommInterface_t interface);
static void printCoding(Message_t* msg, uint8_t* out_buffer, CommInterface_t interface);
static void printEncryption(Message_t* msg, uint8_t* out_buffer, CommInterface_t interface);

static void printStringData(Message_t* msg, uint8_t* out_buffer, CommInterface_t interface);
static void printBitsData(Message_t* msg, uint8_t* out_buffer, CommInterface_t interface);
static void printInteger(Message_t* msg, uint8_t* out_buffer, CommInterface_t interface);
static void printFloat(Message_t* msg, uint8_t* out_buffer, CommInterface_t interface);
static void printEvalMessage(Message_t* msg, uint8_t* out_buffer, CommInterface_t interface);

/* Exported function definitions ---------------------------------------------*/

void Print_DisplayReceivedMessage(Message_t* msg, uint8_t* out_buffer, CommInterface_t interface)
{
  if (print_received_messages == false) {
    return;
  }

  if (cargo_error_behavior == CARGO_ERROR_DROP && msg->error_detected == true) {
    return;
  }

  sprintf((char*) out_buffer, "\r\nReceived a new message at %ds\r\n", (int) msg->timestamp / 1000);
  COMM_TransmitData(out_buffer, CALC_LEN, interface);

  if (cargo_error_behavior == CARGO_ERROR_NOTIFY && msg->error_detected == true) {
    COMM_TransmitData("Message cargo contained errors\r\n", CALC_LEN, interface);
  }

  switch (msg->protocol) {
    case PROTOCOL_CUSTOM:
      printCustomHeader(msg, out_buffer, interface);
      printCustomData(msg, out_buffer, interface);
      break;
    case PROTOCOL_JANUS:
      printJanusHeader(msg, out_buffer, interface);
      printJanusData(msg, out_buffer, interface);
      break;
    default:
      COMM_TransmitData("Internal error when printing message\r\n", CALC_LEN, interface);
  }
  COMM_TransmitData("\r\n\r\n", 4, interface);
}

bool Print_RegisterParams(void)
{
  uint32_t min_u32 = (uint32_t) MIN_PRINT_ENABLED;
  uint32_t max_u32 = (uint32_t) MAX_PRINT_ENABLED;
  if (Param_Register(PARAM_PRINT_ENABLED, "printing received messages",
                     PARAM_TYPE_UINT8, &print_received_messages, sizeof(bool), 
                     &min_u32, &max_u32, NULL, NULL) == false) {
    return false;
  }

  min_u32 = MIN_CARGO_ERROR_BEHAVIOR;
  max_u32 = MAX_CARGO_ERROR_BEHAVIOR;
  if (Param_Register(PARAM_CARGO_ERROR_BEHAVIOR, "cargo error behavior", 
                     PARAM_TYPE_ENUM, &cargo_error_behavior, sizeof(uint8_t), 
                     &min_u32, &max_u32, NULL, 
                     cargo_error_behavior_descriptors) == false) {
    return false;
  }

  return true;
}

/* Private function definitions ----------------------------------------------*/

void printCustomHeader(Message_t* msg, uint8_t* out_buffer, CommInterface_t interface)
{
  sprintf((char*) out_buffer, "Errors Present: %s\r\n", msg->error_detected ? "Yes" : "No");
  COMM_TransmitData(out_buffer, CALC_LEN, interface);

  sprintf((char*) out_buffer, "Sender id: %u\r\n", msg->preamble.modem_id.value);
  COMM_TransmitData(out_buffer, CALC_LEN, interface);

  sprintf((char*) out_buffer, "Message Length (bits): %u\r\n", msg->length_bits);
  COMM_TransmitData(out_buffer, CALC_LEN, interface);

  sprintf((char*) out_buffer, "Mobile sender: %s\r\n", msg->preamble.is_mobile.value ? "Yes" : "No");
  COMM_TransmitData(out_buffer, CALC_LEN, interface);

  float most_recent_snr = Sync_MostRecentSnr();
  sprintf((char*) out_buffer, "SNR: %.2f\r\n", most_recent_snr);
  COMM_TransmitData(out_buffer, CALC_LEN, interface);
}

void printCustomData(Message_t* msg, uint8_t* out_buffer, CommInterface_t interface)
{
  switch (msg->preamble.message_type.value) {
    case STRING:
      sprintf((char*) out_buffer, "String: ");
      COMM_TransmitData(out_buffer, CALC_LEN, interface);
      printStringData(msg, out_buffer, interface);
      break;
    case BITS:
      sprintf((char*) out_buffer, "Bits: ");
      COMM_TransmitData(out_buffer, CALC_LEN, interface);
      printBitsData(msg, out_buffer, interface);
      break;
    case INTEGER:
      sprintf((char*) out_buffer, "Integer: ");
      COMM_TransmitData(out_buffer, CALC_LEN, interface);
      printInteger(msg, out_buffer, interface);
      break;
    case FLOAT:
      sprintf((char*) out_buffer, "Float: ");
      COMM_TransmitData(out_buffer, CALC_LEN, interface);
      printFloat(msg, out_buffer, interface);
      break;
    case EVAL:
      printEvalMessage(msg, out_buffer, interface);
      return;
    default:
      COMM_TransmitData("Unknown data type: N/A", CALC_LEN, interface);
      break;
  }
}

void printJanusHeader(Message_t* msg, uint8_t* out_buffer, CommInterface_t interface)
{
  COMM_TransmitData("Mobility flag: ", CALC_LEN, interface);
  printJanusHeaderParameter(msg->preamble.is_mobile, out_buffer, interface);

  COMM_TransmitData("Schedule flag: ", CALC_LEN, interface);
  printJanusHeaderParameter(msg->preamble.schedule_flag, out_buffer, interface);

  COMM_TransmitData("Tx/Rx flag: ", CALC_LEN, interface);
  printJanusHeaderParameter(msg->preamble.tx_rx_capable, out_buffer, interface);

  COMM_TransmitData("Forwarding capability: ", CALC_LEN, interface);
  printJanusHeaderParameter(msg->preamble.can_forward, out_buffer, interface);

  COMM_TransmitData("Class user i.d.: ", CALC_LEN, interface);
  printJanusHeaderParameter(msg->preamble.class_user_id, out_buffer, interface);

  COMM_TransmitData("Application type: ", CALC_LEN, interface);
  printJanusHeaderParameter(msg->preamble.application_type, out_buffer, interface);

  COMM_TransmitData("Message length (bits): ", CALC_LEN, interface);
  sprintf((char*) out_buffer, "%u\r\n", msg->length_bits);
  COMM_TransmitData(out_buffer, CALC_LEN, interface);

  switch (msg->janus_data_type) {
    case JANUS_011_01_SMS:
      printSenderId(msg, out_buffer, interface);
      printDestinationId(msg, out_buffer, interface);
      printCoding(msg, out_buffer, interface);
      printEncryption(msg, out_buffer, interface);
      break;
    default:
      COMM_TransmitData("Unknown JANUS message!\r\n", CALC_LEN, interface);
  }
}

void printJanusData(Message_t* msg, uint8_t* out_buffer, CommInterface_t interface)
{
  switch (msg->janus_data_type) {
    case JANUS_011_01_SMS:
      COMM_TransmitData("SMS: ", CALC_LEN, interface);
      printStringData(msg, out_buffer, interface);
      break;
    default:
  }
}

void printJanusHeaderParameter(PreambleValue_t parameter, uint8_t* out_buffer, CommInterface_t interface)
{
  if (parameter.valid == true) {
    sprintf((char*) out_buffer, "%u\r\n", parameter.value);
    COMM_TransmitData(out_buffer, CALC_LEN, interface);
  }
  else {
    COMM_TransmitData("Parameter not set in preamble!\r\n", CALC_LEN, interface);
  }
}

void printSenderId(Message_t* msg, uint8_t* out_buffer, CommInterface_t interface)
{
  COMM_TransmitData("Sender i.d.: ", CALC_LEN, interface);
  printJanusHeaderParameter(msg->preamble.modem_id, out_buffer, interface);
}

void printDestinationId(Message_t* msg, uint8_t* out_buffer, CommInterface_t interface)
{
  COMM_TransmitData("Destination i.d.: ", CALC_LEN, interface);
  printJanusHeaderParameter(msg->preamble.destination_id, out_buffer, interface);
}

void printCoding(Message_t* msg, uint8_t* out_buffer, CommInterface_t interface)
{
  COMM_TransmitData("Coding: ", CALC_LEN, interface);
  printJanusHeaderParameter(msg->preamble.coding, out_buffer, interface);
}

void printEncryption(Message_t* msg, uint8_t* out_buffer, CommInterface_t interface)
{
  COMM_TransmitData("Encryption: ", CALC_LEN, interface);
  printJanusHeaderParameter(msg->preamble.encryption, out_buffer, interface);
}

void printStringData(Message_t* msg, uint8_t* out_buffer, CommInterface_t interface)
{
  uint16_t num_characters = msg->uncoded_data_len / 8;
  if (num_characters > PACKET_DATA_MAX_LENGTH_BYTES) {
    sprintf((char*) out_buffer, "Clipped %u : ", num_characters);
    COMM_TransmitData(out_buffer, CALC_LEN, interface);
    num_characters = PACKET_DATA_MAX_LENGTH_BYTES;
  }

  // Printing ASCII values above 127 can cause terminal emulators to become
  // confused and start outputting garbage
  for (uint16_t i = 0; i < num_characters; i++) {
    if (msg->data[i] > 127) {
      msg->data[i] = ' ';
    }
  }

  COMM_TransmitData(msg->data, num_characters, interface);
}

void printBitsData(Message_t* msg, uint8_t* out_buffer, CommInterface_t interface)
{
  uint16_t length_bits = msg->length_bits;

  if (length_bits > PACKET_DATA_MAX_LENGTH_BYTES * 8) {
    sprintf((char*) out_buffer, "Clipped %u : ", length_bits);
    COMM_TransmitData(out_buffer, CALC_LEN, interface);
    length_bits = PACKET_DATA_MAX_LENGTH_BYTES * 8;
  }

  uint16_t remainder_bits = length_bits % 8;
  uint16_t byte_index = 0;

  uint8_t byte = msg->data[byte_index] >> remainder_bits;
  sprintf((char*) out_buffer, "%X ", byte);

  while (length_bits != 0) {
    byte = (msg->data[byte_index] << remainder_bits) | (msg->data[byte_index] >> (7 - remainder_bits));
    sprintf((char*) out_buffer, "%X ", byte);
    COMM_TransmitData(out_buffer, CALC_LEN, interface);
    length_bits -= 8;
    byte_index++;
  }
}

void printInteger(Message_t* msg, uint8_t* out_buffer, CommInterface_t interface)
{
  if (msg->length_bits != sizeof(unsigned int) * 8) {
    sprintf((char*) out_buffer, "Forcing %u to %u: ", msg->length_bits, sizeof(unsigned int) * 8);
    COMM_TransmitData(out_buffer, CALC_LEN, interface);
  }
  sprintf((char*) out_buffer, "%u", *((unsigned int*) &msg->data[0]));
  COMM_TransmitData(out_buffer, CALC_LEN, interface);
}

void printFloat(Message_t* msg, uint8_t* out_buffer, CommInterface_t interface)
{
  float temp_float;
  memcpy(&temp_float, &msg->data[0], sizeof(float));
  sprintf((char*) out_buffer, "%f", temp_float);
  COMM_TransmitData(out_buffer, CALC_LEN, interface);
}

void printEvalMessage(Message_t* msg, uint8_t* out_buffer, CommInterface_t interface)
{
  EvalMessageInfo_t eval_info = msg->eval_info;
  float uncoded_ber = 100.0f * ((float) eval_info.uncoded_errors) / ((float) eval_info.uncoded_bits);
  float coded_ber = 100.0f * ((float) eval_info.coded_errors) / ((float) eval_info.coded_bits);

  COMM_TransmitData("\r\nEvaluation Message:", CALC_LEN, interface);

  sprintf((char*) out_buffer, "\r\nUncoded BER: %hu/%hu, %.3f%%", 
          eval_info.uncoded_errors, eval_info.uncoded_bits, uncoded_ber);
  COMM_TransmitData(out_buffer, CALC_LEN, interface);

  sprintf((char*) out_buffer, "\r\nCoded BER: %hu/%hu, %.3f%%\r\n",
          eval_info.coded_errors, eval_info.coded_bits, coded_ber);
  COMM_TransmitData(out_buffer, CALC_LEN, interface);
}

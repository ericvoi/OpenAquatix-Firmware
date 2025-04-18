/*
 * cfg_import_export.c
 *
 *  Created on: Apr 17, 2025
 *      Author: ericv
 */

/* Private includes ----------------------------------------------------------*/

#include "cfg_import_export.h"
#include "cfg_parameters.h"

#include "comm_menu_system.h"
#include "comm_main.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/

#define START_SEQUENCE    "START"
#define END_SEQUENCE      "END"
// The end of a message is delimited by a newline so a comma is required when
// pasting since otherwise the pasted contents will be cut prematurely
#define DELIMITER         ","

/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/

static ParamIds_t imp_exp_parameters[] = {
    PARAM_BAUD,
    PARAM_FSK_F0,
    PARAM_FSK_F1,
    PARAM_MOD_DEMOD_METHOD,
    PARAM_FC,
    PARAM_FHBFSK_FREQ_SPACING,
    PARAM_FHBFSK_DWELL_TIME,
    PARAM_FHBFSK_NUM_TONES,
    PARAM_ERROR_CORRECTION
};

static const uint16_t num_param = sizeof(imp_exp_parameters) / sizeof(imp_exp_parameters[0]);

/* Private function prototypes -----------------------------------------------*/



/* Exported function definitions ---------------------------------------------*/

bool ImportExport_ExportConfiguration(FunctionContext_t* context)
{
  sprintf((char*) context->output_buffer,"\r\nPlease copy from %s to %s\r\n\r\n",
      START_SEQUENCE, END_SEQUENCE);
  COMM_TransmitData(context->output_buffer, CALC_LEN, context->comm_interface);

  COMM_TransmitData(START_SEQUENCE, CALC_LEN, context->comm_interface);

  COMM_TransmitData(DELIMITER, CALC_LEN, context->comm_interface);

  for (uint16_t i = 0; i < num_param; i++) {
    ParamIds_t id = imp_exp_parameters[i];
    ParamType_t param_type;
    if (Param_GetParamType(id, &param_type) == false) {
      return false;
    }
    uint16_t buffer_index = 0;
    // Add id
    buffer_index += sprintf((char*) context->output_buffer, "%hu-", id);
    // Add data (all data types)
    switch (param_type) {
      case PARAM_TYPE_UINT8: {
        uint8_t value;
        if (Param_GetUint8(id, &value) == false) {
          return false;
        }
        buffer_index += sprintf((char*) &context->output_buffer[buffer_index], "%hu", value);
        break;
      }
      case PARAM_TYPE_INT8: {
        int8_t value;
        if (Param_GetInt8(id, &value) == false) {
          return false;
        }
        buffer_index += sprintf((char*) &context->output_buffer[buffer_index], "%hd", value);
        break;
      }
      case PARAM_TYPE_UINT16: {
        uint16_t value;
        if (Param_GetUint16(id, &value) == false) {
          return false;
        }
        buffer_index += sprintf((char*) &context->output_buffer[buffer_index], "%u", value);
        break;
      }
      case PARAM_TYPE_INT16: {
        int16_t value;
        if (Param_GetInt16(id, &value) == false) {
          return false;
        }
        buffer_index += sprintf((char*) &context->output_buffer[buffer_index], "%d", value);
        break;
      }
      case PARAM_TYPE_UINT32: {
        uint32_t value;
        if (Param_GetUint32(id, &value) == false) {
          return false;
        }
        buffer_index += sprintf((char*) &context->output_buffer[buffer_index], "%lu", value);
        break;
      }
      case PARAM_TYPE_INT32: {
        int32_t value;
        if (Param_GetInt32(id, &value) == false) {
          return false;
        }
        buffer_index += sprintf((char*) &context->output_buffer[buffer_index], "%ld", value);
        break;
      }
      case PARAM_TYPE_FLOAT: {
        float value;
        if (Param_GetFloat(id, &value) == false) {
          return false;
        }
        buffer_index += sprintf((char*) &context->output_buffer[buffer_index], "%f", value);
        break;
      }
      default:
        return false;
    }
    buffer_index += sprintf((char*) &context->output_buffer[buffer_index], DELIMITER);
    COMM_TransmitData(context->output_buffer, buffer_index, context->comm_interface);
  }
  COMM_TransmitData(END_SEQUENCE "\r\n", CALC_LEN, context->comm_interface);
  context->state->state = PARAM_STATE_COMPLETE;
  return true;
}

bool ImportExport_ImportConfiguration(FunctionContext_t* context)
{
  switch (context->state->state) {
    case PARAM_STATE_0:
      COMM_TransmitData("\r\nPlease paste the configuration data from " START_SEQUENCE
          " to " END_SEQUENCE ":\r\n", CALC_LEN, context->comm_interface);
      context->state->state = PARAM_STATE_1;
      return true;
    case PARAM_STATE_1: {
      context->input[context->input_len] = '\0';
      const char* start = strstr(context->input, START_SEQUENCE);
      if (start == NULL) {
        COMM_TransmitData("\r\nError: start sequence not found\r\n", CALC_LEN,
            context->comm_interface);
        context->state->state = PARAM_STATE_COMPLETE;
        return false;
      }
      start += strlen(START_SEQUENCE);

      const char* curr = start;
      uint16_t params_imported = 0;

      const char* end = strstr(start, END_SEQUENCE);
      if (end == NULL) {
        COMM_TransmitData("\r\nError: end sequence not found\r\n", CALC_LEN,
                    context->comm_interface);
                context->state->state = PARAM_STATE_COMPLETE;
                return false;
      }

      while (curr < end) {
        // skip delimiters and whitespace
        while (curr < end && (*curr == ',' || *curr == ' ' || *curr == '\r' || *curr == '\n')) {
          curr++;
        }

        if (curr >= end) break;

        char* end_ptr;
        ParamIds_t id = (ParamIds_t) strtoul(curr, &end_ptr, 10);

        if (id >= NUM_PARAM) {
          COMM_TransmitData("\r\nInvalid id received!\r\n", CALC_LEN, context->comm_interface);
          context->state->state = PARAM_STATE_COMPLETE;
          return false;
        }

        if (*end_ptr != '-') {
          COMM_TransmitData("\r\nInvalid format!\r\n", CALC_LEN, context->comm_interface);
          context->state->state = PARAM_STATE_COMPLETE;
          return false;
        }

        curr = end_ptr + 1;

        ParamType_t param_type;
        if (Param_GetParamType(id, &param_type) == false) {
          sprintf((char*) context->output_buffer, "\r\nUnknown ID: %u\r\n", id);
          COMM_TransmitData(context->output_buffer, CALC_LEN, context->comm_interface);
          context->state->state = PARAM_STATE_COMPLETE;
          return false;
        }

        bool set_result = false;
        switch (param_type) {
          case PARAM_TYPE_UINT8: {
            uint8_t value = (uint8_t) strtoul(curr, &end_ptr, 10);
            set_result = Param_SetUint8(id, &value);
            break;
          }
          case PARAM_TYPE_INT8: {
            int8_t value = (int8_t) strtol(curr, &end_ptr, 10);
            set_result = Param_SetInt8(id, &value);
            break;
          }
          case PARAM_TYPE_UINT16: {
            uint16_t value = (uint16_t) strtoul(curr, &end_ptr, 10);
            set_result = Param_SetUint16(id, &value);
            break;
          }
          case PARAM_TYPE_INT16: {
            int16_t value = (int16_t) strtol(curr, &end_ptr, 10);
            set_result = Param_SetInt16(id, &value);
            break;
          }
          case PARAM_TYPE_UINT32: {
            uint32_t value = (uint32_t) strtoul(curr, &end_ptr, 10);
            set_result = Param_SetUint32(id, &value);
            break;
          }
          case PARAM_TYPE_INT32: {
            int32_t value = (int32_t) strtol(curr, &end_ptr, 10);
            set_result = Param_SetInt32(id, &value);
            break;
          }
          case PARAM_TYPE_FLOAT: {
            float value = strtof(curr, &end_ptr);
            set_result = Param_SetFloat(id, &value);
            break;
          }
          default:
            sprintf((char*) context->output_buffer, "\r\nUnknown parameter type for ID %u\r\n", id);
            COMM_TransmitData(context->output_buffer, CALC_LEN, context->comm_interface);
            context->state->state = PARAM_STATE_COMPLETE;
            return false;
        }

        if (set_result == false) {
          sprintf((char*) context->output_buffer, "\r\nFailed to set parameter with ID %u\r\n", id);
          COMM_TransmitData(context->output_buffer, CALC_LEN, context->comm_interface);
          context->state->state = PARAM_STATE_COMPLETE;
          return false;
        }

        curr = end_ptr;
        params_imported++;
      }
      if (params_imported != num_param) {
        sprintf((char*) context->output_buffer, "\r\nError: Imported %u parameters while %u expected\r\n", params_imported, num_param);
        COMM_TransmitData(context->output_buffer, CALC_LEN, context->comm_interface);
        context->state->state = PARAM_STATE_COMPLETE;
        return false;
      }

      sprintf((char*) context->output_buffer, "\r\nSuccessfully imported %u parameters\r\n", params_imported);
      COMM_TransmitData(context->output_buffer, CALC_LEN, context->comm_interface);

      context->state->state = PARAM_STATE_COMPLETE;
      return true;
    }
    default:
      context->state->state = PARAM_STATE_COMPLETE;
      return false;
  }
}

/* Private function definitions ----------------------------------------------*/

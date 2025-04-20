/*
 * comm_function_loops.c
 *
 *  Created on: Mar 11, 2025
 *      Author: ericv
 */

/* Private includes ----------------------------------------------------------*/

#include "comm_function_loops.h"
#include "comm_menu_system.h"
#include "comm_main.h"
#include "cfg_parameters.h"
#include "check_inputs.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/



/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/

char uninitialized_parameter_message[] = "\r\nThis parameter has not been initialized yet!\r\n";
char error_limits_message[] = "\r\nError getting the range for this parameter\r\n";
char error_updating_message[] = "\r\nError updating parameter\r\n";
char internal_error_message[] = "\r\nInternal Error when trying to get parameter\r\n";

/* Private function prototypes -----------------------------------------------*/



/* Exported function definitions ---------------------------------------------*/

void COMMLoops_LoopUint32(FunctionContext_t* context, ParamIds_t param_id)
{
  ParamState_t old_state = context->state->state;

  char* parameter_name = Param_GetName(param_id);

  if (parameter_name == NULL) {
    COMM_TransmitData(uninitialized_parameter_message, sizeof(uninitialized_parameter_message) - 1, context->comm_interface);
    context->state->state = PARAM_STATE_COMPLETE;
    return;
  }

  uint32_t min, max;
  if (Param_GetUint32Limits(param_id, &min, &max) == false) {
    COMM_TransmitData(error_limits_message, sizeof(error_limits_message) - 1, 
        context->comm_interface);
    context->state->state = PARAM_STATE_COMPLETE;
    return;
  }

  /* This do-while construct allows multiple state transitions in a single call
   * while preventing infinite loops by ensuring we never return to a higher state */
  do {
    switch (context->state->state) {
      case PARAM_STATE_0:
        uint32_t current_value;
        if (Param_GetUint32(param_id, &current_value) == false) {
          sprintf((char*) context->output_buffer, "\r\nError obtaining current"
              " value for %s\r\n", parameter_name);
          COMM_TransmitData(context->output_buffer, CALC_LEN, 
              context->comm_interface);
          context->state->state = PARAM_STATE_COMPLETE;
        }
        else {
          sprintf((char*) context->output_buffer, "\r\n\r\nCurrent value of %s:"
              " %lu\r\n", parameter_name, current_value);
          COMM_TransmitData(context->output_buffer, CALC_LEN, 
              context->comm_interface);
          sprintf((char*) context->output_buffer, "Please enter a new value "
              "from %lu to %lu:\r\n", min, max);
          COMM_TransmitData(context->output_buffer, CALC_LEN, 
              context->comm_interface);
          context->state->state = PARAM_STATE_1;
        }
        break;
      case PARAM_STATE_1:
        uint32_t new_value;
        if (checkUint32(context->input, context->input_len, &new_value, min, max) == true) {
          if (Param_SetUint32(param_id, &new_value) == true) {
            sprintf((char*) context->output_buffer, "\r\n%s successfully set "
                "to new value of %lu\r\n", parameter_name, new_value);
            COMM_TransmitData(context->output_buffer, CALC_LEN, 
                context->comm_interface);
          }
          else {
            COMM_TransmitData(error_updating_message, sizeof(error_updating_message) - 1, 
                context->comm_interface);
          }
          context->state->state = PARAM_STATE_COMPLETE;
        } else {
          sprintf((char*) context->output_buffer, "\r\nValue %lu is outside the"
              " range of %lu and %lu\r\n", new_value, min, max);
          COMM_TransmitData(context->output_buffer, CALC_LEN, 
              context->comm_interface);
          context->state->state = PARAM_STATE_0;
        }
        break;
      default:
        context->state->state = PARAM_STATE_COMPLETE;
        break;
    }
  } while (old_state > context->state->state);
}

void COMMLoops_LoopUint16(FunctionContext_t* context, ParamIds_t param_id)
{
  ParamState_t old_state = context->state->state;

  char* parameter_name = Param_GetName(param_id);

  if (parameter_name == NULL) {
    COMM_TransmitData(uninitialized_parameter_message, sizeof(uninitialized_parameter_message) - 1, 
        context->comm_interface);
    context->state->state = PARAM_STATE_COMPLETE;
    return;
  }

  uint16_t min, max;
  if (Param_GetUint16Limits(param_id, &min, &max) == false) {
    COMM_TransmitData(error_limits_message, sizeof(error_limits_message) - 1, 
        context->comm_interface);
    context->state->state = PARAM_STATE_COMPLETE;
    return;
  }

  /* This do-while construct allows multiple state transitions in a single call
   * while preventing infinite loops by ensuring we never return to a higher state */
  do {
    switch (context->state->state) {
      case PARAM_STATE_0:
        uint16_t current_value;
        if (Param_GetUint16(param_id, &current_value) == false) {
          sprintf((char*) context->output_buffer, "\r\nError obtaining current"
              " value for %s\r\n", parameter_name);
          COMM_TransmitData(context->output_buffer, CALC_LEN, 
              context->comm_interface);
          context->state->state = PARAM_STATE_COMPLETE;
        }
        else {
          sprintf((char*) context->output_buffer, "\r\n\r\nCurrent value of %s:"
              " %u\r\n", parameter_name, current_value);
          COMM_TransmitData(context->output_buffer, CALC_LEN, 
              context->comm_interface);
          sprintf((char*) context->output_buffer, "Please enter a new value "
              "from %u to %u:\r\n", min, max);
          COMM_TransmitData(context->output_buffer, CALC_LEN, 
              context->comm_interface);
          context->state->state = PARAM_STATE_1;
        }
        break;
      case PARAM_STATE_1:
        uint16_t new_value;
        if (checkUint16(context->input, context->input_len, &new_value, min, max) == true) {
          if (Param_SetUint16(param_id, &new_value) == true) {
            sprintf((char*) context->output_buffer, "\r\n%s successfully set "
                "to new value of %u\r\n", parameter_name, new_value);
            COMM_TransmitData(context->output_buffer, CALC_LEN, 
                context->comm_interface);
          }
          else {
            COMM_TransmitData(error_updating_message, sizeof(error_updating_message) - 1, 
                context->comm_interface);
          }
          context->state->state = PARAM_STATE_COMPLETE;
        } else {
          sprintf((char*) context->output_buffer, "\r\nValue %u is outside the"
          " range of %u and %u\r\n", new_value, min, max);
          COMM_TransmitData(context->output_buffer, CALC_LEN, 
              context->comm_interface);
          context->state->state = PARAM_STATE_0;
        }
        break;
      default:
        context->state->state = PARAM_STATE_COMPLETE;
        break;
    }
  } while (old_state > context->state->state);
}

void COMMLoops_LoopUint8(FunctionContext_t* context, ParamIds_t param_id)
{
  ParamState_t old_state = context->state->state;

  char* parameter_name = Param_GetName(param_id);

  if (parameter_name == NULL) {
    COMM_TransmitData(uninitialized_parameter_message, sizeof(uninitialized_parameter_message) - 1, 
        context->comm_interface);
    context->state->state = PARAM_STATE_COMPLETE;
    return;
  }

  uint8_t min, max;
  if (Param_GetUint8Limits(param_id, &min, &max) == false) {
    COMM_TransmitData(error_limits_message, sizeof(error_limits_message) - 1, 
        context->comm_interface);
    context->state->state = PARAM_STATE_COMPLETE;
    return;
  }

  /* This do-while construct allows multiple state transitions in a single call
   * while preventing infinite loops by ensuring we never return to a higher state */
  do {
    switch (context->state->state) {
      case PARAM_STATE_0:
        uint8_t current_value;
        if (Param_GetUint8(param_id, &current_value) == false) {
          sprintf((char*) context->output_buffer, "\r\nError obtaining current"
              " value for %s\r\n", parameter_name);
          COMM_TransmitData(context->output_buffer, CALC_LEN, 
              context->comm_interface);
          context->state->state = PARAM_STATE_COMPLETE;
        }
        else {
          sprintf((char*) context->output_buffer, "\r\n\r\nCurrent value of %s:"
              " %u\r\n", parameter_name, current_value);
          COMM_TransmitData(context->output_buffer, CALC_LEN, 
              context->comm_interface);
          sprintf((char*) context->output_buffer, "Please enter a new value "
              "from %u to %u:\r\n", min, max);
          COMM_TransmitData(context->output_buffer, CALC_LEN, 
              context->comm_interface);
          context->state->state = PARAM_STATE_1;
        }
        break;
      case PARAM_STATE_1:
        uint8_t new_value;
        if (checkUint8(context->input, context->input_len, &new_value, min, max) == true) {
          if (Param_SetUint8(param_id, &new_value) == true) {
            sprintf((char*) context->output_buffer, "\r\n%s successfully set "
                "to new value of %u\r\n", parameter_name, new_value);
            COMM_TransmitData(context->output_buffer, CALC_LEN, 
                context->comm_interface);
          }
          else {
            COMM_TransmitData(error_updating_message, sizeof(error_updating_message) - 1, 
                context->comm_interface);
          }
          context->state->state = PARAM_STATE_COMPLETE;
        } else {
          sprintf((char*) context->output_buffer, "\r\nValue %u is outside the "
              "range of %u and %u\r\n", new_value, min, max);
          COMM_TransmitData(context->output_buffer, CALC_LEN, 
              context->comm_interface);
          context->state->state = PARAM_STATE_0;
        }
        break;
      default:
        context->state->state = PARAM_STATE_COMPLETE;
        break;
    }
  } while (old_state > context->state->state);
}

void COMMLoops_LoopFloat(FunctionContext_t* context, ParamIds_t param_id)
{
  ParamState_t old_state = context->state->state;

  char* parameter_name = Param_GetName(param_id);

  if (parameter_name == NULL) {
    COMM_TransmitData(uninitialized_parameter_message, sizeof(uninitialized_parameter_message) - 1, 
        context->comm_interface);
    context->state->state = PARAM_STATE_COMPLETE;
    return;
  }

  float min, max;
  if (Param_GetFloatLimits(param_id, &min, &max) == false) {
    COMM_TransmitData(error_limits_message, sizeof(error_limits_message) - 1, 
        context->comm_interface);
    context->state->state = PARAM_STATE_COMPLETE;
    return;
  }

  /* This do-while construct allows multiple state transitions in a single call
   * while preventing infinite loops by ensuring we never return to a higher state */
  do {
    switch (context->state->state) {
      case PARAM_STATE_0:
        float current_value;
        if (Param_GetFloat(param_id, &current_value) == false) {
          sprintf((char*) context->output_buffer, "\r\nError obtaining current"
              " value for %s\r\n", parameter_name);
          COMM_TransmitData(context->output_buffer, CALC_LEN, 
              context->comm_interface);
          context->state->state = PARAM_STATE_COMPLETE;
        }
        else {
          sprintf((char*) context->output_buffer, "\r\n\r\nCurrent value of %s:"
              " %.4f\r\n", parameter_name, current_value);
          COMM_TransmitData(context->output_buffer, CALC_LEN, 
              context->comm_interface);
          sprintf((char*) context->output_buffer, "Please enter a new value "
              "from %.4f to %.4f:\r\n", min, max);
          COMM_TransmitData(context->output_buffer, CALC_LEN, 
              context->comm_interface);
          context->state->state = PARAM_STATE_1;
        }
        break;
      case PARAM_STATE_1:
        float new_value;
        if (checkFloat(context->input, &new_value, min, max) == true) {
          if (Param_SetFloat(param_id, &new_value) == true) {
            sprintf((char*) context->output_buffer, "\r\n%s successfully set to"
                " new value of %.4f\r\n", parameter_name, new_value);
            COMM_TransmitData(context->output_buffer, CALC_LEN, context->comm_interface);
          }
          else {
            COMM_TransmitData(error_updating_message, sizeof(error_updating_message) - 1, 
                context->comm_interface);
          }
          context->state->state = PARAM_STATE_COMPLETE;
        } else {
          sprintf((char*) context->output_buffer, "\r\nValue %.4f is outside "
              "the range of %.4f and %.4f\r\n", new_value, min, max);
          COMM_TransmitData(context->output_buffer, CALC_LEN, 
              context->comm_interface);
          context->state->state = PARAM_STATE_0;
        }
        break;
      default:
        context->state->state = PARAM_STATE_COMPLETE;
        break;
    }
  } while (old_state > context->state->state);
}

void COMMLoops_LoopEnum(FunctionContext_t* context, ParamIds_t param_id, 
    char** descriptors, uint16_t num_descriptors)
{
  ParamState_t old_state = context->state->state;

  char* parameter_name = Param_GetName(param_id);

  if (parameter_name == NULL) {
    COMM_TransmitData(uninitialized_parameter_message, sizeof(uninitialized_parameter_message) - 1, 
        context->comm_interface);
    context->state->state = PARAM_STATE_COMPLETE;
    return;
  }

  uint8_t min, max;
  if (Param_GetUint8Limits(param_id, &min, &max) == false) {
    COMM_TransmitData(error_limits_message, sizeof(error_limits_message) - 1, 
        context->comm_interface);
    context->state->state = PARAM_STATE_COMPLETE;
    return;
  }

  if (num_descriptors != (max + 1)) {
    COMM_TransmitData(internal_error_message, sizeof(internal_error_message) - 1, 
        context->comm_interface);
    context->state->state = PARAM_STATE_COMPLETE;
    return;
  }

  /* This do-while construct allows multiple state transitions in a single call
   * while preventing infinite loops by ensuring we never return to a higher state */
  do {
    switch (context->state->state) {
      case PARAM_STATE_0:
        uint8_t current_value;
        if (Param_GetUint8(param_id, &current_value) == false) {
          sprintf((char*) context->output_buffer, "\r\nError obtaining current"
              " value for %s\r\n", parameter_name);
          COMM_TransmitData(context->output_buffer, CALC_LEN, 
              context->comm_interface);
          context->state->state = PARAM_STATE_COMPLETE;
        }
        else {
          sprintf((char*) context->output_buffer, "\r\n\r\nCurrent value of %s"
              " is %u: %s\r\n", parameter_name, current_value, 
              descriptors[current_value]);
          COMM_TransmitData(context->output_buffer, CALC_LEN, 
              context->comm_interface);
          sprintf((char*) context->output_buffer, "Please enter a new value "
              "from %u to %u:\r\n", min, max);
          COMM_TransmitData(context->output_buffer, CALC_LEN, 
              context->comm_interface);
          for (uint8_t i = 0; i <= max; i++) {
            uint16_t buffer_index = 0;
            buffer_index += sprintf((char*) context->output_buffer, "%u: ", i);
            sprintf((char*) &context->output_buffer[buffer_index], "%s\r\n", 
                descriptors[i]);
            COMM_TransmitData(context->output_buffer, CALC_LEN, 
                context->comm_interface);
          }
          context->state->state = PARAM_STATE_1;
        }
        break;
      case PARAM_STATE_1:
        uint8_t new_value;
        if (checkUint8(context->input, context->input_len, &new_value, min, max) == true) {
          if (Param_SetUint8(param_id, &new_value) == true) {
            sprintf((char*) context->output_buffer, "\r\n%s successfully set "
                "to new value of %u: %s\r\n", parameter_name, new_value, 
                descriptors[new_value]);
            COMM_TransmitData(context->output_buffer, CALC_LEN, 
                context->comm_interface);
          }
          else {
            COMM_TransmitData(error_updating_message, sizeof(error_updating_message) - 1, 
                context->comm_interface);
          }
          context->state->state = PARAM_STATE_COMPLETE;
        } else {
          sprintf((char*) context->output_buffer, "\r\nValue %u is outside the "
              "range of %u and %u\r\n", new_value, min, max);
          COMM_TransmitData(context->output_buffer, CALC_LEN, 
              context->comm_interface);
          context->state->state = PARAM_STATE_0;
        }
        break;
      default:
        context->state->state = PARAM_STATE_COMPLETE;
        break;
    }
  } while (old_state > context->state->state);
}

void COMMLoops_LoopToggle(FunctionContext_t* context, ParamIds_t param_id)
{
  ParamState_t old_state = context->state->state;

  static bool current_state;

  char* parameter_name = Param_GetName(param_id);

  if (parameter_name == NULL) {
    COMM_TransmitData(uninitialized_parameter_message, sizeof(uninitialized_parameter_message) - 1, 
        context->comm_interface);
    context->state->state = PARAM_STATE_COMPLETE;
    return;
  }

  /* This do-while construct allows multiple state transitions in a single call
   * while preventing infinite loops by ensuring we never return to a higher state */
  do {
    switch (context->state->state) {
      case PARAM_STATE_0:
        if (Param_GetUint8(param_id, (uint8_t*) &current_state) == false) {
          sprintf((char*) context->output_buffer, "\r\nError obtaining current "
              "state\r\n");
          COMM_TransmitData(context->output_buffer, CALC_LEN, 
              context->comm_interface);
          context->state->state = PARAM_STATE_COMPLETE;
        }
        else {
          sprintf((char*) context->output_buffer, "\r\n%s is currently %s. "
              "Would you like to %s it? (y/n):\r\n", parameter_name, 
              current_state ? "enabled" : "disabled", 
              current_state ? "disable" : "enable");
          COMM_TransmitData(context->output_buffer, CALC_LEN, 
            context->comm_interface);
          context->state->state = PARAM_STATE_1;
        }
        break;
      case PARAM_STATE_1:
        bool confirmed = false;
        if (checkYesNo(*context->input, &confirmed) == true) {
          bool new_value = ! current_state;
          if (confirmed == true) {
            if (Param_SetUint8(param_id, (uint8_t*) &new_value) == true) {
              sprintf((char*) context->output_buffer, "\r\nSuccessfully %s %s\r\n",
                 new_value ? "enabled" : "disabled", parameter_name);
              COMM_TransmitData(context->output_buffer, CALC_LEN, 
                  context->comm_interface);
            }
            else {
              COMM_TransmitData(error_updating_message, sizeof(error_updating_message) - 1, 
                  context->comm_interface);
            }
            context->state->state = PARAM_STATE_COMPLETE;
          }
          else {
            context->state->state = PARAM_STATE_COMPLETE;
          }
        }
        else {
          sprintf((char*) context->output_buffer, "\r\nInvalid Input!\r\n");
          COMM_TransmitData(context->output_buffer, CALC_LEN, 
              context->comm_interface);
          context->state->state = PARAM_STATE_0;
        }
        break;
      default:
      context->state->state = PARAM_STATE_COMPLETE;
      break;
    }
  } while (old_state > context->state->state);
}

void COMMLoops_NotImplemented(FunctionContext_t* context)
{
  COMM_TransmitData("\r\nThis function/parameter has not been implemented yet!\r\n",
                    CALC_LEN, context->comm_interface);

  context->state->state = PARAM_STATE_COMPLETE;
}

/* Private function definitions ----------------------------------------------*/

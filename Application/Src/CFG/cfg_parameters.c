/*
 * cfg_parameters.c
 *
 *  Created on: Mar 11, 2025
 *      Author: ericv
 */

/* Private includes ----------------------------------------------------------*/

#include "cfg_parameters.h"
#include "cfg_main.h"

#include "main.h"
#include "cmsis_os.h"

#include <stdbool.h>
#include <string.h>

/* Private typedef -----------------------------------------------------------*/

#define TASK_NAME_LEN 8

typedef struct {
  TaskIds_t id;
  char name[TASK_NAME_LEN];
  bool is_registered;
} TaskRegistration_t;

/* Private define ------------------------------------------------------------*/

#define MAX_PARAMETERS      128

#define FLASH_PARAM_SECTOR  FLASH_SECTOR_4
#define FLASH_PARAM_ADDR    (FLASH_BASE + 0x00100000) // TODO change later
#define PARAM_SIGNATURE     0x50415241  // PARA in hex
#define PARAM_VERSION       1

/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/

static TaskRegistration_t registered_tasks[NUM_TASKS];
static uint8_t registered_tasks_count = 0;
static uint8_t complete_registrations_count = 0;

static Parameter_t parameters[NUM_PARAM] = {0};

static osMutexId_t param_mutex = NULL;

// static uint32_t param_crc = 0; TODO

/* Private function prototypes -----------------------------------------------*/

static Parameter_t* findParamById(ParamIds_t id);
static bool isParamInitialized(ParamIds_t id);

/* Exported function definitions ---------------------------------------------*/

bool Param_Init(void)
{
  static const osMutexAttr_t mutex_attr = {
      .name = "ParamMutex",
      .attr_bits = osMutexRecursive,
      .cb_mem = NULL,
      .cb_size = 0
  };

  param_mutex = osMutexNew(&mutex_attr);
  if (param_mutex == NULL) {
    return false;
  }
  if (NUM_PARAM > MAX_PARAMETERS) {
    return false;
  }

  for (uint8_t i = 0; i < NUM_TASKS; i++) {
    registered_tasks[i].id = NUM_TASKS; // indicates that the spot has not been filed yet
  }

  return true;
}

bool Param_LoadInit(void)
{
  // Load parameters from flash to overwrite defaults set by register
  return true;
}

// Note: min and max MUST be 32 bytes for uint16, int16, int8 and uint8
bool Param_Register(ParamIds_t id, const char* name, ParamType_t type,
                    void* value_ptr, size_t value_size, void* min, void* max)
{

  if (value_ptr == NULL) {
    return false;
  }

  if (osMutexAcquire(param_mutex, osWaitForever) == osOK) {
    Parameter_t* param = &parameters[id];
    // check if already initialized
    if (param->value_ptr != NULL) {
      osMutexRelease(param_mutex);
      return false;
    }
    param->id = id;
    strncpy(param->name, name, sizeof(param->name) - 1);
    param->type = type;
    param->value_ptr = value_ptr;
    param->value_size = value_size;
    param->is_modified = false;

    switch (type) {
      case PARAM_TYPE_UINT8:
      case PARAM_TYPE_UINT16:
      case PARAM_TYPE_UINT32:
        param->limits.u32.min = *(uint32_t*) min;
        param->limits.u32.max = *(uint32_t*) max;
        break;
      case PARAM_TYPE_INT8:
      case PARAM_TYPE_INT16:
      case PARAM_TYPE_INT32:
        param->limits.i32.min = *(int32_t*) min;
        param->limits.i32.max = *(int32_t*) max;
        break;
      case PARAM_TYPE_FLOAT:
        param->limits.f.min = *(float*) min;
        param->limits.f.max = *(float*) max;
        break;
      default:
        return false;
    }

    osMutexRelease(param_mutex);
    return true;
  }
  else {
    return false;
  }
}

bool Param_GetValue(ParamIds_t id, void* value)
{
  bool success = false;

  if (osMutexAcquire(param_mutex, osWaitForever) == osOK) {
    Parameter_t* param = findParamById(id);
    if (isParamInitialized(id) == true) {
      memcpy(value, param->value_ptr, param->value_size);
      success = true;
    }
    osMutexRelease(param_mutex);
  }
  return success;
}

bool Param_GetUint8(ParamIds_t id, uint8_t* value)
{
  return Param_GetValue(id, value);
}

bool Param_GetInt8(ParamIds_t id, int8_t* value)
{
  return Param_GetValue(id, value);
}
bool Param_GetUint16(ParamIds_t id, uint16_t* value)
{
  return Param_GetValue(id, value);
}

bool Param_GetInt16(ParamIds_t id, int16_t* value)
{
  return Param_GetValue(id, value);
}

bool Param_GetUint32(ParamIds_t id, uint32_t* value)
{
  return Param_GetValue(id, value);
}

bool Param_GetInt32(ParamIds_t id, int32_t* value)
{
  return Param_GetValue(id, value);
}

bool Param_GetFloat(ParamIds_t id, float* value)
{
  return Param_GetValue(id, value);
}

char* Param_GetName (ParamIds_t id)
{
  char* param_name = NULL;
  if (osMutexAcquire(param_mutex, osWaitForever) == osOK) {
    Parameter_t* param = findParamById(id);
    if (isParamInitialized(id) == true) {
      param_name = param->name;
    }
    osMutexRelease(param_mutex);
  }
  return param_name;
}

bool Param_GetLimits(ParamIds_t id, void* min, void* max)
{
  bool success = false;

  if (osMutexAcquire(param_mutex, osWaitForever) == osOK) {
    Parameter_t* param = findParamById(id);
    if (isParamInitialized(id) == true) {
      // Always copies fixed-size data regardless of actual parameter type
      memcpy(min, &param->limits.u32.min, sizeof(uint32_t));
      memcpy(max, &param->limits.u32.max, sizeof(uint32_t));
      success = true;
    }
    osMutexRelease(param_mutex);
  }
  return success;
}

bool Param_GetUint8Limits (ParamIds_t id, uint8_t* min, uint8_t* max)
{
  uint32_t minimum, maximum;
  if (Param_GetLimits(id, &minimum, &maximum) == false) {
    return false;
  }
  *min = (uint8_t) minimum;
  *max = (uint8_t) maximum;
  return true;
}
bool Param_GetInt8Limits  (ParamIds_t id, int8_t* min, int8_t* max)
{
  int32_t minimum, maximum;
  if (Param_GetLimits(id, &minimum, &maximum) == false) {
    return false;
  }
  *min = (int8_t) minimum;
  *max = (int8_t) maximum;
  return true;
}
bool Param_GetUint16Limits(ParamIds_t id, uint16_t* min, uint16_t* max)
{
  uint32_t minimum, maximum;
  if (Param_GetLimits(id, &minimum, &maximum) == false) {
    return false;
  }
  *min = (uint16_t) minimum;
  *max = (uint16_t) maximum;
  return true;
}
bool Param_GetInt16Limits (ParamIds_t id, int16_t* min, int16_t* max)
{
  int32_t minimum, maximum;
  if (Param_GetLimits(id, &minimum, &maximum) == false) {
    return false;
  }
  *min = (int16_t) minimum;
  *max = (int16_t) maximum;
  return true;
}
bool Param_GetUint32Limits(ParamIds_t id, uint32_t* min, uint32_t* max)
{
  return Param_GetLimits(id, min, max);
}
bool Param_GetInt32Limits (ParamIds_t id, int32_t* min, int32_t* max)
{
  return Param_GetLimits(id, min, max);
}
bool Param_GetFloatLimits (ParamIds_t id, float* min, float* max)
{
  return Param_GetLimits(id, min, max);
}

bool Param_SetValue(ParamIds_t id, const void* value)
{
  bool success = false;

  if (osMutexAcquire(param_mutex, osWaitForever) == osOK) {
    Parameter_t* param = findParamById(id);
    if (isParamInitialized(id) == true) {
      bool valid = false;
      switch (param->type) {
        case PARAM_TYPE_UINT8:
        case PARAM_TYPE_UINT16:
        case PARAM_TYPE_UINT32: {
          uint32_t val = 0;
          if (param->type == PARAM_TYPE_UINT8) {
            val = (uint32_t) (*(uint8_t*) value);
          }
          else if (param->type == PARAM_TYPE_UINT16) {
            val = (uint32_t) (*(uint16_t*) value);
          }
          else {
            val = *(uint32_t*) value;
          }
          valid = (val >= param->limits.u32.min &&
                   val <= param->limits.u32.max);
          break;
        }
        case PARAM_TYPE_INT8:
        case PARAM_TYPE_INT16:
        case PARAM_TYPE_INT32: {
          int32_t val = 0;
          if (param->type == PARAM_TYPE_INT8) {
            val = (int32_t) (*(uint8_t*) value);
          }
          else if (param->type == PARAM_TYPE_INT16) {
            val = (int32_t) (*(uint16_t*) value);
          }
          else {
            val = *(int32_t*) value;
          }
          valid = (val >= param->limits.i32.min &&
                   val <= param->limits.i32.max);
          break;
        }
        case PARAM_TYPE_FLOAT: {
          float val = *(float*) value;
          valid = (val >= param->limits.f.min &&
                   val <= param->limits.f.max);
          break;
        }
        default:
          break;
      }

      if (valid) {
        if (memcmp(param->value_ptr, value, param->value_size) != 0) {
          memcpy(param->value_ptr, value, param->value_size);
          param->is_modified = true;
          // TODO: trigger save to flash
        }
        success = true;
      }
    }
    osMutexRelease(param_mutex);
  }
  return success;
}

bool Param_SetUint8(ParamIds_t id, uint8_t* value)
{
  return Param_SetValue(id, value);
}

bool Param_SetInt8(ParamIds_t id, int8_t* value)
{
  return Param_SetValue(id, value);
}

bool Param_SetUint16(ParamIds_t id, uint16_t* value)
{
  return Param_SetValue(id, value);
}

bool Param_SetInt16(ParamIds_t id, int16_t* value)
{
  return Param_SetValue(id, value);
}

bool Param_SetUint32(ParamIds_t id, uint32_t* value)
{
  return Param_SetValue(id, value);
}

bool Param_SetInt32(ParamIds_t id, int32_t* value)
{
  return Param_SetValue(id, value);
}

bool Param_SetFloat(ParamIds_t id, float* value)
{
  return Param_SetValue(id, value);
}

bool Param_RegisterTask(TaskIds_t task_id, const char* task_name)
{
  if (registered_tasks_count >= NUM_TASKS) {
    return false;
  }

  if (osMutexAcquire(param_mutex, osWaitForever) == osOK) {
    for (uint8_t i = 0; i < registered_tasks_count; i++) {
      if (registered_tasks[i].id == task_id) {
        osMutexRelease(param_mutex);
        return false;
      }
    }
    registered_tasks[registered_tasks_count].id = task_id;

    strncpy(registered_tasks[registered_tasks_count].name, task_name,
            TASK_NAME_LEN - 1);
    registered_tasks[registered_tasks_count].name[TASK_NAME_LEN - 1] = '\0';

    registered_tasks[registered_tasks_count].is_registered = false;

    registered_tasks_count++;
    osMutexRelease(param_mutex);
    return true;
  }
  return false;
}

bool Param_TaskRegistrationComplete(TaskIds_t task_id)
{
  if (osMutexAcquire(param_mutex, osWaitForever) == osOK) {
    for (uint8_t i = 0; i < registered_tasks_count; i++) {
      if (registered_tasks[i].id == task_id) {
        if (registered_tasks[i].is_registered == false) {
          registered_tasks[i].is_registered = true;
          complete_registrations_count++;

          if (complete_registrations_count == NUM_TASKS) {
            osEventFlagsSet(param_events, EVENT_ALL_TASKS_REGISTERED);
          }

          osMutexRelease(param_mutex);
          return true;
        }
        break;
      }
    }
    osMutexRelease(param_mutex);
  }
  return false;
}


/* Private function definitions ----------------------------------------------*/

Parameter_t* findParamById(ParamIds_t id)
{
  return &parameters[id];
}

bool isParamInitialized(ParamIds_t id)
{
  Parameter_t* param = findParamById(id);
  if (param == NULL) {
    return false;
  }

  if (param->value_ptr == NULL) {
    return false;
  }

  return true;
}

/*
 * cfg_parameters.c
 *
 *  Created on: Mar 11, 2025
 *      Author: ericv
 */

/* Private includes ----------------------------------------------------------*/

#include "cfg_parameters.h"
#include "cfg_main.h"

#include "stm32h7xx.h"
#include "stm32h7xx_hal.h"

#include "main.h"
#include "cmsis_os.h"

#include <stdbool.h>
#include <string.h>

/* Private typedef -----------------------------------------------------------*/

typedef struct {
  ParamIds_t id;
  char name[32];
  ParamType_t type;
  void* value_ptr;
  size_t value_size;
  union {
    struct {uint32_t min; uint32_t max;} u32;
    struct {int32_t min; int32_t max;} i32;
    struct {float min; float max;} f;
  } limits;
  void (*callback)(void);
  bool is_modified;
} Parameter_t;

#define TASK_NAME_LEN 8

typedef struct {
  TaskIds_t id;
  char name[TASK_NAME_LEN];
  bool is_registered;
} TaskRegistration_t;

typedef struct {
  uint32_t signature;
  uint16_t version;
  uint16_t param_id;
  uint32_t value;
  uint8_t padding[20];
} __attribute__((packed)) ConfigEntry_t;

/* Private define ------------------------------------------------------------*/

#define MAX_PARAMETERS        128

#define FLASH_PARAM_SECTOR    FLASH_SECTOR_3
#define FLASH_PARAM_ADDR      (FLASH_BASE + FLASH_SECTOR_SIZE * FLASH_PARAM_SECTOR)
#define PARAM_SIGNATURE       0x50415241  // PARA in hex
#define PARAM_VERSION         1
#define FLASH_PARAM_ADDR_END  (FLASH_PARAM_ADDR + FLASH_SECTOR_SIZE)
#define FLASH_WORD_SIZE       (FLASH_NB_32BITWORD_IN_FLASHWORD * sizeof(uint32_t))

/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/

static TaskRegistration_t registered_tasks[NUM_TASKS];
static uint8_t registered_tasks_count = 0;
static uint8_t complete_registrations_count = 0;

static Parameter_t parameters[NUM_PARAM] = {0};

static osMutexId_t param_mutex = NULL;

static uint32_t num_erases;
static uint32_t next_write_addr = FLASH_PARAM_ADDR;

static bool flash_load_complete = false;

// static uint32_t param_crc = 0; TODO

/* Private function prototypes -----------------------------------------------*/

static Parameter_t* findParamById(ParamIds_t id);
static bool isParamInitialized(ParamIds_t id);

static bool getNumErases();
static bool updateNumErases();
static bool writeParameterToFlash(uint16_t id);

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

  if (sizeof(ConfigEntry_t) != FLASH_WORD_SIZE) {
    return false;
  }

  if (getNumErases() == false) {
    return false;
  }
  next_write_addr += FLASH_WORD_SIZE;

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
  // Load parameters from flash to overwrite defaults set by registration
  for (; next_write_addr < FLASH_PARAM_ADDR_END; next_write_addr += FLASH_WORD_SIZE) {
    ConfigEntry_t* entry = (ConfigEntry_t*) next_write_addr;
    if (entry->signature != PARAM_SIGNATURE) { // add other signatures as needed
      break;
    }
    if (entry->signature == PARAM_SIGNATURE) {
      if (entry->version != 1) {
        return false;
      }
      if (Param_SetValue(entry->param_id, (void*) &entry->value) == false) {
        return false;
      }
    }
  }

  for (uint16_t i = 0; i < NUM_PARAM; i++) {
    parameters[i].is_modified = false;
  }

  flash_load_complete = true;

  return true;
}

// Note: min and max MUST be 32 bits for uint16, int16, int8 and uint8
bool Param_Register(ParamIds_t id, const char* name, ParamType_t type,
                    void* value_ptr, size_t value_size, void* min, void* max,
                    void (*callback)(void))
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
    param->callback = callback;
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

char* Param_GetName(ParamIds_t id)
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
          if ((param->callback != NULL) && (flash_load_complete == true)) {
            (*param->callback)();
          }
          CFG_SetFlashSaveFlag();
          CFG_IncrementVersionNumber();
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

bool Param_GetParamType(ParamIds_t id, ParamType_t* param_type)
{
  if (osMutexAcquire(param_mutex, osWaitForever) == osOK) {
    Parameter_t* param = findParamById(id);
    if (isParamInitialized(id) == true) {
      *param_type = param->type;
      return true;
    }
  }
  return false;
}

bool Param_SaveToFlash(void)
{
  for (uint16_t i = 0; i < NUM_PARAM; i++) {
    if (parameters[i].is_modified == true) {
      // save the parameter
      if (writeParameterToFlash(i) == false) {
        return false;
      }
      if (next_write_addr >= FLASH_PARAM_ADDR_END) {
        // clear flash, update num erases, add parameters back to flash
        if (Param_FlashReset() == false) {
          return false;
        }
        if (updateNumErases() == false) {
          return false;
        }

        for (uint16_t i = 0; i < NUM_PARAM; i++) {
          if (writeParameterToFlash(i) == false) {
            return false;
          }
        }
      }
    }
  }
  return true;
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

bool Param_FlashReset()
{
  FLASH_EraseInitTypeDef erase = {0};
  erase.TypeErase = FLASH_TYPEERASE_SECTORS;
  erase.Banks = FLASH_BANK_1;
  erase.Sector = FLASH_PARAM_SECTOR;
  erase.NbSectors = 1;
  erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;

  uint32_t error = 0;
  if (HAL_FLASH_Unlock() != HAL_OK) {
    return false;
  }
  if (HAL_FLASHEx_Erase(&erase, &error) != HAL_OK) {
    return false;
  }
  if (HAL_FLASH_Lock() != HAL_OK) {
    return false;
  }
  next_write_addr = FLASH_PARAM_ADDR;
  num_erases++;
  if (updateNumErases() == false) {
    return false;
  }
  next_write_addr += FLASH_WORD_SIZE;
  return true;
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

static bool getNumErases()
{
  uint32_t* num_erases_addr = (uint32_t*) FLASH_PARAM_ADDR;
  num_erases = *num_erases_addr;
  if (num_erases == 0xFFFFFFFF) {
    num_erases = 0;
    if (updateNumErases() == false) {
      return false;
    }
  }
  return true;
}

static bool updateNumErases()
{
  if (HAL_FLASH_Unlock() != HAL_OK) {
    return false;
  }
  uint8_t flash_buffer[FLASH_WORD_SIZE] = {0};
  *((uint32_t*) flash_buffer) = num_erases;
  HAL_StatusTypeDef status = HAL_FLASH_Program(
      FLASH_TYPEPROGRAM_FLASHWORD, FLASH_PARAM_ADDR, (uint32_t) flash_buffer);
  if (status != HAL_OK) {
    return false;
  }
  if (HAL_FLASH_Lock() != HAL_OK) {
    return false;
  }
  return true;
}

static bool writeParameterToFlash(uint16_t id)
{
  if (id > NUM_PARAM) {
    return false;
  }

  ConfigEntry_t entry;
  entry.signature = PARAM_SIGNATURE;
  entry.version = PARAM_VERSION;
  entry.param_id = id;
  entry.value = *((uint32_t*) parameters[id].value_ptr);

  if (HAL_FLASH_Unlock() != HAL_OK) {
    return false;
  }
  HAL_StatusTypeDef status = HAL_FLASH_Program(
      FLASH_TYPEPROGRAM_FLASHWORD, next_write_addr, (uint32_t) &entry);
  if (status != HAL_OK) {
    return false;
  }
  if (HAL_FLASH_Lock() != HAL_OK) {
    return false;
  }
  parameters[id].is_modified = false;
  next_write_addr += FLASH_WORD_SIZE;

  return true;
}

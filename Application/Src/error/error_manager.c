/*
 * error_manager.c
 *
 *  Created on: Feb 20, 2026
 *      Author: ericv
 *
 * Copyright (c) 2026 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

/* Private includes ----------------------------------------------------------*/

#include "error_manager.h"
#include "error_log.h"
#include "error_reset.h"
#include "cfg_main.h"
#include "cfg_parameters.h"
#include "comm_main.h"
#include "ws2812b-driver.h"
#include "cmsis_os.h"
#include <stdio.h>
#include <stdbool.h>

/* Private typedef -----------------------------------------------------------*/

typedef struct {
  osThreadId task_id; // NULL if no task registered
  const char* name;
  bool parameters_registered;
  bool pending_reset;
  bool blocked_by_error;
} TaskInfo_t;

/* Private define ------------------------------------------------------------*/



/* Private macro -------------------------------------------------------------*/

#define XERROR_LUT(error, severity, description, subsystem) {severity, description, subsystem},

/* Private variables ---------------------------------------------------------*/

static const ErrorLutEntry_t error_lut[] = { ERROR_MANAGEMENT_TABLE(XERROR_LUT) };
DEFINE_DESC_TABLE(ERROR_SEVERITY_TABLE, severity_descriptions);
static TaskInfo_t registered_tasks[NUM_TASKS] = {0};

static uint8_t tasks_fully_registered = 0;

/* Private function prototypes -----------------------------------------------*/

static void taskDeathLoop(void);
static void systemDeathLoop(void);
static TaskInfo_t* getTaskInfo(void);
static void performSystemReset(OpenAquatixErrors_t reason);

/* Exported function definitions ---------------------------------------------*/

void Error_RegisterTask(const char* task_name)
{
  if (task_name == NULL) taskDeathLoop();

  osThreadId thread_id = osThreadGetId();
  if (thread_id == NULL) taskDeathLoop();

  for (uint8_t i = 0; i < NUM_TASKS; i++) {
    if (registered_tasks[i].task_id != NULL) continue;

    registered_tasks[i].task_id = thread_id;
    registered_tasks[i].name = task_name;
    registered_tasks[i].parameters_registered = false;
    registered_tasks[i].blocked_by_error = false;
    registered_tasks[i].pending_reset = false;
    return;
  }
  // Should never get here unless incorrect number of tasks
  taskDeathLoop();
}

void Error_ParameterRegistrationComplete(void)
{
  osThreadId thread_id = osThreadGetId();
  if (thread_id == NULL) taskDeathLoop();

  for (uint8_t i = 0; i < NUM_TASKS; i++) {
    if (registered_tasks[i].task_id != thread_id) continue;

    registered_tasks[i].parameters_registered = true;
    tasks_fully_registered++;
    if (tasks_fully_registered == NUM_TASKS) {
      if (param_events == NULL) taskDeathLoop();
      osEventFlagsSet(param_events, EVENT_ALL_TASKS_REGISTERED);
    }
    return;
  }
  // Should never get here unless initial registration not done
  taskDeathLoop();
}

void Error_RegisterError(OpenAquatixErrors_t error_code, const char* file, uint16_t line)
{
  TaskInfo_t* task_info = getTaskInfo();

  ErrorSeverity_t severity = error_lut[error_code].severity;

  ErrorLog_LogError(file, task_info->name, line, error_code);

  switch (severity) {
    case ERROR_SEVERITY_WARN:
      break;
    case ERROR_SEVERITY_ABORT:
      task_info->blocked_by_error = true;
      break;
    case ERROR_SEVERITY_TASK_RESET:
      task_info->pending_reset = true;
      break;
    case ERROR_SEVERITY_RESET_SUBSYS:
      // TODO: reset subsystem
      break;
    case ERROR_SEVERITY_DISABLE_SUBSYS:
      // TODO: disable subsytem
      break;
    case ERROR_SEVERITY_FULL_RESET:
      performSystemReset(error_code);
      break;
    case ERROR_SEVERITY_UNRECOVERABLE:
      systemDeathLoop();
      break;
    default:
      break;
  }
  // TODO: log error on error stream
}

const char* Error_GetDescription(OpenAquatixErrors_t error_code)
{
  if (error_code >= NUM_UNIQUE_ERRORS) return "N/A";

  return error_lut[error_code].description;
}

const char* Error_GetSeverity(OpenAquatixErrors_t error_code)
{
  if (error_code >= NUM_UNIQUE_ERRORS) return "N/A";

  ErrorSeverity_t severity = error_lut[error_code].severity;

  return severity_descriptions[severity];
}

ErrorCheck_t Error_CheckStatus(void)
{
  TaskInfo_t* task_info = getTaskInfo();

  if ((task_info->blocked_by_error == true) || 
      (task_info->pending_reset == true)) {
    return ERROR_QUIT;
  }
  else {
    return ERROR_PROCEED;
  }
}

void Error_ResetAbortFlag(void)
{
  TaskInfo_t* task_info = getTaskInfo();

  task_info->blocked_by_error = false;
}

TaskResetStatus_t Error_CheckModuleReset(void)
{
  TaskInfo_t* task_info = getTaskInfo();

  TaskResetStatus_t ret = (task_info->pending_reset == true) ? (TASK_RESET) : (TASK_PROCEED);
  task_info->pending_reset = false;
  return ret;
}

/* Private function definitions ----------------------------------------------*/

// In case of termination during initialization, the task is sent to a loop
// and updates the LED to be red. Since the error occurred during early
// initialization, it is likely a programming error so resetting will do nothing
void taskDeathLoop(void)
{
  uint8_t brightness;
  Param_GetUint8(PARAM_LED_BRIGHTNESS, &brightness);
  for (;;) {
    osDelay(1);
    Ws2812b_SetColour(255, 0, 0);
    Ws2812b_Update(brightness);
  }
}

// Disable all other running tasks and enter death loop
void systemDeathLoop(void)
{
  TaskInfo_t* task_info = getTaskInfo();
  for (uint16_t i = 0; i < NUM_TASKS; i++) {
    if (&registered_tasks[i] == task_info) continue; // Dont disable yourself

    osThreadTerminate(registered_tasks[i].task_id);
  }
  // TODO: Disable all interrupts besides the WS ones
  taskDeathLoop();
}

TaskInfo_t* getTaskInfo(void)
{
  osThreadId thread_id = osThreadGetId();
  if (thread_id == NULL) taskDeathLoop();

  for (uint8_t i = 0; i < NUM_TASKS; i++) {
    if (registered_tasks[i].task_id == thread_id)
      return &registered_tasks[i];
  }
  taskDeathLoop();
  // Unreachable code
  return NULL;
}

// Always print warning to HMI by whichever task calls error since HMI
// conflict management not worth figuring out if the COMM task is ready to
// receive notifications or the COMM task caused the error
void performSystemReset(OpenAquatixErrors_t reason)
{
  char buf[100];
  snprintf(buf, 100, "Encountered a fatal error (%u), resetting now\r\n", reason);
  COMM_TransmitData(buf, CALC_LEN, COMM_BOTH);
  ErrorLog_PrintLog(COMM_BOTH);
  osDelay(50);
  ErrorReset_WarmReset();
}

/*
 * error_log.c
 *
 *  Created on: Feb 20, 2026
 *      Author: ericv
 *
 * Copyright (c) 2026 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

/* Private includes ----------------------------------------------------------*/

#include "error_log.h"
#include "error_manager.h"
#include "bkpsram_layout.h"
#include "comm_main.h"
#include "main.h"
#include "format_timestamp.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <limits.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/

#define ERROR_CODE_WIDTH                6
#define FILE_NAME_WIDTH                 28
#define TASK_NAME_WIDTH                 5
#define LINE_NUMBER_WIDTH               5
#define SEVERITY_WIDTH                  15
#define ERROR_DESCRIPTION_WIDTH         40
#define OCCURRENCES_WIDTH               12

#define ERROR_LOG_LINE_SIZE             (FORMATTED_TIMESTAMP_SIZE + \
                                         ERROR_CODE_WIDTH + \
                                         FILE_NAME_WIDTH + \
                                         TASK_NAME_WIDTH + \
                                         LINE_NUMBER_WIDTH + \
                                         SEVERITY_WIDTH + \
                                         ERROR_DESCRIPTION_WIDTH + \
                                         OCCURRENCES_WIDTH + \
                                         15) // Padding characters

/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/

static volatile ErrorEntry_t* error_log = bkpsram.error_log;

/* Private function prototypes -----------------------------------------------*/

static uint16_t existingErrorIndex(OpenAquatixErrors_t error_code);
static uint16_t lowestTimestampIndex(void);
static void sortLogByTimestamp(const ErrorEntry_t* src, uint16_t count, ErrorEntry_t* out);
static int compareTimestampAdc(const void* a, const void* b);

/* Exported function definitions ---------------------------------------------*/

void ErrorLog_LogError(const char* file_name, const char* task_name, 
                       uint16_t line, OpenAquatixErrors_t error_code)
{
  if (file_name == NULL || task_name == NULL) return;

  uint64_t timestamp = HAL_AbsoluteTimestamp();

  uint16_t new_index;
  new_index = existingErrorIndex(error_code);
  if (new_index == MAX_ENTRIES_IN_ERROR_LOG) {
    // Overwrite existing entry or brand new entry
    new_index = lowestTimestampIndex();
    error_log[new_index].occurrences = 0;
  }
  error_log[new_index].task_name = task_name;
  error_log[new_index].file_name = file_name;
  error_log[new_index].line_number = line;
  error_log[new_index].timestamp = timestamp;
  error_log[new_index].occurrences++;
  error_log[new_index].error_code = error_code;
  error_log[new_index].reset_count = bkpsram.reset_count;
}

void ErrorLog_PrintLog(CommInterface_t interface)
{
  ErrorEntry_t sorted_log[MAX_ENTRIES_IN_ERROR_LOG];
  uint32_t state = osKernelLock();
  sortLogByTimestamp((ErrorEntry_t*) error_log, MAX_ENTRIES_IN_ERROR_LOG, sorted_log);
  osKernelRestoreLock(state);

  uint16_t current_reset_count = bkpsram.reset_count;
  uint64_t current_timestamp = HAL_AbsoluteTimestamp();

  char formatted_timestamp[FORMATTED_TIMESTAMP_SIZE];
  formatAbsoluteTimestamp(current_reset_count, current_timestamp, 
                          formatted_timestamp);

  char out_buf[ERROR_LOG_LINE_SIZE];
  snprintf(out_buf, ERROR_LOG_LINE_SIZE, "\r\nError log as of %s\r\n\r\n", 
           formatted_timestamp);
  COMM_TransmitData(out_buf, CALC_LEN, interface);

  snprintf(out_buf, ERROR_LOG_LINE_SIZE, "%-*s %-*s %-*s %-*s %-*s %-*s %-*s %-*s\r\n",
           FORMATTED_TIMESTAMP_SIZE, "Timestamp",
           ERROR_CODE_WIDTH, "E#",
           FILE_NAME_WIDTH, "File name",
           TASK_NAME_WIDTH, "Task",
           LINE_NUMBER_WIDTH, "Line",
           SEVERITY_WIDTH, "Severity",
           ERROR_DESCRIPTION_WIDTH, "Description",
           OCCURRENCES_WIDTH, "Occurrences");
  COMM_TransmitData(out_buf, CALC_LEN, interface);
  for (uint16_t i = 0; i < MAX_ENTRIES_IN_ERROR_LOG; i++) {
    if (sorted_log[i].timestamp == 0) continue; // Empty entry
    OpenAquatixErrors_t error_code = sorted_log[i].error_code;
    formatAbsoluteTimestamp(sorted_log[i].reset_count, sorted_log[i].timestamp, 
                            formatted_timestamp);
    snprintf(out_buf, ERROR_LOG_LINE_SIZE, "%-*s %-*u %-*s %-*s %-*lu %-*s %-*s %-*lu\r\n",
             FORMATTED_TIMESTAMP_SIZE, formatted_timestamp,
             ERROR_CODE_WIDTH, error_code,
             FILE_NAME_WIDTH, sorted_log[i].file_name,
             TASK_NAME_WIDTH, sorted_log[i].task_name,
             LINE_NUMBER_WIDTH, sorted_log[i].line_number,
             SEVERITY_WIDTH, Error_GetSeverity(error_code),
             ERROR_DESCRIPTION_WIDTH, Error_GetDescription(error_code),
             OCCURRENCES_WIDTH, sorted_log[i].occurrences);
    COMM_TransmitData(out_buf, CALC_LEN, interface);
  }
}

/* Private function definitions ----------------------------------------------*/

uint16_t existingErrorIndex(OpenAquatixErrors_t error_code)
{
  for (uint16_t i = 0; i < MAX_ENTRIES_IN_ERROR_LOG; i++) {
    if ((error_log[i].timestamp != 0) && 
        (error_log[i].error_code == error_code)) {
      return i;
    }
  }
  return MAX_ENTRIES_IN_ERROR_LOG;
}

uint16_t lowestTimestampIndex(void)
{
  uint16_t min_index;
  uint64_t min_timestamp = ULLONG_MAX;
  uint16_t min_reset_count = USHRT_MAX;
  for (uint16_t i = 0; i < MAX_ENTRIES_IN_ERROR_LOG; i++) {
    if (error_log[i].timestamp < min_timestamp && error_log[i].reset_count <= min_reset_count) {
      min_index = i;
      min_timestamp = error_log[i].timestamp;
      min_reset_count = error_log[i].reset_count;
    }
  }
  return min_index;
}

void sortLogByTimestamp(const ErrorEntry_t* src, uint16_t count, ErrorEntry_t* out)
{
  if (src == NULL || out == NULL || count == 0) return;

  memcpy(out, src, count * sizeof(ErrorEntry_t));

  qsort(out, count, sizeof(ErrorEntry_t), compareTimestampAdc);
}

int compareTimestampAdc(const void* a, const void* b)
{
  const ErrorEntry_t* entry_a = (const ErrorEntry_t*)a;
  const ErrorEntry_t* entry_b = (const ErrorEntry_t*)b;

  if (entry_a->reset_count < entry_b->reset_count) return -1;
  if (entry_a->reset_count > entry_b->reset_count) return  1;

  if (entry_a->timestamp < entry_b->timestamp) return -1;
  if (entry_a->timestamp > entry_b->timestamp) return  1;
  return 0;
}

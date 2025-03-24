/*
 * check_inputs.c
 *
 *  Created on: Feb 17, 2025
 *      Author: ericv
 */

/* Private includes ----------------------------------------------------------*/

#include "check_inputs.h"
#include "stm32h7xx_hal.h"
#include <stdbool.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/



/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/



/* Private function prototypes -----------------------------------------------*/



/* Exported function definitions ---------------------------------------------*/

bool checkUint8(char* num_str, uint16_t num_str_len, uint8_t* ret, uint8_t min, uint8_t max)
{
  if (num_str_len > 3) return false;
  if (num_str_len == 0) {
    *ret = 0;
    if (min == 0) {
      return true;
    }
    else {
      return false;
    }
  }
  for (uint8_t i = 0; i < num_str_len; i++) {
    if (! isdigit((uint8_t) num_str[i])) return false;
  }

  uint16_t ret_buf = (uint16_t) atoi((char*) num_str); // Not uint8 in case of overflow

  *ret = (uint8_t) ret_buf;

  if ((ret_buf >= min) && (ret_buf <= max)) {
    return true;
  }
  else {
    return false;
  }
}

bool checkUint16(char* num_str, uint16_t num_str_len, uint16_t* ret, uint16_t min, uint16_t max)
{
  if (num_str_len > 5) return false;
  if (num_str_len == 0) {
    *ret = 0;
    if (min == 0) {
      return true;
    }
    else {
      return false;
    }
  }
  for (uint8_t i = 0; i < num_str_len; i++) {
    if (! isdigit((uint8_t) num_str[i])) return false;
  }

  uint32_t ret_buf = (uint32_t) strtoul((char*) num_str, NULL, 0);

  *ret = (uint16_t) ret_buf;

  if ((ret_buf >= min) && (ret_buf <= max)) {
    return true;
  }
  else {
    return false;
  }
}

bool checkUint32(char* num_str, uint16_t num_str_len, uint32_t* ret, uint32_t min, uint32_t max)
{
  if (num_str_len > 10) return false;
  if (num_str_len == 0) {
    *ret = 0;
    if (min == 0) {
      return true;
    }
    else {
      return false;
    }
  }
  for (uint8_t i = 0; i < num_str_len; i++) {
    if (! isdigit((uint8_t) num_str[i])) return false;
  }

  uint32_t ret_buf = (uint32_t) strtoul((char*) num_str, NULL, 0);

  *ret = (uint32_t) ret_buf;

  if ((ret_buf >= min) && (ret_buf <= max)) {
    return true;
  }
  else {
    return false;
  }
}

bool checkFloat(char* num_str, float* ret, float min, float max)
{
  char extra;
  if (sscanf(num_str, "%f%c", ret, &extra) == 1) {
    if (*ret >= min && *ret <= max) {
      return true;
    }
  }
  return false;
}

bool checkYesNo(char input, bool* ret)
{
  if (input == 'y' || input == 'Y') {
    *ret = true;
    return true;
  } 
  else if (input == 'n' || input == 'N') {
    *ret = false;
    return true;
  }
  else {
    return false;
  }
}

/* Private function definitions ----------------------------------------------*/

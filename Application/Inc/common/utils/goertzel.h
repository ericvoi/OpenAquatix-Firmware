/*
 * goertzel.h
 *
 *  Created on: Jul 13, 2025
 *      Author: ericv
 */

#ifndef COMMON_UTILS_GOERTZEL_H_
#define COMMON_UTILS_GOERTZEL_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include <stdint.h>

/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/

typedef struct {
  uint16_t buf_len;
  uint16_t data_len;
  uint16_t start_pos;
  uint32_t* f;
  float* e_f;
  float* window;
  uint16_t window_size;
} GoertzelInfo_t;

/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

void goertzel_1(GoertzelInfo_t* goertzel_info);
void goertzel_2(GoertzelInfo_t* goertzel_info);
void goertzel_6(GoertzelInfo_t* goertzel_info);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* COMMON_UTILS_GOERTZEL_H_ */

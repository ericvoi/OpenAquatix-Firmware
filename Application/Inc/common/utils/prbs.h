/*
 * prbs.h
 *
 *  Created on: Jul 13, 2025
 *      Author: ericv
 */

#ifndef COMMON_UTILS_PRBS_H_
#define COMMON_UTILS_PRBS_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include <stdbool.h>

/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/



/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Resets the Pseudo-Random Binary Sequence (PRBS) generator to its
 * initialization value
 */
void PRBS_Reset();

/**
 * @brief Returns the next bit in the Pseudo-Random Bianry Sequence (PRBS)
 * 
 * @return true (1)
 * @return false (0)
 */
bool PRBS_GetNext();

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* COMMON_UTILS_PRBS_H_ */

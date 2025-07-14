/*
 * prbs.c
 *
 *  Created on: Jul 13, 2025
 *      Author: ericv
 */

/* Private includes ----------------------------------------------------------*/

#include "prbs.h"
#include <stdint.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/

#define PRBS_POLY   0xD008u // 1101 0000 0000 1000
#define PRBS_SEED   0x1234u // Any seed will work

/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/

static uint16_t state = PRBS_SEED;

/* Private function prototypes -----------------------------------------------*/



/* Exported function definitions ---------------------------------------------*/

void PRBS_Reset()
{
  state = PRBS_SEED;
}

// Galois LFSR
bool PRBS_GetNext()
{
  bool lsb = state & 1u;
  state >>= 1;
  if (lsb == true) {
    state ^= PRBS_POLY;
  }
  return lsb;
}

/* Private function definitions ----------------------------------------------*/

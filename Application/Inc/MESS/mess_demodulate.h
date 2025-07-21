/*
 * mess_demodulate.h
 *
 *  Created on: Feb 12, 2025
 *      Author: ericv
 */

#ifndef MESS_MESS_DEMODULATE_H_
#define MESS_MESS_DEMODULATE_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
#include "mess_dsp_config.h"
#include <stdbool.h>


/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/

typedef struct {
  uint16_t buf_len;          // length of data_buf
  uint16_t data_len;         // Length of the relevant part of data_buf
  uint16_t data_start_index;
  uint16_t chip_index;       // includes synchronization sequence (if applicable)
  uint16_t bit_index;
  bool decoded_bit;
  bool analysis_done;
  uint32_t f0;
  uint32_t f1;
  float energy_f0;
  float energy_f1;
} DemodulationInfo_t;

typedef enum {
  AMPLITUDE_COMPARISON,
  HISTORICAL_COMPARISON,
  // Others as needed
  NUM_DEMODULATION_DECISION
} DemodulationDecision_t;

typedef enum {
  WINDOW_RECTANGULAR,
  WINDOW_HANN,
  WINDOW_HAMMING,
  NUM_WINDOW_FUNCTIONS
} WindowFunction_t;

/* Exported constants --------------------------------------------------------*/

#define MAX_ANALYSIS_BUFFER_SIZE    64

/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Initializes window used for goertzel
 */
void Demodulate_Init();

/**
 * @brief Performs demodulation on the provided data
 *
 * Executes the appropriate demodulation algorithm based on the currently set
 * modulation method (FSK or FHBFSK). Then applies the selected decision method
 * to determine the decoded bit value.
 *
 * @param data Pointer to demodulation data structure containing input samples
 *             and which will be updated with demodulation results
 *
 * @return true if demodulation was successful, false otherwise
 *
 * @note For HISTORICAL_COMPARISON method, previous demodulation results are
 *       considered to improve the detection when significant energy shifts occur
 * @see Modulate_GetFhbfskFrequency
 */
bool Demodulate_Perform(DemodulationInfo_t* data, const DspConfig_t* cfg);

/**
 * @brief Power normalization factor for current windowing function
 * 
 * @return float Normalization factor that the power must be multiplied by
 * to normalize
 */
float Demodulate_PowerNormalization();

/**
 * @brief Registers demodulation parameters with the parameter management system
 *
 * Registers the decision method parameter to allow it to be accessed and
 * modified through the HMI interface.
 *
 * @return true if registration was successful, false otherwise
 *
 * @see Param_Register
 */
bool Demodulate_RegisterParams(void);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* MESS_MESS_DEMODULATE_H_ */

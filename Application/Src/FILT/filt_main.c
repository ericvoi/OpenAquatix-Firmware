/*
 * filt_main.c
 *
 *  Created on: Feb 20, 2026
 *      Author: ericv
 *
 * Copyright (c) 2026 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

/* Private includes ----------------------------------------------------------*/

#include "filt_main.h"
#include "cfg_parameters.h"
#include "cfg_main.h"
#include "cfg_defaults.h"
#include "sys_error.h"
#include "mess_filt_resources.h"
#include "mess_dsp_config.h"
#include "mess_background_noise.h"
#include "main.h"
#include "cmsis_os.h"
#include "arm_math.h"
#include <stdbool.h>

/* Private typedef -----------------------------------------------------------*/

typedef enum {
  FILT_STATE_RESET,
  FILT_STATE_FLUSHING,
  FILT_STATE_RUNNING
} FiltState_t;

typedef struct {
  FiltState_t state;
  DigitalFilter_t current_filter;
  uint8_t decimation_factor;
  bool use_filter;
} FiltTaskContext_t;

typedef struct {
  uint8_t decimation_factor;
  const bool use_filter;
  // TODO: add filter coefficients
} FilterInfo_t;

typedef struct {
  int32_t acc;
  uint8_t shift;
} DcEstimate_t;

/* Private define ------------------------------------------------------------*/

#define UNKNOWN_FILTER            (NUM_DIGITAL_FILTERS + 1)

#define SCRATCH_BUFFER_SIZE       (ADC_BUFFER_SIZE / 2)

#define ADC_MIDPOINT              (1U << (ADC_BITS - 1U))
#define Q15_UPSHIFT               (16U - ADC_BITS)
#define FLOAT_NORM                (1.0f / (float)ADC_MIDPOINT)
#define DC_SHIFT                  (ADC_BITS - 4U)

/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/

extern volatile uint16_t adc_buffer[];

static DigitalFilter_t fsk_filter = DEFAULT_FSK_FILTER;
static DigitalFilter_t fhbfsk_filter = DEFAULT_FHBFSK_FILTER;
static FilterInfo_t filter_infos[] = {
  [DIGITAL_FILTER_DEC] = {
    .decimation_factor = DEFAULT_DECIMATION_FACTOR,
    .use_filter = false
  },
  [DIGITAL_FILTER_NONE] = {
    .decimation_factor = 1,
    .use_filter = false
  }
};

static DcEstimate_t dc_tracker;

static q15_t filt_in_buffer[SCRATCH_BUFFER_SIZE];
static q15_t filt_out_buffer[SCRATCH_BUFFER_SIZE];
static float dec_buffer[SCRATCH_BUFFER_SIZE];

static uint16_t decimation_index_tracker = 0;

osEventFlagsId_t filt_events = NULL;
static FiltTaskContext_t task_context = {
  .state = FILT_STATE_RESET,
  .current_filter = UNKNOWN_FILTER
};

DEFINE_DESC_TABLE(DIGITAL_FILTER_TABLE, digital_filter_descriptors)

/* Private function prototypes -----------------------------------------------*/

static bool registerFiltParams(void);
static bool createFiltEvents(void);
static void dcEstimateInit(DcEstimate_t *estimator, uint8_t shift, int32_t seed);
static inline int32_t dcEstimateValue(const DcEstimate_t *estimator);
static inline void dcEstimateUpdate(DcEstimate_t *estimator, int32_t x);
static void handleEvents(void);
static void checkFilterChange(void);
static void decimateFilteredData(void);
static void processRawAdcData(bool first_half);
static void resetFmac(void);
static DigitalFilter_t getFilter(void);

/* Exported function definitions ---------------------------------------------*/

void FILT_StartTask(void* argument)
{
  (void)(argument);

  if (Param_RegisterTask(FILT_TASK, "FILT") == false) {
    Error_Routine(ERROR_FILT_INIT);
  }

  if (registerFiltParams() == false) {
    Error_Routine(ERROR_FILT_INIT);
  }

  if (Param_TaskRegistrationComplete(FILT_TASK) == false) {
    Error_Routine(ERROR_FILT_INIT);
  }

  CFG_WaitLoadComplete();

  if (createFiltEvents() == false) {
    Error_Routine(ERROR_FILT_INIT);
  }
  dcEstimateInit(&dc_tracker, DC_SHIFT, ADC_MIDPOINT);
  // Init FMAC (later) TODO

  for (;;) {
    checkFilterChange();
    handleEvents();
    osDelay(1);
  }
}

uint32_t FILT_PassbandToBaseband(uint32_t freq_hz)
{
  uint32_t baseband_bw = FILT_GetBandwidth();
  uint32_t fold_period = 2 * baseband_bw;
  uint32_t remainder = freq_hz % fold_period;

  /* If remainder is in the upper half, it folds (mirrors) back */
  if (remainder > baseband_bw)
    return fold_period - remainder;

  return remainder;
}

uint32_t FILT_GetBandwidth(void)
{
  return FILT_GetSamplingRate() / 2;
}

uint32_t FILT_GetSamplingRate(void)
{
  DigitalFilter_t filter = getFilter();
  uint8_t d = filter_infos[filter].decimation_factor;
  return ADC_SAMPLING_RATE / (d);
}

/* Private function definitions ----------------------------------------------*/

bool registerFiltParams(void)
{
  uint32_t min_u32 = MIN_FILTER;
  uint32_t max_u32 = MAX_FILTER;
  if (Param_Register(PARAM_FSK_FILTER, "FSK filter", PARAM_TYPE_ENUM,
                     &fsk_filter, sizeof(DigitalFilter_t), &min_u32, &max_u32,
                     NULL, digital_filter_descriptors) == false) {
    return false;
  }
  if (Param_Register(PARAM_FHBFSK_FILTER, "FH-BFSK filter", PARAM_TYPE_ENUM,
                     &fhbfsk_filter, sizeof(DigitalFilter_t), &min_u32, &max_u32,
                     NULL, digital_filter_descriptors) == false) {
    return false;
  }
  min_u32 = MIN_DECIMATION_FACTOR;
  max_u32 = MAX_DECIMATION_FACTOR;
  if (Param_Register(PARAM_DEC_FILTER_DEC_FACTOR, 
                     "Decimation filter decimation factor", PARAM_TYPE_UINT8,
                     &filter_infos[DIGITAL_FILTER_DEC].decimation_factor, 
                     sizeof(uint8_t), &min_u32, &max_u32, NULL, NULL) == false) {
    return false;
  }
  return true;
}

bool createFiltEvents(void)
{
  if (filt_events != NULL) return false;

  filt_events = osEventFlagsNew(NULL);
  return filt_events != NULL;
}

void dcEstimateInit(DcEstimate_t* estimator, uint8_t shift, int32_t seed)
{
  estimator->shift = shift;
  estimator->acc   = (int32_t)seed << shift;
}

inline int32_t dcEstimateValue(const DcEstimate_t *estimator)
{
  return estimator->acc >> estimator->shift;
}

inline void dcEstimateUpdate(DcEstimate_t *estimator, int32_t x)
{
  estimator->acc += x - (estimator->acc >> estimator->shift);
}

void handleEvents(void)
{
  uint32_t events = osEventFlagsGet(filt_events);

  bool raw_first_half_ready   = events & FILT_FIRST_HALF_RDY_RAW;
  bool raw_second_half_ready  = events & FILT_SECOND_HALF_RDY_RAW;
  bool fmac_ready             = events & FILT_FMAC_RDY;

  if (raw_first_half_ready && raw_second_half_ready) {
    // TODO: handle error
  }

  if (fmac_ready == true) decimateFilteredData();

  if (raw_first_half_ready == true) {
    osEventFlagsClear(filt_events, FILT_FIRST_HALF_RDY_RAW);
    processRawAdcData(true);
  }
  if (raw_second_half_ready == true) {
    osEventFlagsClear(filt_events, FILT_SECOND_HALF_RDY_RAW);
    processRawAdcData(false);
  }
}

void checkFilterChange(void)
{
  static uint32_t previous_cfg_num = 0;

  uint32_t current_cfg_num = CFG_GetVersionNumber();

  if (current_cfg_num == previous_cfg_num) return;
  previous_cfg_num = current_cfg_num;

  uint8_t mod_method;
  DigitalFilter_t new_filter;
  if (Param_GetEnum(PARAM_MOD_DEMOD_METHOD, &mod_method) == false) return;

  switch (mod_method) {
    case MOD_DEMOD_FSK:
      new_filter = fsk_filter;
      break;
    case MOD_DEMOD_FHBFSK:
      new_filter = fhbfsk_filter;
      break;
    default:
      return;
  }

  task_context.current_filter    = new_filter;
  task_context.decimation_factor = filter_infos[new_filter].decimation_factor;
  task_context.use_filter        = filter_infos[new_filter].use_filter;
  decimation_index_tracker = 0;
  MessFiltResources_InputAdcClear();
  BackgroundNoise_Reset();
  if (task_context.use_filter == true) {
    resetFmac();
    task_context.state = FILT_STATE_FLUSHING;
  }
  else {
    task_context.state = FILT_STATE_RUNNING;
  }
}

void decimateFilteredData(void)
{
  // Decimate the data and track how many samples added then add to buffer
}

void processRawAdcData(bool first_half)
{
  if (task_context.use_filter == true) return;
  uint32_t cyccnt = DWT->CYCCNT;

  uint16_t start = (first_half) ? (0)                   : (ADC_BUFFER_SIZE / 2);
  uint16_t end   = (first_half) ? (ADC_BUFFER_SIZE / 2) : (ADC_BUFFER_SIZE);
  uint16_t len = end - start;
  start += decimation_index_tracker;
  uint16_t d = task_context.decimation_factor;
  decimation_index_tracker = (d - ((len - decimation_index_tracker) % d)) % d;

  uint16_t entries_in_dec_buffer = 0;
  for (uint16_t i = start; i < end; i += d) {
    int32_t x = (int32_t)(adc_buffer[i]);
    dcEstimateUpdate(&dc_tracker, x);
    dec_buffer[entries_in_dec_buffer] = (float)(x - dcEstimateValue(&dc_tracker)) * FLOAT_NORM;
    entries_in_dec_buffer++;
  }

  MessFiltResources_AddFilteredSamples(dec_buffer, entries_in_dec_buffer, cyccnt);
}

void resetFmac(void)
{

}

DigitalFilter_t getFilter(void)
{
  uint8_t mod_method;
  if (Param_GetEnum(PARAM_MOD_DEMOD_METHOD, &mod_method) == false) return DIGITAL_FILTER_NONE;

  switch (mod_method) {
    case MOD_DEMOD_FSK:
      return fsk_filter;
    case MOD_DEMOD_FHBFSK:
      return fhbfsk_filter;
    default:
      return DIGITAL_FILTER_NONE;
  }
}

/*
 * mess_main.c
 *
 *  Created on: Feb 12, 2025
 *      Author: ericv
 */

/* Private includes ----------------------------------------------------------*/

#include "stm32h7xx_hal.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "cmsis_os.h"

#include "mess_main.h"
#include "mess_packet.h"
#include "mess_modulate.h"
#include "mess_adc.h"
#include "mess_input.h"
#include "mess_feedback.h"
#include "mess_evaluate.h"

#include "sys_error.h"

#include "cfg_main.h"
#include "cfg_parameters.h"
#include "cfg_defaults.h"

#include "dac_waveform.h"
#include "PGA113-driver.h"

#include "main.h"
#include <stdbool.h>
#include <string.h>

/* Private typedef -----------------------------------------------------------*/

typedef enum {
  DRIVING_TRANSDUCER,
  LISTENING,
  PROCESSING,
  CHANGING
} ProcessingState_t;

/* Private define ------------------------------------------------------------*/



/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/

QueueHandle_t tx_queue = NULL; // Messages to send
QueueHandle_t rx_queue = NULL; // Messages received

float baud_rate = DEFAULT_BAUD_RATE;
uint32_t fsk_f0 = DEFAULT_FSK_F0;
uint32_t fsk_f1 = DEFAULT_FSK_F1;
ModDemodMethod_t mod_demod_method = DEFAULT_MOD_DEMOD_METHOD;
uint32_t fc = DEFAULT_FC;
uint8_t fhbfsk_num_tones = DEFAULT_FHBFSK_NUM_TONES;
uint8_t fhbfsk_freq_spacing = DEFAULT_FHBFSK_FREQ_SPACING;
uint8_t fhbfsk_dwell_time = DEFAULT_FHBFSK_DWELL_TIME;

static bool evaluation_mode = DEFAULT_EVAL_MODE_STATE;
static uint8_t evaluation_message = DEFAULT_EVAL_MESSAGE;

static ProcessingState_t MESS_TaskState = LISTENING;

static BitMessage_t input_bit_msg;

bool in_feedback = false;
bool print_next_waveform = false;


/* Private function prototypes -----------------------------------------------*/

static void switchState(ProcessingState_t newState);
static void switchTrTransmit();
static void switchTrReceive();
static MessageFlags_t checkFlags();
static bool registerMessParams();
static bool registerMessMainParams();

/* Exported function definitions ---------------------------------------------*/

void MESS_StartTask(void* argument)
{
  (void)(argument);
  osEventFlagsClear(print_event_handle, 0xFFFFFFFF);
  Message_t tx_msg;
  WaveformStep_t message_sequence[PACKET_MAX_LENGTH_BITS];
  uint16_t message_length = 0;
  EvalMessageInfo_t eval_info;

  if (Param_RegisterTask(MESS_TASK, "MESS") == false) {
    Error_Routine(ERROR_MESS_INIT);
  }

  if (registerMessParams() == false) {
    Error_Routine(ERROR_MESS_INIT);
  }

  if (Param_TaskRegistrationComplete(MESS_TASK) == false) {
    Error_Routine(ERROR_MESS_INIT);
  }

  CFG_WaitLoadComplete();

  PGA_Init();
  PGA_Enable();
  osDelay(1);
  PGA_SetGain(PGA_GAIN_1);
  ADC_Init();
  Input_Init();
  Feedback_Init();
  Evaluate_Init();
  DAC_InitWaveformGenerator();
  switchState(LISTENING);
  // MESS_TaskState = LISTENING;

  osDelay(10);
  ADC_StartInput();
  for (;;) {
    switch (MESS_TaskState) {
      case DRIVING_TRANSDUCER:
        // Currently driving transducer so listen to transducer feedback network
        if (DAC_IsRunning() == false) {
          osDelay(1); // Lets the ADC finish in the case of feedback network
          HAL_TIM_Base_Stop(&htim6);
          if (in_feedback == true) {
            Feedback_DumpData();
            in_feedback = false;
          }
          switchState(LISTENING);
        }
        break;
      case LISTENING:
        // Wait for an edge/chirp or send a message if received
        MessageFlags_t flags = checkFlags();

        switch (flags) {
          case MESS_PRINT_REQUEST:
            osEventFlagsClear(print_event_handle, MESS_PRINT_REQUEST);
            Input_PrintNoise();
            osEventFlagsSet(print_event_handle, MESS_PRINT_COMPLETE);
            break;
          case MESS_FREQ_RESP:
            osEventFlagsClear(print_event_handle, MESS_FREQ_RESP);
            Modulate_TestFrequencyResponse();
            in_feedback = true;
            switchState(DRIVING_TRANSDUCER);
            break;
          case MESS_PRINT_WAVEFORM:
            osEventFlagsClear(print_event_handle, MESS_PRINT_WAVEFORM);
            print_next_waveform = true;
            break;
          default:
            break;
        }

        if (MESS_GetMessageFromTxQ(&tx_msg) == pdPASS) {
          BitMessage_t bit_msg;
          if (Packet_PrepareTx(&tx_msg, &bit_msg) == false) {
            // TODO: log error
            break;
          }
          message_length = bit_msg.bit_count;
          // convert to frequencies in message_sequence
          if (Modulate_ConvertToFrequency(&bit_msg, message_sequence) == false) {
            // TODO: log error
            break;
          }

          if (Modulate_ApplyAmplitude(message_sequence, message_length) == false) {
            // TODO: log error
            break;
          }

          if (Modulate_ApplyDuration(message_sequence, message_length) == false) {
            // TODO: log error
            break;
          }
          DAC_SetWaveformSequence(message_sequence, message_length);
          switch (tx_msg.type) {
            case MSG_TRANSMIT_TRANSDUCER:
              switchState(DRIVING_TRANSDUCER);
              break;
            case MSG_TRANSMIT_FEEDBACK:
              DAC_StartWaveformOutput(DAC_CHANNEL_FEEDBACK);
              // Should automatically go to processing once waveform being received without intervention
              break;
            default:
              break;
          }
        }

        if (Input_DetectMessageStart() == true) {
          switchState(PROCESSING);
          break;
        }
        break;
      case PROCESSING:
        // Process ADC input data only
        input_bit_msg.fully_received =
            (input_bit_msg.bit_count >= input_bit_msg.final_length) &&
            (input_bit_msg.preamble_received == true);

        if (Input_SegmentBlocks() == false) {
          Error_Routine(ERROR_MESS_PROCESSING);
          break;
        }
        if (Input_ProcessBlocks(&input_bit_msg, &eval_info) == false) {
          Error_Routine(ERROR_MESS_PROCESSING);
          break;
        }
        if (Input_DecodeBits(&input_bit_msg, evaluation_mode) == false) {
          Error_Routine(ERROR_MESS_PROCESSING);
          break;
        }
        if (evaluation_mode == true) {
          if (input_bit_msg.bit_count >= EVAL_MESSAGE_LENGTH && input_bit_msg.added_to_queue == false) {
            input_bit_msg.fully_received = true;
            Message_t rx_msg;
            rx_msg.data_type = EVAL;
            rx_msg.timestamp = osKernelGetTickCount();
            rx_msg.eval_info = &eval_info;
            rx_msg.eval_info->len_bits = EVAL_MESSAGE_LENGTH;
            rx_msg.eval_info->eval_msg = evaluation_message;
            memcpy(&rx_msg.data, input_bit_msg.data, 100 / 8 + 1);
            MESS_AddMessageToRxQ(&rx_msg);
            input_bit_msg.added_to_queue = true;
          }
        }
        else {
          if (input_bit_msg.fully_received == true && input_bit_msg.added_to_queue == false) {
            Message_t rx_msg;
            // TODO: fix currently incorrect since cant know if transducer or feedback
            rx_msg.type = (tx_msg.type == MSG_TRANSMIT_TRANSDUCER) ?
                          MSG_RECEIVED_TRANSDUCER : MSG_RECEIVED_FEEDBACK;
            rx_msg.timestamp = osKernelGetTickCount();
            rx_msg.length_bits = input_bit_msg.data_len_bits;
            rx_msg.data_type = input_bit_msg.contents_data_type;
            rx_msg.eval_info = &eval_info;
            rx_msg.sender_id = input_bit_msg.sender_id;
            // decode message
            if (Input_DecodeMessage(&input_bit_msg, &rx_msg) == false) {
              Error_Routine(ERROR_MESS_PROCESSING);
              break;
            }

            if (ErrorCorrection_CheckCorrection(&input_bit_msg,
                &rx_msg.error_correction_error) == false) {
              Error_Routine(ERROR_MESS_PROCESSING);
              break;
            }
            // send it via queue
            MESS_AddMessageToRxQ(&rx_msg);
            input_bit_msg.added_to_queue = true;
          }
        }
        if (Input_PrintWaveform(&print_next_waveform, input_bit_msg.fully_received) == false) {
          Error_Routine(ERROR_MESS_PROCESSING);
          break;
        }

        if (input_bit_msg.fully_received == true && print_next_waveform == false) {
          switchState(LISTENING);
        }
        break;
      default:
        break;
    }
    osDelay(1);
  }
}

void MESS_InitializeQueues(void)
{
  tx_queue = xQueueCreate(MSG_QUEUE_SIZE, sizeof(Message_t));
  rx_queue = xQueueCreate(MSG_QUEUE_SIZE, sizeof(Message_t));

  if (tx_queue == NULL || rx_queue == NULL) {
    // TODO: Handle error
  }
}

BaseType_t MESS_GetMessageFromTxQ(Message_t* msg)
{
  if (tx_queue == NULL || msg == NULL) {
    return pdFAIL;
  }

  if (uxQueueMessagesWaiting(tx_queue) > 0) {
    return xQueueReceive(tx_queue, (void*)msg, 0);
  }

  return pdFAIL;
}

BaseType_t MESS_AddMessageToTxQ(Message_t* msg)
{
  if (tx_queue == NULL || msg == NULL) {
    return pdFAIL;
  }

  return xQueueSend(tx_queue, msg, 5);
}
BaseType_t MESS_GetMessageFromRxQ(Message_t* msg)
{
  if (rx_queue == NULL || msg == NULL) {
    return pdFAIL;
  }

  if (uxQueueMessagesWaiting(rx_queue) > 0) {
    return xQueueReceive(rx_queue, (void*)msg, 0);
  }

  return pdFAIL;
}

BaseType_t MESS_AddMessageToRxQ(Message_t* msg)
{
  if (rx_queue == NULL || msg == NULL) {
    return pdFAIL;
  }

  return xQueueSend(rx_queue, msg, 5);
}

void MESS_RoundBaud(float* baud)
{
  float length_multiple = DAC_BUFFER_SIZE / 2; // Length of sequence must be a multiple of half the DAC buffer size
  float length_multiple_us = length_multiple * DAC_SAMPLE_RATE / 1000000.0f; // Converts symbol length multiple into micro seconds

  float baud_duration_us = (1000000.0f / *baud);

  float baud_multiple_durations = roundf(baud_duration_us / length_multiple_us);
  *baud = 1000000.0f / (baud_multiple_durations * length_multiple_us);
}

bool MESS_GetBandwidth(uint32_t* bandwidth, uint32_t* lower_freq, uint32_t* upper_freq)
{
  if (mod_demod_method == MOD_DEMOD_FSK) {
    if (fsk_f0 == fsk_f1) {
      return false;
    }
    if (fsk_f0 < fsk_f1) {
      *lower_freq = fsk_f0;
      *upper_freq = fsk_f1;
      *bandwidth = fsk_f1 - fsk_f0;
    }
    else {
      *lower_freq = fsk_f0;
      *upper_freq = fsk_f1;
      *bandwidth = fsk_f0 - fsk_f1;
    }
    return true;
  }
  else if (mod_demod_method == MOD_DEMOD_FHBFSK) {
    *lower_freq = Modulate_GetFhbfskFrequency(false, 0);

    uint16_t last_bit_index = fhbfsk_num_tones * fhbfsk_dwell_time - 1;
    *upper_freq = Modulate_GetFhbfskFrequency(true, last_bit_index);

    *bandwidth = *upper_freq - *lower_freq;
    return true;
  }
  return false;
}

bool MESS_GetBitPeriod(float* bit_period_ms)
{
  *bit_period_ms =  (1.0f / baud_rate) * 1000;
  return true;
}

/* Private function definitions ----------------------------------------------*/

static void switchState(ProcessingState_t newState)
{
  // First deactivate and clear all adcs, dacs, and all buffers except for the input buffer when transitioning from listening to processing
  MESS_TaskState = CHANGING;
  switch (newState) {
    case DRIVING_TRANSDUCER:
      ADC_StopAll();
      HAL_TIM_Base_Start(&htim6);
      HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_1, DAC_ALIGN_12B_R, 2048);
      HAL_DAC_Start(&hdac1, DAC_CHANNEL_1);
      osDelay(1);
      HAL_TIM_Base_Stop(&htim6);
      HAL_GPIO_WritePin(PAMP_MUTE_GPIO_Port, PAMP_MUTE_Pin, GPIO_PIN_RESET);
      osDelay(1);
      switchTrTransmit();
      osDelay(10);
      Modulate_StartTransducerOutput();
      // start
      MESS_TaskState = DRIVING_TRANSDUCER;
      break;
    case LISTENING:
      DAC_StopWaveformOutput();
      HAL_TIM_Base_Stop(&htim6);
      HAL_DAC_Stop(&hdac1, DAC_CHANNEL_1);
      HAL_GPIO_WritePin(PAMP_MUTE_GPIO_Port, PAMP_MUTE_Pin, GPIO_PIN_SET);
      ADC_StopAll();
      Input_Reset();
      osDelay(100); // I am terrified of the pre-amplifier being exposed to residual voltage from the power amplifier
      switchTrReceive();
      osDelay(5);
      ADC_StartInput();
      MESS_TaskState = LISTENING;
      break;
    case PROCESSING:
      Packet_PrepareRx(&input_bit_msg);
      MESS_TaskState = PROCESSING;
      break;
    default:
      break;
  }
}

static void switchTrTransmit()
{
  HAL_GPIO_WritePin(GPIOD, TR_CTRL_Pin, GPIO_PIN_RESET);
}

static void switchTrReceive()
{
  HAL_GPIO_WritePin(GPIOD, TR_CTRL_Pin, GPIO_PIN_SET);
}

static MessageFlags_t checkFlags()
{
  uint32_t flags;
  if (print_event_handle == NULL) {
    return 0;
  }
  flags = osEventFlagsWait(print_event_handle, MESS_PRINT_REQUEST, osFlagsWaitAny, 0);

  if (flags == osFlagsErrorResource) {
    // Normal nothing returned. Do nothing
  }
  else if (flags & 0x80000000U) {
    // TODO: log error
  }
  else if ((flags & MESS_PRINT_REQUEST) == MESS_PRINT_REQUEST) {
    return MESS_PRINT_REQUEST;
  }

  flags = osEventFlagsWait(print_event_handle, MESS_FREQ_RESP, osFlagsWaitAny, 0);

  if (flags == osFlagsErrorResource) {
    // Normal nothing returned. Do nothing
  }
  else if (flags & 0x80000000U) {
    // TODO: log error
  }
  else if ((flags & MESS_FREQ_RESP) == MESS_FREQ_RESP) {
    return MESS_FREQ_RESP;
  }

  flags = osEventFlagsWait(print_event_handle, MESS_PRINT_WAVEFORM, osFlagsWaitAny, 0);

  if (flags == osFlagsErrorResource) {
    // Normal nothing returned. Do nothing
  }
  else if (flags & 0x80000000U) {
    // TODO: log error
  }
  else if ((flags & MESS_PRINT_WAVEFORM) == MESS_PRINT_WAVEFORM) {
    return MESS_PRINT_WAVEFORM;
  }
  return 0;
}

static bool registerMessParams()
{
  // register all parameters from all files
  if (Modulate_RegisterParams() == false) {
    return false;
  }

  if (registerMessMainParams() == false) {
    return false;
  }

  if (Input_RegisterParams() == false) {
    return false;
  }

  if (Packet_RegisterParams() == false) {
    return false;
  }

  if (ErrorCorrection_RegisterParams() == false) {
    return false;
  }

  if (Demodulate_RegisterParams() == false) {
    return false;
  } 

  return true;
}

static bool registerMessMainParams()
{
  float min_f = MIN_BAUD_RATE;
  float max_f = MAX_BAUD_RATE;
  if (Param_Register(PARAM_BAUD, "baud rate", PARAM_TYPE_FLOAT,
                     &baud_rate, sizeof(float), &min_f, &max_f) == false) {
    return false;
  }

  uint32_t min_u32 = MIN_FSK_FREQUENCY;
  uint32_t max_u32 = MAX_FSK_FREQUENCY;
  if (Param_Register(PARAM_FSK_F0, "FSK 0 frequency", PARAM_TYPE_UINT32,
                     &fsk_f0, sizeof(uint32_t), &min_u32, &max_u32) == false) {
    return false;
  }
  if (Param_Register(PARAM_FSK_F1, "FSK 1 frequency", PARAM_TYPE_UINT32,
                     &fsk_f1, sizeof(uint32_t), &min_u32, &max_u32) == false) {
    return false;
  }

  min_u32 = MIN_MOD_DEMOD_METHOD;
  max_u32 = MAX_MOD_DEMOD_METHOD;
  if (Param_Register(PARAM_MOD_DEMOD_METHOD, "mod/demod method", PARAM_TYPE_UINT8,
                     &mod_demod_method, sizeof(uint8_t), &min_u32, &max_u32) == false) {
    return false;
  }

  min_u32 = MIN_FC;
  max_u32 = MAX_FC;
  if (Param_Register(PARAM_FC, "center frequency", PARAM_TYPE_UINT32,
                     &fc, sizeof(uint32_t), &min_u32, &max_u32) == false) {
    return false;
  }

  min_u32 = MIN_FHBFSK_FREQ_SPACING;
  max_u32 = MAX_FHBFSK_FREQ_SPACING;
  if (Param_Register(PARAM_FHBFSK_FREQ_SPACING, "frequency spacing", PARAM_TYPE_UINT8,
                     &fhbfsk_freq_spacing, sizeof(uint8_t), &min_u32, &max_u32) == false) {
    return false;
  }

  min_u32 = MIN_FHBFSK_DWELL_TIME;
  max_u32 = MAX_FHBFSK_DWELL_TIME;
  if (Param_Register(PARAM_FHBFSK_DWELL_TIME, "dwell time", PARAM_TYPE_UINT8,
                     &fhbfsk_dwell_time, sizeof(uint8_t), &min_u32, &max_u32) == false) {
    return false;
  }

  min_u32 = MIN_FHBFSK_NUM_TONES;
  max_u32 = MAX_FHBFSK_NUM_TONES;
  if (Param_Register(PARAM_FHBFSK_NUM_TONES, "number of tones", PARAM_TYPE_UINT8,
                     &fhbfsk_num_tones, sizeof(uint8_t), &min_u32, &max_u32) == false) {
    return false;
  }

  min_u32 = (uint32_t) MIN_EVAL_MODE_STATE;
  max_u32 = (uint32_t) MAX_EVAL_MODE_STATE;
  if (Param_Register(PARAM_EVAL_MODE_ON, "evaluation mode", PARAM_TYPE_UINT8,
                     &evaluation_mode, sizeof(uint8_t), &min_u32, &max_u32) == false) {
    return false;
  }

  min_u32 = MIN_EVAL_MESSAGE;
  max_u32 = MAX_EVAL_MESSAGE;
  if (Param_Register(PARAM_EVAL_MESSAGE, "evaluation message", PARAM_TYPE_UINT8,
                     &evaluation_message, sizeof(uint8_t), &min_u32, &max_u32) == false) {
    return false;
  }

  return true;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  (void)(GPIO_Pin);
}

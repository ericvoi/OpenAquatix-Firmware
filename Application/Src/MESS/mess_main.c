/*
 * mess_main.c
 *
 *  Created on: Feb 12, 2025
 *      Author: ericv
 * 
 * Copyright (c) 2025 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

/* Private includes ----------------------------------------------------------*/

#include "stm32h7xx_hal.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "cmsis_os.h"

#include "mess_main.h"
#include "mess_packet.h"
#include "mess_modulate.h"
#include "mess_filt_resources.h"
#include "mess_input.h"
#include "mess_feedback.h"
#include "mess_evaluate.h"
#include "mess_calibration.h"
#include "mess_dsp_config.h"
#include "mess_feedback_tests.h"
#include "mess_interleaver.h"
#include "mess_cargo.h"
#include "mess_background_noise.h"
#include "mess_sync.h"
#include "mess_error_correction.h"
#include "mess_demodulate.h"
#include "mess_error_detection.h"
#include "mess_ranging.h"

#include "cfg_main.h"
#include "cfg_parameters.h"
#include "cfg_defaults.h"
#include "cfg_callbacks.h"

#include "error_manager.h"

#include "afe.h"
#include "dac_waveform.h"
#include "pga113-driver.h"

#include "mess_dac_resources.h"

#include "main.h"
#include <stdbool.h>
#include <string.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/



/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/
static MessagingProtocol_t messaging_protocol = DEFAULT_MESSAGING_PROTOCOL;

// Identification parameters
static uint8_t custom_id = DEFAULT_ID;
static bool is_mobile = DEFAULT_STATIONARY_FLAG;
static bool tx_rx_capable = DEFAULT_TX_RX_CAPABLE;
static bool forwarding_capability = DEFAULT_FORWARD_CAPABILITY;
static uint8_t janus_id = DEFAULT_JANUS_ID;
static uint8_t janus_destination_id = DEFAULT_JANUS_DESTINATION;
static CodingInfo_t coding = DEFAULT_CODING;
static EncryptionInfo_t encryption = DEFAULT_ENCRYPTION;

static osMessageQueueId_t tx_queue = NULL; // Messages to send
static osMessageQueueId_t rx_queue = NULL; // Messages received

static ProcessingState_t task_state = CHANGING;

static BitMessage_t input_bit_msg;

static bool in_feedback = false;
static bool print_next_waveform = false;
static DspConfig_t custom_config = {
    .baud_rate = DEFAULT_BAUD_RATE,
    .mod_demod_method = DEFAULT_MOD_DEMOD_METHOD,
    .fsk_f0 = DEFAULT_FSK_F0,
    .fsk_f1 = DEFAULT_FSK_F1,
    .fc = DEFAULT_FC,
    .fhbfsk_freq_spacing = DEFAULT_FHBFSK_FREQ_SPACING,
    .fhbfsk_num_tones = DEFAULT_FHBFSK_NUM_TONES,
    .fhbfsk_dwell_time = DEFAULT_FHBFSK_DWELL_TIME,
    .preamble_validation = DEFAULT_PREAMBLE_ERROR_DETECTION,
    .cargo_validation = DEFAULT_CARGO_ERROR_DETECTION,
    .preamble_ecc_method = DEFAULT_ECC_PREAMBLE,
    .cargo_ecc_method = DEFAULT_ECC_MESSAGE,
    .use_interleaver = DEFAULT_INTERLEAVER_STATE,
    .fhbfsk_hopper = DEFAULT_FHBFSK_HOPPER,
    .sync_method = DEFAULT_SYNC_METHOD,
    .wakeup_tones = DEFAULT_WAKEUP_TONES_STATE,
    .wakeup_tone1 = DEFAULT_WAKEUP_TONE_FREQ1,
    .wakeup_tone2 = DEFAULT_WAKEUP_TONE_FREQ2,
    .wakeup_tone3 = DEFAULT_WAKEUP_TONE_FREQ3,
    .protocol = PROTOCOL_CUSTOM
};
static DspConfig_t janus_config = {
    .baud_rate = JANUS_BAUD,
    .mod_demod_method = JANUS_MOD_DEMOD,
    .fc = JANUS_FC,
    .fhbfsk_freq_spacing = JANUS_FHBFSK_FREQ_SPACING,
    .fhbfsk_num_tones = JANUS_FHBFSK_NUM_TONES,
    .fhbfsk_dwell_time = JANUS_FHBFSK_DWELL_TIME,
    .preamble_validation = JANUS_PREAMBLE_VALIDATION,
    .cargo_validation = JANUS_CARGO_VALIDATION,
    .preamble_ecc_method = JANUS_PREAMBLE_ECC,
    .cargo_ecc_method = JANUS_CARGO_ECC,
    .use_interleaver = JANUS_INTERLEAVER,
    .fhbfsk_hopper = JANUS_HOPPER,
    .sync_method = JANUS_SYNC_METHOD,
    .protocol = PROTOCOL_JANUS
};
static DspConfig_t* cfg = &custom_config;
static BitMessage_t bit_msg;
static uint16_t message_length = 0;

static Message_t tx_msg;
static Message_t rx_msg;

extern osMessageQueueId_t mac_rx_queue;

DEFINE_DESC_TABLE(MOD_DEMOD_METHODS_TABLE, mod_demod_descriptors)
DEFINE_DESC_TABLE(ERROR_DETECTION_METHOD_TABLE, error_detection_descriptors)
DEFINE_DESC_TABLE(ERROR_CORRECTION_METHOD_TABLE, error_correction_descriptors)
DEFINE_DESC_TABLE(FHBFSK_HOPPER_TABLE, fhbfsk_hopper_descriptors)
DEFINE_DESC_TABLE(SYNCHRONIZATION_METHOD_TABLE, synchronization_descriptors)
DEFINE_DESC_TABLE(MESSAGING_PROTOCOL_TABLE, messaging_protocol_descriptors)

DEFINE_DESC_TABLE(CODING_INFO_TABLE, coding_descriptors)
DEFINE_DESC_TABLE(ENCRYPTION_TABLE, encryption_descriptors)

/* Private function prototypes -----------------------------------------------*/

static void switchState(ProcessingState_t newState);
static void handleFlags();
static void sendMessage();
static void handleSync(SyncState_t sync_state);
static void resetTask();
static void registerMessParams();
static void registerMessMainParams();
static void handlePreambleOnlyMessage();
static void getConfig();

/* Exported function definitions ---------------------------------------------*/

void MESS_StartTask(void* argument)
{
  (void)(argument);

  Error_RegisterTask("MESS");
  registerMessParams();
  Error_ParameterRegistrationComplete();

  osEventFlagsClear(print_event_handle, 0xFFFFFFFF);
  MessDacResource_Init();
  BackgroundNoise_CreateShared();

  CFG_WaitLoadComplete();

  resetTask();
  for (;;) {
    switch (task_state) {
      case DRIVING_TRANSDUCER:
        // Currently driving transducer so listen to transducer feedback network
        if (Waveform_IsRunning() == false) {
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

        Input_UpdatePgaGain();
        handleFlags();

        FeedbackTests_GetNext();
        getConfig();

        if (MESS_GetMessageFromTxQ(&tx_msg) == true) {
          getConfig();

          Packet_PrepareTx(&tx_msg, &bit_msg, cfg);
          ErrorCorrection_AddCorrection(&bit_msg, cfg);
          FeedbackTests_CorruptMessage(&bit_msg); // if applicable
          Interleaver_Apply(&bit_msg, cfg);
          message_length = bit_msg.bit_count;
          sendMessage();
        }

        SyncState_t sync_state = Sync_Synchronize(cfg, &rx_msg);
        handleSync(sync_state);

        BackgroundNoise_Calculate(cfg);
        break;
      case PROCESSING:
        // Process ADC input data only
        input_bit_msg.fully_received =
            (input_bit_msg.bit_count >= input_bit_msg.final_length) &&
            (input_bit_msg.preamble_received == true);

        Input_UpdatePgaGain();
        if (input_bit_msg.fully_received == false) {
          Input_SegmentBlocks(cfg);
        }
        Input_ProcessBlocks(&input_bit_msg, cfg);

        bool proceed = true;
        Input_DecodeBits(&input_bit_msg, cfg, &rx_msg, &proceed);
        if (proceed == false) {switchState(LISTENING); break;};

        if (input_bit_msg.fully_received == true && input_bit_msg.added_to_queue == false) {
          // TODO: fix currently incorrect since cant know if transducer or feedback
          rx_msg.type = (tx_msg.type == MSG_TRANSMIT_TRANSDUCER) ?
                        MSG_RECEIVED_TRANSDUCER : MSG_RECEIVED_FEEDBACK;
          rx_msg.timestamp = osKernelGetTickCount();
          rx_msg.length_bits = input_bit_msg.data_len_bits;
          rx_msg.protocol = cfg->protocol;

          if (input_bit_msg.cargo.raw_len == 0) {
            handlePreambleOnlyMessage();
            switchState(LISTENING);
            break;
          }

          Interleaver_Undo(&input_bit_msg, cfg, false);

          // TODO: change to also require message to be custom
          if (rx_msg.preamble.message_type.value == EVAL && (rx_msg.preamble.message_type.valid == true)) {
            Evaluate_UncodedBer(&rx_msg.eval_info, &input_bit_msg, cfg);
          }

          // undo fec
          ErrorCorrection_CheckCorrection(&input_bit_msg, cfg, false,
              &input_bit_msg.error_message, 
              &input_bit_msg.corrected_error_message);
          // decode message
          Cargo_Decode(&input_bit_msg, &rx_msg, cfg);

          ErrorDetection_CheckDetection(&input_bit_msg,
              &rx_msg.error_detected, cfg, false);
          rx_msg.error_detected |= input_bit_msg.error_preamble;
          // send it via queue
          if (FeedbackTests_Check(&rx_msg, &input_bit_msg) == false) {
            osMessageQueuePut(mac_rx_queue, &rx_msg, 0, 0);
          }
          input_bit_msg.added_to_queue = true;
        }
        Input_PrintWaveform(&print_next_waveform, input_bit_msg.fully_received);

        if (input_bit_msg.fully_received == true && print_next_waveform == false) {
          switchState(LISTENING);
        }
        break;
      default:
        break;
    }
    ErrorCheck_t status = Error_CheckStatus();
    TaskResetStatus_t reset_required = Error_CheckModuleReset();
    bool reset_cond1 = (status == ERROR_QUIT) && (task_state != LISTENING);
    bool reset_cond2 = reset_required == TASK_RESET;
    Error_ResetAbortFlag();
    if (reset_cond1 || reset_cond2) resetTask();
    osDelay(1);
  }
}

void MESS_InitializeQueues(void)
{
  tx_queue = osMessageQueueNew(MSG_QUEUE_SIZE, sizeof(Message_t), NULL);
  rx_queue = osMessageQueueNew(MSG_QUEUE_SIZE, sizeof(Message_t), NULL);

  if (tx_queue == NULL || rx_queue == NULL) 
    REGISTER_ERROR(ERROR_QUEUE_INITIALIZATION);
}

bool MESS_GetMessageFromTxQ(Message_t* msg)
{
  RETURN_IF_ERROR_PRESENT_NON_VOID(,false);
  if (tx_queue == NULL || msg == NULL) {
    REGISTER_ERROR_NON_VOID(ERROR_NULL_PTR, false);
    return false;
  }

  if (osMessageQueueGetCount(tx_queue) > 0) {
    if (osMessageQueueGet(tx_queue, (void*) msg, NULL, 0) != osOK) {
      REGISTER_ERROR_NON_VOID(ERROR_QUEUE_RUNNING, false);
      return false;
    }
    return true;
  }

  return false;
}

bool MESS_AddMessageToTxQ(const Message_t* msg)
{
  RETURN_IF_ERROR_PRESENT_NON_VOID(,false);
  if (tx_queue == NULL || msg == NULL) {
    REGISTER_ERROR_NON_VOID(ERROR_NULL_PTR, false);
    return false;
  }

  if (osMessageQueuePut(tx_queue, msg, 0, 0) != osOK) {
    REGISTER_ERROR_NON_VOID(ERROR_QUEUE_RUNNING, false);
    return false;
  }
  return true;
}

bool MESS_GetMessageFromRxQ(Message_t* msg)
{
  RETURN_IF_ERROR_PRESENT_NON_VOID(, false);
  if (rx_queue == NULL || msg == NULL) {
    REGISTER_ERROR_NON_VOID(ERROR_NULL_PTR, false);
    return false;
  }

  if (osMessageQueueGetCount(rx_queue) > 0) {
    if (osMessageQueueGet(rx_queue, (void*) msg, NULL, 0) != osOK) {
      REGISTER_ERROR_NON_VOID(ERROR_QUEUE_RUNNING, false);
      return false;
    }
    return true;
  }

  return false;
}

bool MESS_AddMessageToRxQ(const Message_t* msg)
{
  RETURN_IF_ERROR_PRESENT_NON_VOID(,false);
  if (tx_queue == NULL || msg == NULL) {
    REGISTER_ERROR_NON_VOID(ERROR_NULL_PTR, false);
    return false;
  }

  if (osMessageQueuePut(rx_queue, msg, 0, 0) != osOK) {
    REGISTER_ERROR_NON_VOID(ERROR_QUEUE_RUNNING, false);
    return false;
  }
  return true;
}

bool MESS_PriorityTransmission(const Message_t* msg)
{
  RETURN_IF_ERROR_PRESENT_NON_VOID(,false);
  if (tx_queue == NULL || msg == NULL) {
    REGISTER_ERROR_NON_VOID(ERROR_NULL_PTR, false);
    return false;
  }

  if (osMessageQueueReset(tx_queue) != osOK) {
    REGISTER_ERROR_NON_VOID(ERROR_QUEUE_RUNNING, false);
    return false;
  }

  if (osMessageQueuePut(tx_queue, msg, 0, 0) != osOK) {
    REGISTER_ERROR_NON_VOID(ERROR_QUEUE_RUNNING, false);
    return false;
  }
  return true;
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
  if (custom_config.mod_demod_method == MOD_DEMOD_FSK) {
    if (custom_config.fsk_f0 < custom_config.fsk_f1) {
      *lower_freq = custom_config.fsk_f0;
      *upper_freq = custom_config.fsk_f1;
      *bandwidth = *upper_freq - *lower_freq;
    }
    else {
      *lower_freq = custom_config.fsk_f1;
      *upper_freq = custom_config.fsk_f0;
      *bandwidth = *upper_freq - *lower_freq;
    }
    return true;
  }
  else if (custom_config.mod_demod_method == MOD_DEMOD_FHBFSK) {
    DspConfig_t temp_cfg;
    memcpy(&temp_cfg, &custom_config, sizeof(DspConfig_t));
    temp_cfg.fhbfsk_hopper = HOPPER_INCREMENT;
    *lower_freq = Modulate_GetFhbfskFrequency(false, 0, &temp_cfg);

    uint16_t last_bit_index = temp_cfg.fhbfsk_num_tones * temp_cfg.fhbfsk_dwell_time - 1;
    *upper_freq = Modulate_GetFhbfskFrequency(true, last_bit_index, &temp_cfg);

    *bandwidth = *upper_freq - *lower_freq;
    return true;
  }
  REGISTER_ERROR_NON_VOID(ERROR_UNHANDLED_CASE, false);
  return false;
}

void MESS_GetBitPeriod(float* bit_period_ms)
{
  *bit_period_ms = (1.0f / custom_config.baud_rate) * 1000;
}

ProcessingState_t MESS_GetState()
{
  return task_state;
}

/* Private function definitions ----------------------------------------------*/

void switchState(ProcessingState_t newState)
{
  RETURN_IF_ERROR_PRESENT();
  // First deactivate and clear all adcs, dacs, and all buffers except for the input buffer when transitioning from listening to processing
  task_state = CHANGING;
  switch (newState) {
    case DRIVING_TRANSDUCER:
      RETURN_IF_ERROR_PRESENT(AFE_SetMode(AFE_MODE_TX)); // TODO: change to include feedback for input and output
      RETURN_IF_ERROR_PRESENT(Modulate_StartTransducerOutput(message_length, cfg, &bit_msg, &tx_msg));
      task_state = DRIVING_TRANSDUCER;
      break;
    case LISTENING:
      Sync_Reset();
      cfg = &custom_config;
      if (Waveform_StopWaveformOutput() == false) 
        REGISTER_ERROR(ERROR_STOPPING_TRANSDUCER_OUTPUT);
      
      RETURN_IF_ERROR_PRESENT(AFE_SetMode(AFE_MODE_RX));
      task_state = LISTENING;
      break;
    case PROCESSING:
      Packet_PrepareRx(&rx_msg, &input_bit_msg, cfg);
      task_state = PROCESSING;
      break;
    default:
      break;
  }
}

void handleFlags()
{
  uint32_t flags = osEventFlagsWait(print_event_handle, 0xFFFF, osFlagsWaitAny, 0);

  if (flags == osFlagsErrorResource) {
    return;
  }

  if (flags & 0x80000000U) {
    REGISTER_ERROR(ERROR_FLAGS_RUNNING);
  }

  if (flags & MESS_PRINT_REQUEST) {
    osEventFlagsClear(print_event_handle, MESS_PRINT_REQUEST);
    Input_PrintNoise();
    osEventFlagsSet(print_event_handle, MESS_PRINT_COMPLETE);
  }
  else if (flags & MESS_FREQ_RESP) {
    osEventFlagsClear(print_event_handle, MESS_FREQ_RESP);
    Modulate_TestFrequencyResponse();
    in_feedback = true;
    switchState(DRIVING_TRANSDUCER);
  }
  else if (flags & MESS_PRINT_WAVEFORM) {
    osEventFlagsClear(print_event_handle, MESS_PRINT_WAVEFORM);
    print_next_waveform = true;
  }
  else if (flags & MESS_FEEDBACK_TESTS) {
    osEventFlagsClear(print_event_handle, MESS_FEEDBACK_TESTS);
    FeedbackTests_Start();
  }
  else if (flags & MESS_INPUT_FFT) {
    osEventFlagsClear(print_event_handle, MESS_INPUT_FFT);
    Input_NoiseFft();
    osEventFlagsSet(print_event_handle, MESS_PRINT_COMPLETE);
  }
  else if (flags & MESS_REQUEST_RANGE_FEEDBACK) {
    osEventFlagsClear(print_event_handle, MESS_REQUEST_RANGE_FEEDBACK);
    Ranging_Request(cfg, true);
  }
  else if (flags & MESS_REQUEST_RANGE_TRANSDUCER) {
    osEventFlagsClear(print_event_handle, MESS_REQUEST_RANGE_TRANSDUCER);
    Ranging_Request(cfg, false);
  }
}

void handlePreambleOnlyMessage()
{
  switch (cfg->protocol) {
    case PROTOCOL_CUSTOM:
      switch (rx_msg.preamble.message_type.value) {
        case STRING:
        case BITS:
        case INTEGER:
        case FLOAT:
          REGISTER_ERROR(ERROR_UNKNOWN_MESSAGE);
          return;
        case RANGING_REQUEST:
          Ranging_Respond(rx_msg.rx_cyccnt, rx_msg.type == MSG_RECEIVED_FEEDBACK);
          return;
        case RANGING_RESPONSE:
          Ranging_LogResponse(&rx_msg);
          return;
        default:
          REGISTER_ERROR(ERROR_UNKNOWN_MESSAGE);
          return;
      }
    case PROTOCOL_JANUS:
      REGISTER_ERROR(ERROR_UNKNOWN_JANUS);
      return;
    default:
      REGISTER_ERROR(ERROR_UNKNOWN_JANUS);
      return;
  }
}

void sendMessage()
{
  RETURN_IF_ERROR_PRESENT();
  switch (tx_msg.type) {
    case MSG_TRANSMIT_TRANSDUCER:
      switchState(DRIVING_TRANSDUCER);
      break;
    case MSG_TRANSMIT_FEEDBACK:
      AFE_SetMode(AFE_MODE_RX_FEEDBACK);
      Modulate_StartFeedbackOutput(message_length, cfg, &bit_msg, &tx_msg);
      // Should automatically go to processing once waveform being received without intervention
      // TODO: what if above unsuccessful?
      break;
    default:
      break;
  }
}

void handleSync(SyncState_t sync_state)
{
  RETURN_IF_ERROR_PRESENT();
  switch (sync_state) {
    case SYNC_SUCCESS:
      switchState(PROCESSING);
      break;
    case SYNC_OK:
      break; // Do nothing, still synchronizing
    default:
      REGISTER_ERROR(ERROR_UNHANDLED_CASE);
      break;
  }
}

void resetTask()
{
  Pga113_Init();
  osDelay(1);
  Pga113_SetGain(PGA_GAIN_1);
  MessFiltResources_Init();
  Input_Init();
  Feedback_Init();
  FeedbackTests_Init();
  Demodulate_Init();
  BackgroundNoise_Reset();
  switchState(LISTENING);

  osDelay(10);
  Waveform_Flush();
  MessFiltResources_StartInputAdc();
}

void registerMessParams()
{
  // register all parameters from all files
  Modulate_RegisterParams();
  registerMessMainParams();
  Input_RegisterParams();
  Packet_RegisterParams();
  ErrorDetection_RegisterParams();
  Demodulate_RegisterParams();
  Calibrate_RegisterParams();
  Evaluate_RegisterParams();
}

void registerMessMainParams()
{
  float min_f = MIN_BAUD_RATE;
  float max_f = MAX_BAUD_RATE;
  if (Param_Register(PARAM_BAUD, "baud rate", PARAM_TYPE_FLOAT,
                     &custom_config.baud_rate, sizeof(float),
                     &min_f, &max_f, NULL, NULL) == false) {
    REGISTER_ERROR(ERROR_PARAMETER_REGISTRATION);
  }

  uint32_t min_u32 = MIN_FSK_FREQUENCY;
  uint32_t max_u32 = MAX_FSK_FREQUENCY;
  if (Param_Register(PARAM_FSK_F0, "FSK 0 frequency", PARAM_TYPE_UINT32,
                     &custom_config.fsk_f0, sizeof(uint32_t),
                     &min_u32, &max_u32, NULL, NULL) == false) {
    REGISTER_ERROR(ERROR_PARAMETER_REGISTRATION);
  }
  if (Param_Register(PARAM_FSK_F1, "FSK 1 frequency", PARAM_TYPE_UINT32,
                     &custom_config.fsk_f1, sizeof(uint32_t),
                     &min_u32, &max_u32, NULL, NULL) == false) {
    REGISTER_ERROR(ERROR_PARAMETER_REGISTRATION);
  }

  min_u32 = MIN_MOD_DEMOD_METHOD;
  max_u32 = MAX_MOD_DEMOD_METHOD;
  if (Param_Register(PARAM_MOD_DEMOD_METHOD, "mod/demod method", PARAM_TYPE_ENUM,
                     &custom_config.mod_demod_method, sizeof(uint8_t),
                     &min_u32, &max_u32, NULL, mod_demod_descriptors) == false) {
    REGISTER_ERROR(ERROR_PARAMETER_REGISTRATION);
  }

  min_u32 = MIN_FC;
  max_u32 = MAX_FC;
  if (Param_Register(PARAM_FC, "center frequency", PARAM_TYPE_UINT32,
                     &custom_config.fc, sizeof(uint32_t),
                     &min_u32, &max_u32, NULL, NULL) == false) {
    REGISTER_ERROR(ERROR_PARAMETER_REGISTRATION);
  }

  min_u32 = MIN_FHBFSK_FREQ_SPACING;
  max_u32 = MAX_FHBFSK_FREQ_SPACING;
  if (Param_Register(PARAM_FHBFSK_FREQ_SPACING, "frequency spacing", PARAM_TYPE_UINT8,
                     &custom_config.fhbfsk_freq_spacing, sizeof(uint8_t),
                     &min_u32, &max_u32, NULL, NULL) == false) {
    REGISTER_ERROR(ERROR_PARAMETER_REGISTRATION);
  }

  min_u32 = MIN_FHBFSK_DWELL_TIME;
  max_u32 = MAX_FHBFSK_DWELL_TIME;
  if (Param_Register(PARAM_FHBFSK_DWELL_TIME, "dwell time", PARAM_TYPE_UINT8,
                     &custom_config.fhbfsk_dwell_time, sizeof(uint8_t),
                     &min_u32, &max_u32, NULL, NULL) == false) {
    REGISTER_ERROR(ERROR_PARAMETER_REGISTRATION);
  }

  min_u32 = MIN_FHBFSK_NUM_TONES;
  max_u32 = MAX_FHBFSK_NUM_TONES;
  if (Param_Register(PARAM_FHBFSK_NUM_TONES, "number of tones", 
                     PARAM_TYPE_UINT8, &custom_config.fhbfsk_num_tones, 
                     sizeof(uint8_t), &min_u32, &max_u32, NULL, NULL) == false) {
    REGISTER_ERROR(ERROR_PARAMETER_REGISTRATION);
  }

  min_u32 = MIN_ERROR_DETECTION;
  max_u32 = MAX_ERROR_DETECTION;
  if (Param_Register(PARAM_PREAMBLE_ERROR_DETECTION, 
                     "preamble error detection method", PARAM_TYPE_ENUM,
                     &custom_config.preamble_validation, sizeof(uint8_t),
                     &min_u32, &max_u32, NULL, error_detection_descriptors
                     ) == false) {
    REGISTER_ERROR(ERROR_PARAMETER_REGISTRATION);
  }

  if (Param_Register(PARAM_CARGO_ERROR_DETECTION, 
                     "cargo error detection method", PARAM_TYPE_ENUM,
                     &custom_config.cargo_validation, sizeof(uint8_t),
                     &min_u32, &max_u32, NULL, error_detection_descriptors
                     ) == false) {
    REGISTER_ERROR(ERROR_PARAMETER_REGISTRATION);
  }

  min_u32 = MIN_ECC_METHOD;
  max_u32 = MAX_ECC_METHOD;
  if (Param_Register(PARAM_ECC_PREAMBLE, "preamble ECC", PARAM_TYPE_ENUM,
                     &custom_config.preamble_ecc_method, sizeof(uint8_t),
                     &min_u32, &max_u32, NULL, error_correction_descriptors
                     ) == false) {
    REGISTER_ERROR(ERROR_PARAMETER_REGISTRATION);
  }

  // Using the same bounds as ^
  if (Param_Register(PARAM_ECC_MESSAGE, "message ECC", PARAM_TYPE_ENUM,
                     &custom_config.cargo_ecc_method, sizeof(uint8_t),
                     &min_u32, &max_u32, NULL, error_correction_descriptors
                     ) == false) {
    REGISTER_ERROR(ERROR_PARAMETER_REGISTRATION);
  }

  min_u32 = MIN_INTERLEAVER_STATE;
  max_u32 = MAX_INTERLEAVER_STATE;
  if (Param_Register(PARAM_USE_INTERLEAVER, "message interleaving", PARAM_TYPE_UINT8,
                     &custom_config.use_interleaver, sizeof(bool),
                     &min_u32, &max_u32, NULL, NULL) == false) {
    REGISTER_ERROR(ERROR_PARAMETER_REGISTRATION);
  }

  min_u32 = MIN_FHBFSK_HOPPER;
  max_u32 = MAX_FHBFSK_HOPPER;
  if (Param_Register(PARAM_FHBFSK_HOPPER, "hopper method", PARAM_TYPE_ENUM,
                     &custom_config.fhbfsk_hopper, sizeof(uint8_t), 
                     &min_u32, &max_u32, NULL, fhbfsk_hopper_descriptors
                     ) == false) {
    REGISTER_ERROR(ERROR_PARAMETER_REGISTRATION);
  }

  min_u32 = MIN_SYNC_METHOD;
  max_u32 = MAX_SYNC_METHOD;
  if (Param_Register(PARAM_SYNC_METHOD, "synchronization method", PARAM_TYPE_ENUM,
                     &custom_config.sync_method, sizeof(uint8_t),
                     &min_u32, &max_u32, NULL, synchronization_descriptors
                     ) == false) {
    REGISTER_ERROR(ERROR_PARAMETER_REGISTRATION);
  }

  min_u32 = MIN_ID;
  max_u32 = MAX_ID;
  if (Param_Register(PARAM_ID, "the modem identifier", PARAM_TYPE_UINT8,
                     &custom_id, sizeof(uint8_t), &min_u32, 
                     &max_u32, NULL, NULL) == false) {
    REGISTER_ERROR(ERROR_PARAMETER_REGISTRATION);
  }

  min_u32 = MIN_STATIONARY_FLAG;
  max_u32 = MAX_STATIONARY_FLAG;
  if (Param_Register(PARAM_STATIONARY_FLAG, "stationary flag", PARAM_TYPE_UINT8,
                     &is_mobile, sizeof(uint8_t), &min_u32, 
                     &max_u32, NULL, NULL) == false) {
    REGISTER_ERROR(ERROR_PARAMETER_REGISTRATION);
  }

  min_u32 = MIN_WAKEUP_TONES_STATE;
  max_u32 = MAX_WAKEUP_TONES_STATE;
  if (Param_Register(PARAM_WAKEUP_TONES_STATE, "wakeup tones", PARAM_TYPE_UINT8,
                     &custom_config.wakeup_tones, sizeof(bool), &min_u32, 
                     &max_u32, NULL, NULL) == false) {
    REGISTER_ERROR(ERROR_PARAMETER_REGISTRATION);
  }

  min_u32 = MIN_WAKEUP_TONE_FREQ;
  max_u32 = MAX_WAKEUP_TONE_FREQ;
  if (Param_Register(PARAM_WAKEUP_TONE1, "wakeup tone 1", PARAM_TYPE_UINT32,
                     &custom_config.wakeup_tone1, sizeof(uint32_t), &min_u32, 
                     &max_u32, NULL, NULL) == false) {
    REGISTER_ERROR(ERROR_PARAMETER_REGISTRATION);
  }
  if (Param_Register(PARAM_WAKEUP_TONE2, "wakeup tone 2", PARAM_TYPE_UINT32,
                     &custom_config.wakeup_tone2, sizeof(uint32_t), &min_u32, 
                     &max_u32, NULL, NULL) == false) {
    REGISTER_ERROR(ERROR_PARAMETER_REGISTRATION);
  }
  if (Param_Register(PARAM_WAKEUP_TONE3, "wakeup tone 3", PARAM_TYPE_UINT32,
                     &custom_config.wakeup_tone3, sizeof(uint32_t), &min_u32, 
                     &max_u32, NULL, NULL) == false) {
    REGISTER_ERROR(ERROR_PARAMETER_REGISTRATION);
  }

  min_u32 = MIN_MESSAGING_PROTOCOL;
  max_u32 = MAX_MESSAGING_PROTOCOL;
  if (Param_Register(PARAM_PROTOCOL, "the messaging protocol", PARAM_TYPE_UINT8,
                     &messaging_protocol, sizeof(uint8_t), &min_u32, 
                     &max_u32, NULL, NULL) == false) {
    REGISTER_ERROR(ERROR_PARAMETER_REGISTRATION);
  }

  min_u32 = MIN_TX_RX_CAPABLE;
  max_u32 = MAX_TX_RX_CAPABLE;
  if (Param_Register(PARAM_TX_RX_ABILITY, "Tx/Rx ability flag", PARAM_TYPE_ENUM,
                     &tx_rx_capable, sizeof(bool), &min_u32,
                     &max_u32, NULL, messaging_protocol_descriptors) == false) {
    REGISTER_ERROR(ERROR_PARAMETER_REGISTRATION);
  }

  min_u32 = MIN_FORWARD_CAPABILITY;
  max_u32 = MAX_FORWARD_CAPABILITY;
  if (Param_Register(PARAM_FORWARD_CAPABILITY, "packet forward ability flag", 
                     PARAM_TYPE_UINT8, &forwarding_capability, sizeof(bool),
                     &min_u32, &max_u32, NULL, NULL) == false) {
    REGISTER_ERROR(ERROR_PARAMETER_REGISTRATION);
  }

  min_u32 = MIN_JANUS_ID;
  max_u32 = MAX_JANUS_ID;
  if (Param_Register(PARAM_JANUS_ID, "JANUS ID", PARAM_TYPE_UINT8,
                     &janus_id, sizeof(uint8_t), &min_u32, &max_u32, 
                     NULL, NULL) == false) {
    REGISTER_ERROR(ERROR_PARAMETER_REGISTRATION);
  }

  min_u32 = MIN_JANUS_DESTINATION;
  max_u32 = MAX_JANUS_DESTINATION;
  if (Param_Register(PARAM_JANUS_DESTINATION, "JANUS destination ID",
                     PARAM_TYPE_UINT8, &janus_destination_id, sizeof(uint8_t),
                     &min_u32, &max_u32, NULL, NULL) == false) {
    REGISTER_ERROR(ERROR_PARAMETER_REGISTRATION);
  }

  min_u32 = MIN_CODING;
  max_u32 = MAX_CODING;
  if (Param_Register(PARAM_CODING, "string coding", PARAM_TYPE_ENUM,
                     &coding, sizeof(uint8_t), &min_u32, &max_u32, 
                     NULL, coding_descriptors) == false) {
    REGISTER_ERROR(ERROR_PARAMETER_REGISTRATION);
  }

  min_u32 = MIN_ENCRYPTION;
  max_u32 = MAX_ENCRYPTION;
  if (Param_Register(PARAM_ENCRYPTION, "cargo encryption", PARAM_TYPE_ENUM,
                     &encryption, sizeof(uint8_t), &min_u32, &max_u32, 
                     NULL, encryption_descriptors) == false) {
    REGISTER_ERROR(ERROR_PARAMETER_REGISTRATION);
  }
}

void getConfig()
{
  RETURN_IF_ERROR_PRESENT();
  if (FeedbackTests_GetConfig(&cfg) == true) return;

  switch (messaging_protocol) {
    case PROTOCOL_CUSTOM:
      cfg = &custom_config;
      break;
    case PROTOCOL_JANUS:
      cfg = &janus_config;
      break;
    default:
      break;
  }
}

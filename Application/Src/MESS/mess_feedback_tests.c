/*
 * mess_feedback_tests.c
 *
 *  Created on: Apr 21, 2025
 *      Author: ericv
 */

/* Private includes ----------------------------------------------------------*/

#include "mess_feedback_tests.h"
#include "mess_dsp_config.h"
#include "mess_packet.h"
#include "mess_main.h"

#include "comm_main.h"

#include "cfg_main.h"

#include "cmsis_os.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/* Private typedef -----------------------------------------------------------*/

typedef enum {
  IDENTICAL,
  ERRORS
} ExpectedResult_t;

typedef struct {
  Message_t test_msg;
} ReferenceMessage_t;

typedef struct {
  DspConfig_t cfg;
  const ExpectedResult_t expected_result;
  ReferenceMessage_t* reference_message;
  const uint16_t errors_added;
  const uint16_t repetitions;
  // Statistics
  uint16_t messages_with_errors_detected;
  uint16_t messages_with_any_errors;
  uint16_t messages_with_incorrect_length;
  uint16_t messages_with_header_errors;
  uint16_t failed_tests;
} FeedbackTests_t;

typedef enum {
  SENT_MESSAGE,
  DECODED_MESSAGE
} LastAction_t;

typedef enum {
  MESSAGE_LEN_8    = 0,
  MESSAGE_LEN_16   = 1,
  MESSAGE_LEN_32   = 2,
  MESSAGE_LEN_64   = 3,
  MESSAGE_LEN_128  = 4,
  MESSAGE_LEN_256  = 5,
  MESSAGE_LEN_512  = 6,
  MESSAGE_LEN_1024 = 7
} MessageLengths_t;

/* Private define ------------------------------------------------------------*/



/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/

static ReferenceMessage_t reference_messages[] = {
    {
        .test_msg = {
            .type = MSG_TRANSMIT_FEEDBACK,
            .data = {0x12},
            .length_bits = 8 << 0,
            .data_type = BITS,
            .sender_id = 1,
            .eval_info = NULL
        }
    },
    {
        .test_msg = {
            .type = MSG_TRANSMIT_FEEDBACK,
            .data = {0x12, 0x34},
            .length_bits = 8 << 1,
            .data_type = BITS,
            .sender_id = 1,
            .eval_info = NULL
        }
    },
    {
        .test_msg = {
            .type = MSG_TRANSMIT_FEEDBACK,
            .data = {0x12, 0x34, 0x56, 0x78},
            .length_bits = 8 << 2,
            .data_type = BITS,
            .sender_id = 1,
            .eval_info = NULL
        }
    },
    {
        .test_msg = {
            .type = MSG_TRANSMIT_FEEDBACK,
            .data = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF},
            .length_bits = 8 << 3,
            .data_type = BITS,
            .sender_id = 1,
            .eval_info = NULL
        }
    },
    {
        .test_msg = {
            .type = MSG_TRANSMIT_FEEDBACK,
            .data = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
                     0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF},
            .length_bits = 8 << 4,
            .data_type = BITS,
            .sender_id = 1,
            .eval_info = NULL
        }
    },
    {
        .test_msg = {
            .type = MSG_TRANSMIT_FEEDBACK,
            .data = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
                     0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
                     0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
                     0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF},
            .length_bits = 8 << 5,
            .data_type = BITS,
            .sender_id = 1,
            .eval_info = NULL
        }
    },
    {
        .test_msg = {
            .type = MSG_TRANSMIT_FEEDBACK,
            .data = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
                     0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
                     0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
                     0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
                     0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
                     0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
                     0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
                     0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF},
            .length_bits = 8 << 6,
            .data_type = BITS,
            .sender_id = 1,
            .eval_info = NULL
        }
    },
    {
        .test_msg = {
            .type = MSG_TRANSMIT_FEEDBACK,
            .data = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
                     0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
                     0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
                     0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
                     0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
                     0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
                     0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
                     0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
                     0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
                     0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
                     0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
                     0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
                     0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
                     0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
                     0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
                     0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF},
            .length_bits = 8 << 7,
            .data_type = BITS,
            .sender_id = 1,
            .eval_info = NULL
        }
    }
};

static FeedbackTests_t feedback_tests[] = {
  // Base FH-BFSK test
    {
        .cfg = {
            .baud_rate = 100.0f,
            .mod_demod_method = MOD_DEMOD_FHBFSK,
            .fsk_f0 = 30000,
            .fsk_f1 = 33000,
            .fc = 31000,
            .fhbfsk_freq_spacing = 1,
            .fhbfsk_num_tones = 10,
            .fhbfsk_dwell_time = 1,
            .preamble_validation = CRC_8,
            .cargo_validation = CRC_16,
            .preamble_ecc_method = NO_ECC,
            .cargo_ecc_method = NO_ECC,
            .use_interleaver = false,
            .fhbfsk_hopper = HOPPER_INCREMENT,
            .sync_method = NO_SYNC
        },
        .expected_result = IDENTICAL,
        .reference_message = &reference_messages[MESSAGE_LEN_32],
        .errors_added = 0,
        .repetitions = 1
    },
    // Base FSK test
    {
        .cfg = {
            .baud_rate = 100.0f,
            .mod_demod_method = MOD_DEMOD_FSK,
            .fsk_f0 = 30000,
            .fsk_f1 = 33000,
            .fc = 31000,
            .fhbfsk_freq_spacing = 1,
            .fhbfsk_num_tones = 10,
            .fhbfsk_dwell_time = 1,
            .preamble_validation = CRC_8,
            .cargo_validation = CRC_16,
            .preamble_ecc_method = NO_ECC,
            .cargo_ecc_method = NO_ECC,
            .use_interleaver = false,
            .fhbfsk_hopper = HOPPER_INCREMENT,
            .sync_method = NO_SYNC
        },
        .expected_result = IDENTICAL,
        .reference_message = &reference_messages[MESSAGE_LEN_32],
        .errors_added = 0,
        .repetitions = 1
    },
    // Base Hamming code test
    {
        .cfg = {
            .baud_rate = 1000.0f,
            .mod_demod_method = MOD_DEMOD_FSK,
            .fsk_f0 = 30000,
            .fsk_f1 = 33000,
            .fc = 31000,
            .fhbfsk_freq_spacing = 1,
            .fhbfsk_num_tones = 10,
            .fhbfsk_dwell_time = 1,
            .preamble_validation = CRC_8,
            .cargo_validation = CRC_16,
            .preamble_ecc_method = HAMMING_CODE,
            .cargo_ecc_method = HAMMING_CODE,
            .use_interleaver = false,
            .fhbfsk_hopper = HOPPER_INCREMENT,
            .sync_method = NO_SYNC
        },
        .expected_result = IDENTICAL,
        .reference_message = &reference_messages[MESSAGE_LEN_128],
        .errors_added = 1,
        .repetitions = 20
    },
    // Base convolutional code test
    {
      .cfg = {
          .baud_rate = 1000.0f,
          .mod_demod_method = MOD_DEMOD_FSK,
          .fsk_f0 = 29000,
          .fsk_f1 = 33000,
          .fc = 31000,
          .fhbfsk_freq_spacing = 1,
          .fhbfsk_num_tones = 10,
          .fhbfsk_dwell_time = 1,
          .preamble_validation = CRC_8,
          .cargo_validation = CRC_16,
          .preamble_ecc_method = JANUS_CONVOLUTIONAL,
          .cargo_ecc_method = JANUS_CONVOLUTIONAL,
          .use_interleaver = false,
          .fhbfsk_hopper = HOPPER_INCREMENT,
          .sync_method = NO_SYNC
      },
      .expected_result = IDENTICAL,
      .reference_message = &reference_messages[MESSAGE_LEN_128],
      .errors_added = 2,
      .repetitions = 20
    },
    // Base interleaver test
    {
      .cfg = {
          .baud_rate = 1000.0f,
          .mod_demod_method = MOD_DEMOD_FSK,
          .fsk_f0 = 29000,
          .fsk_f1 = 33000,
          .fc = 31000,
          .fhbfsk_freq_spacing = 1,
          .fhbfsk_num_tones = 10,
          .fhbfsk_dwell_time = 1,
          .preamble_validation = CRC_8,
          .cargo_validation = CRC_16,
          .preamble_ecc_method = NO_ECC,
          .cargo_ecc_method = NO_ECC,
          .use_interleaver = true,
          .fhbfsk_hopper = HOPPER_INCREMENT,
          .sync_method = NO_SYNC
      },
      .expected_result = IDENTICAL,
      .reference_message = &reference_messages[MESSAGE_LEN_128],
      .errors_added = 0,
      .repetitions = 1
    },
    // Advanced interleaver/convolutional test (long)
    {
      .cfg = {
          .baud_rate = 1000.0f,
          .mod_demod_method = MOD_DEMOD_FSK,
          .fsk_f0 = 29000,
          .fsk_f1 = 33000,
          .fc = 31000,
          .fhbfsk_freq_spacing = 1,
          .fhbfsk_num_tones = 10,
          .fhbfsk_dwell_time = 1,
          .preamble_validation = CRC_8,
          .cargo_validation = CRC_16,
          .preamble_ecc_method = JANUS_CONVOLUTIONAL,
          .cargo_ecc_method = JANUS_CONVOLUTIONAL,
          .use_interleaver = true,
          .fhbfsk_hopper = HOPPER_INCREMENT,
          .sync_method = NO_SYNC
      },
      .expected_result = IDENTICAL,
      .reference_message = &reference_messages[MESSAGE_LEN_1024],
      .errors_added = 5,
      .repetitions = 1
    },
    // PN synchronization test
    {
      .cfg = {
          .baud_rate = 1000.0f,
          .mod_demod_method = MOD_DEMOD_FSK,
          .fsk_f0 = 29000,
          .fsk_f1 = 33000,
          .fc = 31000,
          .fhbfsk_freq_spacing = 1,
          .fhbfsk_num_tones = 10,
          .fhbfsk_dwell_time = 1,
          .preamble_validation = CRC_8,
          .cargo_validation = CRC_16,
          .preamble_ecc_method = NO_ECC,
          .cargo_ecc_method = NO_ECC,
          .use_interleaver = false,
          .fhbfsk_hopper = HOPPER_GALOIS,
          .sync_method = SYNC_PN_32_JANUS
      },
      .expected_result = IDENTICAL,
      .reference_message = &reference_messages[MESSAGE_LEN_128],
      .errors_added = 0,
      .repetitions = 1
    },
};

// change to be dependent so that errors can be injected multiple times for single case
static const uint16_t unique_tests = sizeof(feedback_tests) / sizeof(feedback_tests[0]);

static uint16_t total_tests;

static bool performing_test = false;
static uint16_t current_test = 0;
static uint16_t call_count = 0;
static LastAction_t last_action = DECODED_MESSAGE;

/* Private function prototypes -----------------------------------------------*/

static bool getTestIndex(uint16_t* index);
static bool compareHeaders(const Message_t* msg1, const Message_t* msg2, bool* identical);
static bool generateUniqueIndices(uint16_t* indices, uint16_t num_indices, uint16_t min_index, uint16_t max_index);
static bool testFailed(ExpectedResult_t expected_result, Message_t* msg, bool bit_error);
static void printStatistics(void);

/* Exported function definitions ---------------------------------------------*/

bool FeedbackTests_Init()
{
  total_tests = 0;

  for (uint8_t i = 0; i < unique_tests; i++) {
    uint16_t repetitions = feedback_tests[i].repetitions;
    uint16_t errors_added = feedback_tests[i].errors_added;

    // If there are no errors added then there is no point in having multiple repetitions
    if (errors_added == 0 && repetitions != 1) {
      return false;
    }

    total_tests += repetitions;
  }

  return true;
}

void FeedbackTests_Start()
{
  current_test = 0;
  performing_test = true;
  last_action = DECODED_MESSAGE;

  // Reset statistics
  for (uint16_t i = 0; i < unique_tests; i++) {
    feedback_tests[i].messages_with_errors_detected = 0;
    feedback_tests[i].messages_with_any_errors = 0;
    feedback_tests[i].messages_with_incorrect_length = 0;
    feedback_tests[i].messages_with_header_errors = 0;
    feedback_tests[i].failed_tests = 0;
  }
}

void FeedbackTests_GetNext()
{
  if (performing_test == false) {
    return;
  }

  if (current_test >= total_tests) {
    performing_test = false;
    return;
  }

  if (last_action == SENT_MESSAGE) {
    return;
  }

  uint16_t test_index;

  if (getTestIndex(&test_index) == false) {
    return;
  }

  MESS_AddMessageToTxQ(&feedback_tests[test_index].reference_message->test_msg);
  call_count++;
  last_action = SENT_MESSAGE;
}

bool FeedbackTests_CorruptMessage(BitMessage_t* bit_msg)
{
  if (bit_msg == NULL) {
    return false;
  }
  if (performing_test == false) {
    return true;
  }

  uint16_t test_index;

  if (getTestIndex(&test_index) == false) {
    return false;
  }

  uint16_t num_errors = feedback_tests[test_index].errors_added;

  if (num_errors == 0) {
    return true;
  }

  uint16_t error_indices[num_errors];

  if (generateUniqueIndices(error_indices, num_errors, 
      bit_msg->preamble.ecc_start_index, 
      bit_msg->preamble.ecc_start_index + bit_msg->final_length - 1) == false) {
    return false;
  }

  for (uint16_t i = 0; i < num_errors; i++) {
    if (Packet_FlipBit(bit_msg, error_indices[i]) == false) {
      return false;
    }
  }
  return true;
}

bool FeedbackTests_Check(Message_t* received_msg, BitMessage_t* received_bit_msg)
{
  if (performing_test == false) {
    return false;
  }

  uint16_t test_index;
  if (getTestIndex(&test_index) == false) {
    return false;
  }

  BitMessage_t bit_msg;

  if (Packet_PrepareTx(&feedback_tests[test_index].reference_message->test_msg,
      &bit_msg, &feedback_tests[test_index].cfg) == false) {
    return false;
  }

  // Compares the bit messages to see if the bits match up
  bool identical_bits;
  if (Packet_Compare(&bit_msg,
      received_bit_msg, &identical_bits) == false) {
    return false;
  }
  if (identical_bits == false) {
    feedback_tests[test_index].messages_with_any_errors++;
  }

  // Compare headers
  bool identical_headers;
  if (compareHeaders(&feedback_tests[test_index].reference_message->test_msg,
      received_msg, &identical_headers) == false) {
    return false;
  }
  if (identical_headers == false) {
    feedback_tests[test_index].messages_with_header_errors++;
  }

  if (received_msg->error_detected == true) {
    feedback_tests[test_index].messages_with_errors_detected++;
  }

  if (received_bit_msg->data_len_bits != bit_msg.data_len_bits) {
    feedback_tests[test_index].messages_with_incorrect_length++;
  }

  if (testFailed(feedback_tests[test_index].expected_result, received_msg,
      ! identical_bits) == true) {
    feedback_tests[test_index].failed_tests++;
  }

  // check if the message matches what was sent and print output

  char output_buffer[64];
  snprintf(output_buffer, 256, "\rCompleted Test %u/%u", current_test + 1,
      total_tests);
  COMM_TransmitData(output_buffer, CALC_LEN, COMM_USB);

  current_test++;
  last_action = DECODED_MESSAGE;

  if (current_test >= total_tests) {
    performing_test = false;

    printStatistics();
  }

  return true;
}

bool FeedbackTests_GetConfig(DspConfig_t** cfg)
{
  if (performing_test == false) {
    return false;
  }

  uint16_t test_index;
  if (getTestIndex(&test_index) == false) {
    return false;
  }

  CFG_IncrementVersionNumber();
  *cfg = &feedback_tests[test_index].cfg;
  return true;
}

/* Private function definitions ----------------------------------------------*/

bool getTestIndex(uint16_t* index)
{
  uint16_t counter = 0;
  for (uint16_t i = 0; i < unique_tests; i++)
  {
    counter += feedback_tests[i].repetitions;
    if (current_test <= (counter - 1)) {
      *index = i;
      return true;
    }
  }
  if (current_test >= total_tests) {
    return false;
  }
  else {
    *index = unique_tests - 1;
    return true;
  }
}

static bool compareHeaders(const Message_t* msg1, const Message_t* msg2, bool* identical)
{
  if (msg1 == NULL || msg2 == NULL || identical == NULL) {
    return false;
  }

  *identical = true;
  if (msg1->data_type != msg2->data_type) {
    *identical = false;
  }
  else if (msg1->sender_id != msg2->sender_id) {
    *identical = false;
  }
  else if (msg1->length_bits != msg2->length_bits) {
    *identical = false;
  }

  return true;
}

bool generateUniqueIndices(uint16_t* indices, uint16_t num_indices, uint16_t min_index, uint16_t max_index) {

  if (min_index > max_index || num_indices <= 0 || num_indices > 5) {
    return false;
  }

  uint16_t range_size = max_index - min_index + 1;

  // Ensure we don't try to generate more unique indices than the range size
  if (num_indices > range_size) {
    num_indices = range_size;
  }

  uint32_t current_time = osKernelGetTickCount();
  srand(current_time);

  for (uint16_t i = 0; i < num_indices; i++) {
    uint16_t index;
    bool is_unique;

    do {
      is_unique = true;
      index = min_index + (rand() % range_size);

      // Check if this index is already in our array
      for (uint16_t j = 0; j < i; j++) {
        if (indices[j] == index) {
          is_unique = false;
          break;
        }
      }
    } while (is_unique == false);

    indices[i] = index;
  }

  return true;
}

bool testFailed(ExpectedResult_t expected_result, Message_t* msg, bool bit_error)
{
  bool errors = msg->error_detected;

  switch (expected_result) {
    case IDENTICAL:
      return bit_error == true;
    case ERRORS:
      return errors != true;
    default:
      return true;
  }
}

static void printStatistics(void)
{
  char output_buffer[128];
  snprintf(output_buffer, 128, "\r\nTests completed!\r\nResults:\r\n");
  COMM_TransmitData(output_buffer, CALC_LEN, COMM_USB);

  for (uint16_t i = 0; i < unique_tests; i++) {
    snprintf(output_buffer, 128, "\r\nTest case %u:\r\n", i + 1);
    COMM_TransmitData(output_buffer, CALC_LEN, COMM_USB);

    if (feedback_tests[i].failed_tests == 0) {
      COMM_TransmitData("No failed tests!\r\n", CALC_LEN, COMM_USB);
    }

    const DspConfig_t* cfg = &feedback_tests[i].cfg;

    snprintf(output_buffer, 128, "Baud rate: %.2f\r\nMod/Demod method: %u\r\n"
        "Error detection method: %u %u\r\nError correction method: %u %u\r\n",
        cfg->baud_rate, cfg->mod_demod_method, cfg->preamble_validation, cfg->cargo_validation,
        cfg->preamble_ecc_method, cfg->cargo_ecc_method);
    COMM_TransmitData(output_buffer, CALC_LEN, COMM_USB);

    snprintf(output_buffer, 128, "interleaver: %u\r\n", cfg->use_interleaver);
    COMM_TransmitData(output_buffer, CALC_LEN, COMM_USB);

    if (cfg->mod_demod_method == MOD_DEMOD_FSK) {
      snprintf(output_buffer, 128, "f0: %lu\r\nf1: %lu\r\n", cfg->fsk_f0,
          cfg->fsk_f1);
      COMM_TransmitData(output_buffer, CALC_LEN, COMM_USB);
    }
    else if (cfg->mod_demod_method == MOD_DEMOD_FHBFSK) {
      snprintf(output_buffer, 128, "fc: %lu\r\nFrequency spacing: %hu\r\n"
          "Tones: %hu\r\nDwell: %hu\r\n", cfg->fc, cfg->fhbfsk_freq_spacing,
          cfg->fhbfsk_num_tones, cfg->fhbfsk_dwell_time);
      COMM_TransmitData(output_buffer, CALC_LEN, COMM_USB);
    }

    snprintf(output_buffer, 128, "Results %u:\r\n", i + 1);
    COMM_TransmitData(output_buffer, CALC_LEN, COMM_USB);

    uint16_t repetitions = feedback_tests[i].repetitions;

    snprintf(output_buffer, 128, "Failed tests: %u/%u\r\nMessages with errors: "
        "%u/%u\r\nMessages with errors detected: %u/%u\r\n",
        feedback_tests[i].failed_tests, repetitions,
        feedback_tests[i].messages_with_any_errors, repetitions,
        feedback_tests[i].messages_with_errors_detected, repetitions);
    COMM_TransmitData(output_buffer, CALC_LEN, COMM_USB);

    snprintf(output_buffer, 128, "Messages with header errors: %u/%u\r\n"
        "Messages with length errors: %u/%u\r\n\r\n",
        feedback_tests[i].messages_with_header_errors, repetitions,
        feedback_tests[i].messages_with_incorrect_length, repetitions);
    COMM_TransmitData(output_buffer, CALC_LEN, COMM_USB);
  }
}


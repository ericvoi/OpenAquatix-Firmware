/*
 * mess_main.h
 *
 *  Created on: Feb 12, 2025
 *      Author: ericv
 */

#ifndef MESS_MESS_MAIN_H_
#define MESS_MESS_MAIN_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
#include "FreeRTOS.h"
#include "queue.h"
#include <stdbool.h>


/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/

#define EVAL_MESSAGE_LENGTH               100

#define PACKET_SENDER_ID_BITS             4
#define PACKET_MESSAGE_TYPE_BITS          4
#define PACKET_LENGTH_BITS                3
#define PACKET_STATIONARY_BITS            1

#define PACKET_PREAMBLE_LENGTH_BITS       (PACKET_SENDER_ID_BITS + \
                                           PACKET_MESSAGE_TYPE_BITS + \
                                           PACKET_LENGTH_BITS + \
                                           PACKET_STATIONARY_BITS)

#define PACKET_DATA_MIN_LENGTH_BITS       (8 * 1)   // If the packet length is 0
#define PACKET_DATA_MAX_LENGTH_BITS       (8 * 128) // If the packet length is 7
#define PACKET_MAX_ERROR_DETECTION_BITS   32
// This is the number of bits in the *raw* message and is not to be used for
// any ecc operation
#define PACKET_MAX_LENGTH_BITS            (PACKET_PREAMBLE_LENGTH_BITS + \
                                           PACKET_DATA_MAX_LENGTH_BITS + \
                                           PACKET_MAX_ERROR_DETECTION_BITS)

// Since there are multiple instances of BitMessage_t (The only time where the
// packet max length in bytes is used), a static allocation was preferred over
// a dynamic one that takes into account the actual number of bytes needed.
// This method will tend to overallocate memory and could cause insufficient
// memory if stronger, non-linear ECC is added to the preamble. The use of a
// factor is based on the idea that convolutional codes will add the most 
// number of errors to a message and they add bits as a multiplicative factor
#define FACTOR_FOR_ECC                    2
// At most 8 bits are added to flush the encoder in the janus convolutional encoder
#define ADDED_ECC_BITS                    8

// TODO: Add a check to see if the number of bytes is sufficient
#define PACKET_MAX_LENGTH_BYTES           (((PACKET_MAX_LENGTH_BITS + \
                                          ADDED_ECC_BITS * 2 + 7) / 8) * \
                                          FACTOR_FOR_ECC)
// The data max length does not have ECC applied and is sanitized for a user
#define PACKET_DATA_MAX_LENGTH_BYTES      (PACKET_DATA_MAX_LENGTH_BITS / 8)

typedef enum {
  MSG_RECEIVED_TRANSDUCER,     // Received message from the transducer
  MSG_TRANSMIT_TRANSDUCER,     // Message needs to be transmitted via transducer
  MSG_RECEIVED_FEEDBACK,       // Received message from the feedback network
  MSG_TRANSMIT_FEEDBACK,       // Message needs to be transmitted via network
  MSG_ERROR                    // Error in message processing
} MessageType_t;

typedef enum {
  INTEGER,
  STRING,
  FLOAT,
  BITS,
  // Add new message data types here
  UNKNOWN,
  EVAL
} MessageData_t;

typedef struct {
  uint16_t len_bits; // length of evaluation message
  float bit_error_rate;
  uint8_t eval_msg;
  float energy_f0[EVAL_MESSAGE_LENGTH];
  float energy_f1[EVAL_MESSAGE_LENGTH];
  uint32_t f0[EVAL_MESSAGE_LENGTH];
  uint32_t f1[EVAL_MESSAGE_LENGTH];
} EvalMessageInfo_t;

typedef struct {
  MessageType_t type;
  uint8_t data[PACKET_DATA_MAX_LENGTH_BYTES];
  uint16_t length_bits;              // length of message in bits
  uint32_t timestamp;
  MessageData_t data_type;
  uint8_t sender_id;
  bool error_detected;
  EvalMessageInfo_t* eval_info;
} Message_t;

// defines the structure for analysis of the waveform
typedef struct {
  uint16_t start_index;
  uint16_t end_index;   // Will never exceed array length
  uint16_t bit_index;   // Index of the bit in the message
} ProcessingData_t;

typedef enum {
  DRIVING_TRANSDUCER,
  LISTENING,
  PROCESSING,
  CHANGING
} ProcessingState_t;

/* Exported constants --------------------------------------------------------*/

#define MSG_QUEUE_SIZE    4

#define DAC_CHANNEL_TRANSDUCER  DAC_CHANNEL_1
#define DAC_CHANNEL_FEEDBACK    DAC_CHANNEL_2


typedef enum {
  MESS_PRINT_REQUEST = 1 << 0,
  MESS_PRINT_COMPLETE = 1 << 1,
  MESS_FREQ_RESP = 1 << 2,
  MESS_PRINT_WAVEFORM = 1 << 3,
  MESS_FEEDBACK_TESTS = 1 << 4,
  MESS_DAC_READY = 1 << 5,
  MESS_INPUT_FFT = 1 << 6
} MessageFlags_t;

/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Main messaging system task for underwater acoustic modem operation
 *
 * Implements a state machine with three primary states:
 * - LISTENING: Waits for message detection or transmit requests
 * - DRIVING_TRANSDUCER: Actively transmitting via transducer
 * - PROCESSING: Decoding and processing received acoustic signals
 *
 * Handles initialization of analog components (PGA, ADC, DAC), parameter registration,
 * message modulation/demodulation, and error correction.
 *
 * @param argument Task argument (unused)
 *
 * @note This is a long-running RTOS task that never returns
 * @note Uses several hardware peripherals including ADC and PGA
 */
void MESS_StartTask(void* argument);

/**
 * @brief Initialize message transmission and reception queues
 *
 * Creates fixed-size FreeRTOS queues for handling message transfer between
 * the messaging system and other components.
 *
 * @warning Must be called before any queue operations are performed
 * @note Does not currently implement robust error handling for failed queue creation
 */
void MESS_InitializeQueues(void);

/**
 * @brief Retrieve a message from the transmission queue
 *
 * @param msg Pointer to Message_t structure where the retrieved message will be stored
 *
 * @return pdPASS if message was successfully retrieved, pdFAIL otherwise
 *
 * @note Non-blocking - returns immediately if no message is available
 */
BaseType_t MESS_GetMessageFromTxQ(Message_t* msg);

/**
 * @brief Add a message to the transmission queue
 *
 * @param msg Pointer to Message_t structure containing the message to transmit
 *
 * @return pdPASS if message was successfully added, pdFAIL otherwise
 *
 * @note Uses a timeout of 5 ticks when attempting to add to the queue
 */
BaseType_t MESS_AddMessageToTxQ(Message_t* msg);

/**
 * @brief Retrieve a message from the reception queue
 *
 * @param msg Pointer to Message_t structure where the retrieved message will be stored
 *
 * @return pdPASS if message was successfully retrieved, pdFAIL otherwise
 *
 * @note Non-blocking - returns immediately if no message is available
 */
BaseType_t MESS_GetMessageFromRxQ(Message_t* msg);

/**
 * @brief Add a message to the reception queue
 *
 * @param msg Pointer to Message_t structure containing the received message
 *
 * @return pdPASS if message was successfully added, pdFAIL otherwise
 *
 * @note Uses a timeout of 5 ticks when attempting to add to the queue
 */
BaseType_t MESS_AddMessageToRxQ(Message_t* msg);

/**
 * @brief Adjust baud rate to conform to hardware constraints
 *
 * Rounds the baud rate to ensure it aligns with DAC buffer size requirements.
 * This ensures that symbol lengths are compatible with the DAC's buffering system.
 *
 * @param baud Pointer to float containing baud rate to adjust (modified in-place)
 *
 * @note Ensures symbols have a duration that is a multiple of half the DAC buffer size
 */
void MESS_RoundBaud(float* baud);

/**
 * @brief Gets bandwidth
 *
 * @param bandwidth Pointer to uint32 containing bandwidth (modified)
 * @param lower_freq Pointer to uint32 containing lowest used frequency (modified)
 * @param upper_freq Pointer to uint32 containing highest used frequency (modified)
 *
 * @return true unless an internal parameter like frequency was set incorrectly
 */
bool MESS_GetBandwidth(uint32_t* bandwidth, uint32_t* lower_freq, uint32_t* upper_freq);

/**
 * @brief Gets the current bit period
 *
 * Returns the bit period in ms as a float
 *
 * @param bit_period_ms Pointer to float containing the bit period (modified)
 *
 * @return true always
 */
bool MESS_GetBitPeriod(float* bit_period_ms);

/**
 * @brief State of the MESS task
 *
 * @return DRIVING_TRANSDUCER Driving the output transducer
 * @return LISTENING Listening to the input for a message to start
 * @return PROCESSING Decoding ongoing message
 * @return CHANGING Changing between states
 */
ProcessingState_t MESS_GetState(void);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* MESS_MESS_MAIN_H_ */

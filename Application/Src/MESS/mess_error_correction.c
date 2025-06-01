/*
 * mess_error_correction.c
 *
 *  Created on: Apr 30, 2025
 *      Author: ericv
 */

 // Important note: All error correction codes must not require the input bit
 // sequence to be byte aligned.

/* Private includes ----------------------------------------------------------*/

#include "main.h"
#include "mess_error_correction.h"
#include "mess_packet.h"
#include "number_utils.h"
#include <stdbool.h>
#include <string.h>

/* Private typedef -----------------------------------------------------------*/

typedef struct {
  uint16_t register_state;
} ConvEncoder_t;

typedef struct {
  uint16_t section_length;
  uint16_t start_raw_index;
  uint16_t start_ecc_index;
  uint16_t raw_len;
  uint16_t ecc_len;
} SectionInfo_t;

/* Private define ------------------------------------------------------------*/

/* 1:2 convolutional encoder defines -----------------------------------------*/

// JANUS 1:2 convolutional encoder
// g1(x) = x^8 + x^7 + x^5 + x^3 + x^2 + x^1 + 1
// g2(x) = x^8 + x^4 + x^3 + x^2 + 1
#define CE_G1_JANUS             0x01AF
#define CE_G2_JANUS             0x011D

#define JANUS_FLUSH_LENGTH      8
#define JANUS_CONSTRAINT_LENGTH 9
#define JANUS_NUM_STATES        (1 << (JANUS_CONSTRAINT_LENGTH - 1)) // 256 states
#define JANUS_TRACEBACK_LENGTH  (JANUS_CONSTRAINT_LENGTH * 5)
#define JANUS_MAX_METRIC        (100000.0f)
#define JANUS_DECISION_LAG      (JANUS_CONSTRAINT_LENGTH * 3)

#define JANUS_SOFT_DECISION     (false) // true to use soft decision and false otherwise

typedef struct {
  float path_metrics[JANUS_NUM_STATES];                         // Current path metrics
  float next_path_metrics[JANUS_NUM_STATES];                    // Next iteration path metrics
  uint16_t traceback[JANUS_TRACEBACK_LENGTH][JANUS_NUM_STATES]; // Survivor paths
  uint16_t traceback_index;                                     // Current position in traceback array
  float error_metric;                                           // Final error metric after decoding
} JanusVitrebiDecoder_t;

/* Private macro -------------------------------------------------------------*/

#define MIN(x, y) ((x < y) ? (x) : (y))

/* Private variables ---------------------------------------------------------*/

static uint8_t message_buffer[PACKET_MAX_LENGTH_BYTES] = {0};
static JanusVitrebiDecoder_t janus_decoder;

/* Private function prototypes -----------------------------------------------*/

static bool addHamming(BitMessage_t* bit_msg, bool is_preamble, uint16_t* bits_added, const DspConfig_t* cfg);
static bool decodeHamming(BitMessage_t* bit_msg, bool is_preamble, bool* error_detected, bool* error_corrected, const DspConfig_t* cfg);

static bool addJanusConvolutional(BitMessage_t* bit_msg, bool is_preamble, uint16_t* bits_added, const DspConfig_t* cfg);
static bool decodeJanusConvolutional(BitMessage_t* bit_msg, bool is_preamble, bool* error_detected, bool* error_corrected, const DspConfig_t* cfg);

// Hamming functions
static uint16_t calculateNumParityBits(const uint16_t num_bits);

// JANUS 1:2 convolutional encoder functions
static void janusConvEncoderInit(ConvEncoder_t* encoder);
static void janusConvEncodeBit(ConvEncoder_t* encoder, bool input_bit, bool output_bits[2]);
static void janusVitrebiInit(JanusVitrebiDecoder_t* decoder);
static float janusComputeBranchMetric(bool received_bit1, bool received_bit2,
                                      bool expected_bit1, bool expected_bit2);
static void janusCalculateOutput(uint16_t state, bool input_bit, bool output[2]);
static void janusVitrebiDecodePair(JanusVitrebiDecoder_t* decoder, bool received_bit1, bool received_bit2, bool is_flush_bit);
static bool janusVitrebiTraceback(JanusVitrebiDecoder_t* decoder, bool* bit, uint16_t bit_index);
static uint16_t janusVitrebiFindBestState(JanusVitrebiDecoder_t* decoder);

// General helper functions
static void clearBuffer(void);
static bool setBitInBuffer(bool bit, uint16_t position);
static bool getBitFromBuffer(uint16_t position, bool* bit);
static void copyBufferToMessage(BitMessage_t* bit_msg, uint16_t new_len);
static void calculateSectionInfo(SectionInfo_t* section_info, const BitMessage_t* bit_msg, const DspConfig_t* cfg, bool is_preamble);

/* Exported function definitions ---------------------------------------------*/

bool ErrorCorrection_AddCorrection(BitMessage_t* bit_msg, 
                                   const DspConfig_t* cfg)
{
  uint16_t bits_added = 0;
  clearBuffer();
  switch (cfg->ecc_method_preamble) {
    case NO_ECC:
      return true;
    case HAMMING_CODE:
      if (addHamming(bit_msg, true, &bits_added, cfg) == false) {
        return false;
      }
      break;
    case JANUS_CONVOLUTIONAL:
      if (addJanusConvolutional(bit_msg, true, &bits_added, cfg) == false) {
        return false;
      }
      break;
    default:
      return false;
  }

  switch (cfg->ecc_method_message) {
    case NO_ECC:
      return true;
    case HAMMING_CODE:
      if (addHamming(bit_msg, false, &bits_added, cfg) == false) {
        return false;
      }
      break;
    case JANUS_CONVOLUTIONAL:
      if (addJanusConvolutional(bit_msg, false, &bits_added, cfg) == false) {
        return false;
      }
      break;
    default:
      return false;
  }
  copyBufferToMessage(bit_msg, bits_added);
  return true;
}

bool ErrorCorrection_CheckCorrection(BitMessage_t* bit_msg, 
                                     const DspConfig_t* cfg, 
                                     bool is_preamble, 
                                     bool* error_detected, 
                                     bool* error_corrected)
{
  *error_detected = false;
  *error_corrected = false;
  bit_msg->combined_message_len = bit_msg->non_preamble_length +
        PACKET_PREAMBLE_LENGTH_BITS;
  if (is_preamble == true) {
    SectionInfo_t section_info;
    calculateSectionInfo(&section_info, bit_msg, cfg, is_preamble);
    bit_msg->final_length = section_info.ecc_len;
  }
  if (is_preamble) {
    switch (cfg->ecc_method_preamble) {
      case NO_ECC:
        *error_detected = false;
        *error_corrected = false;
        return true;
      case HAMMING_CODE:
        return decodeHamming(bit_msg, true, error_detected, error_corrected,
                             cfg);
      case JANUS_CONVOLUTIONAL:
        return decodeJanusConvolutional(bit_msg, true, error_detected,
                                        error_corrected, cfg);
      default:
        return false;
    }
  }
  else {
    switch (cfg->ecc_method_message) {
      case NO_ECC:
        *error_detected = false;
        *error_corrected = false;
        return true;
      case HAMMING_CODE:
        return decodeHamming(bit_msg, false, error_detected, error_corrected,
                             cfg);
      case JANUS_CONVOLUTIONAL:
        return decodeJanusConvolutional(bit_msg, false, error_detected,
                                        error_corrected, cfg);
      default:
        return false;
    }
  }
}

uint16_t ErrorCorrection_GetLength(const uint16_t length, 
                                   const ErrorCorrectionMethod_t method)
{
  switch (method) {
    case NO_ECC:
      return length;
    case HAMMING_CODE:
      return calculateNumParityBits(length) + length;
    case JANUS_CONVOLUTIONAL:
      return 2 * (length + JANUS_FLUSH_LENGTH);
    default:
      return 0;
  }
}

/* Private function definitions ----------------------------------------------*/

/*
 * Hamming codes work by placing parity bits at bit positions that are powers
 * of 2. The number of parity bits for a given number of message bits is given
 * by 2^parity_bits >= data_bits + parity_bits + 1. The bit positions that are 
 * checked by this parity bit are all the bits where this bit is set. For 
 * example, parity bit 3 at 0x04 or 0b00000100 checks all bit posiitons 
 * following the convention 0bxxxxx1xx. Bit position indices include parity
 * bits
 * 
 * Example bit sequence: 0100
 * Data bit indices are (3 5 6 7)
 * Requires 3 parity bits since 2^3 = 8 = 4 + 3 + 1
 * Note: bit positions are 1-indexed
 * p1 (bit position 1): 1*0 + 1*1 + 0*0 + 1*0 = 1
 * p2 (bit position 2): 1*0 + 0*1 + 1*0 + 1*0 = 0
 * p3 (bit position 4): 0*0 + 1*1 + 1*0 + 1*0 = 1
 * 
 * Encoded message: 1001100
 */
bool addHamming(BitMessage_t* bit_msg, 
                bool is_preamble, 
                uint16_t* bits_added, 
                const DspConfig_t* cfg)
{
  SectionInfo_t section_info;
  calculateSectionInfo(&section_info, bit_msg, cfg, is_preamble);
  
  uint16_t parity_bits = calculateNumParityBits(section_info.section_length);

  uint16_t final_length = section_info.section_length + parity_bits;
  uint16_t message_bits_added = 0;
  // Add message bits to mesage
  for (uint16_t i = 0; i < final_length; i++) {
    if (NumberUtils_IsPowerOf2(i + 1) == true) {
      continue;
    }
    bool bit_to_add;
    if (Packet_GetBit(bit_msg, section_info.start_raw_index + message_bits_added++, 
                      &bit_to_add) == false) {
      return false;
    }
    if (setBitInBuffer(bit_to_add, section_info.start_ecc_index + i) == false) {
      return false;
    }
    if (message_bits_added >= section_info.section_length) break;
  }
  if (message_bits_added != section_info.section_length) {
    return false;
  }

  // Add parity bits at indices that are powers of 2 offset from the start index
  for (uint16_t p = 0; p < parity_bits; p++) {
    uint16_t parity_pos = (1 << p) - 1;
    bool parity = false;

    for (uint16_t i = parity_pos; i < final_length; i++) {
      if ((i + 1) & (1 << p)) {
        bool bit;
        if (getBitFromBuffer(i + section_info.start_ecc_index, &bit) == false) {
          return false;
        }
        parity ^= bit;
      }
    }

    if (setBitInBuffer(parity, parity_pos + section_info.start_ecc_index) == false) {
      return false;
    }
  }


  *bits_added += final_length;
  return true;
}

/*
 * Decosing a hamming code message works by checking to see if each parity
 * condition in the received message still holds. If a parity condition does
 * not hold then it is known that there is a bit error at a position where the
 * bit index corresponding to the parity bit checked is set. For example, in a
 * message where there is one bit error and the first parity check on parity
 * bit 1 (bit position 0x01) fails, then the bit error is at an odd location.
 * 
 * Following the encoding example, 1001100 -> 1001101 (1 bit error at position 7)
 * New parity bits (first bit is the extra parity bit):
 * p1 (bit position 1 checking 1, 3, 5, 7): 1 + 0 + 1 + 1= 1
 * p2 (bit position 2 checking 2, 3, 6, 7): 0 + 0 + 0 + 1= 1
 * p3 (bit position 4 checking 4, 5, 6, 7): 0 + 1 + 1 + 1= 1
 * 
 * Parity check fails for all parity bits meaning that the bit error occurs at
 * a position that satisfies xx1, x1x, and 1xx. This is 111 or bit position 7,
 * precisely where the error was!
 */
bool decodeHamming(BitMessage_t* bit_msg, 
                   bool is_preamble, 
                   bool* error_detected, 
                   bool* error_corrected,
                   const DspConfig_t* cfg)
{
  (void) (error_detected);
  SectionInfo_t section_info;
  calculateSectionInfo(&section_info, bit_msg, cfg, is_preamble);

  uint16_t syndrome = 0;
  uint16_t parity_bits = 0;
  while ((1 << parity_bits) < section_info.ecc_len) {
    uint16_t p = parity_bits;
    bool parity = false;

    for (uint16_t i = 0; i < section_info.ecc_len; i++) {
      if ((i + 1) & (1 << p)) {
        bool bit;
        if (Packet_GetBit(bit_msg, i + section_info.start_ecc_index, &bit) == false) {
          return false;
        }
        parity ^= bit;
      }
    }
    if (parity != 0) {
      syndrome |= (1 << p);
    }

    parity_bits++;
  }

  if (syndrome != 0 && syndrome < section_info.ecc_len) {
    if (Packet_FlipBit(bit_msg, section_info.start_ecc_index + syndrome - 1) == false) {
      return false;
    }
    *error_corrected = true;
  }

  uint16_t decoded_pos = 0;
  for (uint16_t i = 0; i < section_info.ecc_len; i++) {
    if (NumberUtils_IsPowerOf2(i + 1) == false) {
      bool bit;
      if (Packet_GetBit(bit_msg, i + section_info.start_ecc_index, &bit) == false) {
        return false;
      }
      if (Packet_SetBit(bit_msg, decoded_pos + section_info.start_raw_index, bit) == false) {
        return false;
      }
      decoded_pos++;
      if (decoded_pos >= section_info.raw_len) break;
    }
  }

  return true;
}

/*
 * This convolutional code follows the code set out by the JANUS standard in
 * ANEP-87 (https://nso.nato.int/nso/nsdd/main/standards?search=ANEP-87)
 * 
 * Creating a convolutional code is relatively straightforward and, in essence,
 * is simply outputting parity bits that correspond to a message. The number of
 * parity bits outputted by the encoder is always greater than the number of
 * input bits and JANUS uses a 1:2 encoder so 2 bits are outputted for every
 * input bit. These two output bits are parity bits corresponding to the input
 * bit stream using the polynomials g1 and g2 that define which bits in the
 * previous K number of bits are used to determine parity. K is the constraint
 * length and defines how many bits to look back on. For example, a polynomial
 * 0b00001011 has a constraint length of 4.
 * 
 * Take the message 1101 (MSB first) with polynomials 1101 and 1001
 * Bit 0 (register state: 1)
 * g1(0) = 1*0 + 1*0 + 0*0 + 1*1 = 1
 * g2(0) = 1*0 + 0*0 + 0*0 + 1*1 = 1
 * Bit 1 (register state: 11)
 * g1(1) = 1*0 + 1*0 + 0*1 + 1*1 = 1
 * g2(1) = 1*0 + 0*0 + 0*1 + 1*1 = 1
 * Bit 2 (register state: 110)
 * g1(2) = 1*0 + 1*1 + 0*1 + 1*0 = 1
 * g2(2) = 1*0 + 0*1 + 0*1 + 1*0 = 0
 * Bit 3 (register state: 1101)
 * g1(3) = 1*1 + 1*1 + 0*0 + 1*1 = 1
 * g2(3) = 1*1 + 0*1 + 0*0 + 1*1 = 0
 * 
 * So the output message is 11 11 10 10 (not including any flush bits)
 * 
 * A common method to increase reliability is to flush the encoder by adding
 * zeroes to the end of the data to flush the encoder and make the last bits
 * impact more bits as otherwise there decoding reliability goes down. JANUS
 * uses 8 flush bits (K - 1). Note: in the standard, they say that the zeroes
 * are prepended but this means that they are added before encoding *not* that
 * the zeroes are added to the start of the data stream.
 */
bool addJanusConvolutional(BitMessage_t* bit_msg, 
                           bool is_preamble, 
                           uint16_t* bits_added, 
                           const DspConfig_t* cfg)
{
  ConvEncoder_t encoder;
  janusConvEncoderInit(&encoder);
  SectionInfo_t section_info;
  calculateSectionInfo(&section_info, bit_msg, cfg, is_preamble);

  bool output_bits[2];
  uint16_t output_index = section_info.start_ecc_index;

  // First add the actual message bits
  for (uint16_t i = 0; i < section_info.section_length; i++) {
    bool input_bit;
    if (Packet_GetBit(bit_msg, i + section_info.start_raw_index, &input_bit) == false) {
      return false;
    }

    janusConvEncodeBit(&encoder, input_bit, output_bits);

    if (setBitInBuffer(output_bits[0], output_index++) == false) {
      return false;
    }
    if (setBitInBuffer(output_bits[1], output_index++) == false) {
      return false;
    }
  }

  // Then flush the encoder
  for (uint16_t i = 0; i < JANUS_FLUSH_LENGTH; i++) {
      janusConvEncodeBit(&encoder, false, output_bits);
  
      if (setBitInBuffer(output_bits[0], output_index++) == false) {
        return false;
      }
      if (setBitInBuffer(output_bits[1], output_index++) == false) {
        return false;
      }
    }
  *bits_added += output_index;
  return true;
}

/*
 * Decoding a convolutionally encoded message uses a highest likelihood method
 * to determine the most likley bit sequence corresponding to a received 
 * bit stream. For each pair of input bits, two possible pairs are computed:
 * one for if the latest bit corresponding to the pair is a 0 and another for 
 * the 1 case. The Hamming distance between the received pair and the expected
 * pair (if the bit was a 1 or 0 given previous state history). This is
 * repeated for every previous state (if it has a valid path to it) and an
 * accumulated error metric is tracked. Bits are decoded by tracking a history
 * of 45 (K*5) received bit pairs. When a bit needs to be decoded (bit pair
 * history is full) a traceback occurs where the lowest accumulated error
 * metric path is traced back to find the bit desired.
 * 
 * A special case is applied to the final 16 received bits as these are known
 * to correspond to 0 bits. The decoding process for these bits proceeds as if
 * only a 0 was sent. There are other methods for this, but this one was 
 * selected due to its simplicity. JANUS does not specify a method to decode
 * the final bits and an evaluate of BER vs noise should be done to determine
 * the optimal way to deal with the final flush bits. An alternative method is
 * to determine the encoder output for each of the 256 states after being
 * flushed with 8 bits and then determine the number of bit errors. The
 * encoder output can then be compared to the final 16 bits received to
 * determine an error metric. This can then be used in conjunction with each
 * of the path metrics to determine the best path. 
 */
bool decodeJanusConvolutional(BitMessage_t* bit_msg, 
                              bool is_preamble, 
                              bool* error_detected, 
                              bool* error_corrected,
                              const DspConfig_t* cfg)
{
  SectionInfo_t section_info;
  calculateSectionInfo(&section_info, bit_msg, cfg, is_preamble);
  janusVitrebiInit(&janus_decoder);

  uint16_t output_bit_index = 0;

  // Decode all received bits and decide bits that we have enough information for
  for (uint16_t i = 0; i < section_info.ecc_len; i += 2) {
    // Flush bits are handled by forcing the decoding process to only use a 0
    bool is_flush_bit = i >= (section_info.ecc_len - 2 * JANUS_FLUSH_LENGTH);
    uint16_t bit_position = section_info.start_ecc_index + i;
    bool bit1, bit2;
    if (Packet_GetBit(bit_msg, bit_position, &bit1) == false) {
      return false;
    }
    if (Packet_GetBit(bit_msg, bit_position + 1, &bit2) == false) {
      return false;
    }

    janusVitrebiDecodePair(&janus_decoder, bit1, bit2, is_flush_bit);

    // Only decode bits if we absolutely have to. Waiting any longer would
    // result in the history buffer being overwritten
    if (janus_decoder.traceback_index >= JANUS_TRACEBACK_LENGTH) {
      bool bit;
      if (janusVitrebiTraceback(&janus_decoder, &bit, output_bit_index) == false) {
        return false;
      }
      if (Packet_SetBit(bit_msg, section_info.start_raw_index + output_bit_index, bit) == false) {
        return false;
      }
      output_bit_index++;
    }
  } 

  // There are no more input bits so no more information can be gathered about
  // the sequence.
  uint16_t remaining_bits = MIN(section_info.raw_len, JANUS_TRACEBACK_LENGTH - 1);
  for (uint16_t i = 0; i < remaining_bits; i++) {
    bool bit;
    if (janusVitrebiTraceback(&janus_decoder, &bit, output_bit_index) == false) {
      return false;
    }
    if (Packet_SetBit(bit_msg, section_info.start_raw_index + output_bit_index, bit) == false) {
      return false;
    }
    output_bit_index++;
  }

  bit_msg->normalized_vitrebi_error_metric =
      (float) janus_decoder.error_metric / (section_info.ecc_len / 2.0f);

  *error_detected = bit_msg->normalized_vitrebi_error_metric != 0.0f;
  *error_corrected = *error_detected;

  return true;
}

uint16_t calculateNumParityBits(const uint16_t num_bits)
{
  uint16_t parity_bits = 0;

  while ((1 << parity_bits) < num_bits + parity_bits + 1) {
    parity_bits++;
  }
  return parity_bits;
}

void janusConvEncoderInit(ConvEncoder_t* encoder)
{
  encoder->register_state = 0;
}

void janusConvEncodeBit(ConvEncoder_t* encoder, bool input_bit, bool output_bits[2])
{
  // Latest K (9) bits
  encoder->register_state =
      ((encoder->register_state << 1) | (input_bit & 0x0001)) & 0x01FF;
  uint16_t state = encoder->register_state;

  output_bits[0] = __builtin_parity(state & CE_G1_JANUS);
  output_bits[1] = __builtin_parity(state & CE_G2_JANUS);
}

void janusVitrebiInit(JanusVitrebiDecoder_t* decoder)
{
  memset(decoder, 0, sizeof(JanusVitrebiDecoder_t));

  decoder->path_metrics[0] = 0.0f;
  for (uint16_t i = 1; i < JANUS_NUM_STATES; i++) {
    decoder->path_metrics[i] = JANUS_MAX_METRIC;
  }

  for (uint16_t i = 0; i < JANUS_NUM_STATES; i++) {
    decoder->next_path_metrics[i] = JANUS_MAX_METRIC;
  }

  decoder->traceback_index = 0;
  decoder->error_metric = 0.0f;
}

float janusComputeBranchMetric(bool received_bit1, bool received_bit2,
                               bool expected_bit1, bool expected_bit2)
{
  // Soft decision vitrebi traceback uses a cotninuous range to decide how
  // far off the branch is from actual. The continuous range is ususally a
  // 4-byte float, but a 1- or 2-byte value could be used.
  // Keeping the energies of each frequency for each bit would use 4 bytes
  // of ram per bit which is acceptable with the current message lengths of
  // ~ 2,000 but this could be a large hassle for longer message types. To
  // alleviate this issue, a history of received bit energies could be kept
  // that is say the most recent 10 decoded bit energies. The soft decision
  // function could then call a function that would return the energy in the
  // history array corresponding to the bit position. Considering how the
  // MESS task is such a high priority task that is run often, this could
  // be viable as the current maximum baud rate of 1000 gives 10 ms with
  // an energy history of 10. TODO (investigate soft decision vitrebi)
  #if JANUS_SOFT_DECISION
    return (received_bit1 - expected_bit1) * (received_bit1 - expected_bit1) + 
           (received_bit2 - expected_bit2) * (received_bit2 - expected_bit2);
  #else /* JANUS_SOFT_DECISION */
    return (float) (received_bit1 != expected_bit1) + (received_bit2 != expected_bit2);
  #endif /* JANUS_SOFT_DECISION*/
}

void janusCalculateOutput(uint16_t state, bool input_bit, bool output[2])
{
  uint16_t full_state = ((state << 1) | input_bit) & 0x01FF;

  output[0] = __builtin_parity(full_state & CE_G1_JANUS);
  output[1] = __builtin_parity(full_state & CE_G2_JANUS);
}

void janusVitrebiDecodePair(JanusVitrebiDecoder_t* decoder,
                            bool received_bit1,
                            bool received_bit2,
                            bool is_flush_bit)
{
  // Prevents paths with no path to them from being reached
  for (uint16_t i = 0; i < JANUS_NUM_STATES; i++) {
    decoder->next_path_metrics[i] = JANUS_MAX_METRIC;
  }

  uint16_t tb_index = decoder->traceback_index % JANUS_TRACEBACK_LENGTH;

  for (uint16_t current_state = 0; current_state < JANUS_NUM_STATES; current_state++) {
    // States with infinite metrics are ignored since they dont have a path to them
    if (decoder->path_metrics[current_state] >= JANUS_MAX_METRIC) continue;

    // Calculate branch metrics as if actual bit was each of its possibilities
    uint8_t max_bit = is_flush_bit ? 0 : 1;
    for (uint8_t input_bit = 0; input_bit <= max_bit; input_bit++) {
      bool expected_output[2];
      janusCalculateOutput(current_state, input_bit, expected_output);

      float branch_metric = janusComputeBranchMetric(
        received_bit1,      received_bit2,
        expected_output[0], expected_output[1]
      );

      uint16_t next_state = ((current_state << 1) | input_bit) & 0x00FF;

      float new_path_metric = decoder->path_metrics[current_state] + branch_metric;

      if (new_path_metric < decoder->next_path_metrics[next_state]) {
        decoder->next_path_metrics[next_state] = new_path_metric;
        decoder->traceback[tb_index][next_state] = current_state;
      }
    }
  }

  decoder->traceback_index++;
  memcpy(decoder->path_metrics, decoder->next_path_metrics, sizeof(decoder->path_metrics));
}

// TODO: optimize for repeated tracebacks with the same path
bool janusVitrebiTraceback(JanusVitrebiDecoder_t* decoder, bool* bit, uint16_t bit_index)
{
  if (bit_index >= decoder->traceback_index) {
    return false;
  }
  uint16_t current_state = janusVitrebiFindBestState(decoder);

  uint16_t available_symbols = decoder->traceback_index - bit_index - 1;

  for (uint16_t i = 0; i < available_symbols; i++) {
    uint16_t tb_index = (decoder->traceback_index - 1 - i) % JANUS_TRACEBACK_LENGTH;
    // The traceback points to to the previous state
    current_state = decoder->traceback[tb_index][current_state];
  }

  *bit = current_state & 0x0001;
  return true;
}

static uint16_t janusVitrebiFindBestState(JanusVitrebiDecoder_t* decoder)
{
  uint16_t best_state = 0;
  float min_metric = decoder->path_metrics[0];

  for (uint16_t i = 1; i < JANUS_NUM_STATES; i++) {
    if (decoder->path_metrics[i] < min_metric) {
      min_metric = decoder->path_metrics[i];
      best_state = i;
    }
  }
  return best_state;
}

void clearBuffer(void)
{
  memset(message_buffer, 0, sizeof(message_buffer) / sizeof(message_buffer[0]));
}

bool setBitInBuffer(bool bit, uint16_t position)
{
  if (position >= sizeof(message_buffer) * 8) {
    return false;
  }

  uint16_t byte_index = position / 8;
  uint8_t bit_position = position % 8;

  if (bit == true) {
    message_buffer[byte_index] |= (1 << (7 - bit_position));
  } else {
    message_buffer[byte_index] &= ~(1 << (7 - bit_position));
  }
  return true;
}

bool getBitFromBuffer(uint16_t position, bool* bit)
{
  if (position >= sizeof(message_buffer) * 8) {
    return false;
  }

  uint16_t byte_index = position / 8;
  uint8_t bit_position = position % 8;

  *bit = (message_buffer[byte_index] & (1 << (7 - bit_position))) != 0;

  return true;
}

void copyBufferToMessage(BitMessage_t* bit_msg, uint16_t new_len)
{
  memcpy(bit_msg->data, message_buffer, sizeof(message_buffer) / sizeof(message_buffer[0]));
  bit_msg->bit_count = new_len;
  bit_msg->final_length = new_len;
}

static void calculateSectionInfo(SectionInfo_t* section_info,
                                 const BitMessage_t* bit_msg,
                                 const DspConfig_t* cfg,
                                 bool is_preamble)
{
  section_info->section_length = is_preamble ? PACKET_PREAMBLE_LENGTH_BITS :
      (bit_msg->final_length - PACKET_PREAMBLE_LENGTH_BITS);
  section_info->start_raw_index = is_preamble ? 0 : PACKET_PREAMBLE_LENGTH_BITS;
  section_info->start_ecc_index = is_preamble ? 0 : 
      ErrorCorrection_GetLength(section_info->start_raw_index, cfg->ecc_method_preamble);
  section_info->raw_len = is_preamble ? PACKET_PREAMBLE_LENGTH_BITS :
      (bit_msg->non_preamble_length);
  section_info->ecc_len = ErrorCorrection_GetLength(section_info->raw_len,
      is_preamble ? cfg->ecc_method_preamble : cfg->ecc_method_message);
}

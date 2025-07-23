// sys_INA219.c


/* Private includes ----------------------------------------------------------*/
#include "INA219-driver.h"
#include "sys_INA219.h"
#include <stdbool.h>
#include <stdint.h>

/* Private typedef -----------------------------------------------------------*/

// Structure to hold each power reading and matching timestamp
typedef struct { 
    uint32_t timestamp; // Timestamp in milliseconds
    float power; // Power reading in watts
} PowerReading_t;

/* Private define ------------------------------------------------------------*/

#define power_buffer_size 50 // Size of the power buffer for averaging

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static uint16_t raw_power; // Raw reading of a power register value from INA219.
static PowerReading_t power_buffer[power_buffer_size]; // Buffer to hold power readings and timestamps
static float INA219_POWER_LSB = 20 * INA219_CURRENT_LSB; // The Power LSB is 20 times the current LSB

static volatile uint8_t buffer_index = 0;
static volatile uint32_t ms_counter = 0;
static volatile bool reading_in_progress = false;

/* External variables --------------------------------------------------------*/
extern I2C_HandleTypeDef hi2c1; // External i2c handle

/* Private function prototypes -----------------------------------------------*/
static void StartPowerReading(void);


/* Exported function definitions ---------------------------------------------*/
bool INA219_System_Init(void)
{
    // Initialize INA219
    HAL_StatusTypeDef status;
    do {
        status = INA219_Init();
        if (status != HAL_OK) {
            return false;
        }
    } while (status == HAL_BUSY); // Keep trying until successful

    // Clear the power buffer for fresh readings
    memset(power_buffer, 0, sizeof(power_buffer));
    buffer_index = 0;
    ms_counter = 0;

    return true;
}

// Called from timer interrupt (sys_sensor_timer.c)
void INA219_Timer_Callback(void)
{
    ms_counter++;

    // Start new reading of not in progress
    if (!reading_in_progress) {
        StartPowerReading();
    }
}

// Callback for completed DMA transfer
void INA219_ReadComplete_Callback(bool success)
{
    if (success) {
        // Byte swap, because INA219 sends in big-endian format
        raw_power = __REV16(power_buffer[buffer_index].power); 
        power_buffer[buffer_index].power = raw_power * INA219_POWER_LSB; // Convert raw value to power in watts
        power_buffer[buffer_index].timestamp = ms_counter; // Store the timestamp of the reading
        
        buffer_index = (buffer_index + 1) % power_buffer_size; // Circular buffer
    }
    reading_in_progress = false; // Reading is complete
}

// Calculates the recent average of power readings, up to the size of the buffer
float Power_GetRecentAverage(uint8_t numsamples)
{
    if (numsamples == 0 || numsamples > power_buffer_size) {
        return 0.0f; // Invalid sample size. Add error handling later
    }

    float sum = 0.0f;
    uint8_t count = 0;
    int16_t start_index;

    // Find starting index, based on where the last reading was stored.
    if (buffer_index >= numsamples) { // If the index is ahead of the amount of requested samples
        start_index = buffer_index - numsamples; // Start from the last reading minus the number of samples
    } else { // Else need to wrap back around the buffer
        start_index = power_buffer_size - (numsamples - buffer_index);
    }

    // Sum recent samples
    for (uint8_t i=0; i < numsamples; i++) {
        uint8_t index = (start_index + i) % power_buffer_size;
        // Only include non-zero readings to handle initialization period
        if (power_buffer[index].power != 0.0f) {
            sum += power_buffer[index].power;
            count++;
        }
    }
    // Return the average in watts as long as there is at least one valid reading. Otherwise, return 0.
    return (count > 0) ? (sum / count) : 0.0f; 

}

/* Private function definitions ----------------------------------------------*/

static void StartPowerReading(void)
{
    // Start power reading
    reading_in_progress = true;
    if (HAL_I2C_Mem_Read_DMA(&hi2c1, 
                             INA219_ADDRESS << 1,
                             POWER_ADDRESS,
                             I2C_MEMADD_SIZE_8BIT,
                             (uint8_t*)&power_buffer[buffer_index].power,
                            sizeof(uint16_t)) != HAL_OK) {
        reading_in_progress = false;
    } 
}

// This function is called when the DMA transfer is complete
void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
  if (hi2c->Instance == hi2c1.Instance) {
    INA219_ReadComplete_Callback(true);
  }
}


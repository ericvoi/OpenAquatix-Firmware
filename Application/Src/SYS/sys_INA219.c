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
    uint16_t power; // Power reading in watts
} PowerReading_t;

/* Private define ------------------------------------------------------------*/

#define power_buffer_size 40 // Size of the power buffer for averaging

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static PowerReading_t power_buffer[power_buffer_size]; // Buffer to hold power readings
static uint8_t buffer_index = 0;
static uint32_t ms_counter = 0;
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
        power_buffer[buffer_index].timestamp = ms_counter;
        
        buffer_index = (buffer_index + 1) % power_buffer_size; // Circular buffer
    }
    reading_in_progress = false; // Reading is complete
}

/* Private function definitions ----------------------------------------------*/

static void StartPowerReading(void)
{
    // Start power reading
    reading_in_progress = true;
    if (HAL_I2C_Mem_Read_DMA(&hi2c1, 
                             INA219_ADDRESS << 1,
                             POWER_ADDRESS,
                             I2C_MEMADD_SIZE_16BIT,
                             (uint8_t*)&power_buffer[buffer_index].power,
                            sizeof(uint16_t)) != HAL_OK) {
        reading_in_progress = false;
    } 
}

void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
  if (hi2c->Instance == hi2c1.Instance) {
    INA219_ReadComplete_Callback(true);
  }
}
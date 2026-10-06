/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Synchronized occupancy data collection firmware
  *
  * Sensors: 2x PIR, VL53L0X, BME280, BH1750, SSD1306 OLED
  * Controls: 2 buttons, red/green LEDs, LD2, active buzzer
  * Output: one DATA CSV record per second over USB UART or optional HC-06
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include "./BME280/bme280.h"
#include "fonts.h"
#include "ssd1306.h"
#include "vl53l0x_api.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum
{
  LABEL_EMPTY = 0,
  LABEL_LOW,
  LABEL_HIGH
} OccupancyLabel_t;

typedef struct
{
  GPIO_PinState raw_state;
  GPIO_PinState stable_state;
  uint32_t change_time_ms;
} ButtonDebounce_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* Keep this 0 for the current ST-LINK USB serial connection (USART2, 115200).
   Change to 1 only after CubeMX has generated USART1 for HC-06 at 9600 baud. */
#define DATA_LINK_USE_HC06         0U

#if DATA_LINK_USE_HC06
#define DATA_UART_HANDLE           huart1
#else
#define DATA_UART_HANDLE           huart2
#endif

#define SAMPLE_INTERVAL_MS         1000U
#define BUTTON_DEBOUNCE_MS         35U
#define PIR_WARMUP_MS              60000U
#define PIR_FILTER_READS           7U

#define VL53L0X_ADDRESS_HAL        (0x29U << 1)

#define BH1750_ADDR_LOW            0x23U
#define BH1750_ADDR_HIGH           0x5CU
#define BH1750_POWER_ON            0x01U
#define BH1750_RESET               0x07U
#define BH1750_CONT_HIGH_RES_MODE  0x10U

/* Tested module behavior: PB4 LOW = sound, PB4 HIGH = silent. */
#define BUZZER_ACTIVE_LEVEL        GPIO_PIN_RESET
#define BUZZER_INACTIVE_LEVEL      GPIO_PIN_SET
/* USER CODE END PD */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
char tx_text[300];
char oled_text[32];

GPIO_PinState pir1_state;
GPIO_PinState pir2_state;
uint32_t pir_start_time_ms;

struct bme280_dev bme_device;
struct bme280_data bme_data;
int8_t bme_status;
uint8_t bme_initialized;
uint8_t bme_read_ok;
float temperature_c;
float humidity_percent;
float pressure_hpa;

VL53L0X_Dev_t tof_device;
VL53L0X_RangingMeasurementData_t tof_measurement;
VL53L0X_Error tof_api_status;
uint32_t tof_ref_spad_count;
uint8_t tof_is_aperture_spads;
uint8_t tof_vhv_settings;
uint8_t tof_phase_cal;
uint8_t tof_initialized;
uint16_t tof_distance_mm;
uint8_t tof_range_status = 255U;

uint16_t bh1750_addr_hal;
uint8_t bh1750_addr_7bit;
uint8_t bh1750_initialized;
uint8_t bh1750_read_ok;
uint16_t bh1750_raw;
uint32_t light_lux_x10;

OccupancyLabel_t selected_label = LABEL_EMPTY;
uint8_t logging_enabled;
uint32_t session_number;
uint32_t sample_sequence;
uint32_t last_sample_time_ms;

ButtonDebounce_t button1;
ButtonDebounce_t button2;

uint8_t buzzer_beeps_remaining;
uint8_t buzzer_phase_on;
uint32_t buzzer_next_time_ms;
uint32_t activity_led_restore_time_ms;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void Link_Print(const char *message);
void Link_SendHelp(void);
void Link_ProcessCommands(void);
void I2C_Scan(void);

int8_t BME_I2C_Read(uint8_t id, uint8_t reg, uint8_t *data, uint16_t len);
int8_t BME_I2C_Write(uint8_t id, uint8_t reg, uint8_t *data, uint16_t len);
void BME_DelayMs(uint32_t period);
uint8_t BME_Init(void);
uint8_t BME_Read(void);

VL53L0X_Error TOF_Init(void);
uint8_t TOF_Read(void);

HAL_StatusTypeDef BH1750_DetectAddress(void);
HAL_StatusTypeDef BH1750_WriteCommand(uint8_t command);
uint8_t BH1750_Init(void);
uint8_t BH1750_Read(void);

const char *Label_Name(OccupancyLabel_t label);
void Label_Select(OccupancyLabel_t label);
void Outputs_SetForLabel(OccupancyLabel_t label);
uint8_t Hardware_Ready(void);
void Logging_Start(void);
void Logging_Stop(void);

void Buzzer_Pattern(uint8_t beep_count);
void Buzzer_Update(void);
void Activity_LED_Pulse(void);
void Activity_LED_Update(void);

void Buttons_Init(void);
uint8_t Button_Pressed(ButtonDebounce_t *button,
                       GPIO_TypeDef *port,
                       uint16_t pin);
void Buttons_Update(void);

void OLED_WriteLine(uint8_t y, const char *text);
void Display_Update(void);
GPIO_PinState PIR_ReadFiltered(GPIO_TypeDef *port, uint16_t pin);
void Sample_AllSensors(void);
void Output_DataRow(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void Link_Print(const char *message)
{
  HAL_UART_Transmit(&DATA_UART_HANDLE,
                    (uint8_t *)message,
                    (uint16_t)strlen(message),
                    HAL_MAX_DELAY);
}

void Link_SendHelp(void)
{
  Link_Print("COMMANDS,E=EMPTY,L=LOW,H=HIGH,S=START,X=STOP,?=STATUS\r\n");
  snprintf(tx_text,
           sizeof(tx_text),
           "STATUS,label=%s,logging=%u,session=%lu,bme=%u,tof=%u,bh1750=%u\r\n",
           Label_Name(selected_label),
           (unsigned int)logging_enabled,
           (unsigned long)session_number,
           (unsigned int)bme_initialized,
           (unsigned int)tof_initialized,
           (unsigned int)bh1750_initialized);
  Link_Print(tx_text);
}

void Link_ProcessCommands(void)
{
  uint8_t byte;

  if (HAL_UART_Receive(&DATA_UART_HANDLE, &byte, 1U, 0U) != HAL_OK)
  {
    return;
  }

  if ((byte >= 'a') && (byte <= 'z'))
  {
    byte = (uint8_t)(byte - ('a' - 'A'));
  }

  switch (byte)
  {
    case 'E': Label_Select(LABEL_EMPTY); break;
    case 'L': Label_Select(LABEL_LOW);   break;
    case 'H': Label_Select(LABEL_HIGH);  break;
    case 'S': Logging_Start();           break;
    case 'X': Logging_Stop();            break;
    case '?': Link_SendHelp();            break;
    default:                              break;
  }
}

void I2C_Scan(void)
{
  uint8_t address;
  uint8_t count = 0U;

  Link_Print("I2C_SCAN,BEGIN\r\n");
  for (address = 0x08U; address <= 0x77U; address++)
  {
    if (HAL_I2C_IsDeviceReady(&hi2c1,
                              (uint16_t)(address << 1),
                              2U,
                              20U) == HAL_OK)
    {
      snprintf(tx_text,
               sizeof(tx_text),
               "I2C_DEVICE,0x%02X\r\n",
               (unsigned int)address);
      Link_Print(tx_text);
      count++;
    }
  }
  snprintf(tx_text,
           sizeof(tx_text),
           "I2C_SCAN,END,count=%u\r\n",
           (unsigned int)count);
  Link_Print(tx_text);
}

int8_t BME_I2C_Read(uint8_t id, uint8_t reg, uint8_t *data, uint16_t len)
{
  return (HAL_I2C_Mem_Read(&hi2c1,
                           (uint16_t)(id << 1),
                           reg,
                           I2C_MEMADD_SIZE_8BIT,
                           data,
                           len,
                           100U) == HAL_OK) ? 0 : -1;
}

int8_t BME_I2C_Write(uint8_t id, uint8_t reg, uint8_t *data, uint16_t len)
{
  return (HAL_I2C_Mem_Write(&hi2c1,
                            (uint16_t)(id << 1),
                            reg,
                            I2C_MEMADD_SIZE_8BIT,
                            data,
                            len,
                            100U) == HAL_OK) ? 0 : -1;
}

void BME_DelayMs(uint32_t period)
{
  HAL_Delay(period);
}

uint8_t BME_Init(void)
{
  uint8_t address;

  if (HAL_I2C_IsDeviceReady(&hi2c1, (uint16_t)(0x76U << 1), 3U, 100U) == HAL_OK)
  {
    address = 0x76U;
  }
  else if (HAL_I2C_IsDeviceReady(&hi2c1,
                                 (uint16_t)(0x77U << 1),
                                 3U,
                                 100U) == HAL_OK)
  {
    address = 0x77U;
  }
  else
  {
    Link_Print("ERROR,BME280_NOT_FOUND\r\n");
    return 0U;
  }

  memset(&bme_device, 0, sizeof(bme_device));
  bme_device.dev_id = address;
  bme_device.intf = BME280_I2C_INTF;
  bme_device.read = BME_I2C_Read;
  bme_device.write = BME_I2C_Write;
  bme_device.delay_ms = BME_DelayMs;

  bme_status = bme280_init(&bme_device);
  if (bme_status != BME280_OK) return 0U;

  bme_device.settings.osr_h = BME280_OVERSAMPLING_1X;
  bme_device.settings.osr_p = BME280_OVERSAMPLING_16X;
  bme_device.settings.osr_t = BME280_OVERSAMPLING_2X;
  bme_device.settings.filter = BME280_FILTER_COEFF_16;

  bme_status = bme280_set_sensor_settings(
      BME280_OSR_PRESS_SEL | BME280_OSR_TEMP_SEL |
      BME280_OSR_HUM_SEL | BME280_FILTER_SEL,
      &bme_device);
  if (bme_status != BME280_OK) return 0U;

  snprintf(tx_text,
           sizeof(tx_text),
           "READY,BME280,0x%02X\r\n",
           (unsigned int)address);
  Link_Print(tx_text);
  return 1U;
}

uint8_t BME_Read(void)
{
  if (bme_initialized == 0U) return 0U;
  if (bme280_set_sensor_mode(BME280_FORCED_MODE, &bme_device) != BME280_OK)
  {
    return 0U;
  }
  HAL_Delay(40U);
  if (bme280_get_sensor_data(BME280_ALL, &bme_data, &bme_device) != BME280_OK)
  {
    return 0U;
  }

  temperature_c = bme_data.temperature / 100.0f;
  humidity_percent = bme_data.humidity / 1024.0f;
  pressure_hpa = bme_data.pressure / 10000.0f;
  return 1U;
}

VL53L0X_Error TOF_Init(void)
{
  VL53L0X_Error status;

  if (HAL_I2C_IsDeviceReady(&hi2c1, VL53L0X_ADDRESS_HAL, 3U, 100U) != HAL_OK)
  {
    Link_Print("ERROR,VL53L0X_NOT_FOUND\r\n");
    return VL53L0X_ERROR_CONTROL_INTERFACE;
  }

  memset(&tof_device, 0, sizeof(tof_device));
  tof_device.I2cDevAddr = VL53L0X_ADDRESS_HAL;

  status = VL53L0X_DataInit(&tof_device);
  if (status != VL53L0X_ERROR_NONE) return status;
  status = VL53L0X_StaticInit(&tof_device);
  if (status != VL53L0X_ERROR_NONE) return status;
  status = VL53L0X_PerformRefSpadManagement(&tof_device,
                                             &tof_ref_spad_count,
                                             &tof_is_aperture_spads);
  if (status != VL53L0X_ERROR_NONE) return status;
  status = VL53L0X_PerformRefCalibration(&tof_device,
                                          &tof_vhv_settings,
                                          &tof_phase_cal);
  if (status != VL53L0X_ERROR_NONE) return status;
  status = VL53L0X_SetDeviceMode(&tof_device, VL53L0X_DEVICEMODE_SINGLE_RANGING);
  if (status != VL53L0X_ERROR_NONE) return status;
  status = VL53L0X_SetLimitCheckValue(
      &tof_device,
      VL53L0X_CHECKENABLE_SIGNAL_RATE_FINAL_RANGE,
      (FixPoint1616_t)16384U); /* 0.25 MCps */
  if (status != VL53L0X_ERROR_NONE) return status;
  status = VL53L0X_SetLimitCheckValue(
      &tof_device,
      VL53L0X_CHECKENABLE_SIGMA_FINAL_RANGE,
      (FixPoint1616_t)1179648U); /* 18 mm */
  if (status != VL53L0X_ERROR_NONE) return status;
  status = VL53L0X_SetMeasurementTimingBudgetMicroSeconds(&tof_device, 200000U);
  return status;
}

uint8_t TOF_Read(void)
{
  if (tof_initialized == 0U)
  {
    tof_distance_mm = 0U;
    tof_range_status = 255U;
    return 0U;
  }

  tof_api_status = VL53L0X_PerformSingleRangingMeasurement(&tof_device,
                                                            &tof_measurement);
  if (tof_api_status != VL53L0X_ERROR_NONE)
  {
    tof_distance_mm = 0U;
    tof_range_status = 255U;
    return 0U;
  }

  tof_distance_mm = tof_measurement.RangeMilliMeter;
  tof_range_status = tof_measurement.RangeStatus;
  return (tof_range_status == 0U) ? 1U : 0U;
}

HAL_StatusTypeDef BH1750_DetectAddress(void)
{
  if (HAL_I2C_IsDeviceReady(&hi2c1,
                            (uint16_t)(BH1750_ADDR_LOW << 1),
                            3U,
                            100U) == HAL_OK)
  {
    bh1750_addr_7bit = BH1750_ADDR_LOW;
  }
  else if (HAL_I2C_IsDeviceReady(&hi2c1,
                                 (uint16_t)(BH1750_ADDR_HIGH << 1),
                                 3U,
                                 100U) == HAL_OK)
  {
    bh1750_addr_7bit = BH1750_ADDR_HIGH;
  }
  else
  {
    bh1750_addr_hal = 0U;
    return HAL_ERROR;
  }

  bh1750_addr_hal = (uint16_t)(bh1750_addr_7bit << 1);
  return HAL_OK;
}

HAL_StatusTypeDef BH1750_WriteCommand(uint8_t command)
{
  if (bh1750_addr_hal == 0U) return HAL_ERROR;
  return HAL_I2C_Master_Transmit(&hi2c1,
                                 bh1750_addr_hal,
                                 &command,
                                 1U,
                                 100U);
}

uint8_t BH1750_Init(void)
{
  if (BH1750_DetectAddress() != HAL_OK)
  {
    Link_Print("ERROR,BH1750_NOT_FOUND\r\n");
    return 0U;
  }
  if (BH1750_WriteCommand(BH1750_POWER_ON) != HAL_OK) return 0U;
  HAL_Delay(10U);
  if (BH1750_WriteCommand(BH1750_RESET) != HAL_OK) return 0U;
  HAL_Delay(10U);
  if (BH1750_WriteCommand(BH1750_CONT_HIGH_RES_MODE) != HAL_OK) return 0U;
  HAL_Delay(180U);

  snprintf(tx_text,
           sizeof(tx_text),
           "READY,BH1750,0x%02X\r\n",
           (unsigned int)bh1750_addr_7bit);
  Link_Print(tx_text);
  return 1U;
}

uint8_t BH1750_Read(void)
{
  uint8_t data[2];

  if (bh1750_initialized == 0U) return 0U;
  if (HAL_I2C_Master_Receive(&hi2c1,
                             bh1750_addr_hal,
                             data,
                             2U,
                             100U) != HAL_OK)
  {
    return 0U;
  }

  bh1750_raw = ((uint16_t)data[0] << 8) | data[1];
  light_lux_x10 = (((uint32_t)bh1750_raw * 100U) + 6U) / 12U;
  return 1U;
}

const char *Label_Name(OccupancyLabel_t label)
{
  switch (label)
  {
    case LABEL_EMPTY: return "EMPTY";
    case LABEL_LOW:   return "LOW";
    case LABEL_HIGH:  return "HIGH";
    default:          return "UNKNOWN";
  }
}

void Outputs_SetForLabel(OccupancyLabel_t label)
{
  HAL_GPIO_WritePin(LED_Green_GPIO_Port,
                    LED_Green_Pin,
                    (label != LABEL_HIGH) ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_WritePin(LED_Red_GPIO_Port,
                    LED_Red_Pin,
                    (label != LABEL_EMPTY) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void Label_Select(OccupancyLabel_t label)
{
  if (logging_enabled != 0U)
  {
    Link_Print("EVENT,REJECTED,LABEL_LOCKED_WHILE_LOGGING\r\n");
    Buzzer_Pattern(3U);
    return;
  }

  selected_label = label;
  Outputs_SetForLabel(label);
  snprintf(tx_text, sizeof(tx_text), "EVENT,LABEL,%s\r\n", Label_Name(label));
  Link_Print(tx_text);
  Buzzer_Pattern(1U);
}

uint8_t Hardware_Ready(void)
{
  return ((bme_initialized != 0U) &&
          (tof_initialized != 0U) &&
          (bh1750_initialized != 0U)) ? 1U : 0U;
}

void Logging_Start(void)
{
  if (logging_enabled != 0U) return;

  if ((HAL_GetTick() - pir_start_time_ms) < PIR_WARMUP_MS)
  {
    Link_Print("EVENT,WAIT,PIR_WARMUP_60_SECONDS\r\n");
    Buzzer_Pattern(2U);
    return;
  }

  if (Hardware_Ready() == 0U)
  {
    Link_Print("EVENT,ERROR,HARDWARE_NOT_READY\r\n");
    Buzzer_Pattern(3U);
    return;
  }

  session_number++;
  sample_sequence = 0U;
  logging_enabled = 1U;
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
  snprintf(tx_text,
           sizeof(tx_text),
           "EVENT,START,SESSION_%03lu,%s\r\n",
           (unsigned long)session_number,
           Label_Name(selected_label));
  Link_Print(tx_text);
  Link_Print("CSV_HEADER,sequence,mcu_time_ms,session_id,pir_1,pir_2,tof_mm,tof_status,temperature_c,humidity_percent,pressure_hpa,light_lux,sample_valid,occupancy_label\r\n");
  last_sample_time_ms = HAL_GetTick() - SAMPLE_INTERVAL_MS;
  Buzzer_Pattern(1U);
}

void Logging_Stop(void)
{
  if (logging_enabled == 0U) return;
  logging_enabled = 0U;
  activity_led_restore_time_ms = 0U;
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
  snprintf(tx_text,
           sizeof(tx_text),
           "EVENT,STOP,SESSION_%03lu,rows=%lu\r\n",
           (unsigned long)session_number,
           (unsigned long)sample_sequence);
  Link_Print(tx_text);
  Buzzer_Pattern(2U);
}

void Buzzer_Pattern(uint8_t beep_count)
{
  HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, BUZZER_INACTIVE_LEVEL);
  buzzer_beeps_remaining = beep_count;
  buzzer_phase_on = 0U;
  buzzer_next_time_ms = HAL_GetTick();
}

void Buzzer_Update(void)
{
  if ((int32_t)(HAL_GetTick() - buzzer_next_time_ms) < 0) return;

  if (buzzer_phase_on != 0U)
  {
    HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, BUZZER_INACTIVE_LEVEL);
    buzzer_phase_on = 0U;
    buzzer_next_time_ms = HAL_GetTick() + 90U;
  }
  else if (buzzer_beeps_remaining != 0U)
  {
    HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, BUZZER_ACTIVE_LEVEL);
    buzzer_phase_on = 1U;
    buzzer_beeps_remaining--;
    buzzer_next_time_ms = HAL_GetTick() + 80U;
  }
}

void Activity_LED_Pulse(void)
{
  if (logging_enabled != 0U)
  {
    HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
    activity_led_restore_time_ms = HAL_GetTick() + 60U;
  }
}

void Activity_LED_Update(void)
{
  if ((logging_enabled != 0U) &&
      (activity_led_restore_time_ms != 0U) &&
      ((int32_t)(HAL_GetTick() - activity_led_restore_time_ms) >= 0))
  {
    HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
    activity_led_restore_time_ms = 0U;
  }
}

void Buttons_Init(void)
{
  button1.raw_state = HAL_GPIO_ReadPin(Push_button_1_GPIO_Port,
                                       Push_button_1_Pin);
  button1.stable_state = button1.raw_state;
  button1.change_time_ms = HAL_GetTick();
  button2.raw_state = HAL_GPIO_ReadPin(Push_button_2_GPIO_Port,
                                       Push_button_2_Pin);
  button2.stable_state = button2.raw_state;
  button2.change_time_ms = HAL_GetTick();
}

uint8_t Button_Pressed(ButtonDebounce_t *button,
                       GPIO_TypeDef *port,
                       uint16_t pin)
{
  GPIO_PinState new_state = HAL_GPIO_ReadPin(port, pin);

  if (new_state != button->raw_state)
  {
    button->raw_state = new_state;
    button->change_time_ms = HAL_GetTick();
  }
  if (((HAL_GetTick() - button->change_time_ms) >= BUTTON_DEBOUNCE_MS) &&
      (button->stable_state != button->raw_state))
  {
    button->stable_state = button->raw_state;
    if (button->stable_state == GPIO_PIN_RESET) return 1U;
  }
  return 0U;
}

void Buttons_Update(void)
{
  if (Button_Pressed(&button1,
                     Push_button_1_GPIO_Port,
                     Push_button_1_Pin) != 0U)
  {
    Label_Select((OccupancyLabel_t)(((uint8_t)selected_label + 1U) % 3U));
  }
  if (Button_Pressed(&button2,
                     Push_button_2_GPIO_Port,
                     Push_button_2_Pin) != 0U)
  {
    if (logging_enabled != 0U) Logging_Stop();
    else Logging_Start();
  }
}

void OLED_WriteLine(uint8_t y, const char *text)
{
  char padded[19];
  snprintf(padded, sizeof(padded), "%-18.18s", text);
  SSD1306_GotoXY(0U, y);
  SSD1306_Puts(padded, &Font_7x10, 1U);
}

void Display_Update(void)
{
  SSD1306_Fill(SSD1306_COLOR_BLACK);

  snprintf(oled_text,
           sizeof(oled_text),
           "T:%4.1f H:%4.1f",
           temperature_c,
           humidity_percent);
  OLED_WriteLine(0U, oled_text);

  snprintf(oled_text,
           sizeof(oled_text),
           "Lux:%lu.%lu",
           (unsigned long)(light_lux_x10 / 10U),
           (unsigned long)(light_lux_x10 % 10U));
  OLED_WriteLine(11U, oled_text);

  snprintf(oled_text,
           sizeof(oled_text),
           "ToF:%u S:%u",
           (unsigned int)tof_distance_mm,
           (unsigned int)tof_range_status);
  OLED_WriteLine(22U, oled_text);

  snprintf(oled_text,
           sizeof(oled_text),
           "PIR:%u/%u Read:%u",
           (pir1_state == GPIO_PIN_SET) ? 1U : 0U,
           (pir2_state == GPIO_PIN_SET) ? 1U : 0U,
           (unsigned int)(bme_read_ok && bh1750_read_ok));
  OLED_WriteLine(33U, oled_text);

  snprintf(oled_text,
           sizeof(oled_text),
           "%s LOG:%s",
           Label_Name(selected_label),
           (logging_enabled != 0U) ? "ON" : "OFF");
  OLED_WriteLine(44U, oled_text);

  snprintf(oled_text,
           sizeof(oled_text),
           "S:%lu N:%lu",
           (unsigned long)session_number,
           (unsigned long)sample_sequence);
  OLED_WriteLine(54U, oled_text);
  SSD1306_UpdateScreen();
}

GPIO_PinState PIR_ReadFiltered(GPIO_TypeDef *port, uint16_t pin)
{
  uint8_t high_count = 0U;

  for (uint8_t i = 0U; i < PIR_FILTER_READS; i++)
  {
    if (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_SET)
    {
      high_count++;
    }

    HAL_Delay(5U);
  }

  return (high_count >= 4U) ? GPIO_PIN_SET : GPIO_PIN_RESET;
}

void Sample_AllSensors(void)
{
  if ((HAL_GetTick() - pir_start_time_ms) < PIR_WARMUP_MS)
  {
    pir1_state = GPIO_PIN_RESET;
    pir2_state = GPIO_PIN_RESET;
  }
  else
  {
    pir1_state = PIR_ReadFiltered(PIR_1_GPIO_Port, PIR_1_Pin);
    pir2_state = PIR_ReadFiltered(PIR_2_GPIO_Port, PIR_2_Pin);
  }

  bme_read_ok = BME_Read();
  (void)TOF_Read();
  bh1750_read_ok = BH1750_Read();
}

void Output_DataRow(void)
{
  unsigned int pir1 = (pir1_state == GPIO_PIN_SET) ? 1U : 0U;
  unsigned int pir2 = (pir2_state == GPIO_PIN_SET) ? 1U : 0U;
  unsigned int sample_valid = (bme_read_ok && bh1750_read_ok) ? 1U : 0U;

  if (logging_enabled == 0U) return;

  sample_sequence++;
  snprintf(tx_text,
           sizeof(tx_text),
           "DATA,%lu,%lu,SESSION_%03lu,%u,%u,%u,%u,%.2f,%.2f,%.2f,%lu.%lu,%u,%s\r\n",
           (unsigned long)sample_sequence,
           (unsigned long)HAL_GetTick(),
           (unsigned long)session_number,
           pir1,
           pir2,
           (unsigned int)tof_distance_mm,
           (unsigned int)tof_range_status,
           temperature_c,
           humidity_percent,
           pressure_hpa,
           (unsigned long)(light_lux_x10 / 10U),
           (unsigned long)(light_lux_x10 % 10U),
           sample_valid,
           Label_Name(selected_label));
  Link_Print(tx_text);
  Activity_LED_Pulse();
}
/* USER CODE END 0 */

int main(void)
{
  HAL_Init();
  SystemClock_Config();

  MX_GPIO_Init();
  MX_USART2_UART_Init();
#if DATA_LINK_USE_HC06
  MX_USART1_UART_Init();
#endif
  MX_I2C1_Init();

  /* USER CODE BEGIN 2 */
  pir_start_time_ms = HAL_GetTick();
  HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, BUZZER_INACTIVE_LEVEL);
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
  Outputs_SetForLabel(selected_label);
  Buttons_Init();

  Link_Print("BOOT,OCCUPANCY_DATA_COLLECTOR\r\n");
  I2C_Scan();
  SSD1306_Init();
  SSD1306_Fill(SSD1306_COLOR_BLACK);
  OLED_WriteLine(0U, "System starting");
  SSD1306_UpdateScreen();

  bme_initialized = BME_Init();
  tof_api_status = TOF_Init();
  tof_initialized = (tof_api_status == VL53L0X_ERROR_NONE) ? 1U : 0U;
  if (tof_initialized != 0U) Link_Print("READY,VL53L0X,0x29\r\n");
  bh1750_initialized = BH1750_Init();

  Link_SendHelp();
  Buzzer_Pattern((Hardware_Ready() != 0U) ? 1U : 3U);
  last_sample_time_ms = HAL_GetTick() - SAMPLE_INTERVAL_MS;
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    Link_ProcessCommands();
    Buttons_Update();
    Buzzer_Update();
    Activity_LED_Update();

    if ((HAL_GetTick() - last_sample_time_ms) >= SAMPLE_INTERVAL_MS)
    {
      last_sample_time_ms = HAL_GetTick();
      Sample_AllSensors();
      Display_Update();
      Output_DataRow();
    }

    HAL_Delay(5U);
  }
  /* USER CODE END WHILE */
}

/* Keep your CubeMX-generated SystemClock_Config(), Error_Handler(), and
   assert_failed() implementations below this point. */

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 10;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK |
                                RCC_CLOCKTYPE_SYSCLK |
                                RCC_CLOCKTYPE_PCLK1 |
                                RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
  (void)file;
  (void)line;
}
#endif


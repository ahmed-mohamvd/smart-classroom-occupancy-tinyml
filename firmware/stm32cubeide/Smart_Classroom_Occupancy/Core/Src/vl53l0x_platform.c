/**
  ******************************************************************************
  * @file    vl53l0x_platform.c
  * @brief   VL53L0X official API platform layer for STM32 HAL and I2C1
  ******************************************************************************
  * Rename this file to vl53l0x_platform.c and use it with STSW-IMG005.
  * tof_device.I2cDevAddr must be 0x52 (the 7-bit address 0x29 shifted left).
  ******************************************************************************
  */

#include "main.h"
#include "i2c.h"
#include "vl53l0x_api.h"
#include "vl53l0x_platform.h"

#define VL53L0X_I2C_TIMEOUT_MS  100U

static VL53L0X_Error VL53L0X_FromHalStatus(HAL_StatusTypeDef hal_status)
{
  return (hal_status == HAL_OK) ? VL53L0X_ERROR_NONE
                                : VL53L0X_ERROR_CONTROL_INTERFACE;
}

VL53L0X_Error VL53L0X_WriteMulti(VL53L0X_DEV Dev,
                                  uint8_t index,
                                  uint8_t *pdata,
                                  uint32_t count)
{
  return VL53L0X_FromHalStatus(
      HAL_I2C_Mem_Write(&hi2c1,
                        Dev->I2cDevAddr,
                        index,
                        I2C_MEMADD_SIZE_8BIT,
                        pdata,
                        (uint16_t)count,
                        VL53L0X_I2C_TIMEOUT_MS));
}

VL53L0X_Error VL53L0X_ReadMulti(VL53L0X_DEV Dev,
                                 uint8_t index,
                                 uint8_t *pdata,
                                 uint32_t count)
{
  return VL53L0X_FromHalStatus(
      HAL_I2C_Mem_Read(&hi2c1,
                       Dev->I2cDevAddr,
                       index,
                       I2C_MEMADD_SIZE_8BIT,
                       pdata,
                       (uint16_t)count,
                       VL53L0X_I2C_TIMEOUT_MS));
}

VL53L0X_Error VL53L0X_WrByte(VL53L0X_DEV Dev,
                              uint8_t index,
                              uint8_t data)
{
  return VL53L0X_WriteMulti(Dev, index, &data, 1U);
}

VL53L0X_Error VL53L0X_WrWord(VL53L0X_DEV Dev,
                              uint8_t index,
                              uint16_t data)
{
  uint8_t buffer[2];
  buffer[0] = (uint8_t)(data >> 8);
  buffer[1] = (uint8_t)(data & 0xFFU);
  return VL53L0X_WriteMulti(Dev, index, buffer, 2U);
}

VL53L0X_Error VL53L0X_WrDWord(VL53L0X_DEV Dev,
                               uint8_t index,
                               uint32_t data)
{
  uint8_t buffer[4];
  buffer[0] = (uint8_t)(data >> 24);
  buffer[1] = (uint8_t)(data >> 16);
  buffer[2] = (uint8_t)(data >> 8);
  buffer[3] = (uint8_t)(data & 0xFFU);
  return VL53L0X_WriteMulti(Dev, index, buffer, 4U);
}

VL53L0X_Error VL53L0X_RdByte(VL53L0X_DEV Dev,
                              uint8_t index,
                              uint8_t *data)
{
  return VL53L0X_ReadMulti(Dev, index, data, 1U);
}

VL53L0X_Error VL53L0X_RdWord(VL53L0X_DEV Dev,
                              uint8_t index,
                              uint16_t *data)
{
  uint8_t buffer[2];
  VL53L0X_Error status = VL53L0X_ReadMulti(Dev, index, buffer, 2U);

  if (status == VL53L0X_ERROR_NONE)
  {
    *data = ((uint16_t)buffer[0] << 8) | buffer[1];
  }

  return status;
}

VL53L0X_Error VL53L0X_RdDWord(VL53L0X_DEV Dev,
                               uint8_t index,
                               uint32_t *data)
{
  uint8_t buffer[4];
  VL53L0X_Error status = VL53L0X_ReadMulti(Dev, index, buffer, 4U);

  if (status == VL53L0X_ERROR_NONE)
  {
    *data = ((uint32_t)buffer[0] << 24) |
            ((uint32_t)buffer[1] << 16) |
            ((uint32_t)buffer[2] << 8)  |
            (uint32_t)buffer[3];
  }

  return status;
}

VL53L0X_Error VL53L0X_UpdateByte(VL53L0X_DEV Dev,
                                  uint8_t index,
                                  uint8_t AndData,
                                  uint8_t OrData)
{
  uint8_t data;
  VL53L0X_Error status = VL53L0X_RdByte(Dev, index, &data);

  if (status == VL53L0X_ERROR_NONE)
  {
    data = (uint8_t)((data & AndData) | OrData);
    status = VL53L0X_WrByte(Dev, index, data);
  }

  return status;
}

VL53L0X_Error VL53L0X_PollingDelay(VL53L0X_DEV Dev)
{
  (void)Dev;
  HAL_Delay(1U);
  return VL53L0X_ERROR_NONE;
}

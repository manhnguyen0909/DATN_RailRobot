// platform.h
#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>
#include <string.h>
#include "stm32f4xx_hal.h"

/* VL53L1X_ERROR đã được định nghĩa trong VL53L1X_api.h của ST
   KHÔNG khai báo lại ở đây                                      */

typedef uint16_t Dev_t;
#define VL53L1DEV_DEFAULT  ((Dev_t)0x52)

/* ------------------------------------------------------------------ */
/*  Khai báo các hàm platform                                          */
/* ------------------------------------------------------------------ */

int8_t VL53L1_WriteMulti(uint16_t dev, uint16_t index,
                          uint8_t *pdata, uint32_t count);

int8_t VL53L1_ReadMulti(uint16_t dev, uint16_t index,
                         uint8_t *pdata, uint32_t count);

int8_t VL53L1_WrByte (uint16_t dev, uint16_t index, uint8_t  data);
int8_t VL53L1_WrWord (uint16_t dev, uint16_t index, uint16_t data);
int8_t VL53L1_WrDWord(uint16_t dev, uint16_t index, uint32_t data);

int8_t VL53L1_RdByte (uint16_t dev, uint16_t index, uint8_t  *data);
int8_t VL53L1_RdWord (uint16_t dev, uint16_t index, uint16_t *data);
int8_t VL53L1_RdDWord(uint16_t dev, uint16_t index, uint32_t *data);

int8_t VL53L1_WaitMs(uint16_t dev, int32_t wait_ms);

void VL53L1_GetTickCount(uint32_t *ptick_count_ms);

#endif /* PLATFORM_H */


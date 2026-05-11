// Nhớ đổi tên include này thành đúng tên file .h của bạn nhé (ví dụ: "platform.h" hoặc "vl53l1_platform.h")
#include "vl53l1_platform.h" 
#include "i2c.h" 

int8_t VL53L1_WriteMulti(uint16_t dev, uint16_t index, uint8_t *pdata, uint32_t count) {
    uint8_t buf[count + 2];
    buf[0] = (index >> 8) & 0xFF;
    buf[1] = index & 0xFF;
    memcpy(&buf[2], pdata, count);
    HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(&hi2c1, dev, buf, count + 2, 100);
    return (status == HAL_OK) ? 0 : -1;
}

int8_t VL53L1_ReadMulti(uint16_t dev, uint16_t index, uint8_t *pdata, uint32_t count) {
    uint8_t reg[2] = { (index >> 8) & 0xFF, index & 0xFF };
    HAL_StatusTypeDef status;
    status = HAL_I2C_Master_Transmit(&hi2c1, dev, reg, 2, 100);
    if (status != HAL_OK) return -1;
    status = HAL_I2C_Master_Receive(&hi2c1, dev, pdata, count, 100);
    return (status == HAL_OK) ? 0 : -1;
}

int8_t VL53L1_WrByte(uint16_t dev, uint16_t index, uint8_t data) {
    return VL53L1_WriteMulti(dev, index, &data, 1);
}

int8_t VL53L1_RdByte(uint16_t dev, uint16_t index, uint8_t *data) {
    return VL53L1_ReadMulti(dev, index, data, 1);
}

int8_t VL53L1_WrWord(uint16_t dev, uint16_t index, uint16_t data) {
    uint8_t buf[2] = { (data >> 8) & 0xFF, data & 0xFF };
    return VL53L1_WriteMulti(dev, index, buf, 2);
}

int8_t VL53L1_RdWord(uint16_t dev, uint16_t index, uint16_t *data) {
    uint8_t buf[2];
    int8_t err = VL53L1_ReadMulti(dev, index, buf, 2);
    *data = ((uint16_t)buf[0] << 8) | buf[1];
    return err;
}

int8_t VL53L1_WrDWord(uint16_t dev, uint16_t index, uint32_t data) {
    uint8_t buf[4] = { (data >> 24) & 0xFF, (data >> 16) & 0xFF, (data >> 8) & 0xFF, data & 0xFF };
    return VL53L1_WriteMulti(dev, index, buf, 4);
}

int8_t VL53L1_RdDWord(uint16_t dev, uint16_t index, uint32_t *data) {
    uint8_t buf[4];
    int8_t err = VL53L1_ReadMulti(dev, index, buf, 4);
    *data = ((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16) | ((uint32_t)buf[2] << 8) | buf[3];
    return err;
}

int8_t VL53L1_WaitMs(uint16_t dev, int32_t wait_ms) {
    HAL_Delay(wait_ms);
    return 0;
}

void VL53L1_GetTickCount(uint32_t *ptick_count_ms) {
    *ptick_count_ms = HAL_GetTick();
}

#ifndef UART_APP_H
#define UART_APP_H

#include "stdint.h"

// Khởi tạo UART1 và tạo Task lắng nghe dữ liệu
void UART_App_Init(void);

// Hàm dùng để gửi chuỗi lệnh từ ESP32 xuống STM32
void UART_Send_String(const char* str);

void UART_Send_Robot_Command(int speed_L, int speed_R);
#endif /* UART_APP_H */
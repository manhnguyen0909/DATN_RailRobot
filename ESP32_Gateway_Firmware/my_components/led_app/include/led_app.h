#ifndef LED_APP_H
#define LED_APP_H

#include <stdbool.h>

// Khởi tạo các chân LED
void LED_App_Init(void);

// Đặt trạng thái LED WiFi và MQTT (true = Sáng, false = Tắt)
void LED_WiFi_Set(bool state);
void LED_MQTT_Set(bool state);

// Nháy LED LoRa báo hiệu có data (Sáng 50ms rồi tắt)
void LED_LoRa_Blink(void);

#endif // LED_APP_H
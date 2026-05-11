#include "led_app.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Định nghĩa chân
#define LED_WIFI_PIN    2
#define LED_MQTT_PIN    1
#define LED_LORA_PIN    0

void LED_App_Init(void)
{
    // Reset cấu hình chân về mặc định
    gpio_reset_pin(LED_WIFI_PIN);
    gpio_reset_pin(LED_MQTT_PIN);
    gpio_reset_pin(LED_LORA_PIN);
    
    // Cài đặt làm chân Output
    gpio_set_direction(LED_WIFI_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_MQTT_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_LORA_PIN, GPIO_MODE_OUTPUT);
    
    // Mặc định tắt hết khi mới khởi động
    gpio_set_level(LED_WIFI_PIN, 0);
    gpio_set_level(LED_MQTT_PIN, 0);
    gpio_set_level(LED_LORA_PIN, 0);
}

void LED_WiFi_Set(bool state)
{
    gpio_set_level(LED_WIFI_PIN, state ? 1 : 0);
}

void LED_MQTT_Set(bool state)
{
    gpio_set_level(LED_MQTT_PIN, state ? 1 : 0);
}

void LED_LoRa_Blink(void)
{
    // Bật đèn
    gpio_set_level(LED_LORA_PIN, 1);
    // Trễ một chút để mắt người kịp nhìn thấy (50ms - 100ms)
    vTaskDelay(pdMS_TO_TICKS(50));
    // Tắt đèn
    gpio_set_level(LED_LORA_PIN, 0);
}
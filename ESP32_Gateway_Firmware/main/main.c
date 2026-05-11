#include "stdint.h"
#include "stdio.h"
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "spi_flash_mmap.h"
#include "esp_flash.h"
#include "esp_log.h"
#include "ota_server.h"
#include "wifi_manager.h"
#include "nvs_flash.h"
#include "uart_app.h"
#include "mqtt_app.h"
#include "led_app.h"


static const char *TAG = "MAIN_APP";

void app_main(void)
{
    LED_App_Init();
    // 1. NVS Init
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "NVS initialized successfully.");
    wifi_manager_init(); // Kết nối WiFi
    ESP_LOGI(TAG, "WiFi initialized successfully.");
    // wifi_init_softap();
    // Đợi một chút cho mạng ổn định (Optional nhưng tốt)
    vTaskDelay(pdMS_TO_TICKS(100));
    
    start_ota_server(); // Bắt đầu server OTA
    ESP_LOGI(TAG, "OTA Server started successfully.");

    // Khởi tạo các module
    UART_App_Init();
    MQTT_App_Init();
    
    while(1){
        // Test gửi 1 câu chào xuống STM32
        //UART_Send_String("Hello STM32, tao la ESP32 day!\n");
        //UART_Send_Robot_Command(150, 150); // Cho xe tiến
        // UART_Send_Robot_Command(0, 0);     // Cho xe dừng
        vTaskDelay(pdMS_TO_TICKS(10));
        
    }
    
    
}
#include "uart_app.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "string.h"
#include "stdio.h"    // Thêm thư viện này để dùng sprintf
#include "mqtt_app.h" // Gọi thư viện MQTT để đẩy dữ liệu lên Web
#include "led_app.h"  // Gọi thư viện LED để nháy đèn báo hiệu khi có data từ LoRa

static const char *TAG = "UART_APP";

#define UART_PORT_NUM UART_NUM_1
#define UART_TXD_PIN 20
#define UART_RXD_PIN 21
#define UART_BUF_SIZE 1024

// Mật khẩu bảo mật
#define MY_TOKEN "MANH_VIP"

static QueueHandle_t uart_event_queue;

/* ====================================================================
 * TASK LẮNG NGHE SỰ KIỆN UART (Đã tích hợp lọc Token)
 * ==================================================================== */
static void uart_event_task(void *pvParameters)
{
    uart_event_t event;
    uint8_t *dtmp = (uint8_t *)malloc(UART_BUF_SIZE);

    // Tạo bộ đệm tĩnh để hứng từng mảnh dữ liệu cho đến khi đủ câu
    char rx_buffer[512];
    int rx_index = 0;

    while (1)
    {
        if (xQueueReceive(uart_event_queue, (void *)&event, (TickType_t)portMAX_DELAY))
        {

            bzero(dtmp, UART_BUF_SIZE);

            switch (event.type)
            {
            case UART_DATA:
                uart_read_bytes(UART_PORT_NUM, dtmp, event.size, portMAX_DELAY);

                // --- LOGIC GOM CHUỖI VÀ XỬ LÝ LORA ---
                for (int i = 0; i < event.size; i++)
                {
                    char c = (char)dtmp[i];
                    rx_buffer[rx_index++] = c;

                    // Nếu gặp ký tự kết thúc gói tin (!)
                    if (c == '!')
                    {
                        rx_buffer[rx_index] = '\0'; // Khóa đuôi chuỗi

                        // 1. Kiểm tra Token bảo mật
                        if (strncmp(rx_buffer, MY_TOKEN, strlen(MY_TOKEN)) == 0)
                        {
                            LED_LoRa_Blink(); // Nháy đèn báo hiệu có data từ LoRa
                            // Nếu hợp lệ: Cắt bỏ phần Token để lấy ruột (payload)
                            char *payload = rx_buffer + strlen(MY_TOKEN);
                            ESP_LOGI(TAG, "[HOP LE] Du lieu tu xe: %s", payload);
                            MQTT_Publish_Telemetry(payload);
                            // ==========================================================
                            if (has_pending_command == true)
                            {
                                // Bắn lệnh xuống STM32
                                UART_Send_Robot_Command(pending_speed_L, pending_speed_R);
                                ESP_LOGI(TAG, "=> Đã bắn lệnh ghim trong bộ đệm xuống xe: L=%d, R=%d", pending_speed_L, pending_speed_R);

                                // Xóa cờ để không bắn lặp lại ở chu kỳ sau
                                has_pending_command = false;
                            }
                        }
                        else if (strncmp(rx_buffer, "ECHO", strlen("ECHO")) == 0)
                        {
                            ESP_LOGI(TAG, "====> [XAC NHAN] Robot da nhan lenh: %s", rx_buffer);
                        }
                        
                        else
                        {
                            ESP_LOGW(TAG, "[CANH BAO] Nhieu song/Sai Token: %s", rx_buffer);
                        }

                        // 2. Dọn dẹp bộ đệm để đón câu tiếp theo
                        rx_index = 0;
                        bzero(rx_buffer, sizeof(rx_buffer));
                    }

                    // Chống tràn bộ đệm tĩnh
                    if (rx_index >= sizeof(rx_buffer) - 1)
                    {
                        ESP_LOGE(TAG, "Loi: Chuoi LoRa qua dai khong co dau cham than!");
                        rx_index = 0;
                    }
                }
                break;

            case UART_FIFO_OVF:
            case UART_BUFFER_FULL:
                ESP_LOGW(TAG, "Loi tran bo dem UART. Dang reset...");
                uart_flush_input(UART_PORT_NUM);
                xQueueReset(uart_event_queue);
                break;

            default:
                break;
            }
        }
    }
    free(dtmp);
    dtmp = NULL;
    vTaskDelete(NULL);
}

/* ====================================================================
 * CÁC HÀM PUBLIC
 * ==================================================================== */
void UART_App_Init(void)
{
    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    uart_param_config(UART_PORT_NUM, &uart_config);
    uart_set_pin(UART_PORT_NUM, UART_TXD_PIN, UART_RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_PORT_NUM, UART_BUF_SIZE * 2, UART_BUF_SIZE * 2, 20, &uart_event_queue, 0);
    xTaskCreate(uart_event_task, "uart_event_task", 4096, NULL, 5, NULL);

    ESP_LOGI(TAG, "Da khoi tao UART1 LoRa o toc do 9600");
}

// Hàm gửi lệnh chung chung
void UART_Send_String(const char *str)
{
    uart_write_bytes(UART_PORT_NUM, str, strlen(str));
    ESP_LOGI(TAG, "Da gui chuoi len STM32: %s", str);
    LED_LoRa_Blink(); // Nháy đèn báo hiệu có lệnh mới được gửi xuống xe
}

// HÀM MỚI: Đóng gói và gửi lệnh điều khiển xuống Robot
void UART_Send_Robot_Command(int speed_L, int speed_R)
{
    char cmd_str[64];
    // Đóng gói cấu trúc: MANH_VIP$SET:150,150!
    sprintf(cmd_str, "%s$SET:%d,%d!", MY_TOKEN, speed_L, speed_R);

    // Gửi qua UART
    uart_write_bytes(UART_PORT_NUM, cmd_str, strlen(cmd_str));
    ESP_LOGI(TAG, "Da gui lenh xuong Robot: %s", cmd_str);
    LED_LoRa_Blink(); // Nháy đèn báo hiệu có lệnh điều khiển mới từ Web
}
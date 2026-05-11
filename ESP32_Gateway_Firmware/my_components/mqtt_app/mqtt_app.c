#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_log.h"
#include "mqtt_client.h"
#include "mqtt_app.h"
#include "uart_app.h" // Gọi thư viện UART để truyền lệnh cho LoRa
#include "led_app.h"  // Gọi thư viện LED để nháy đèn báo hiệu
#include <stdbool.h>
#include "config_mqtt.h"

static const char *TAG = "MQTT_APP";
static esp_mqtt_client_handle_t client = NULL;

// Định nghĩa các Topic (Đổi tên tùy theo đồ án của bạn)
#define TOPIC_TELEMETRY "manh_vip/robot/telemetry"  // Chiều Up: Xe -> Web
#define TOPIC_COMMAND   "manh_vip/robot/command"    // Chiều Down: Web -> Xe
// ==========================================
// BIẾN TOÀN CỤC CANH NHỊP LORA
// ==========================================
int pending_speed_L = 0;
int pending_speed_R = 0;
bool has_pending_command = false;

/* ====================================================================
 * HÀM XỬ LÝ SỰ KIỆN MQTT TỪ SERVER TRẢ VỀ
 * ==================================================================== */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;

    switch ((esp_mqtt_event_id_t)event_id) {
        // 1. KHI KẾT NỐI BROKER THÀNH CÔNG
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "Da ket noi thanh cong den Broker!");
            LED_MQTT_Set(true); // Bật đèn MQTT

            // Ngay lập tức đăng ký (Subscribe) lắng nghe lệnh điều khiển từ Web
            esp_mqtt_client_subscribe(client, MQTT_TOPIC_COMMAND, 0);
            ESP_LOGI(TAG, "Da subscribe vao topic: %s", MQTT_TOPIC_COMMAND);
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "Mat ket noi Broker MQTT!");
            LED_MQTT_Set(false); // Tắt đèn MQTT
            break;

        // 2. KHI CÓ TIN NHẮN TỪ WEB ĐẨY XUỐNG
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "Nhan duoc tin nhan moi:");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);

            // KIỂM TRA: Có đúng là lệnh điều khiển không?
            if (strncmp(event->topic, MQTT_TOPIC_COMMAND, event->topic_len) == 0) {
                
                // Giả sử Web gửi xuống chuỗi "150,150" (Tốc độ trái, phải)
                int speed_L = 0, speed_R = 0;
                char payload_str[32];
                snprintf(payload_str, sizeof(payload_str), "%.*s", event->data_len, event->data);
                
                // Cắt chuỗi lấy 2 số nguyên
                if (sscanf(payload_str, "%d,%d", &speed_L, &speed_R) == 2) {
                    ESP_LOGI(TAG, "Lenh hop le. Truyen lenh cho LoRa: L=%d, R=%d", speed_L, speed_R);
                    // GĂM LỆNH LẠI, CHỜ KHE HỞ LORA
                    pending_speed_L = speed_L;
                    pending_speed_R = speed_R;
                    has_pending_command = true;
                } else {
                    ESP_LOGW(TAG, "Sai dinh dang lenh MQTT (Goi y: 150,150): %s", payload_str);
                }
            }
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "Loi giao thuc MQTT!");
            break;
            
        default:
            break;
    }
}

/* ====================================================================
 * CÁC HÀM PUBLIC
 * ==================================================================== */
void MQTT_App_Init(void)
{
    // 4. TRUYỀN VÀO CÁC BIẾN CẤU HÌNH TỪ FILE CONFIG
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URI,
        .credentials.username = MQTT_USERNAME,
        .credentials.authentication.password = MQTT_PASSWORD,
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}
void MQTT_Publish_Telemetry(const char* payload)
{
    if (client != NULL) {
        // Hàm Publish: (client, topic, dữ_liệu, len, qos, retain)
        // Set len=0 thì nó tự động tính strlen(payload)
        int msg_id = esp_mqtt_client_publish(client, MQTT_TOPIC_TELEMETRY, payload, 0, 0, 0);
        ESP_LOGI(TAG, "Da Publish Data [%s], msg_id=%d", payload, msg_id);
    } else {
        ESP_LOGE(TAG, "Loi: MQTT Client chua khoi tao hoac mat ket noi!");
    }
}
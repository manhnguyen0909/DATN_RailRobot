# ESP32-C3 IoT Gateway - Railway Inspection Robot Project

## 📝 Giới thiệu
Đây là mã nguồn **Gateway** thuộc dự án **Robot kiểm tra khuyết tật ray đường sắt**. Gateway đóng vai trò là cầu nối truyền thông trung gian (Bridge), chuyển đổi dữ liệu từ mạng nội bộ không dây công suất thấp (**LoRa**) sang môi trường internet (**WiFi/MQTT**) để giám sát trên Web Dashboard.

Sử dụng vi điều khiển **ESP32-C3** (RISC-V) với ưu thế về kích thước nhỏ gọn và hỗ trợ bảo mật mạnh mẽ cho các ứng dụng IoT.

## 🚀 Chức năng chính
* **LoRa to MQTT Bridge:** Nhận bản tin Telemetry từ Robot (qua module LoRa giao tiếp UART) và đóng gói đẩy lên HiveMQ Cloud.
* **Command Downlink:** Lắng nghe lệnh điều khiển từ Web (MQTT Subscribe) và chuyển tiếp tức thì xuống Robot qua LoRa.
* **Edge Processing:** Xử lý lọc nhiễu dữ liệu thô, kiểm tra tính toàn vẹn của bản tin trước khi gửi lên Server.
* **Status Indication:** Hệ thống đèn LED báo hiệu trạng thái kết nối WiFi và trạng thái hoạt động của Broker MQTT.
* **Auto Reconnect:** Tự động khôi phục kết nối WiFi và MQTT khi gặp sự cố đường truyền.

## 🛠 Công nghệ sử dụng
* **Framework:** ESP-IDF (Espressif IoT Development Framework).
* **Giao thức truyền thông:**
    * **LoRa:** Truyền thông tầm xa giữa Robot và Gateway.
    * **MQTT (MQTTS):** Giao thức truyền tin gọn nhẹ qua SSL/TLS (Port 8883) đảm bảo an toàn dữ liệu.
* **Cloud Service:** HiveMQ Cloud (MQTT Broker).

## 📂 Cấu trúc thư mục
```text
ESP32_Gateway_Firmware/
├── .vscode/                   # Cấu hình VS Code (nếu có)
├── build/                     # Thư mục build của ESP-IDF
├── main/
│   ├── main.c                 # Khởi tạo hệ thống và điều phối Task
│   └── CMakeLists.txt         # Cấu hình build cho thư mục main
├── my_components/             # Các component tùy chỉnh
├── partitions.csv             # Bảng phân vùng flash
├── sdkconfig                  # Cấu hình ESP-IDF
├── sdkconfig.old              # Bản sao cấu hình cũ
├── CMakeLists.txt             # File cấu hình build project chính
└── README.md                  # Tài liệu hướng dẫn
```
## ⚙️ Cấu hình hệ thống
* Để bảo mật thông tin cá nhân, các thông số kết nối đã được tách ra khỏi mã nguồn chính.

* Tại thư mục main/, sao chép file config_mqtt_template.h và đổi tên thành config_mqtt.h.

* Mở config_mqtt.h và điền thông số Broker MQTT, WiFi SSID và Password của bạn.
## 🔌 Sơ đồ kết nối phần cứng
* Module LoRa (Ebyte/v.v): 
 
 TX -> ESP32-C3 RX (GPIO 20)

 RX -> ESP32-C3 TX (GPIO 21)
* LED Status: 

WiFi LED -> GPIO 7

MQTT LED -> GPIO 8

Lora LED -> GPIO 9
## Cách cài đặt và build

1. **Clone hoặc sao chép dự án**:
   ```bash
   cd ESP32_Gateway_Firmware
   ```

2. **Cấu hình ESP-IDF**:
   ```bash
   idf.py set-target esp32c3
   idf.py menuconfig
   ```

3. **Build dự án**:
   ```bash
   idf.py build
   ```

4. **Flash vào ESP32**:
   ```bash
   idf.py -p COM3 flash
   ```
   (Thay `COM3` bằng cổng serial của bạn)

5. **Xem log**:
   ```bash
   idf.py -p COM3 monitor
   ```
## Xử lý sự cố

- **Không kết nối WiFi**: Kiểm tra SSID/password, khoảng cách từ router
- **Không nhận dữ liệu LoRa**: Kiểm tra kết nối UART (TX/RX), baud rate, RST pin
- **Tín hiệu LoRa yếu**: Kiểm tra vị trí antenna, obstacles, spreading factor
- **OTA không hoạt động**: Đảm bảo ESP32-C3 có địa chỉ IP hợp lệ, server OTA đang chạy

## Đóng góp

Vui lòng tạo issue hoặc pull request cho các cải tiến.


## Liên hệ

[email: manhnguyen090903@gmail.com]

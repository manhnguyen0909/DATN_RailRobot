# ESP32-C3 Gateway Firmware

## Mô tả dự án

Đây là firmware cho cổng kết nối (Gateway) ESP32-C3 trong hệ thống robot tự động chạy trên đường ray. Thiết bị Gateway đóng vai trò trung gian giao tiếp giữa robot (STM32) và server/ứng dụng điều khiển thông qua WiFi, MQTT, và module LoRa.

## Tính năng chính

- **Kết nối WiFi**: Hỗ trợ WiFi Manager cho quản lý kết nối
- **MQTT**: Giao tiếp với MQTT broker để điều khiển robot từ xa
- **LoRa**: Giao tiếp với STM32 Robot thông qua module LoRa (SX1276/SX1278)
- **OTA (Over-The-Air)**: Cập nhật firmware từ xa thông qua server OTA
- **NVS Flash**: Lưu trữ cấu hình WiFi và dữ liệu quan trọng
- **LED Indicator**: Chỉ báo trạng thái hoạt động
- **FreeRTOS**: Hỗ trợ đa nhiệm

## Yêu cầu phần cứng

- **Vi điều khiển**: ESP32-C3 (lõi đơn, tiết kiệm năng lượng)
- **Module WiFi**: Tích hợp sẵn trong ESP32-C3
- **Module LoRa**: SX1276 hoặc SX1278 (kết nối qua Uart)
- **Kết nối ngoại vi**:
  - Uart: Kết nối tới module LoRa
  - GPIO: NSS (Chip Select), RST (Reset), DIO0-DIO4 (Interrupt pins)
  - LED: Chỉ báo trạng thái
  - Nguồn điện: Micro USB hoặc pin

## Yêu cầu phần mềm

- **ESP-IDF**: Phiên bản 4.4 trở lên
- **Python**: 3.7 trở lên (cho idf.py)
- **MQTT Broker**: (Mosquitto, HiveMQ, hoặc AWS IoT Core)

## Cấu trúc dự án

```
ESP32_Gateway_Firmware/
├── CMakeLists.txt              # Cấu hình CMake
├── main/
│   ├── CMakeLists.txt
│   └── main.c                  # Hàm main
├── my_components/              # Custom components
│   ├── wifi_manager/           # Quản lý WiFi
│   ├── mqtt_app/               # MQTT client
│   ├── lora_app/               # LoRa communication
│   ├── ota_server/             # OTA server
│   └── led_app/                # LED control
├── partitions.csv              # Bảng phân vùng flash
├── sdkconfig                   # Cấu hình ESP-IDF
└── README.md                   # Tệp này
```

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

## Cấu hình

### WiFi
- Chỉnh sửa SSID và password:
    ```bash
    idf.py menuconfig 
    ```
    -> Component config -> Wifi configuratione

### MQTT
- Cấu hình broker address, username, password trong `my_components/mqtt_app/`
- Các topic mặc định:
  - `robot/command`: Nhận lệnh điều khiển
  - `robot/status`: Gửi trạng thái robot
  - `robot/sensor`: Gửi dữ liệu cảm biến

### LoRa
- Cấu hình SPI pins, frequency, bandwidth trong `my_components/lora_app/`
- Mặc định:
  - Frequency: 433 MHz hoặc 868 MHz (tuỳ theo vùng)
  - Bandwidth: 125 kHz
  - Spreading Factor: 7-12
  - Coding Rate: 4/5 đến 4/8
- Mapping pins SPI:
  - MOSI: GPIO10
  - MISO: GPIO9
  - SCK: GPIO8
  - NSS: GPIO7
  - RST: GPIO6
  - DIO0: GPIO5

## Luồng hoạt động chính

1. Khởi tạo LED indicator
2. Khởi tạo NVS Flash để lưu cấu hình
3. Khởi tạo module LoRa SPI
4. Kết nối WiFi thông qua WiFi Manager
5. Khởi chạy OTA Server
6. Khởi tạo LoRa driver để giao tiếp với STM32
7. Khởi tạo MQTT client
8. Đợi lệnh điều khiển từ MQTT và gửi tới STM32 qua LoRa
9. Nhận dữ liệu từ STM32 qua LoRa và phát hành lên MQTT broker

## Giao tiếp giữa các thành phần

```
Internet (MQTT Broker)
        ↓
  ESP32-C3 Gateway
   (WiFi + LoRa)
        ↓
Module LoRa SPI
        ↓
   STM32 Robot
  (LoRa Module)
```

## Xử lý sự cố

- **Không kết nối WiFi**: Kiểm tra SSID/password, khoảng cách từ router
- **Không nhận dữ liệu LoRa**: Kiểm tra kết nối SPI, frequency, RST pin, antenna
- **Tín hiệu LoRa yếu**: Kiểm tra vị trí antenna, obstacles, spreading factor
- **OTA không hoạt động**: Đảm bảo ESP32-C3 có địa chỉ IP hợp lệ, server OTA đang chạy

## Đóng góp

Vui lòng tạo issue hoặc pull request cho các cải tiến.

## Giấy phép

Dự án này được phân phối dưới giấy phép MIT.

## Liên hệ

[Thông tin liên hệ của tác giả]

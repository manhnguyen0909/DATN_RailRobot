# DATN_RailRobot

## Tổng quan dự án

DATN_RailRobot là hệ thống robot tự hành kiểm tra khuyết tật đường ray tàu hỏa, kết hợp AI Vision và IoT. Dự án gồm nhiều thành phần phần cứng và phần mềm, phối hợp với nhau để giám sát đường ray, điều khiển robot, truyền dữ liệu và hiển thị kết quả.

## Các thành phần chính

1. `STM32_Robot_Firmware/`
   - Firmware cho robot chạy trên đường ray sử dụng vi điều khiển STM32F4.
   - Bao gồm điều khiển động cơ, cảm biến IMU (MPU6050), ToF (VL53L1X), ADC/DMA và FreeRTOS.

2. `ESP32_Gateway_Firmware/`
   - Firmware cho ESP32 Gateway chịu trách nhiệm kết nối IoT.
   - Thực hiện giao tiếp với mạng WiFi/MQTT và kết nối module LoRa hoặc UART tới robot.
   - Hỗ trợ OTA, lưu cấu hình NVS và đệm dữ liệu trước khi gửi lên server.

3. `RaspberryPi_AI_Vision/`
   - Ứng dụng AI Vision chạy trên Raspberry Pi để xử lý hình ảnh và phát hiện khuyết tật đường ray.
   - Bao gồm mô hình TensorFlow Lite `railway_model_pi3_3.tflite` và mã Python để phân tích ảnh.

4. `Web_Dashboard/`
   - Giao diện web để giám sát dữ liệu, trạng thái robot và kết quả kiểm tra.
   - Xây dựng bằng HTML/CSS/JavaScript với cấu hình server nhẹ.

## Mục tiêu chính

- Phát hiện và báo cáo khuyết tật trên đường ray bằng AI Vision.
- Điều khiển robot tự hành di chuyển và khảo sát đường ray.
- Kết nối dữ liệu từ robot tới hệ thống đám mây qua IoT.
- Hiển thị kết quả kiểm tra và trạng thái hệ thống trên dashboard web.

## Cấu trúc thư mục

```
DATN_RailRobot/
├── ESP32_Gateway_Firmware/      # Firmware IoT cho ESP32
│   ├── CMakeLists.txt
│   ├── main/
│   ├── my_components/
│   ├── partitions.csv
│   ├── sdkconfig
│   └── README.md
├── RaspberryPi_AI_Vision/       # AI Vision trên Raspberry Pi
│   ├── railway_model_pi3_3.tflite
│   ├── Rasp_Pi_fn.py
│   ├── requirements.txt
│   └── README.md
├── STM32_Robot_Firmware/        # Firmware cho robot STM32
│   ├── README.md
│   └── STM32_Robot_Firmware/
│       ├── Core/
│       ├── Drivers/
│       ├── MDK-ARM/
│       ├── Middlewares/
│       └── STM32_Robot_Firmware.ioc
├── Web_Dashboard/               # Dashboard web giám sát
│   ├── app.js
│   ├── config.js
│   ├── config_template.js
│   ├── index.html
│   ├── README.md
│   └── style.css
└── README.md                    # Tài liệu dự án tổng quát
```

## Hướng dẫn nhanh

### 1. STM32 Robot Firmware

- Mở project Keil trong `STM32_Robot_Firmware/STM32_Robot_Firmware/MDK-ARM/STM32_Robot_Firmware.uvprojx`
- Build và flash bằng Keil MDK-ARM.
- Firmware này điều khiển robot, đọc cảm biến và giao tiếp với gateway.

### 2. ESP32 Gateway Firmware

- Vào thư mục `ESP32_Gateway_Firmware`
- Cấu hình ESP-IDF target, build và flash:
  ```bash
  idf.py set-target esp32c3
  idf.py menuconfig
  idf.py build
  idf.py -p COMx flash
  ```
- Cấu hình WiFi/MQTT và LoRa/UART theo điều kiện phần cứng.

### 3. Raspberry Pi AI Vision

- Cài đặt dependencies:
  ```bash
  pip install -r requirements.txt
  ```
- Chạy ứng dụng Python để kiểm tra ảnh và phát hiện khuyết tật.
- Mô hình AI được lưu trong `railway_model_pi3_3.tflite`.

### 4. Web Dashboard

- Mở `Web_Dashboard/index.html` trong trình duyệt hoặc chạy server nếu cần.
- Cấu hình `config.js`/`config_template.js` để kết nối tới nguồn dữ liệu MQTT hoặc API.

## Kiểm tra và phát triển

- Đảm bảo từng module hoạt động độc lập trước khi tích hợp.
- Kiểm tra kết nối giữa ESP32 Gateway và STM32 qua module LoRa/UART.
- Kiểm tra khả năng nhận diện khuyết tật từ Raspberry Pi AI Vision bằng tập ảnh mẫu.
- Kiểm tra dashboard hiển thị trạng thái và dữ liệu đúng.


## Liên hệ

Thông tin liên hệ tác giả 
* Mạnh Nguyễn
* email: manhnguyen090903@gmail.com
# STM32 Railway Inspection Robot Firmware

## 📝 Giới thiệu
Đây là mã nguồn điều khiển trung tâm (Firmware) cho hệ thống **Robot tự hành kiểm tra khuyết tật đường sắt**. Dự án tích hợp các công nghệ Nhúng, IoT và Thị giác máy tính để tự động hóa quá trình giám sát hạ tầng đường sắt.

Hệ thống sử dụng vi điều khiển **STM32F401** làm bộ não chính, vận hành trên hệ điều hành thời gian thực (**FreeRTOS**) để đảm bảo tính chính xác và ổn định trong việc điều khiển động cơ và thu thập dữ liệu đa cảm biến.

## 🚀 Tính năng chính
- **Real-time Multitasking:** Sử dụng FreeRTOS để quản lý đồng thời các tác vụ: Điều khiển (Motor Control), Thu thập dữ liệu (Sensor Acquisition) và Giao tiếp (Communication).
- **Motion Control:** Thuật toán **PID** kết hợp dữ liệu từ **Encoder** để kiểm soát chính xác tốc độ và quỹ đạo di chuyển của Robot.
- **Multi-Sensor Fusion:**
    - **IMU (MPU6050):** Giám sát góc nghiêng và độ ổn định của ray.
    - **ToF (VL53L1X):** Đo khoảng cách khẩu độ ray với độ chính xác milimet.
    - **GPS:** Định vị tọa độ thực của các vị trí phát hiện khuyết tật.
- **Giao tiếp thông minh:**
    - Truyền dữ liệu không dây tầm xa qua **LoRa** về Gateway.
    - Đồng bộ hóa dữ liệu UART tốc độ cao với **Raspberry Pi (Edge AI)** để nhận diện khuyết tật bề mặt bằng mô hình CNN.
- **Hardware Protection:** Cơ chế tự động khôi phục Bus I2C (Self-healing) và giám sát điện áp Pin (ADC) để bảo vệ hệ thống.

## 🏗 Kiến trúc hệ thống
Hệ thống được chia thành 3 Task chính:
1. **Motor Task (Priority: High):** Xử lý PID và xuất xung PWM (100ms/chu kỳ).
2. **Sensor Task (Priority: Low):** Đọc ADC, MPU6050, và VL53L1X.
3. **Comm Task (Priority: Normal):** Xử lý giao thức UART (LoRa/Raspberry Pi/ GPS) và giải mã lệnh điều khiển.

## 🛠 Công nghệ sử dụng
- **Ngôn ngữ:** C .
- **Framework:** STM32 HAL Driver.
- **RTOS:** FreeRTOS (CMSIS-RTOS V1).
- **Công cụ cấu hình:** STM32CubeMX.
- **IDE:** Keil MDK-ARM V5.

## 📂 Cấu trúc dự án
```text
STM32_Robot_Firmware/
├── STM32_Robot_Firmware.ioc    # File cấu hình STM32CubeMX
├── Core/
│   ├── Inc/                    # File Header cấu hình và Drivers cảm biến
│   └── Src/                    
│       ├── main.c              # Khởi tạo phần cứng và entry point
│       ├── freertos.c          # Logic chi tiết của các Task RTOS
│       └── usart.c             # Xử lý ngắt UART/DMA cho GPS & LoRa
├── Drivers/                    # Thư viện HAL và CMSIS từ ST
├── MDK-ARM/                    # Project file cho Keil uVision
└── Middlewares/                # Kernel FreeRTOS
```
## ⚙️ Cài đặt và Bảo mật
* Để bảo mật các thông tin định danh thiết bị trong mạng LoRa/IoT:

1. Tìm file Core/Inc/secrets_template.h.

2. Sao chép và đổi tên thành secrets.h.

3. Cấu hình ROBOT_TOKEN riêng biệt của bạn bên trong file này.


## 🔨 Cách Build và Flash
1. Mở file MDK-ARM/STM32_Robot_Firmware.uvprojx bằng Keil uVision.

2. Nhấn F7 (Build) để biên dịch mã nguồn.

3. Kết nối mạch nạp ST-Link và nhấn F8 (Download) để nạp firmware vào vi điều khiển.

## Đóng góp

Vui lòng tạo issue hoặc pull request cho các cải tiến.


## Liên hệ

[email: manhnguyen090903@gmail.com]

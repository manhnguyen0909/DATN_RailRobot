# STM32 Robot Firmware

## Mô tả dự án

Đây là firmware cho robot chạy trên đường ray sử dụng vi điều khiển STM32F4. Dự án này được phát triển như một phần của đề tài nghiên cứu khoa học (ĐATN) về robot tự động.

## Tính năng chính

- Điều khiển động cơ và servo
- Đọc dữ liệu từ cảm biến IMU (MPU6050)
- Đo khoảng cách bằng cảm biến ToF (VL53L1X)
- Giao tiếp UART và I2C
- Hỗ trợ FreeRTOS cho đa nhiệm
- Cấu hình ADC và DMA cho xử lý tín hiệu

## Yêu cầu phần cứng

- Vi điều khiển: STM32F401RCTx
- Cảm biến: MPU6050 (IMU), VL53L1X (Time-of-Flight)
- Động cơ servo và DC motor
- Module giao tiếp (UART, I2C)

## Yêu cầu phần mềm

- Keil MDK-ARM (phiên bản 5.0 trở lên)
- STM32CubeMX (cho cấu hình IOC)
- STM32 HAL Driver
- FreeRTOS

## Cấu trúc dự án

```
STM32_Robot_Firmware/
├── STM32_Robot_Firmware.ioc          # Cấu hình STM32CubeMX
├── Core/
│   ├── Inc/                          # Header files
│   └── Src/                          # Source files
├── Drivers/
│   ├── CMSIS/                        # ARM CMSIS
│   └── STM32F4xx_HAL_Driver/          # STM32 HAL Driver
├── MDK-ARM/                          # Keil project files
└── Middlewares/
    └── Third_Party/
        └── FreeRTOS/                  # FreeRTOS kernel
```

## Cách build và flash

1. Mở project trong Keil MDK-ARM:
   - File: `MDK-ARM/STM32_Robot_Firmware.uvprojx`

2. Build project:
   - Project > Build target

3. Flash firmware:
   - Flash > Download (hoặc sử dụng ST-Link)

## Cách sử dụng

1. Cấu hình các chân GPIO và ngoại vi trong STM32CubeMX
2. Khởi tạo cảm biến trong `main.c`
3. Chạy các task FreeRTOS cho điều khiển robot
4. Giám sát qua Uart/ Module Lora

## Cấu hình

- cấu hình ROBOT_TOKEN cho LoraWan của bạn ở secret_template.h, đổi tên secret_template.h thành secret.h

## Đóng góp

Vui lòng tạo issue hoặc pull request cho các cải tiến.


## Liên hệ

[email: manhnguyen090903@gmail.com]
# Raspberry Pi Edge AI Vision

Thư mục này chứa mã nguồn xử lý ảnh tại biên (Edge Computing) chạy trên Raspberry Pi 3. Hệ thống sử dụng Camera để thu thập hình ảnh đường ray và chạy mô hình mạng nơ-ron tích chập (CNN - TensorFlow Lite) để phân loại 3 trạng thái: Bình thường, Nứt (Crack), Lún (Squats).
## 📂 Cấu trúc thư mục
```text
RaspberryPi_AI_Vision/
├── Rasp_Pi_fn.py       
├── railway_model_pi3_3.tflite          
├── requirements.txt            
└── README.md   
```
## Yêu cầu phần cứng
- Máy tính nhúng: Raspberry Pi 3 Model B (hoặc mới hơn).
- Camera: Raspberry Pi Camera Module / USB Webcam.
- Kết nối: Nối chéo chân UART (TX/RX) từ Pi sang STM32.

## Cài đặt và Khởi chạy
1. Cài đặt các thư viện phụ thuộc:
   ```bash
   pip install -r requirements.txt
   ```
2. Chạy chương trình chính:
   ```bash
   python3 Rasp_Pi_fn.py
   ```
3. Xem video stream trực tiếp qua luồng Flask:
* Truy cập http://IP_CUA_PI:5000 trên trình duyệt.
# -*- coding: utf-8 -*-
import cv2
import numpy as np
import threading
import os
import serial
import time
from flask import Flask, Response

try:
    from tflite_runtime.interpreter import Interpreter
except ImportError:
    from tensorflow.lite.python.interpreter import Interpreter

app = Flask(__name__)

# --- CẤU HÌNH ---
MODEL_PATH = 'railway_model_pi3_3.tflite'
CLASS_NAMES = ['crack', 'railway', 'squats']
SKIP_AI_FRAMES = 3

# --- CẤU HÌNH UART ---
UART_PORT = '/dev/serial0'
BAUD_RATE = 9600

try:
    uart = serial.Serial(port=UART_PORT, baudrate=BAUD_RATE, timeout=1)
    print(f"Mở cổng UART {UART_PORT} thành công!")
except Exception as e:
    print(f"Lỗi mở cổng UART: {e}")
    uart = None

# Khởi tạo TFLite
interpreter = Interpreter(model_path=MODEL_PATH)
interpreter.allocate_tensors()
input_details = interpreter.get_input_details()
output_details = interpreter.get_output_details()
_, img_height, img_width, _ = input_details[0]['shape']

# Biến dùng chung
lock = threading.Lock()
raw_frame = None
display_frame = None
last_ai_result = {"label": "Scanning...", "color": (0, 255, 0), "roi": (80, 240)}


# --- HÀM TÌM MÉP RAY ĐỘNG (KHÔNG THU HẸP) ---
def get_dynamic_roi(frame):
    # Resize ảnh nhỏ lại để xử lý Canny nhanh hơn
    small = cv2.resize(frame, (160, 120))
    gray = cv2.cvtColor(small, cv2.COLOR_BGR2GRAY)
    edges = cv2.Canny(gray, 50, 150)

    # Chiếu dọc để tìm mép ray
    col_sums = np.sum(edges, axis=0)
    mid = 80
    x1_s = np.argmax(col_sums[:mid])
    x2_s = mid + np.argmax(col_sums[mid:])

    width_s = x2_s - x1_s
    # Nếu không tìm thấy mép ray hợp lý, trả về giá trị mặc định an toàn
    if width_s < 40 or width_s > 120:
        return 80, 240

    # Lấy toàn bộ vùng mặt ray đã tìm được (nhân 2 để đưa về kích thước frame gốc 320x240)
    return int(x1_s * 2), int(x2_s * 2)
# --- LUỒNG AI (CHẠY NGẦM) ---
def ai_worker():
    global raw_frame, last_ai_result
    frame_count = 0
    last_uart_send_time = 0
    last_sent_class = None  # Lưu trữ loại lỗi đã gửi gần nhất

    while True:
        if raw_frame is not None:
            frame_count += 1
            if frame_count % SKIP_AI_FRAMES == 0:
                # 1. Tìm mép ray
                x1, x2 = get_dynamic_roi(raw_frame)
                roi = raw_frame[:, x1:x2]

                # 2. Inference
                img_input = cv2.resize(cv2.cvtColor(roi, cv2.COLOR_BGR2RGB), (img_width, img_height))
                input_data = np.expand_dims(img_input, axis=0).astype(np.float32)

                interpreter.set_tensor(input_details[0]['index'], input_data)
                interpreter.invoke()
                preds = interpreter.get_tensor(output_details[0]['index'])[0]

                idx = np.argmax(preds)
                conf = preds[idx] * 100
                detected_class = CLASS_NAMES[idx]

                # 3. Lưu kết quả hiển thị
                last_ai_result = {
                    "label": f"{detected_class.upper()} {conf:.1f}%",
                    "color": (0, 255, 0) if detected_class == 'railway' else (0, 0, 255),
                    "roi": (x1, x2)
                }

                current_time = time.time()

                # 4. LOGIC GỬI UART THÔNG MINH

                current_time = time.time()

                # TRƯỜNG HỢP 1: ĐƯỜNG RAY BÌNH THƯỜNG (RAILWAY)
                if detected_class == 'railway':
                    if conf >= 95.0:
                        # Khi gặp Railway tin cậy cao, reset bộ nhớ lỗi
                        last_sent_class = None

                        if (current_time - last_uart_send_time) >= 2.0:
                            # Đổi định dạng thành: NORMAL 98.1%
                            uart_data = f"NORMAL {conf:.1f}%\n"

                            if uart and uart.is_open:
                                try:
                                    uart.write(uart_data.encode('utf-8'))
                                    last_uart_send_time = current_time
                                    print(f"[UART] {uart_data.strip()}")
                                except:
                                    pass

                # TRƯỜNG HỢP 2: PHÁT HIỆN LỖI (CRACK, SQUATS)
                else:
                    if conf >= 50.0:
                        # CHỈ GỬI NẾU LÀ LOẠI LỖI MỚI
                        if detected_class != last_sent_class:
                            # Đổi định dạng thành: CRACK 75.5% hoặc SQUATS 82.0%
                            uart_data = f"{detected_class.upper()} {conf:.1f}%\n"

                            if uart and uart.is_open:
                                try:
                                    uart.write(uart_data.encode('utf-8'))
                                    last_sent_class = detected_class  # Ghi nhớ lỗi
                                    last_uart_send_time = current_time
                                    print(f"[UART ALARM] {uart_data.strip()}")
                                except:
                                    pass


# --- LUỒNG CAMERA & VẼ ---
def capture_and_draw():
    global raw_frame, display_frame
    cap = cv2.VideoCapture(0, cv2.CAP_V4L2)
    cap.set(cv2.CAP_PROP_FRAME_WIDTH, 320)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 240)

    while True:
        ret, frame = cap.read()
        if not ret: continue
        raw_frame = frame.copy()

        res = last_ai_result
        x1, x2 = res["roi"]
        label = res["label"]
        color = res["color"]

        # 1. TIÊU ĐỀ
        cv2.rectangle(frame, (0, 0), (320, 30), (40, 40, 40), -1)
        cv2.putText(frame, "RAILWAY DEFECTS MONITORING", (10, 20),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.45, (255, 255, 255), 1)

        # 2. VẼ VÙNG NHẬN DIỆN (Kéo dài dọc)
        cv2.rectangle(frame, (x1, 30), (x2, 240), color, 2)
        cv2.rectangle(frame, (x1, 30), (x1 + 130, 52), (0, 0, 0), -1)
        cv2.putText(frame, label, (x1 + 5, 47),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.4, color, 1)

        # 3. TRẠNG THÁI UART
        cv2.rectangle(frame, (0, 215), (90, 240), (0, 0, 0), -1)
        uart_status = "UART: OK" if (uart and uart.is_open) else "UART: ERR"
        st_color = (0, 255, 0) if "OK" in uart_status else (0, 0, 255)
        cv2.putText(frame, uart_status, (5, 232), cv2.FONT_HERSHEY_SIMPLEX, 0.35, st_color, 1)

        # 4. CẢNH BÁO ALARM TRÊN MÀN HÌNH
        if "RAILWAY" not in label and "Scanning" not in label:
            cv2.rectangle(frame, (0, 30), (320, 240), (0, 0, 255), 2)
            cv2.putText(frame, "ALARM!", (250, 20), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 0, 255), 2)

        with lock:
            display_frame = frame.copy()


# --- WEB STREAMING ---
def generate():
    global display_frame
    while True:
        if display_frame is None: continue
        res, encoded = cv2.imencode(".jpg", display_frame, [int(cv2.IMWRITE_JPEG_QUALITY), 70])
        yield (b'--frame\r\n' b'Content-Type: image/jpeg\r\n\r\n' + bytearray(encoded) + b'\r\n')


@app.route("/")
def index():
    return Response(generate(), mimetype="multipart/x-mixed-replace; boundary=frame")


if __name__ == '__main__':
    threading.Thread(target=ai_worker, daemon=True).start()
    threading.Thread(target=capture_and_draw, daemon=True).start()
    print("Hệ thống RAIL-BÁM-RAY v2.0 đang chạy...")
    app.run(host='0.0.0.0', port=5000, debug=False, threaded=True)
# Web SCADA Dashboard - Rail-Bot Monitor

## 📝 Giới thiệu
Đây là giao diện điều khiển và giám sát trung tâm (SCADA) thuộc dự án **Robot kiểm tra khuyết tật ray đường sắt**. Web Dashboard được thiết kế để cung cấp cái nhìn toàn diện về trạng thái vận hành của robot thông qua các biểu đồ thời gian thực, bản đồ định vị và hệ thống cảnh báo thông minh.

Giao diện được tối ưu hóa để hiển thị dữ liệu từ trạm Gateway ESP32 truyền lên qua giao thức **MQTT (WebSockets)**.

## 🚀 Tính năng chính
* **Real-time Telemetry:** Hiển thị tức thời các thông số: Khoảng cách khẩu độ ray (ToF), Góc nghiêng (IMU), Tốc độ động cơ (RPM) và dung lượng Pin.
* **Live Mapping:** Tích hợp bản đồ **Leaflet** để theo dõi lộ trình di chuyển của robot thông qua dữ liệu GPS.
* **Dynamic Charting:** Sử dụng **Chart.js** để vẽ biểu đồ trực quan hóa sự biến động của cảm biến, giúp dễ dàng nhận diện các đoạn ray bất thường.
* **Remote Control:** Giao diện điều khiển từ xa cho phép gửi lệnh thiết lập tốc độ (Command Downlink) trực tiếp xuống robot.
* **Smart Logging & Alerts:** Hệ thống nhật ký (Log) lưu trữ lịch sử vận hành và tự động đưa ra cảnh báo (Toast notifications) khi các thông số vượt ngưỡng an toàn.

## 🛠 Công nghệ sử dụng
* **Frontend:** HTML5, CSS3 (Modern UI/UX), JavaScript (ES6+).
* **Libraries:**
    * **MQTT.js:** Kết nối và xử lý giao thức WebSockets MQTT.
    * **Chart.js:** Hiển thị biểu đồ dữ liệu thời gian thực.
    * **Leaflet.js:** Bản đồ số theo dõi tọa độ GPS.
* **Communication:** Giao thức WSS (WebSockets Secure) qua Port 8884.

## 📂 Cấu trúc thư mục
```text
Web_SCADA_Dashboard/
├── css/
│   └── style.css          # Giao diện chính và hiệu ứng Dashboard
├── js/
│   ├── app.js             # Logic xử lý dữ liệu và điều khiển MQTT
│   ├── config_template.js # File mẫu cấu hình Broker
│   └── config.js          # File cấu hình thật (Chứa mật khẩu - Bị bỏ qua bởi Git)
├── index.html             # Cấu trúc giao diện Web
└── README.md              # Tài liệu hướng dẫn
```
## ⚙️ Cấu hình và Khởi chạy
Để đảm bảo an toàn cho các thông tin đăng nhập Broker, hãy thực hiện các bước sau:

1. Tại thư mục js/, sao chép file config_template.js thành config.js.

2. Mở config.js và điền thông số Broker URL, Username và Password của bạn.

```text
// Ví dụ cấu hình trong config.js
const APP_CONFIG = {
    MQTT: {
        BROKER_URL: "wss://your-id.s1.eu.hivemq.cloud:8884/mqtt",
        USERNAME: "NguyenManh",
        PASSWORD: "Your_Password"
    }
};
```
🖥 Cách sử dụng
Mở file index.html bằng bất kỳ trình duyệt hiện đại nào (Chrome, Edge, Firefox).

1. Nhập các thông số cấu hình MQTT (nếu chưa điền vào file config) và nhấn Connect.

2. Trạng thái kết nối sẽ được báo hiệu qua đèn LED ảo trên giao diện.

3. Dữ liệu từ Robot sẽ tự động cập nhật lên các ô thông số và biểu đồ.
## Đóng góp

Vui lòng tạo issue hoặc pull request cho các cải tiến.


## Liên hệ

[email: manhnguyen090903@gmail.com]

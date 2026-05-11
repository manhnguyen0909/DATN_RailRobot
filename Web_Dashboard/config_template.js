// HƯỚNG DẪN: Đổi tên file này thành config.js và điền thông tin của bạn vào
// CẢNH BÁO: Tuyệt đối không push file config.js (đã điền mật khẩu) lên GitHub!

const APP_CONFIG = {
    MQTT: {
        BROKER_URL: "wss://<your-id>.s1.eu.hivemq.cloud:8884/mqtt",
        USERNAME: "YOUR_USERNAME",
        PASSWORD: "YOUR_PASSWORD",
        TOPIC_TELEMETRY: "your_topic/robot/telemetry",
        TOPIC_COMMAND: "your_topic/robot/command"
    },
    THRESHOLDS: {
        TOF: 50,
        IMU: 15,
        BAT_LOW: 20
    }
};
// HƯỚNG DẪN: Đổi tên file này thành config_mqtt.h và điền thông tin của bạn vào

#ifndef CONFIG_MQTT_H
#define CONFIG_MQTT_H

/* ====================================================================
 * CẤU HÌNH KẾT NỐI BROKER MQTT
 * ==================================================================== */
#define MQTT_BROKER_URI "MQTT_BROKER_URI:8883"
#define MQTT_USERNAME   "YOUR_MQTT_USERNAME"
#define MQTT_PASSWORD   "YOUR_MQTT_PASSWORD"

/* ====================================================================
 * ĐỊNH NGHĨA CÁC TOPIC GIAO TIẾP CHO ROBOT
 * ==================================================================== */
#define MQTT_TOPIC_TELEMETRY "your_topic/robot/telemetry"
#define MQTT_TOPIC_COMMAND   "your_topic/robot/command"

#endif // CONFIG_MQTT_H
#ifndef CONFIG_MQTT_H
#define CONFIG_MQTT_H

/* ====================================================================
 * CẤU HÌNH KẾT NỐI BROKER MQTT (HIVEMQ CLOUD)
 * ==================================================================== */
#define MQTT_BROKER_URI "mqtts://fd553ba641bf43729bad8a7af8400930.s1.eu.hivemq.cloud:8883"
#define MQTT_USERNAME   "NguyenManh"
#define MQTT_PASSWORD   "Manh09092003"

/* ====================================================================
 * ĐỊNH NGHĨA CÁC TOPIC GIAO TIẾP CHO ROBOT
 * ==================================================================== */
#define MQTT_TOPIC_TELEMETRY "manh_vip/robot/telemetry"  // Chiều Up: Xe -> Web
#define MQTT_TOPIC_COMMAND   "manh_vip/robot/command"    // Chiều Down: Web -> Xe

#endif // CONFIG_MQTT_H
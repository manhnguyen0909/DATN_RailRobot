#ifndef MQTT_APP_H
#define MQTT_APP_H
#include <stdbool.h>
// Báo cho các file khác biết sự tồn tại của 3 biến này
extern int pending_speed_L;
extern int pending_speed_R;
extern bool has_pending_command;
void MQTT_App_Init(void);

// Hàm publish dữ liệu của xe lên Web
void MQTT_Publish_Telemetry(const char* payload);

#endif // MQTT_APP_H
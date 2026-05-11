idf_component_register(SRCS "mqtt_app.c"
                       INCLUDE_DIRS "include"
                       REQUIRES mqtt esp_timer esp_netif 
                        wifi_manager nvs_flash esp_http_client uart_app led_app
                       )
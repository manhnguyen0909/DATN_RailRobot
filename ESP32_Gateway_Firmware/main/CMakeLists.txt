idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES 
                        wifi_manager
                        nvs_flash
                        ota_server
                        esp_http_client 
                        uart_app
                        mqtt_app
                        led_app
                    )

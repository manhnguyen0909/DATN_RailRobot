idf_component_register(SRCS "uart_app.c"
                       INCLUDE_DIRS "include"
                       REQUIRES driver log freertos mqtt_app led_app)
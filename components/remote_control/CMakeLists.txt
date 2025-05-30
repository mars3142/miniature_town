idf_component_register(SRCS 
                        "capability_service.c"
                        "device_service.c"
                        "led_service.c"
                        "remote_control.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES
                        bt
                        esp_app_format
                        led_matrix
)

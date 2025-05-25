idf_component_register(SRCS "led_service.c" "device_service.c" "remote_control.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES
                        bt
                        led_matrix
)

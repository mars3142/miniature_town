idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES
                        led_matrix
                        remote_control
                        nvs_flash
)

idf_component_register(SRCS "remote_control.c"
                    INCLUDE_DIRS "include"
                    REQUIRES
                        bt
                        esp_timer
                        nvs_flash
)

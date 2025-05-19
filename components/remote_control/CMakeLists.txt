idf_component_register(SRCS "remote_control.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES
                        bt
                        nvs_flash
)

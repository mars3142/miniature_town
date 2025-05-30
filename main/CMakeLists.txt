idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES
                        led_matrix
                        remote_control
                        persistence
)
spiffs_create_partition_image(storage ../data FLASH_IN_PROJECT)

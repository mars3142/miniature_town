idf_component_register(SRCS 
                    main.cpp
                INCLUDE_DIRS "."
                PRIV_REQUIRES
                    driver
                    esp_timer
                    led_strip
                    led_matrix
                    remote_control
                    persistence
)
spiffs_create_partition_image(storage ../data FLASH_IN_PROJECT)

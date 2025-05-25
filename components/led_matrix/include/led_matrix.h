#pragma once

#include <stdint.h>

void led_matrix_init(void *args);
uint32_t led_matrix_get_size();
void led_matrix_set_pixel(uint32_t index, uint8_t red, uint8_t green, uint8_t blue);

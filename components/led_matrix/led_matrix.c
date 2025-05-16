#include <stdio.h>
#include "led_matrix.h"

#include "led_strip.h"

#define LED_STRIP_RMT_RES_HZ (10 * 1000 * 1000)

led_strip_handle_t led_strip_init(uint8_t gpio_pin, uint32_t width, uint32_t height)
{
    led_strip_config_t strip_config = {
        .strip_gpio_num = gpio_pin,
        .max_leds = width * height,
        .led_model = LED_MODEL_WS2812,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_RGB,
        .flags = {
            .invert_out = false,
        }};

    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = LED_STRIP_RMT_RES_HZ,
        .mem_block_symbols = 64,
        .flags = {
            .with_dma = true,
        }};

    led_strip_handle_t led_strip;
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    return led_strip;
}

void init_led(void)
{
    led_strip_handle_t led = led_strip_init(14, 8, 8);
    // led_strip_set_pixel(led, 0, 255, 0, 0);
    // led_strip_set_pixel(led, 1, 0, 255, 0);
    // led_strip_set_pixel(led, 2, 0, 0, 255);
    led_strip_refresh(led);
}

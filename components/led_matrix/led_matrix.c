#include "led_matrix.h"

#include "freertos/FreeRTOS.h"
#include "led_strip.h"
#include "esp_log.h"
#include "sdkconfig.h"

typedef struct
{
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} led_data_t;

typedef struct
{
    led_strip_handle_t led_strip;
    led_data_t *data;
    uint32_t size;
} led_matrix_t;

led_matrix_t led_matrix;

static void led_strip_init(uint8_t gpio_pin, uint32_t size)
{
    led_matrix.size = size;
    led_matrix.data = (led_data_t *)malloc(sizeof(led_data_t) * size);

    led_strip_config_t strip_config = {
        .strip_gpio_num = gpio_pin,
        .max_leds = size,
        .led_model = LED_MODEL_WS2812,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_RGB,
        .flags = {
            .invert_out = false,
        }};

    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 0,
        .mem_block_symbols = 0,
        .flags = {
            .with_dma = true,
        }};

    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_matrix.led_strip));
}

void led_matrix_init(void *args)
{
    ESP_LOGI(pcTaskGetName(NULL), "Calling led_matrix_init()");

    led_strip_init(CONFIG_WLED_DIN_PIN, CONFIG_WLED_LED_COUNT);
    int value = 0;
    for (int i = 0; i < led_matrix.size; i++)
    {
        led_strip_set_pixel(led_matrix.led_strip, i, value, 0, 0);
    }
    led_strip_refresh(led_matrix.led_strip);

    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    free(led_matrix.data);

    ESP_LOGI(pcTaskGetName(NULL), "Exiting led_matrix_init()");
    vTaskDelete(NULL);
}

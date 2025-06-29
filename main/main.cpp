#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_timer.h" // For a high-resolution timer
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led_strip.h" // The ESP-IDF LED strip component
#include <stdio.h>
#include <string.h>

// --- Adaptation of the FastLED CRGB structure and helper functions ---
// Since we are not directly using FastLED, we need to define CRGB and Blend ourselves.
// Alternatively, we can use esp_color_rgb_t and convert. Here's an adapted way.

typedef struct
{
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} CRGB;

// Simple color blending function (linear interpolation)
CRGB blend(CRGB c1, CRGB c2, uint8_t ratio)
{
    return (CRGB){.red = (uint8_t)(((uint16_t)c1.red * (255 - ratio) + (uint16_t)c2.red * ratio) / 255),
                  .green = (uint8_t)(((uint16_t)c1.green * (255 - ratio) + (uint16_t)c2.green * ratio) / 255),
                  .blue = (uint8_t)(((uint16_t)c1.blue * (255 - ratio) + (uint16_t)c2.blue * ratio) / 255)};
}

// Definitions for colors (similar to FastLED::CRGB)
const CRGB CRGB_Black = {0, 0, 0};
const CRGB CRGB_White = {255, 255, 255};
// Add more colors as needed
// --- End of FastLED CRGB Adaptation ---

static const char *TAG = "MiWuLa_LED"; // For ESP_LOG

#define LED_PIN GPIO_NUM_14 // Choose a GPIO pin (e.g., GPIO27 for ESP32-DevKitC)
#define NUM_LEDS 64         // Number of LEDs in the strip
#define MAX_BRIGHTNESS 100

// Duration of each phase in microseconds (approximate values, based on MiWuLa)
#define DAY_DURATION_US (3 * 60 * 1000000ULL) // original 11
#define SUNSET_DURATION_US (1 * 60 * 1000000ULL)
#define NIGHT_DURATION_US (2 * 60 * 1000000ULL)
#define SUNRISE_DURATION_US (1 * 60 * 1000000ULL)

// Total cycle duration in microseconds (original 15 minutes)
#define CYCLE_DURATION_US (DAY_DURATION_US + SUNSET_DURATION_US + NIGHT_DURATION_US + SUNRISE_DURATION_US)

led_strip_handle_t led_strip_handle; // Handle for the LED strip

CRGB leds[NUM_LEDS]; // Local buffer for LED colors

// For simulating interior lighting
const int NUM_HOUSES = 8;     // Number of "houses" with individual lighting
int houseLEDs[NUM_HOUSES];    // Start LED index for each house
CRGB houseColors[NUM_HOUSES]; // Color for each house (warm white/yellowish)

// Function prototypes
void set_all_leds(CRGB color);
void set_daylight(uint8_t brightness);
void set_nightlight();
void control_house_lights(bool turn_on, float dim_progress);
void flash_effect();
void led_control_task(void *pvParameters);

extern "C" void app_main()
{
    // app_main is the entry point in ESP-IDF
    ESP_LOGI(TAG, "Initializing LED strip");

    // LED strip configuration
    led_strip_config_t strip_config = {.strip_gpio_num = LED_PIN,
                                       .max_leds = NUM_LEDS,
                                       .led_model = LED_MODEL_WS2812,
                                       .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_RGB,
                                       .flags = {.invert_out = false}};
    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT, .resolution_hz = 0, .mem_block_symbols = 0, .flags = {.with_dma = true}};

    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip_handle));
    ESP_LOGI(TAG, "LED strip initialized");

    // Random assignment of LEDs to "houses"
    // Note: random() is not thread-safe in FreeRTOS. Use esp_random() for better randomness
    srand(esp_random()); // Seed for rand()

    for (int i = 0; i < NUM_HOUSES; i++)
    {
        houseLEDs[i] = rand() % NUM_LEDS; // Random LED as "house light"
        houseColors[i] = (CRGB){(uint8_t)(180 + rand() % 76), (uint8_t)(150 + rand() % 106),
                                (uint8_t)(50 + rand() % 51)}; // Warm white/yellow
    }

    // Create a FreeRTOS task to control the LED cycle
    xTaskCreate(led_control_task, "LED_Control_Task", 4096, NULL, 5, NULL);
}

void set_all_leds(CRGB color)
{
    // Set all LEDs to the specified color
    for (int i = 0; i < NUM_LEDS; i++)
    {
        leds[i] = color;
    }
}

// Note: The `brightness` here is the overall brightness of the entire setup (0 - MAX_BRIGHTNESS)
// The `led_strip_set_pixel` function of the component accepts RGB values (0 - 255)
void set_daylight(uint8_t brightness)
{
    CRGB daylightColor = CRGB_White;
    // Scale the color based on brightness (0-255)
    daylightColor.red = (uint8_t)((uint16_t)daylightColor.red * brightness / 255);
    daylightColor.green = (uint8_t)((uint16_t)daylightColor.green * brightness / 255);
    daylightColor.blue = (uint8_t)((uint16_t)daylightColor.blue * brightness / 255);
    set_all_leds(daylightColor);
}

void set_nightlight()
{
    set_all_leds(CRGB_Black); // Set to black as the base
}

void control_house_lights(bool turn_on, float dim_progress)
{
    for (int i = 0; i < NUM_HOUSES; i++)
    {
        CRGB current_color = houseColors[i];
        if (turn_on)
        {
            // Turn lights on or dim up
            current_color.red = (uint8_t)((uint16_t)current_color.red * dim_progress);
            current_color.green = (uint8_t)((uint16_t)current_color.green * dim_progress);
            current_color.blue = (uint8_t)((uint16_t)current_color.blue * dim_progress);
            leds[houseLEDs[i]] = current_color;
        }
        else
        {
            // Turn lights off or dim down
            current_color.red = (uint8_t)((uint16_t)current_color.red * (1.0f - dim_progress));
            current_color.green = (uint8_t)((uint16_t)current_color.green * (1.0f - dim_progress));
            current_color.blue = (uint8_t)((uint16_t)current_color.blue * (1.0f - dim_progress));
            leds[houseLEDs[i]] = current_color;
        }
    }
}

void flash_effect()
{
    ESP_LOGI(TAG, "Lightning flash!");
    CRGB original_colors[NUM_LEDS];
    memcpy(original_colors, leds, NUM_LEDS * sizeof(CRGB));

    set_all_leds(CRGB_White);
    for (int i = 0; i < NUM_LEDS; i++)
    {
        led_strip_set_pixel(led_strip_handle, i, leds[i].red, leds[i].green, leds[i].blue);
    }
    led_strip_refresh(led_strip_handle);
    vTaskDelay(pdMS_TO_TICKS(50)); // Flash duration

    for (int i = 0; i < NUM_LEDS; i++)
    {
        led_strip_set_pixel(led_strip_handle, i, original_colors[i].red, original_colors[i].green,
                            original_colors[i].blue);
    }
    led_strip_refresh(led_strip_handle);
}

// The main logic for the day-night cycle is implemented in this FreeRTOS task.
void led_control_task(void *pvParameters)
{
    unsigned long long cycle_start_time_us = esp_timer_get_time(); // Start time in microseconds

    while (1)
    {
        unsigned long long current_time_us = esp_timer_get_time();
        unsigned long long time_in_cycle_us = (current_time_us - cycle_start_time_us) % CYCLE_DURATION_US;

        // --- Day-Night Cycle Phase Control ---
        if (time_in_cycle_us < DAY_DURATION_US)
        {
            // Phase 1: Day
            set_daylight(MAX_BRIGHTNESS);      // Set maximum daylight brightness
            control_house_lights(false, 0.0f); // Turn off house lights
        }
        else if (time_in_cycle_us < DAY_DURATION_US + SUNSET_DURATION_US)
        {
            // Phase 2: Sunset
            unsigned long long phase_time_us = time_in_cycle_us - DAY_DURATION_US;
            float progress = (float)phase_time_us / SUNSET_DURATION_US; // Progress from 0.0 to 1.0

            uint8_t brightness = (uint8_t)(MAX_BRIGHTNESS * (1.0f - progress)); // Gradual brightness reduction
            CRGB day_color = CRGB_White;
            CRGB sunset_color = {255, 100, 0}; // Orange-red color for sunset
            CRGB mixed_color = blend(day_color, sunset_color, (uint8_t)(progress * 255));

            // Set all LEDs to the blended color with adjusted brightness
            mixed_color.red = (uint8_t)((uint16_t)mixed_color.red * brightness / 255);
            mixed_color.green = (uint8_t)((uint16_t)mixed_color.green * brightness / 255);
            mixed_color.blue = (uint8_t)((uint16_t)mixed_color.blue * brightness / 255);
            set_all_leds(mixed_color);

            control_house_lights(true, progress); // Gradually turn on house lights with progress
        }
        else if (time_in_cycle_us < DAY_DURATION_US + SUNSET_DURATION_US + NIGHT_DURATION_US)
        {
            // Phase 3: Night
            set_nightlight();                 // Turn LEDs to black or night mode
            control_house_lights(true, 1.0f); // Turn on house lights to full brightness

            // Example for simulating lightning strikes (only at night)
            if (rand() % 5000 == 0)
            { // Random lightning flash every few seconds
                flash_effect();
            }
        }
        else
        {
            // Phase 4: Sunrise
            unsigned long long phase_time_us =
                time_in_cycle_us - (DAY_DURATION_US + SUNSET_DURATION_US + NIGHT_DURATION_US);
            float progress = (float)phase_time_us / SUNRISE_DURATION_US; // Progress from 0.0 to 1.0

            uint8_t brightness = (uint8_t)(MAX_BRIGHTNESS * progress); // Gradual brightness increase
            CRGB night_color = CRGB_Black;                             // Or a very dark blue {0, 0, 50}
            CRGB sunrise_color = {255, 180, 50};                       // Warm yellow-orange for sunrise
            CRGB mixed_color = blend(night_color, sunrise_color, (uint8_t)(progress * 255));

            // Set all LEDs to the blended color with rising brightness
            mixed_color.red = (uint8_t)((uint16_t)mixed_color.red * brightness / 255);
            mixed_color.green = (uint8_t)((uint16_t)mixed_color.green * brightness / 255);
            mixed_color.blue = (uint8_t)((uint16_t)mixed_color.blue * brightness / 255);
            set_all_leds(mixed_color);

            control_house_lights(false, 1.0f - progress); // Gradually turn off house lights
        }

        // Now transfer data from the local `leds` buffer to the LED strip
        for (int i = 0; i < NUM_LEDS; i++)
        {
            led_strip_set_pixel(led_strip_handle, i, leds[i].red, leds[i].green, leds[i].blue);
        }
        led_strip_refresh(led_strip_handle); // Send data to the LEDs

        vTaskDelay(pdMS_TO_TICKS(20)); // Short delay (20ms) for the FreeRTOS scheduler
    }
}

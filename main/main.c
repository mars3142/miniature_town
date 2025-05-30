#include "freertos/FreeRTOS.h"
#include "led_matrix.h"
#include "persistence.h"
#include "remote_control.h"

void app_main(void)
{
    persistence_init("miniature_town");
    ble_init();
    xTaskCreatePinnedToCore(led_matrix_init, "led_matrix", configMINIMAL_STACK_SIZE * 2, NULL, 5, NULL, 1);
}

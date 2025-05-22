#include "led_matrix.h"
#include "persistence.h"
#include "remote_control.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void app_main(void)
{
    persistence_init("miniature_town");

    xTaskCreatePinnedToCore(led_matrix_init, "led_matrix", configMINIMAL_STACK_SIZE * 2, NULL, 5, NULL, 1);
    ble_init(NULL);
}

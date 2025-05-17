#include "remote_control.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void ble_init(void *args)
{
    ESP_LOGI(pcTaskGetName(NULL), "Calling ble_init()");

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    ESP_LOGI(pcTaskGetName(NULL), "Exiting ble_init()");
    vTaskDelete(NULL);
}

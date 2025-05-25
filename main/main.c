#include "led_matrix.h"
#include "persistence.h"
#include "remote_control.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void app_main(void)
{
    persistence_init("miniature_town");

    ble_init(NULL);
}

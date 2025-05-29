#include "persistence.h"
#include "remote_control.h"

void app_main(void)
{
    persistence_init("miniature_town");
    ble_init();
}

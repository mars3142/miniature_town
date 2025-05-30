#include "include/device_service.h"
#include "esp_app_desc.h"
#include <stdio.h>
#include <string.h>

int ds_model_number_read(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    char model_number_str[65];
    const esp_app_desc_t *app_desc = esp_app_get_description();

    if (app_desc->project_name[0] != '\0' && app_desc->version[0] != '\0')
    {
        snprintf(model_number_str, sizeof(model_number_str), "%s v%s", app_desc->project_name, app_desc->version);
    }
    else
    {
        snprintf(model_number_str, sizeof(model_number_str), "undefined");
    }

    os_mbuf_append(ctxt->om, model_number_str, strlen(model_number_str));
    return 0;
}

int ds_manufacturer_read(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    char *manufacturer = "mars3142";
    os_mbuf_append(ctxt->om, manufacturer, strlen(manufacturer));
    return 0;
}

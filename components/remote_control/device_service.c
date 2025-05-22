#include "include/device_service.h"

int ds_model_number_read(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    char *model_number = "Miniature Town v1";
    os_mbuf_append(ctxt->om, model_number, strlen(model_number));
    return 0;
}

int ds_manufacturer_read(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    char *manufacturer = "mars3142";
    os_mbuf_append(ctxt->om, manufacturer, strlen(manufacturer));
    return 0;
}

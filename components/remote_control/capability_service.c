#include "capability_service.h"

static const char *capa_json = "{"
                               "}";

int capa_read(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    char *content = "JSON";
    os_mbuf_append(ctxt->om, content, strlen(content));
    return 0;
}

int capa_char_1979_user_desc(uint16_t con_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    const char *desc = "Capabilities of the device";
    os_mbuf_append(ctxt->om, desc, strlen(desc));
    return 0;
}

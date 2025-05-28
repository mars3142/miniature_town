#pragma once

#include "host/ble_hs.h"
#include <stdio.h>

/// LED Service Characteristic Callbacks
int ls_write(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);
int ls_capabilities_read(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);

/// LED Service Characteristic User Description
int ls_char_a000_user_desc_read(uint16_t con_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt,
                                void *arg);
int ls_char_dead_user_desc_read(uint16_t con_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt,
                                void *arg);

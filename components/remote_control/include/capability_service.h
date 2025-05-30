#pragma once

#include "host/ble_hs.h"
#include <stdio.h>

/// Service Characteristics Callback
int capa_read(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);
void capa_notify_data(uint16_t conn_handle, uint16_t char_val_handle);

/// Service Characteristics User Description
int capa_char_1979_user_desc(uint16_t con_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);

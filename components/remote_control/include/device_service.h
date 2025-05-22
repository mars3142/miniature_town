#pragma once

#include <stdio.h>
#include "host/ble_hs.h"

int ds_model_number_read(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);
int ds_manufacturer_read(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);

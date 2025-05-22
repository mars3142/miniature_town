#include "include/led_service.h"

static const char *TAG = "led_service";

// Write data to ESP32 defined as server
int ls_write(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    const char *received_payload = (const char *)ctxt->om->om_data;
    uint16_t payload_len = ctxt->om->om_len;

    // Define command strings
    const char CMD_LIGHT_ON[] = "LIGHT ON";
    const char CMD_LIGHT_OFF[] = "LIGHT OFF";
    const char CMD_FAN_ON[] = "FAN ON";
    const char CMD_FAN_OFF[] = "FAN OFF";

    if (payload_len == (sizeof(CMD_LIGHT_ON) - 1) &&
        strncmp(received_payload, CMD_LIGHT_ON, payload_len) == 0)
    {
        ESP_LOGI(TAG, "LIGHT ON");
        // TODO: Implement action for LIGHT ON
    }
    else if (payload_len == (sizeof(CMD_LIGHT_OFF) - 1) &&
             strncmp(received_payload, CMD_LIGHT_OFF, payload_len) == 0)
    {
        ESP_LOGI(TAG, "LIGHT OFF");
        // TODO: Implement action for LIGHT OFF
    }
    else if (payload_len == (sizeof(CMD_FAN_ON) - 1) &&
             strncmp(received_payload, CMD_FAN_ON, payload_len) == 0)
    {
        ESP_LOGI(TAG, "FAN ON");
        // TODO: Implement action for FAN ON
    }
    else if (payload_len == (sizeof(CMD_FAN_OFF) - 1) &&
             strncmp(received_payload, CMD_FAN_OFF, payload_len) == 0)
    {
        ESP_LOGI(TAG, "FAN OFF");
        // TODO: Implement action for FAN OFF
    }
    else
    {
        char temp_buffer[payload_len + 1];
        memcpy(temp_buffer, received_payload, payload_len);
        temp_buffer[payload_len] = '\0';

        ESP_LOGI(TAG, "Unknown command from client: %s", temp_buffer);
    }

    return 0;
}

// Read data from ESP32 defined as server
int ls_read(uint16_t con_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    char *data = "Data from the server";
    os_mbuf_append(ctxt->om, data, strlen(data));
    return 0;
}

#include <stdio.h>
#include <string.h>

#include "capability_service.h"
#include "esp_event.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "host/ble_hs.h"
#include "host/ble_sm.h"
#include "host/ble_uuid.h"
#include "include/device_service.h"
#include "include/led_service.h"
#include "led_matrix.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "sdkconfig.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

static const char *TAG = "remote_control";

static const ble_uuid16_t device_service_uuid = BLE_UUID16_INIT(0x180A);
static const ble_uuid128_t capability_service_uuid =
    BLE_UUID128_INIT(0x91, 0xB6, 0xCA, 0x95, 0xB2, 0xC6, 0x7B, 0x90, 0x31, 0x45, 0x77, 0xE6, 0x67, 0x10, 0x68, 0xB9);
static const ble_uuid16_t led_service_uuid = BLE_UUID16_INIT(0x1007);

uint8_t ble_addr_type;

// Handle for the capability characteristic value
static uint16_t g_capa_char_val_handle;

static void ble_app_advertise(void);

static struct ble_gatt_dsc_def char_0xA000_descs[] = {{
                                                          .uuid = BLE_UUID16_DECLARE(0x2901),
                                                          .att_flags = BLE_ATT_F_READ,
                                                          .access_cb = ls_char_a000_user_desc,
                                                      },
                                                      {0}};

static struct ble_gatt_dsc_def char_0xDEAD_descs[] = {{
                                                          .uuid = BLE_UUID16_DECLARE(0x2901),
                                                          .att_flags = BLE_ATT_F_WRITE,
                                                          .access_cb = ls_char_dead_user_desc,
                                                      },
                                                      {0}};

static struct ble_gatt_dsc_def char_0x1979_desc[] = {{
                                                         .uuid = BLE_UUID16_DECLARE(0x2901),
                                                         .att_flags = BLE_ATT_F_READ,
                                                         .access_cb = capa_char_1979_user_desc,
                                                     },
                                                     {0}};

// Array of pointers to other service definitions
static const struct ble_gatt_svc_def gatt_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &device_service_uuid.u,
        .characteristics =
            (struct ble_gatt_chr_def[]){
                {.uuid = BLE_UUID16_DECLARE(0x2A24), .flags = BLE_GATT_CHR_F_READ, .access_cb = ds_model_number_read},
                {.uuid = BLE_UUID16_DECLARE(0x2A29), .flags = BLE_GATT_CHR_F_READ, .access_cb = ds_manufacturer_read},
                {0}},
    },
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &capability_service_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]){{
                                                           .uuid = BLE_UUID16_DECLARE(0x1979),
                                                           .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
                                                           .access_cb = capa_read,
                                                           .val_handle = &g_capa_char_val_handle,
                                                           .descriptors = char_0x1979_desc,
                                                       },
                                                       {0}},
    },
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &led_service_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]){{
                                                           .uuid = BLE_UUID16_DECLARE(0xA000),
                                                           .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
                                                           .access_cb = ls_capabilities_read,
                                                           .descriptors = char_0xA000_descs,
                                                       },
                                                       {
                                                           .uuid = BLE_UUID16_DECLARE(0xDEAD),
                                                           .flags = BLE_GATT_CHR_F_WRITE,
                                                           .access_cb = ls_write,
                                                           .descriptors = char_0xDEAD_descs,
                                                       },
                                                       {0}},
    },
    {0}};

// BLE event handling
static int ble_gap_event(struct ble_gap_event *event, void *arg)
{
    switch (event->type)
    {
    case BLE_GAP_EVENT_CONNECT:
        ESP_LOGI(TAG, "BLE GAP EVENT CONNECT %s", event->connect.status == 0 ? "OK!" : "FAILED!");
        if (event->connect.status != 0)
        {
            // Re-advertise if connection failed
            ble_app_advertise();
        }
        break;

    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGI(TAG, "BLE GAP EVENT DISCONNECTED");
        // Re-advertise after disconnection
        ble_app_advertise();
        break;

    case BLE_GAP_EVENT_ADV_COMPLETE:
        ESP_LOGI(TAG, "BLE GAP EVENT ADV COMPLETE");
        // Re-advertise to continue accepting new clients
        ble_app_advertise();
        break;

    case BLE_GAP_EVENT_SUBSCRIBE:
        ESP_LOGI(TAG,
                 "BLE GAP EVENT SUBSCRIBE conn_handle=%d attr_handle=%d reason=%d "
                 "prev_notify=%d cur_notify=%d prev_indicate=%d cur_indicate=%d",
                 event->subscribe.conn_handle, event->subscribe.attr_handle, event->subscribe.reason,
                 event->subscribe.prev_notify, event->subscribe.cur_notify, event->subscribe.prev_indicate,
                 event->subscribe.cur_indicate);

        // Check if subscription is for the capability characteristic's CCCD.
        // The CCCD handle is typically the characteristic value handle + 1.
        // g_capa_char_val_handle stores the handle of the characteristic value itself.
        if (event->subscribe.attr_handle == g_capa_char_val_handle + 1)
        {
            if (event->subscribe.cur_notify)
            {
                ESP_LOGI(TAG, "Client subscribed to capability notifications. Sending data...");
                // Call the function to send capability data via notifications
                capa_notify_data(event->subscribe.conn_handle, g_capa_char_val_handle);
            }
            else
            {
                ESP_LOGI(TAG, "Client unsubscribed from capability notifications.");
            }
        }
        break;

    default:
        break;
    }
    return 0;
}

// Define the BLE connection
static void ble_app_advertise(void)
{
    int ret;

    // GAP - advertising definition
    struct ble_hs_adv_fields fields;
    memset(&fields, 0, sizeof(fields));
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.uuids128 = (ble_uuid128_t[]){capability_service_uuid};
    fields.num_uuids128 = 1;
    fields.uuids128_is_complete = 1;

    ret = ble_gap_adv_set_fields(&fields);
    if (ret != 0)
    {
        ESP_LOGE(TAG, "Failed to set advertising data (err: %d)", ret);
        return;
    }

    // GAP - device connectivity definition
    struct ble_gap_adv_params adv_params;
    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND; // connectable or non-connectable
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN; // discoverable or non-discoverable
    ret = ble_gap_adv_start(ble_addr_type, NULL, BLE_HS_FOREVER, &adv_params, ble_gap_event, NULL);
    if (ret != 0)
    {
        ESP_LOGE(TAG, "Advertising failed to start (err %d)", ret);
        return;
    }

    // --- Configure Scan Response Data (SCAN_RSP) ---
    struct ble_hs_adv_fields scan_rsp_fields;
    memset(&scan_rsp_fields, 0, sizeof(scan_rsp_fields));

    // Get the device name
    const char *device_name;
    device_name = ble_svc_gap_device_name();
    scan_rsp_fields.name = (uint8_t *)device_name;
    scan_rsp_fields.name_len = strlen(device_name);
    scan_rsp_fields.name_is_complete = 1;

    // Optionally, add TX power level to scan response
    scan_rsp_fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;
    scan_rsp_fields.tx_pwr_lvl_is_present = 1;

    ret = ble_gap_adv_rsp_set_fields(&scan_rsp_fields);
    if (ret != 0)
    {
        ESP_LOGE(TAG, "Error setting scan response data; rc=%d", ret);
        return;
    }
}

// The application
static void ble_app_on_sync(void)
{
    uint8_t ble_addr[6] = {0};
    int ret = ble_hs_id_copy_addr(BLE_ADDR_PUBLIC, ble_addr, NULL);
    if (ret != 0)
    {
        ESP_LOGE(TAG, "Failed to get BLE MAC address (err: %d)", ret);
        return;
    }

    char formatted_name[32];
    snprintf(formatted_name, sizeof(formatted_name), "Miniature Town %02X%02X", ble_addr[4], ble_addr[5]);
    ble_svc_gap_device_name_set(formatted_name);

    // Start Advertising
    ble_hs_id_infer_auto(0, &ble_addr_type); // Determines the best address type automatically
    ble_app_advertise();
}

// The infinite task
static void host_task(void *param)
{
    nimble_port_run(); // This function will return only when nimble_port_stop() is executed
}

void ble_init(void)
{
    nimble_port_init();
    ble_svc_gap_init();
    ble_svc_gatt_init();
    ble_gatts_count_cfg(gatt_svcs);
    ble_gatts_add_svcs(gatt_svcs);

    // Callback für Synchronisation
    ble_hs_cfg.sync_cb = ble_app_on_sync;

    nimble_port_freertos_init(host_task); // Start BLE-Host-Task
}

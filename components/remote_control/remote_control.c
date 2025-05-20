#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_nimble_hci.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/ble_sm.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "sdkconfig.h"

static const char *TAG = "remote_control";
static const char *DEVICE_NAME = "Miniature Town";

uint8_t ble_addr_type;

void ble_app_advertise(void);

// Write data to ESP32 defined as server
static int device_write(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    // printf("Data from the client: %.*s\n", ctxt->om->om_len, ctxt->om->om_data);

    char *data = (char *)ctxt->om->om_data;
    printf("%d\n", strcmp(data, (char *)"LIGHT ON") == 0);
    if (strcmp(data, (char *)"LIGHT ON\0") == 0)
    {
        printf("LIGHT ON\n");
    }
    else if (strcmp(data, (char *)"LIGHT OFF\0") == 0)
    {
        printf("LIGHT OFF\n");
    }
    else if (strcmp(data, (char *)"FAN ON\0") == 0)
    {
        printf("FAN ON\n");
    }
    else if (strcmp(data, (char *)"FAN OFF\0") == 0)
    {
        printf("FAN OFF\n");
    }
    else
    {
        printf("Data from the client: %.*s\n", ctxt->om->om_len, ctxt->om->om_data);
    }

    return 0;
}

// Read data from ESP32 defined as server
static int device_read(uint16_t con_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    char *data = "Data from the server";
    os_mbuf_append(ctxt->om, data, strlen(data));
    return 0;
}

static int device_name_read(uint16_t onn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    os_mbuf_append(ctxt->om, DEVICE_NAME, strlen(DEVICE_NAME));
    return 0;
}

static int model_number_read(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    char *model_number = "Model Number";
    os_mbuf_append(ctxt->om, model_number, strlen(model_number));
    return 0;
}
static int manufacturer_read(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    char *manufacturer = "mars3142";
    os_mbuf_append(ctxt->om, manufacturer, strlen(manufacturer));
    return 0;
}

// Array of pointers to other service definitions
static const struct ble_gatt_svc_def gatt_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(0x180A),
        .characteristics = (struct ble_gatt_chr_def[]){
            {.uuid = BLE_UUID16_DECLARE(0x2A00), .flags = BLE_GATT_CHR_F_READ, .access_cb = device_name_read},
            {.uuid = BLE_UUID16_DECLARE(0x2A24), .flags = BLE_GATT_CHR_F_READ, .access_cb = model_number_read},
            {.uuid = BLE_UUID16_DECLARE(0x2A29), .flags = BLE_GATT_CHR_F_READ, .access_cb = manufacturer_read},
            {0}},
    },
    {.type = BLE_GATT_SVC_TYPE_PRIMARY, .uuid = BLE_UUID16_DECLARE(0x180), .characteristics = (struct ble_gatt_chr_def[]){{.uuid = BLE_UUID16_DECLARE(0xFEF4), .flags = BLE_GATT_CHR_F_READ, .access_cb = device_read}, {.uuid = BLE_UUID16_DECLARE(0xDEAD), .flags = BLE_GATT_CHR_F_WRITE, .access_cb = device_write}, {0}}},
    {0}};

// BLE event handling
static int ble_gap_event(struct ble_gap_event *event, void *arg)
{
    switch (event->type)
    {
    case BLE_GAP_EVENT_CONNECT:
        ESP_LOGI(TAG, "BLE GAP EVENT CONNECT %s", event->connect.status == 0 ? "OK!" : "FAILED!");
        if (event->connect.status == 0)
        {
            // Start security pairing without disconnecting
            int ret = ble_gap_security_initiate(event->connect.conn_handle);
            ESP_LOGI(TAG, "BLE GAP SECURITY INITIATE %s", ret == 0 ? "OK!" : "FAILED!");
        }
        else
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

    case BLE_GAP_EVENT_ENC_CHANGE:
        if (event->enc_change.status == 0)
        {
            ESP_LOGI(TAG, "Encryption enabled for connection");
        }
        else
        {
            ESP_LOGE(TAG, "Failed to enable encryption, status=%d", event->enc_change.status);
        }
        break;

    default:
        break;
    }
    return 0;
}

// Define the BLE connection
void ble_app_advertise(void)
{
    // GAP - device name definition
    struct ble_hs_adv_fields fields;
    const char *device_name;
    memset(&fields, 0, sizeof(fields));
    device_name = ble_svc_gap_device_name(); // Read the BLE device name
    fields.name = (uint8_t *)device_name;
    fields.name_len = strlen(device_name);
    fields.name_is_complete = 1;
    ble_gap_adv_set_fields(&fields);

    // GAP - device connectivity definition
    struct ble_gap_adv_params adv_params;
    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND; // connectable or non-connectable
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN; // discoverable or non-discoverable
    ble_gap_adv_start(ble_addr_type, NULL, BLE_HS_FOREVER, &adv_params, ble_gap_event, NULL);
}

// The application
void ble_app_on_sync(void)
{
    ble_hs_id_infer_auto(0, &ble_addr_type); // Determines the best address type automatically
    ble_app_advertise();                     // Define the BLE connection
}

// The infinite task
void host_task(void *param)
{
    nimble_port_run(); // This function will return only when nimble_port_stop() is executed
}

void ble_init(void *args)
{
    nimble_port_init();                       // 3 - Initialize the host stack
    ble_svc_gap_device_name_set(DEVICE_NAME); // 4 - Initialize NimBLE configuration - server name
    ble_svc_gap_init();                       // 4 - Initialize NimBLE configuration - gap service
    ble_svc_gatt_init();                      // 4 - Initialize NimBLE configuration - gatt service
    ble_gatts_count_cfg(gatt_svcs);           // 4 - Initialize NimBLE configuration - config gatt services
    ble_gatts_add_svcs(gatt_svcs);            // 4 - Initialize NimBLE configuration - queues gatt services.
    ble_hs_cfg.sync_cb = ble_app_on_sync;     // 5 - Initialize application

    // Configure security settings
    ble_hs_cfg.sm_bonding = 1;                             // Enable bonding
    ble_hs_cfg.sm_sc = 0;                                  // Enable Secure Connections (LE SC)
    ble_hs_cfg.sm_our_key_dist = BLE_SM_PAIR_KEY_DIST_ENC; // Encryption key distribution
    ble_hs_cfg.sm_their_key_dist = BLE_SM_PAIR_KEY_DIST_ENC;

    nimble_port_freertos_init(host_task); // Run the host task
}

#include "capability_service.h"
#include "esp_log.h"
#include "storage.h"
#include <string.h>
#include <stdlib.h>           // For malloc, free
#include "host/ble_hs.h"      // For ble_hs_mbuf_from_flat, ble_att_mtu
#include "host/ble_uuid.h"    // For BLE_ATT_MTU_DFLT (often included via ble_hs.h)
#include "nimble/nimble_port.h" // For os_mbuf related functions

static const char *TAG_CS = "capability_service";

#define CAPA_READ_CHUNK_SIZE 200 // Maximale Bytes pro Lesevorgang aus dem Storage

int capa_read(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    esp_err_t storage_status = storage_init();
    if (storage_status != ESP_OK)
    {
        ESP_LOGE(TAG_CS, "Failed to initialize storage: %s", esp_err_to_name(storage_status));
        const char *err_msg = "Error: Storage init failed";
        os_mbuf_append(ctxt->om, err_msg, strlen(err_msg));
        // storage_uninit() sollte hier nicht aufgerufen werden, da die Initialisierung fehlschlug.
        return 0;
    }

    char *content = "";
    os_mbuf_append(ctxt->om, content, strlen(content));
    const char *filename = "/storage/capability.json"; // Die zu lesende Datei
    char read_buffer[CAPA_READ_CHUNK_SIZE];
    ssize_t bytes_read;
    int os_err;

    ESP_LOGI(TAG_CS, "Reading capabilities from %s", filename);

    // Schleife, um die Datei in Chunks zu lesen und an den mbuf anzuhängen
    while ((bytes_read = storage_read(filename, read_buffer, sizeof(read_buffer))) > 0)
    {
        ESP_LOGD(TAG_CS, "Read %zd bytes from storage", bytes_read);
        // Den gelesenen Chunk an den BLE-Antwortpuffer anhängen
        os_err = os_mbuf_append(ctxt->om, read_buffer, bytes_read);
        if (os_err != 0)
        {
            ESP_LOGE(TAG_CS, "Failed to append to mbuf (error %d). May be out of space.", os_err);
            // Der mbuf könnte voll sein. Stoppe das Anhängen.
            // Bereits angehängte Daten werden gesendet.
            break;
        }
    }

    // Fehlerbehandlung oder EOF
    if (bytes_read < 0)
    {
        ESP_LOGE(TAG_CS, "Error reading from storage (file: %s, error_code: %zd)", filename, bytes_read);
        // Wenn noch nichts angehängt wurde, sende eine Fehlermeldung.
        if (ctxt->om->om_len == 0)
        {
            const char *err_msg = "Error: Failed to read capability data";
            // Hier könnten spezifischere Fehlermeldungen basierend auf bytes_read eingefügt werden
            os_mbuf_append(ctxt->om, err_msg, strlen(err_msg));
        }
    }
    else
    { // bytes_read == 0, bedeutet EOF (Ende der Datei)
        ESP_LOGI(TAG_CS, "Successfully read and appended all data from %s to mbuf.", filename);
    }

    storage_uninit();
    return 0;
}

int capa_char_1979_user_desc(uint16_t con_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    const char *desc = "Capabilities of the device";
    os_mbuf_append(ctxt->om, desc, strlen(desc));
    return 0;
}

void capa_notify_data(uint16_t conn_handle, uint16_t char_val_handle)
{
    esp_err_t storage_status = storage_init();
    if (storage_status != ESP_OK)
    {
        ESP_LOGE(TAG_CS, "Notify: Failed to initialize storage: %s", esp_err_to_name(storage_status));
        return;
    }

    const char *filename = "/storage/capability.json";

    uint16_t mtu = ble_att_mtu(conn_handle);
    if (mtu == 0) { // Should not happen for an active connection, fallback
        ESP_LOGW(TAG_CS, "Notify: ble_att_mtu returned 0, using default MTU %d.", BLE_ATT_MTU_DFLT);
        mtu = BLE_ATT_MTU_DFLT;
    }

    // Max payload for notification is MTU - 3 (1 byte opcode for Notification, 2 bytes attribute handle)
    size_t notify_chunk_size = (mtu > 3) ? (mtu - 3) : (BLE_ATT_MTU_DFLT - 3); // Ensure mtu > 3

    // Further cap by CAPA_READ_CHUNK_SIZE if it's smaller and meant as an upper limit for any single read op
    if (notify_chunk_size > CAPA_READ_CHUNK_SIZE) {
        notify_chunk_size = CAPA_READ_CHUNK_SIZE;
    }

    if (notify_chunk_size == 0) { // Safety check
        ESP_LOGE(TAG_CS, "Notify: Calculated notify_chunk_size is 0. Aborting.");
        storage_uninit();
        return;
    }

    char *read_buffer = malloc(notify_chunk_size);
    if (!read_buffer)
    {
        ESP_LOGE(TAG_CS, "Notify: Failed to allocate read_buffer (%zu bytes)", notify_chunk_size);
        storage_uninit();
        return;
    }

    ESP_LOGI(TAG_CS, "Notify: Reading capabilities from %s to notify conn %u, attr %u (chunk size %zu, MTU %u)",
             filename, conn_handle, char_val_handle, notify_chunk_size, mtu);

    FILE *fp = fopen(filename, "r");
    if (!fp)
    {
        ESP_LOGE(TAG_CS, "Notify: Failed to open %s for reading.", filename);
        free(read_buffer);
        storage_uninit();
        return;
    }

    ssize_t bytes_read;
    while ((bytes_read = fread(read_buffer, 1, notify_chunk_size, fp)) > 0)
    {
        ESP_LOGD(TAG_CS, "Notify: Read %zd bytes from storage for notification", bytes_read);

        struct os_mbuf *om = ble_hs_mbuf_from_flat(read_buffer, bytes_read);
        if (!om) {
            ESP_LOGE(TAG_CS, "Notify: Failed to allocate mbuf for notification. Stopping.");
            break; // Stop sending if mbuf allocation fails
        }

        int rc = ble_gatts_notify_custom(conn_handle, char_val_handle, om);
        if (rc != 0) {
            ESP_LOGE(TAG_CS, "Notify: Error sending notification (rc=%d). Stopping.", rc);
            // ble_gatts_notify_custom frees 'om' on success or if it takes ownership even on some errors.
            // If it returns an error where 'om' is not freed, we might need os_mbuf_free_chain(om);
            // For now, assume NimBLE handles 'om' correctly in error cases like BLE_HS_ENOMEM.
            break; // Stop if notification fails
        }
        ESP_LOGD(TAG_CS, "Notify: Sent %zd bytes successfully.", bytes_read);
    }

    if (ferror(fp)) { ESP_LOGE(TAG_CS, "Notify: File read error from %s.", filename); }
    fclose(fp);
    free(read_buffer);
    storage_uninit();
    ESP_LOGI(TAG_CS, "Notify: Finished sending capability data for conn %u.", conn_handle);
}

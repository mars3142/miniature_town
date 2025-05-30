#include "capability_service.h"
#include "esp_log.h"
#include "storage.h"
#include <string.h>

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

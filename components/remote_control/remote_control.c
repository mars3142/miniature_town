#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"

// NimBLE-Header
#include "esp_nimble_hci.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"

// Tag für Logging
static const char *TAG = "remote_control";

#define DEVICE_NAME "Miniature Town"

uint8_t ble_addr_type;

/**
 * Konfiguriert und startet das BLE-Advertising.
 */
static void ble_app_advertise(void)
{
    struct ble_gap_adv_params adv_params;
    struct ble_hs_adv_fields fields;
    int rc;

    // Fülle die Advertising-Felder
    memset(&fields, 0, sizeof(fields));

    // Flags:
    // - BLE_HS_ADV_F_DISC_GEN: General discoverable mode
    // - BLE_HS_ADV_F_BREDR_UNSUP: BR/EDR (Classic Bluetooth) nicht unterstützt
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;

    // Device Name
    // Stelle sicher, dass der Name + Overhead in das Advertising-Paket passt.
    // Bei Bedarf muss der Name gekürzt werden oder in Scan Response platziert werden.
    fields.name = (uint8_t *)DEVICE_NAME;
    fields.name_len = strlen(DEVICE_NAME);
    fields.name_is_complete = 1; // Der Name ist vollständig im Advertising-Paket

    // Setze die Advertising-Daten
    rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0)
    {
        ESP_LOGE(TAG, "Fehler beim Setzen der Advertising-Daten; rc=%d", rc);
        return;
    }

    // Konfiguriere die Advertising-Parameter
    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND; // Undirected connectable
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN; // General discoverable

    // Starte das Advertising
    // Parameter:
    // - own_addr_type: Typ der eigenen Adresse (wird von ble_hs_id_infer_auto gesetzt)
    // - peer_addr: NULL für undirected advertising
    // - duration_ms: BLE_HS_FOREVER für kontinuierliches Advertising
    // - adv_params: Die oben konfigurierten Parameter
    // - cb: Callback-Funktion bei GAP-Events (z.B. Verbindung, Trennung)
    // - cb_arg: Argument für die Callback-Funktion
    rc = ble_gap_adv_start(ble_addr_type, NULL, BLE_HS_FOREVER, &adv_params, NULL, NULL);
    if (rc != 0)
    {
        ESP_LOGE(TAG, "Fehler beim Starten des Advertising; rc=%d", rc);
        return;
    }
    ESP_LOGI(TAG, "Advertising gestartet. Gerät sollte sichtbar sein als '%s'", DEVICE_NAME);
}

/**
 * Callback-Funktion, die aufgerufen wird, wenn der BLE-Host-Stack synchronisiert (bereit) ist.
 */
static void ble_app_on_sync(void)
{
    int rc;

    // Setze eine zufällige statische Adresse oder eine öffentliche Adresse
    // Hier verwenden wir eine zufällige statische Adresse als Beispiel
    rc = ble_hs_id_infer_auto(0, &ble_addr_type);
    if (rc != 0)
    {
        ESP_LOGE(TAG, "Fehler beim Bestimmen der Adresse; rc=%d", rc);
        return;
    }

    // Die Adresse wurde gesetzt, starte Advertising
    ble_app_advertise();
}

/**
 * Host-Task für NimBLE.
 */
void ble_host_task(void *param)
{
    ESP_LOGI(TAG, "BLE Host Task gestartet");
    // Dieser Aufruf blockiert, bis nimble_port_stop() aufgerufen wird
    nimble_port_run();

    nimble_port_freertos_deinit();
}

void ble_init(void *args)
{
    int rc;

    // Initialisiere NVS (Non-Volatile Storage)
    // Dies ist für BLE erforderlich, um Controller-Konfigurationen zu speichern
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialisiere den NimBLE-Port (FreeRTOS spezifisch)
    nimble_port_init();

    // Konfiguriere den BLE-Host-Stack:
    // - Setze den Callback für "on_sync" (wenn der Stack bereit ist)
    // - Setze den Callback für "on_reset" (wenn der Controller zurückgesetzt wird)
    ble_hs_cfg.sync_cb = ble_app_on_sync;
    ble_hs_cfg.reset_cb = NULL; // Hier nicht explizit benötigt für einfaches Advertising

    // Optional: Setze den Gerätenamen direkt im GAP Service.
    // Dies ist eine andere Stelle als das Advertising-Paket, aber gute Praxis.
    rc = ble_svc_gap_device_name_set(DEVICE_NAME);
    assert(rc == 0);

    // Initialisiere das NimBLE HCI Layer
    // ESP_ERROR_CHECK(esp_nimble_hci_and_controller_init()); // Für ESP32, ESP32-C3, ESP32-S3 (kombinierter Host & Controller)
    // Für ESP32-C2, ESP32-C6, ESP32-H2, die VHCI verwenden, wird eine andere Initialisierung benötigt.
    // Aber für typische ESP32 ist dies korrekt.

    // Starte den NimBLE Host Task in einem eigenen Thread.
    // Die Priorität und Stack-Größe können angepasst werden.
    // NIMBLE_STACK_SIZE ist in nimble_port_freertos.h definiert
    nimble_port_freertos_init(ble_host_task);

    ESP_LOGI(TAG, "BLE App Main abgeschlossen. BLE Host Task läuft.");
}

#include "persistence.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static const char *TAG = "persistence";

static nvs_handle_t persistence_handle;
static SemaphoreHandle_t persistence_mutex;

void persistence_init(const char *namespace_name)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(nvs_open(namespace_name, NVS_READWRITE, &persistence_handle));

    persistence_mutex = xSemaphoreCreateMutex();
    if (persistence_mutex == NULL)
    {
        ESP_LOGE(TAG, "Failed to create mutex");
    }
}

void persistence_save(persistence_value_type_t value_type, const char *key, const void *value)
{
    if (persistence_mutex != NULL)
    {
        if (xSemaphoreTake(persistence_mutex, portMAX_DELAY) == pdTRUE)
        {
            esp_err_t err = ESP_ERR_INVALID_ARG;

            switch (value_type)
            {
            case VALUE_TYPE_STRING:
                err = nvs_set_str(persistence_handle, key, (char *)value);
                break;

            case VALUE_TYPE_INT32:
                err = nvs_set_i32(persistence_handle, key, *(int32_t *)value);
                break;

            default:
                ESP_LOGE(TAG, "Unsupported value type");
                break;
            }

            if (err == ESP_OK)
            {
                ESP_ERROR_CHECK(nvs_commit(persistence_handle));
            }
            else
            {
                ESP_LOGE(TAG, "Error saving key %s: %s", key, esp_err_to_name(err));
            }

            xSemaphoreGive(persistence_mutex);
        }
    }
}

void *persistence_load(persistence_value_type_t value_type, const char *key, void *out)
{
    if (persistence_mutex != NULL)
    {
        if (xSemaphoreTake(persistence_mutex, portMAX_DELAY) == pdTRUE)
        {
            esp_err_t err = ESP_ERR_INVALID_ARG;

            switch (value_type)
            {
            case VALUE_TYPE_STRING:
                err = nvs_get_str(persistence_handle, key, (char *)out, NULL);
                break;

            case VALUE_TYPE_INT32:
                err = nvs_get_i32(persistence_handle, key, (int32_t *)out);
                break;

            default:
                ESP_LOGE(TAG, "Unsupported value type");
                break;
            }

            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "Error loading key %s: %s", key, esp_err_to_name(err));
            }

            xSemaphoreGive(persistence_mutex);
        }
    }

    return out;
}

void persistence_deinit()
{
    if (persistence_mutex != NULL)
    {
        vSemaphoreDelete(persistence_mutex);
        persistence_mutex = NULL;
    }

    nvs_close(persistence_handle);
}

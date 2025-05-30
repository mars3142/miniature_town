#include "storage.h"

#include "esp_log.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "esp_spiffs.h"

static const char *TAG = "storage";

static FILE *s_current_file = NULL;
static char s_current_filename[256] = {0}; // Buffer to store the current filename

esp_err_t storage_init(void)
{
    ESP_LOGI(TAG, "Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/storage",       // Path where the filesystem will be mounted
        .partition_label = "storage",  // Partition label (must match partitions.csv)
        .max_files = 5,                // Maximum number of files that can be open at the same time
        .format_if_mount_failed = true // Format partition if mount fails
    };

    // Initialize and mount SPIFFS
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        }
        else if (ret == ESP_ERR_NOT_FOUND)
        {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        }
        else
        {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return ret;
    }

    ESP_LOGI(TAG, "SPIFFS mounted");
    return ESP_OK;
}

void storage_uninit(void)
{
    if (s_current_file != NULL)
    {
        // Log before clearing s_current_filename, using its value.
        ESP_LOGW(TAG, "File '%s' was still open during uninit. Closing it.", s_current_filename);
        fclose(s_current_file); // Close the file
        s_current_file = NULL;
        s_current_filename[0] = '\0';
    }
    esp_vfs_spiffs_unregister("storage");
    ESP_LOGI(TAG, "SPIFFS unmounted");
}

ssize_t storage_read(const char *filename, char *buffer, size_t max_bytes)
{
    // Input validation
    if (filename == NULL || filename[0] == '\0' || buffer == NULL || max_bytes == 0)
    {
        ESP_LOGE(TAG, "Invalid input parameters for storage_read");
        return -1;
    }

    // If no file is currently open, open the requested one
    if (s_current_file == NULL)
    {
        s_current_file = fopen(filename, "r");
        if (s_current_file == NULL)
        {
            ESP_LOGE(TAG, "Failed to open file for reading: %s", filename);
            s_current_filename[0] = '\0'; // Clear filename as open failed
            return -2;                    // Failed to open file
        }
        // Store the filename for subsequent calls
        strncpy(s_current_filename, filename, sizeof(s_current_filename) - 1);
        s_current_filename[sizeof(s_current_filename) - 1] = '\0';
        ESP_LOGI(TAG, "Opened file: %s", s_current_filename);
    }
    else
    {
        // If a file is open, ensure it's the same file requested
        if (strcmp(filename, s_current_filename) != 0)
        {
            ESP_LOGE(TAG, "Filename mismatch. Expected '%s', got '%s'. Close the current file first.",
                     s_current_filename, filename);
            // Note: File remains open. Caller might need a separate close function
            // or ensure read loop finishes (returns 0 or error) before changing filename.
            return -3; // Filename mismatch
        }
    }

    // Read a chunk
    size_t bytes_read = fread(buffer, 1, max_bytes, s_current_file);

    // Check for read errors
    if (ferror(s_current_file))
    {
        ESP_LOGE(TAG, "Error reading file: %s", s_current_filename);
        fclose(s_current_file);
        s_current_file = NULL;
        s_current_filename[0] = '\0';
        return -4; // Read error
    }

    // If fread returns 0 and it's not an error, it means EOF or an empty file.
    // The file should be closed only when no more bytes can be read (i.e., bytes_read == 0).
    if (bytes_read == 0) // Indicates EOF or empty file (and no ferror)
    {
        // Log before clearing s_current_filename.
        // feof(s_current_file) should be true if end of file was actually reached.
        if (feof(s_current_file))
        {
            ESP_LOGI(TAG, "EOF reached for file: %s. Closing file.", s_current_filename);
        }
        else
        {
            ESP_LOGI(TAG, "Read 0 bytes from file: %s (possibly empty or already at EOF). Closing file.",
                     s_current_filename);
        }
        fclose(s_current_file);
        s_current_file = NULL;
        s_current_filename[0] = '\0';
        // Return 0 as per contract: "Returns 0 when the end of the file is reached."
    }
    // If bytes_read > 0, return the number of bytes read.
    // The file remains open (even if EOF was hit during this read and bytes_read < max_bytes).
    // The next call to storage_read for this file will then result in fread returning 0,
    // which will then hit the (bytes_read == 0) condition above and close the file.
    return bytes_read;
}

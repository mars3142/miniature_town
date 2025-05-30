#pragma once

#include "esp_err.h"
#include <sys/types.h> // For ssize_t

/**
 * @brief Initializes the SPIFFS filesystem.
 * This function should be called once before any file operations.
 * @return ESP_OK on success, error code otherwise.
 */
esp_err_t storage_init(void);

/**
 * @brief Unregisters the SPIFFS filesystem.
 * This function should be called once when file operations are finished.
 */
void storage_uninit(void);

/**
 * @brief Reads a chunk of data from a file on SPIFFS.
 * This function maintains internal state (file pointer) to allow reading
 * the same file chunk by chunk across multiple calls.
 *
 * @param filename The path to the file to read (e.g., "/storage/my_file.txt").
 *                 Must be the same filename for consecutive calls to read the same file.
 * @param buffer   Buffer to store the read data. Must be large enough for max_bytes.
 * @param max_bytes The maximum number of bytes to read in this call. Must be > 0.
 * @return The number of bytes read on success (>= 0).
 *         Returns 0 when the end of the file is reached.
 *         Returns a negative value on error:
 *         -1: Invalid input parameters (filename, buffer is NULL, or max_bytes is 0).
 *         -2: Failed to open the file (first call for this filename).
 *         -3: Filename mismatch with the currently open file (subsequent call).
 *         -4: Read error occurred.
 */
ssize_t storage_read(const char *filename, char *buffer, size_t max_bytes);

/**
 * @brief Reads a chunk of data from a file on SPIFFS, starting at a specific offset.
 * This function is stateless regarding an internally managed file pointer for sequential reads;
 * it opens, seeks, reads, and closes the file on each call.
 *
 * @param filename The path to the file to read (e.g., "/storage/my_file.txt").
 * @param buffer   Buffer to store the read data. Must be large enough for nbytes.
 * @param offset   The offset in the file to start reading from.
 * @param nbytes   The maximum number of bytes to read in this call. Must be > 0.
 * @return The number of bytes read on success (>= 0).
 *         Returns 0 when the end of the file is reached from the given offset.
 *         Returns a negative value on error:
 *         -1: Invalid input parameters (filename, buffer is NULL, nbytes is 0, or offset is negative).
 *         -2: Failed to open the file.
 *         -5: Seek error occurred.
 *         -4: Read error occurred.
 */
ssize_t storage_read_at(const char *filename, char *buffer, off_t offset, size_t nbytes);

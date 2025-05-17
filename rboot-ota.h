/**
 * @file rboot-ota.h
 * @brief rBoot OTA (Over-The-Air) Update Support
 * @author Richard A Burton <richardaburton@gmail.com>
 * @version 1.0
 * @date 2023
 * 
 * @copyright Copyright (c) 2023 Richard A Burton. See license.txt for license terms.
 */

#ifndef __RBOOT_OTA_H__
#define __RBOOT_OTA_H__

#include "rboot.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup ota OTA Update Configuration
 *  @brief Configuration options for OTA updates
 *  @{
 */

// Uncomment to enable OTA update support
//#define BOOT_OTA_ENABLED

// OTA update timeout in milliseconds
#ifndef OTA_UPDATE_TIMEOUT_MS
#define OTA_UPDATE_TIMEOUT_MS 300000  // 5 minutes
#endif

// OTA buffer size (must be multiple of flash sector size)
#ifndef OTA_BUFFER_SIZE
#define OTA_BUFFER_SIZE 4096
#endif

// OTA status codes
typedef enum {
    OTA_OK = 0,            // Operation successful
    OTA_ERR_INVALID_ARGS,   // Invalid arguments
    OTA_ERR_NO_MEM,         // Not enough memory
    OTA_ERR_FLASH,          // Flash operation failed
    OTA_ERR_INVALID_IMAGE,  // Invalid firmware image
    OTA_ERR_TIMEOUT,        // Operation timed out
    OTA_ERR_VERIFY,         // Verification failed
    OTA_ERR_WRITE,          // Write operation failed
    OTA_ERR_ERASE,          // Erase operation failed
    OTA_ERR_IN_PROGRESS,    // OTA already in progress
    OTA_ERR_NO_UPDATE       // No update available
} ota_result_t;

// OTA update state
typedef enum {
    OTA_STATE_READY = 0,    // Ready for update
    OTA_STATE_STARTED,      // Update started
    OTA_STATE_WRITING,      // Writing data
    OTA_STATE_VERIFYING,    // Verifying data
    OTA_STATE_COMPLETE,     // Update complete
    OTA_STATE_ERROR         // Error occurred
} ota_state_t;

// OTA update handle
typedef struct {
    uint32_t target_addr;    // Target flash address
    uint32_t write_offset;   // Current write offset
    uint32_t total_size;     // Total size of update
    uint32_t written_size;   // Number of bytes written
    uint8_t *buffer;         // Data buffer
    uint32_t buffer_size;    // Size of data buffer
    ota_state_t state;       // Current state
    uint8_t target_rom;      // Target ROM slot
} ota_handle_t;

/** @} */ // end of ota

/**
 * @brief Initialize OTA update
 * 
 * @param handle Pointer to OTA handle structure
 * @param target_rom Target ROM slot for update
 * @param max_size Maximum size of the update
 * @return ota_result_t OTA_OK on success, error code otherwise
 */
ota_result_t rboot_ota_begin(ota_handle_t *handle, uint8_t target_rom, uint32_t max_size);

/**
 * @brief Write data to flash during OTA update
 * 
 * @param handle Pointer to OTA handle structure
 * @param data Pointer to data to write
 * @param size Size of data to write
 * @return ota_result_t OTA_OK on success, error code otherwise
 */
ota_result_t rboot_ota_write(ota_handle_t *handle, const uint8_t *data, uint32_t size);

/**
 * @brief Finalize OTA update
 * 
 * @param handle Pointer to OTA handle structure
 * @return ota_result_t OTA_OK on success, error code otherwise
 */
ota_result_t rboot_ota_end(ota_handle_t *handle);

/**
 * @brief Cancel OTA update and clean up
 * 
 * @param handle Pointer to OTA handle structure
 */
void rboot_ota_cancel(ota_handle_t *handle);

/**
 * @brief Set next boot ROM
 * 
 * @param rom ROM slot to boot from
 * @return ota_result_t OTA_OK on success, error code otherwise
 */
ota_result_t rboot_set_boot_rom(uint8_t rom);

/**
 * @brief Get current boot ROM
 * 
 * @return uint8_t Current boot ROM slot
 */
uint8_t rboot_get_current_rom(void);

/**
 * @brief Check if OTA update is in progress
 * 
 * @return uint8_t 1 if OTA is in progress, 0 otherwise
 */
uint8_t rboot_ota_is_in_progress(void);

/**
 * @brief Get OTA update status
 * 
 * @param handle Pointer to OTA handle structure
 * @param progress Pointer to store progress percentage (0-100)
 * @return ota_state_t Current OTA state
 */
ota_state_t rboot_ota_get_status(ota_handle_t *handle, uint8_t *progress);

#ifdef __cplusplus
}
#endif

#endif /* __RBOOT_OTA_H__ */

/**
 * @file rboot-ota.c
 * @brief rBoot OTA (Over-The-Air) Update Implementation
 * @author Richard A Burton <richardaburton@gmail.com>
 * @version 1.0
 * @date 2023
 * 
 * @copyright Copyright (c) 2023 Richard A Burton. See license.txt for license terms.
 */

#include "rboot-ota.h"
#include "rboot-private.h"
#include <string.h>

// Static variables
static ota_handle_t *current_ota = NULL;

// Forward declarations
static ota_result_t verify_image(uint32_t addr, uint32_t length);
static uint32_t get_rom_address(uint8_t rom);

// External functions
extern uint32_t SPIRead(uint32_t addr, void *outptr, uint32_t len);
extern uint32_t SPIEraseSector(int sector);
extern uint32_t SPIWrite(uint32_t addr, void *inptr, uint32_t len);

// Internal buffer for OTA operations
static uint8_t ota_buffer[OTA_BUFFER_SIZE] __attribute__((aligned(4)));

// ROM header structure for verification
typedef struct {
    uint8_t magic;
    uint8_t count;
    uint8_t flags1;
    uint8_t flags2;
    void *entry;
} rom_header_t;

ota_result_t rboot_ota_begin(ota_handle_t *handle, uint8_t target_rom, uint32_t max_size) {
    if (!handle || target_rom >= MAX_ROMS) {
        return OTA_ERR_INVALID_ARGS;
    }

    // Check if another OTA is in progress
    if (rboot_ota_is_in_progress()) {
        return OTA_ERR_IN_PROGRESS;
    }

    // Initialize handle
    memset(handle, 0, sizeof(ota_handle_t));
    handle->target_rom = target_rom;
    handle->target_addr = get_rom_address(target_rom);
    handle->buffer = ota_buffer;
    handle->buffer_size = OTA_BUFFER_SIZE;
    handle->state = OTA_STATE_READY;
    
    // Allocate buffer if not already done
    if (!handle->buffer) {
        return OTA_ERR_NO_MEM;
    }

    // Set as current OTA operation
    current_ota = handle;
    handle->state = OTA_STATE_STARTED;
    
    return OTA_OK;
}

ota_result_t rboot_ota_write(ota_handle_t *handle, const uint8_t *data, uint32_t size) {
    if (!handle || !data || size == 0) {
        return OTA_ERR_INVALID_ARGS;
    }

    if (handle->state != OTA_STATE_STARTED && handle->state != OTA_STATE_WRITING) {
        return OTA_ERR_INVALID_ARGS;
    }

    handle->state = OTA_STATE_WRITING;
    uint32_t remaining = size;
    const uint8_t *src = data;
    
    while (remaining > 0) {
        uint32_t to_copy = (remaining < handle->buffer_size) ? remaining : handle->buffer_size;
        
        // Copy data to buffer
        memcpy(handle->buffer, src, to_copy);
        
        // Write to flash
        if (SPIWrite(handle->target_addr + handle->write_offset, handle->buffer, to_copy) != 0) {
            handle->state = OTA_STATE_ERROR;
            return OTA_ERR_WRITE;
        }
        
        // Update counters
        handle->write_offset += to_copy;
        handle->written_size += to_copy;
        src += to_copy;
        remaining -= to_copy;
        
        // Erase next sector if needed
        if ((handle->write_offset % SECTOR_SIZE) == 0) {
            int sector = (handle->target_addr + handle->write_offset) / SECTOR_SIZE;
            if (SPIEraseSector(sector) != 0) {
                handle->state = OTA_STATE_ERROR;
                return OTA_ERR_ERASE;
            }
        }
    }
    
    return OTA_OK;
}

ota_result_t rboot_ota_end(ota_handle_t *handle) {
    if (!handle || handle->state != OTA_STATE_WRITING) {
        return OTA_ERR_INVALID_ARGS;
    }

    // Verify the written data
    handle->state = OTA_STATE_VERIFYING;
    ota_result_t result = verify_image(handle->target_addr, handle->write_offset);
    
    if (result == OTA_OK) {
        handle->state = OTA_STATE_COMPLETE;
        // Set this as the boot ROM for next boot
        rboot_set_boot_rom(handle->target_rom);
    } else {
        handle->state = OTA_STATE_ERROR;
    }
    
    // Clear current OTA operation
    current_ota = NULL;
    
    return result;
}

void rboot_ota_cancel(ota_handle_t *handle) {
    if (handle) {
        handle->state = OTA_STATE_ERROR;
        if (current_ota == handle) {
            current_ota = NULL;
        }
    }
}

uint8_t rboot_ota_is_in_progress(void) {
    return (current_ota != NULL) ? 1 : 0;
}

ota_state_t rboot_ota_get_status(ota_handle_t *handle, uint8_t *progress) {
    if (!handle) {
        return OTA_STATE_ERROR;
    }
    
    if (progress) {
        if (handle->total_size > 0) {
            *progress = (handle->written_size * 100) / handle->total_size;
        } else {
            *progress = 0;
        }
    }
    
    return handle->state;
}

// Helper functions
static ota_result_t verify_image(uint32_t addr, uint32_t length) {
    // Simple verification: check ROM header magic number
    rom_header_t header;
    if (SPIRead(addr, &header, sizeof(header)) != 0) {
        return OTA_ERR_VERIFY;
    }
    
    if (header.magic != 0xE9) {  // Standard ROM magic number
        return OTA_ERR_VERIFY;
    }
    
    return OTA_OK;
}

static uint32_t get_rom_address(uint8_t rom) {
    // Get the ROM address from the configuration
    rboot_config config;
    if (SPIRead(BOOT_CONFIG_SECTOR * SECTOR_SIZE, &config, sizeof(config)) != 0) {
        return 0;
    }
    
    if (rom >= config.count) {
        return 0;
    }
    
    return config.roms[rom];
}

// Implement these functions in rboot.c
extern uint8_t rboot_get_current_rom(void);
extern ota_result_t rboot_set_boot_rom(uint8_t rom);

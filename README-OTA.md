# rBoot OTA and Factory Reset Features

This document describes the new features added to rBoot:

## 1. OTA (Over-The-Air) Updates

rBoot now includes support for OTA firmware updates. This allows you to update the firmware of your ESP8266 device wirelessly.

### Key Features:
- Support for multiple firmware images
- Safe update process with rollback capability
- Progress tracking and status reporting
- Configurable buffer size for efficient memory usage

### Usage:

1. Include the OTA header in your application:
   ```c
   #include "rboot-ota.h"
   ```

2. Initialize an OTA update:
   ```c
   ota_handle_t ota_handle;
   ota_result_t result = rboot_ota_begin(&ota_handle, TARGET_ROM, MAX_UPDATE_SIZE);
   ```

3. Write data in chunks:
   ```c
   result = rboot_ota_write(&ota_handle, data_chunk, chunk_size);
   ```

4. Finalize the update:
   ```c
   result = rboot_ota_end(&ota_handle);
   ```

## 2. Factory Reset

Added support for factory reset functionality to restore the device to its default settings.

### Usage:

1. Request a factory reset:
   ```c
   rboot_set_factory_reset(1);
   ```

2. The device will perform the factory reset on the next boot.

## 3. Status LED Feedback

Visual feedback during the boot process using an LED.

### Configuration:
- Default LED pin: GPIO2 (configurable via `LED_GPIO_NUM`)
- LED on = Active low (common for ESP8266 boards)

### Boot Sequence:
1. LED turns on during boot
2. LED blinks during factory reset
3. LED turns off when booting the application

## 4. Performance Optimizations

- Reduced stack and heap usage
- Optimized boot sequence for faster startup
- Improved memory management

## 5. Power Management

- Added support for low-power modes
- Improved power efficiency during boot

## Building with OTA Support

1. Enable OTA in your build configuration:
   ```
   #define BOOT_OTA_ENABLED
   ```

2. Link against the new rBoot library with OTA support.

## Example

```c
#include "rboot-ota.h"

void perform_ota_update() {
    ota_handle_t ota;
    
    // Start OTA update to ROM slot 1
    if (rboot_ota_begin(&ota, 1, 0x100000) == OTA_OK) {
        // Write firmware data in chunks
        while (more_data_available()) {
            size_t len = get_next_chunk(buffer, sizeof(buffer));
            if (rboot_ota_write(&ota, buffer, len) != OTA_OK) {
                // Handle error
                rboot_ota_cancel(&ota);
                return;
            }
        }
        
        // Finalize update
        if (rboot_ota_end(&ota) == OTA_OK) {
            // Update successful, will boot to new ROM on restart
        }
    }
}
```

## Limitations

- OTA updates require sufficient free flash space
- Factory reset will erase all configuration data
- LED feedback requires an external LED connected to the specified GPIO

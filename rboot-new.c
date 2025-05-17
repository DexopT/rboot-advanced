#include "rboot-private.h"
#include "rboot-ota.h"
#include <rboot-hex2a.h>

#ifndef UART_CLK_FREQ
// reset apb freq = 2x crystal freq: http://esp8266-re.foogod.com/wiki/Serial_UART
#define UART_CLK_FREQ	(26000000 * 2)
#endif

// LED control macros for status feedback
#ifndef LED_GPIO_NUM
#define LED_GPIO_NUM 2  // Default to GPIO2 (common for ESP-12 modules)
#endif

// Factory reset flag in RTC memory
#define FACTORY_RESET_MAGIC 0x1AC3F5E7
#define FACTORY_RESET_FLAG_ADDR (RBOOT_RTC_ADDR + 128) // After rBoot RTC data

// LED control functions
static inline void led_init(void) {
    // Configure LED GPIO as output
    GPIO_REG_WRITE(GPIO_ENABLE_W1TS_ADDRESS, (1 << LED_GPIO_NUM));
    // Turn LED off (active low for most ESP8266 boards)
    GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, (1 << LED_GPIO_NUM));
}

static inline void led_on(void) {
    // Turn LED on (active low)
    GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, (1 << LED_GPIO_NUM));
}

static inline void led_off(void) {
    // Turn LED off
    GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, (1 << LED_GPIO_NUM));
}

static inline void led_blink(int count, int delay_ms) {
    for (int i = 0; i < count; i++) {
        led_on();
        ets_delay_us(delay_ms * 1000);
        led_off();
        if (i < count - 1) {
            ets_delay_us(delay_ms * 1000);
        }
    }
}

// Check if factory reset is requested
static uint8_t check_factory_reset(void) {
    uint32_t flag = 0;
    system_rtc_mem(FACTORY_RESET_FLAG_ADDR, &flag, sizeof(flag), RBOOT_RTC_READ);
    return (flag == FACTORY_RESET_MAGIC);
}

// Set factory reset flag
void rboot_set_factory_reset(uint8_t enable) {
    uint32_t flag = enable ? FACTORY_RESET_MAGIC : 0;
    system_rtc_mem(FACTORY_RESET_FLAG_ADDR, &flag, sizeof(flag), RBOOT_RTC_WRITE);
}

// Get current boot ROM
uint8_t rboot_get_current_rom(void) {
    rboot_config romconf;
    SPIRead(BOOT_CONFIG_SECTOR * SECTOR_SIZE, &romconf, sizeof(romconf));
    return romconf.current_rom;
}

// Set boot ROM for next boot
uint8_t rboot_set_boot_rom(uint8_t rom) {
    rboot_config romconf;
    if (SPIRead(BOOT_CONFIG_SECTOR * SECTOR_SIZE, &romconf, sizeof(romconf)) != 0) {
        return 0;
    }
    
    if (rom >= romconf.count) {
        return 0;
    }
    
    romconf.current_rom = rom;
    
    // Write back to flash
    if (SPIEraseSector(BOOT_CONFIG_SECTOR) != 0) {
        return 0;
    }
    
    if (SPIWrite(BOOT_CONFIG_SECTOR * SECTOR_SIZE, &romconf, sizeof(romconf)) != 0) {
        return 0;
    }
    
    return 1;
}

// Perform factory reset
static void perform_factory_reset(void) {
    // Blink LED to indicate factory reset
    for (int i = 0; i < 5; i++) {
        led_on();
        ets_delay_us(100000); // 100ms
        led_off();
        ets_delay_us(100000);
    }
    
    // Reset to default configuration
    rboot_config romconf;
    uint32_t flashsize = 0x100000; // Default to 1MB
    
    // Get actual flash size if possible
    uint32_t id = 0;
    if (SPIRead(0, &id, 4) == 0) {
        // Parse flash ID to get size (simplified)
        if ((id & 0xFF) == 0x40) { // 16MBit
            flashsize = 0x200000;
        } else if ((id & 0xFF) == 0x30) { // 32MBit
            flashsize = 0x400000;
        }
    }
    
    // Create default config
    default_config(&romconf, flashsize);
    
    // Write config to flash
    SPIEraseSector(BOOT_CONFIG_SECTOR);
    SPIWrite(BOOT_CONFIG_SECTOR * SECTOR_SIZE, &romconf, sizeof(romconf));
    
    // Clear factory reset flag
    rboot_set_factory_reset(0);
    
    // Reboot
    asm volatile ("break 0,0");
}

// Rest of the existing code...
[Previous code content remains the same until call_user_start]

#ifdef BOOT_NO_ASM

// small stub method to ensure minimum stack space used
void call_user_start(void) {
    uint32_t addr;
    stage2a *loader;

    // Initialize LED for status feedback
    led_init();
    
    // Check for factory reset
    if (check_factory_reset()) {
        perform_factory_reset();
    }
    
    // Indicate boot start with LED
    led_on();
    
    // Find and boot image
    addr = find_image();
    
    // Indicate successful boot with LED
    led_off();
    
    if (addr != 0) {
        loader = (stage2a*)entry_addr;
        loader(addr);
    }
}

#else

// assembler stub uses no stack space
// works with gcc
void call_user_start(void) {
    // Initialize LED for status feedback
    led_init();
    
    // Check for factory reset
    if (check_factory_reset()) {
        perform_factory_reset();
    }
    
    // Indicate boot start with LED
    led_on();
    
    // Find and boot image
    __asm volatile (
        "call0 find_image\n\t"    // find a good rom to boot
        "movi a2, 0\n\t"          // clear a2 (just in case)
        "jx a0"                   // now jump to rom code
    );
}

#endif

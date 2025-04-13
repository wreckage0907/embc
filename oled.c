/**
 * OLED Display Test with Enhanced I2C Detection
 * 
 * Tests a 0.91" SSD1306 OLED display (128x32) connected via I2C
 * with improved debugging for I2C detection issues
 * 
 * Connections:
 * - SDA to GPIO 2 (Pin 4)
 * - SCL to GPIO 3 (Pin 5)
 * - VCC to 3.3V
 * - GND to GND
 */

 #include <stdio.h>
 #include <string.h>
 #include "pico/stdlib.h"
 #include "hardware/i2c.h"
 #include "pico/binary_info.h"
 
 // Pin definitions
 #define I2C_SDA_PIN 4
 #define I2C_SCL_PIN 5
 #define PICO_LED_PIN 25  // Built-in LED on most Pico boards
 
 // Try both common OLED addresses
 #define OLED_ADDR_1 0x3C
 #define OLED_ADDR_2 0x3D
 
 // 0.91 inch OLED is typically 128x32
 #define OLED_WIDTH 128
 #define OLED_HEIGHT 32
 #define OLED_PAGES (OLED_HEIGHT / 8)
 
 // SSD1306 Commands
 #define OLED_CONTROL_BYTE_CMD 0x00
 #define OLED_CONTROL_BYTE_DATA 0x40
 #define OLED_CMD_DISPLAY_OFF 0xAE
 #define OLED_CMD_DISPLAY_ON 0xAF
 
 // Global variables
 uint8_t g_oled_addr = 0; // Will store the detected OLED address
 
 // Function to scan the I2C bus for devices
 void scan_i2c_bus() {
     printf("\nScanning I2C bus for devices...\n");
     
     // Initialize the built-in LED for status indication
     gpio_init(PICO_LED_PIN);
     gpio_set_dir(PICO_LED_PIN, GPIO_OUT);
     
     // Try each address in turn
     for (int addr = 0; addr < 128; addr++) {
         // Skip invalid I2C addresses
         if (addr < 0x08 || addr > 0x77) continue;
         
         // Toggle LED for visual feedback
         gpio_put(PICO_LED_PIN, addr % 2);
         
         // This is a simple way to check if a device responds at this address
         uint8_t rxdata;
         int ret = i2c_read_blocking(i2c0, addr, &rxdata, 1, false);
         
         // If device is detected, print its address
         if (ret >= 0) {
             printf("I2C device found at address 0x%02X\n", addr);
             
             // Check if this is potentially our OLED display
             if (addr == OLED_ADDR_1 || addr == OLED_ADDR_2) {
                 g_oled_addr = addr;
                 printf("Potential OLED display detected at 0x%02X\n", addr);
             }
         }
     }
     
     if (g_oled_addr == 0) {
         printf("No OLED display found on the I2C bus!\n");
     }
     
     printf("I2C bus scan complete.\n\n");
 }
 
 // Function to send command to OLED
 bool ssd1306_cmd(uint8_t cmd) {
     uint8_t buf[2] = {OLED_CONTROL_BYTE_CMD, cmd};
     int ret = i2c_write_blocking(i2c0, g_oled_addr, buf, 2, false);
     return (ret > 0);
 }
 
 // Function to initialize OLED
 bool ssd1306_init() {
     // Just try the most basic commands for testing
     bool success = true;
     
     // Turn off display
     success &= ssd1306_cmd(OLED_CMD_DISPLAY_OFF);
     sleep_ms(1);
     
     // Turn on display
     success &= ssd1306_cmd(OLED_CMD_DISPLAY_ON);
     
     printf("OLED initialization %s\n", success ? "successful" : "failed");
     return success;
 }
 
 int main() {
     // Initialize UART for debug output
     stdio_init_all();
     sleep_ms(3000);  // Wait for serial to initialize
     
     printf("\nEnhanced OLED Display Test\n");
     
     // Initialize I2C with slower speed for better compatibility
     printf("Initializing I2C at 100kHz\n");
     i2c_init(i2c0, 100000);  // 100 kHz
     
     // Configure pins
     printf("Configuring I2C pins: SDA=%d, SCL=%d\n", I2C_SDA_PIN, I2C_SCL_PIN);
     gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
     gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
     
     // Enable pull-ups
     printf("Enabling internal pull-ups on I2C pins\n");
     gpio_pull_up(I2C_SDA_PIN);
     gpio_pull_up(I2C_SCL_PIN);
     
     // Scan the I2C bus to find all connected devices
     scan_i2c_bus();
     
     // If we didn't find an OLED display, try the common addresses anyway
     if (g_oled_addr == 0) {
         printf("Trying common OLED addresses directly...\n");
         
         // Try first common address
         printf("Trying address 0x%02X...\n", OLED_ADDR_1);
         g_oled_addr = OLED_ADDR_1;
         if (ssd1306_init()) {
             printf("OLED responded at 0x%02X!\n", OLED_ADDR_1);
         } else {
             // Try second common address
             printf("Trying address 0x%02X...\n", OLED_ADDR_2);
             g_oled_addr = OLED_ADDR_2;
             if (ssd1306_init()) {
                 printf("OLED responded at 0x%02X!\n", OLED_ADDR_2);
             } else {
                 g_oled_addr = 0;
                 printf("OLED not responding to either common address.\n");
             }
         }
     } else {
         // Try to initialize the OLED display
         ssd1306_init();
     }
     
     // If we found an OLED, indicate success
     if (g_oled_addr != 0) {
         printf("OLED display detected and initialized at address 0x%02X\n", g_oled_addr);
     } else {
         printf("OLED display detection failed!\n");
         printf("Please check your connections and try again.\n");
     }
     
     // Blink the LED to indicate test completion status
     gpio_init(PICO_LED_PIN);
     gpio_set_dir(PICO_LED_PIN, GPIO_OUT);
     
     while (1) {
         if (g_oled_addr != 0) {
             // Slow blink for success
             gpio_put(PICO_LED_PIN, 1);
             sleep_ms(1000);
             gpio_put(PICO_LED_PIN, 0);
             sleep_ms(1000);
         } else {
             // Fast blink for failure
             gpio_put(PICO_LED_PIN, 1);
             sleep_ms(200);
             gpio_put(PICO_LED_PIN, 0);
             sleep_ms(200);
         }
     }
     
     return 0;
 }
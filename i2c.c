/**
 * I2C Diagnostic Test for Raspberry Pi Pico
 * 
 * This program tests both I2C peripherals on the Raspberry Pi Pico
 * by scanning for connected devices and reporting their addresses.
 * 
 * Connection:
 * - I2C0: GPIO 0 (SDA), GPIO 1 (SCL)
 * - I2C1: GPIO 2 (SDA), GPIO 3 (SCL)
 * 
 * Build with Pico SDK using CMake
 */

 #include <stdio.h>
 #include "pico/stdlib.h"
 #include "hardware/i2c.h"
 #include "pico/binary_info.h"
 
 // I2C Configurations
 #define I2C0_SDA_PIN 0
 #define I2C0_SCL_PIN 1
 #define I2C1_SDA_PIN 2
 #define I2C1_SCL_PIN 3
 #define I2C_FREQ 100000  // 100 kHz
 
 // Function to scan I2C bus and detect devices
 void scan_i2c_bus(i2c_inst_t *i2c_instance, const char *i2c_name) {
     printf("\n%s I2C Bus Scan\n", i2c_name);
     printf("   0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n");
 
     // Check for devices at each address
     int devices_found = 0;
     for (int base = 0; base < 8; base++) {
         printf("%02x ", base * 16);
         for (int addr = 0; addr < 16; addr++) {
             int address = base * 16 + addr;
             
             // Skip addresses that are invalid for I2C
             if (address < 0x08 || address > 0x77) {
                 printf("   ");
                 continue;
             }
             
             // Create a single-byte buffer for the I2C operation
             uint8_t rxdata;
             
             // Use a read operation with timeout to check if device responds
             int result = i2c_read_timeout_us(i2c_instance, address, &rxdata, 1, false, 5000);
             
             if (result >= 0) {
                 // Device responded
                 printf("%02x ", address);
                 devices_found++;
             } else {
                 // No response
                 printf("-- ");
             }
         }
         printf("\n");
     }
 
     if (devices_found == 0) {
         printf("\nNo devices detected on %s bus. This may indicate:\n", i2c_name);
         printf("1. No I2C devices are connected\n");
         printf("2. There might be incorrect wiring\n");
         printf("3. Devices might be at unexpected addresses\n");
         printf("4. The %s peripheral may be damaged\n", i2c_name);
     } else {
         printf("\n%d device(s) found on %s bus\n", devices_found, i2c_name);
         printf("The %s peripheral is working!\n", i2c_name);
     }
 }
 
 // Try to read a specific number of bytes from an address
 // Returns true if successful, false otherwise
 bool test_i2c_device(i2c_inst_t *i2c_instance, uint8_t addr, uint8_t *rxdata, int len) {
     int ret = i2c_read_timeout_us(i2c_instance, addr, rxdata, len, false, 10000);
     return (ret >= 0);
 }
 
 int main() {
     // Initialize Pico standard library (for UART output)
     stdio_init_all();
     
     // Give the user's terminal time to connect
     sleep_ms(2000);
     
     printf("\n\nRaspberry Pi Pico I2C Diagnostic Test\n");
     printf("=====================================\n");
     
     // Test the status LED first to show the program is running
     const uint LED_PIN = PICO_DEFAULT_LED_PIN;
     gpio_init(LED_PIN);
     gpio_set_dir(LED_PIN, GPIO_OUT);
     
     printf("Testing onboard LED on pin %d...\n", LED_PIN);
     for (int i = 0; i < 3; i++) {
         gpio_put(LED_PIN, 1);
         printf("LED ON\n");
         sleep_ms(300);
         gpio_put(LED_PIN, 0);
         printf("LED OFF\n");
         sleep_ms(300);
     }
     
     // Initialize I2C0
     printf("\nInitializing I2C0 (SDA: %d, SCL: %d)...\n", I2C0_SDA_PIN, I2C0_SCL_PIN);
     i2c_init(i2c0, I2C_FREQ);
     gpio_set_function(I2C0_SDA_PIN, GPIO_FUNC_I2C);
     gpio_set_function(I2C0_SCL_PIN, GPIO_FUNC_I2C);
     
     // Enable internal pull-ups for I2C0
     gpio_pull_up(I2C0_SDA_PIN);
     gpio_pull_up(I2C0_SCL_PIN);
     
     // Initialize I2C1
     printf("Initializing I2C1 (SDA: %d, SCL: %d)...\n", I2C1_SDA_PIN, I2C1_SCL_PIN);
     i2c_init(i2c1, I2C_FREQ);
     gpio_set_function(I2C1_SDA_PIN, GPIO_FUNC_I2C);
     gpio_set_function(I2C1_SCL_PIN, GPIO_FUNC_I2C);
     
     // Enable internal pull-ups for I2C1
     gpio_pull_up(I2C1_SDA_PIN);
     gpio_pull_up(I2C1_SCL_PIN);
     
     // Scan both I2C buses
     scan_i2c_bus(i2c0, "I2C0");
     scan_i2c_bus(i2c1, "I2C1");
     
     // Keep the LED blinking to show the program is still running
     printf("\nTest complete! Blinking LED indefinitely...\n");
     while (1) {
         gpio_put(LED_PIN, 1);
         sleep_ms(500);
         gpio_put(LED_PIN, 0);
         sleep_ms(500);
     }
     
     return 0;
 }
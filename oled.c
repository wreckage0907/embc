/**
 * Improved OLED Display Test using I2C0 peripheral
 * 
 * Tests a 0.91" SSD1306 OLED display (128x32) connected via I2C0
 * Actually displays content on the screen
 * 
 * Connections:
 * - SDA to GPIO 0 (Pin 1)
 * - SCL to GPIO 1 (Pin 2)
 * - VCC to 3.3V
 * - GND to GND
 */

 #include <stdio.h>
 #include <string.h>
 #include "pico/stdlib.h"
 #include "hardware/i2c.h"
 #include "pico/binary_info.h"
 
 // Pin definitions for I2C0    
 #define I2C_SDA_PIN 0
 #define I2C_SCL_PIN 1
 #define PICO_LED_PIN 25  // Built-in LED on Pico
 
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
 #define OLED_CMD_SET_CONTRAST 0x81
 #define OLED_CMD_RESUME_TO_RAM_CONTENT 0xA4
 #define OLED_CMD_NORMAL_DISPLAY 0xA6
 #define OLED_CMD_SET_MULTIPLEX 0xA8
 #define OLED_CMD_SET_DISPLAY_OFFSET 0xD3
 #define OLED_CMD_SET_DISPLAY_CLOCK_DIV 0xD5
 #define OLED_CMD_SET_PRECHARGE 0xD9
 #define OLED_CMD_SET_COM_PINS 0xDA
 #define OLED_CMD_SET_VCOM_DETECT 0xDB
 #define OLED_CMD_SET_MEMORY_ADDR_MODE 0x20
 #define OLED_CMD_SET_COLUMN_ADDR 0x21
 #define OLED_CMD_SET_PAGE_ADDR 0x22
 #define OLED_CMD_SET_CHARGE_PUMP 0x8D
 #define OLED_CMD_DEACTIVATE_SCROLL 0x2E
 
 // Global variables
 uint8_t g_oled_addr = 0; // Will store the detected OLED address
 uint8_t g_buffer[OLED_WIDTH * OLED_PAGES]; // Buffer to hold display data
 
 // Function to scan the I2C bus for devices
 void scan_i2c_bus() {
     printf("\nScanning I2C0 bus for devices...\n");
     
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
         printf("No OLED display found on the I2C0 bus!\n");
     }
     
     printf("I2C0 bus scan complete.\n\n");
 }
 
 // Function to send command to OLED
 bool ssd1306_cmd(uint8_t cmd) {
     uint8_t buf[2] = {OLED_CONTROL_BYTE_CMD, cmd};
     int ret = i2c_write_blocking(i2c0, g_oled_addr, buf, 2, false);
     return (ret > 0);
 }
 
 // Function to send multiple commands to OLED
 bool ssd1306_cmd_list(const uint8_t *cmds, int num_cmds) {
     bool success = true;
     for (int i = 0; i < num_cmds; i++) {
         success &= ssd1306_cmd(cmds[i]);
     }
     return success;
 }
 
 // Function to send data to OLED
 bool ssd1306_data(const uint8_t *data, size_t len) {
     uint8_t *buf = malloc(len + 1);
     if (buf == NULL) {
         printf("Failed to allocate memory for data transfer\n");
         return false;
     }
     
     buf[0] = OLED_CONTROL_BYTE_DATA;
     memcpy(&buf[1], data, len);
     
     int ret = i2c_write_blocking(i2c0, g_oled_addr, buf, len + 1, false);
     free(buf);
     
     return (ret > 0);
 }
 
 // Function to clear the display buffer
 void clear_buffer() {
     memset(g_buffer, 0, sizeof(g_buffer));
 }
 
 // Function to render buffer to OLED
 bool ssd1306_render() {
     // Set column address range
     uint8_t col_cmds[] = {
         OLED_CMD_SET_COLUMN_ADDR,
         0,                  // Start column
         OLED_WIDTH - 1      // End column
     };
     
     // Set page address range
     uint8_t page_cmds[] = {
         OLED_CMD_SET_PAGE_ADDR,
         0,                  // Start page
         OLED_PAGES - 1      // End page
     };
     
     bool success = true;
     success &= ssd1306_cmd_list(col_cmds, 3);
     success &= ssd1306_cmd_list(page_cmds, 3);
     success &= ssd1306_data(g_buffer, sizeof(g_buffer));
     
     return success;
 }
 
 // Function to initialize OLED
 bool ssd1306_init() {
     // Full initialization sequence for SSD1306
     uint8_t init_cmds[] = {
         OLED_CMD_DISPLAY_OFF,           // 0xAE
         OLED_CMD_SET_DISPLAY_CLOCK_DIV, // 0xD5
         0x80,                           // Suggested ratio
         OLED_CMD_SET_MULTIPLEX,         // 0xA8
         OLED_HEIGHT - 1,                // For 32 pixel height
         OLED_CMD_SET_DISPLAY_OFFSET,    // 0xD3
         0x00,                           // No offset
         0x40,                           // Set start line to 0
         OLED_CMD_SET_CHARGE_PUMP,       // 0x8D
         0x14,                           // Enable charge pump
         OLED_CMD_SET_MEMORY_ADDR_MODE,  // 0x20
         0x00,                           // Horizontal addressing mode
         0xA1,                           // Segment remap (flips display horizontally)
         0xC8,                           // COM Output scan direction (flips vertically)
         OLED_CMD_SET_COM_PINS,          // 0xDA
         0x02,                           // For 128x32 displays
         OLED_CMD_SET_CONTRAST,          // 0x81
         0x8F,                           // Medium contrast (adjust as needed)
         OLED_CMD_SET_PRECHARGE,         // 0xD9
         0xF1,                           // Default for SSD1306
         OLED_CMD_SET_VCOM_DETECT,       // 0xDB
         0x40,                           // Default value
         OLED_CMD_RESUME_TO_RAM_CONTENT, // 0xA4
         OLED_CMD_NORMAL_DISPLAY,        // 0xA6
         OLED_CMD_DEACTIVATE_SCROLL,     // 0x2E
         OLED_CMD_DISPLAY_ON             // 0xAF
     };
 
     bool success = ssd1306_cmd_list(init_cmds, sizeof(init_cmds));
 
     // Clear the display after initialization
     clear_buffer();
     success &= ssd1306_render();
     
     printf("OLED initialization %s\n", success ? "successful" : "failed");
     return success;
 }
 
 // Function to set a single pixel in the buffer
 void set_pixel(int x, int y, bool on) {
     if (x < 0 || x >= OLED_WIDTH || y < 0 || y >= OLED_HEIGHT) {
         return;  // Out of bounds
     }
     
     // Calculate the byte position and bit position within that byte
     int byte_pos = x + (y / 8) * OLED_WIDTH;
     int bit_pos = y % 8;
     
     if (on) {
         g_buffer[byte_pos] |= (1 << bit_pos);
     } else {
         g_buffer[byte_pos] &= ~(1 << bit_pos);
     }
 }
 
 // Function to draw a filled rectangle
 void draw_filled_rect(int x, int y, int width, int height) {
     for (int i = x; i < x + width; i++) {
         for (int j = y; j < y + height; j++) {
             set_pixel(i, j, true);
         }
     }
 }
 
 // Function to create a test pattern
 void create_test_pattern() {
     clear_buffer();
     
     // Draw a border
     for (int i = 0; i < OLED_WIDTH; i++) {
         set_pixel(i, 0, true);
         set_pixel(i, OLED_HEIGHT - 1, true);
     }
     for (int i = 0; i < OLED_HEIGHT; i++) {
         set_pixel(0, i, true);
         set_pixel(OLED_WIDTH - 1, i, true);
     }
     
     // Draw some rectangles
     draw_filled_rect(5, 5, 20, 10);
     draw_filled_rect(40, 15, 20, 10);
     draw_filled_rect(80, 8, 20, 15);
     
     // Draw a diagonal line
     for (int i = 10; i < OLED_WIDTH - 10; i++) {
         set_pixel(i, i / 4, true);
     }
 }
 
 int main() {
     // Initialize UART for debug output
     stdio_init_all();
     sleep_ms(3000);  // Wait for serial to initialize
     
     printf("\nImproved OLED Display Test using I2C0 peripheral\n");
     
     // Initialize I2C0 with slower speed for better compatibility
     printf("Initializing I2C0 at 100kHz\n");
     i2c_init(i2c0, 100000);  // 100 kHz for normal operation
     
     // Configure pins
     printf("Configuring I2C0 pins: SDA=GPIO %d, SCL=GPIO %d\n", I2C_SDA_PIN, I2C_SCL_PIN);
     gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
     gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
     
     // Enable pull-ups
     printf("Enabling internal pull-ups on I2C0 pins\n");
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
     
     // If we found an OLED, draw a test pattern
     if (g_oled_addr != 0) {
         printf("Drawing test pattern on OLED display...\n");
         
         create_test_pattern();
         if (ssd1306_render()) {
             printf("Test pattern rendered successfully!\n");
         } else {
             printf("Failed to render test pattern.\n");
         }
     } else {
         printf("OLED display detection failed!\n");
         printf("Please check your connections and try again.\n");
         printf("Consider adding external 4.7kΩ pull-up resistors.\n");
     }
     
     // Blink the LED to indicate test completion status
     gpio_init(PICO_LED_PIN);
     gpio_set_dir(PICO_LED_PIN, GPIO_OUT);
     
     int counter = 0;
     while (1) {
         if (g_oled_addr != 0) {
             // Slow blink for success
             gpio_put(PICO_LED_PIN, 1);
             sleep_ms(1000);
             gpio_put(PICO_LED_PIN, 0);
             sleep_ms(1000);
             
             // Every 10 cycles, change the display
             counter++;
             if (counter % 5 == 0) {
                 // Invert the display for visual feedback
                 ssd1306_cmd(counter % 10 == 0 ? 0xA6 : 0xA7); // Toggle normal/inverted display
                 
                 // Optionally create a new pattern
                 if (counter % 20 == 0) {
                     clear_buffer();
                     for (int i = 0; i < OLED_WIDTH; i += 8) {
                         for (int j = 0; j < OLED_HEIGHT; j += 8) {
                             if ((i + j) % 16 == 0) {
                                 draw_filled_rect(i, j, 4, 4);
                             }
                         }
                     }
                     ssd1306_render();
                 }
             }
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
/**
 * Environmental Monitoring System for Raspberry Pi Pico
 * 
 * Connections:
 * - DHT22 Data pin to GPIO 16
 * - MQ135 AO (Analog Output) to GPIO 26 (ADC0)
 * - SSD1306 OLED SDA to GPIO 0 (I2C0 SDA)
 * - SSD1306 OLED SCL to GPIO 1 (I2C0 SCL)
 */

 #include <stdio.h>
 #include <string.h>
 #include <math.h>
 #include "pico/stdlib.h"
 #include "hardware/gpio.h"
 #include "hardware/adc.h"
 #include "hardware/i2c.h"
 #include "pico/time.h"
 
 // Pin definitions
 #define DHT_PIN 16
 #define MQ135_PIN 26
 #define LED_PIN 25
 
 // I2C pins for OLED
 #define I2C_SDA_PIN 0
 #define I2C_SCL_PIN 1
 
 // DHT22 timeout
 #define DHT_TIMEOUT_US 100000
 
 // MQ135 parameters
 #define VOLTAGE_REF 3.3
 #define ADC_RESOLUTION 4095
 #define R_LOAD 10.0
 #define RZERO 76.63
 #define PARA 116.6020682
 
 // OLED display parameters
 #define OLED_WIDTH 128
 #define OLED_HEIGHT 32
 #define OLED_PAGES (OLED_HEIGHT / 8)
 #define OLED_ADDRESS 0x3C
 
 // OLED commands
 #define OLED_CONTROL_BYTE_CMD 0x00
 #define OLED_CONTROL_BYTE_DATA 0x40
 #define OLED_CMD_DISPLAY_OFF 0xAE
 #define OLED_CMD_DISPLAY_ON 0xAF
 #define OLED_CMD_DISPLAY_NORMAL 0xA6
 #define OLED_CMD_SET_CONTRAST 0x81
 #define OLED_CMD_SET_MEMORY_MODE 0x20
 #define OLED_CMD_SET_DISPLAY_OFFSET 0xD3
 #define OLED_CMD_SET_START_LINE 0x40
 #define OLED_CMD_SEG_REMAP 0xA0
 #define OLED_CMD_COM_SCAN_DEC 0xC8
 #define OLED_CMD_SET_DISPLAY_CLOCK_DIV 0xD5
 #define OLED_CMD_SET_PRECHARGE 0xD9
 #define OLED_CMD_SET_VCOM_DETECT 0xDB
 #define OLED_CMD_SET_MULTIPLEX 0xA8
 #define OLED_CMD_CHARGE_PUMP 0x8D
 #define OLED_CMD_SET_COM_PINS 0xDA
 #define OLED_CMD_COLUMN_ADDR 0x21
 #define OLED_CMD_PAGE_ADDR 0x22
 
 // Buffer for OLED display
 uint8_t buffer[OLED_WIDTH * OLED_PAGES];
 
 // DHT22 data structure
 typedef struct {
     float humidity;
     float temp;
     bool error;
 } dht_reading;
 
 // Structure to hold all sensor data
 typedef struct {
     dht_reading dht;
     float co2_ppm;
     int aqi;
 } sensor_data;
 
 // Function prototypes
 dht_reading read_dht22();
 float get_resistance(uint16_t adc_value);
 float get_ppm(float ratio);
 int calculate_aqi(float ppm);
 bool ssd1306_init();
 void ssd1306_cmd(uint8_t cmd);
 void ssd1306_clear();
 void ssd1306_display();
 void draw_char(int x, int y, char c);
 void draw_string(int x, int y, const char* str);
 void draw_char_2x(int x, int y, char c);
 void draw_string_2x(int x, int y, const char* str);
 const char* get_air_quality_label(float ppm);
 
 // OLED address
 uint8_t oled_address = OLED_ADDRESS;
 bool oled_found = false;
 
 // 5x8 character set (only including 0-9 and a few symbols to save space)
 const uint8_t font[] = {
     0x00, 0x00, 0x00, 0x00, 0x00, // Space
     0x00, 0x00, 0x5F, 0x00, 0x00, // !
     0x00, 0x07, 0x00, 0x07, 0x00, // "
     0x14, 0x7F, 0x14, 0x7F, 0x14, // #
     0x24, 0x2A, 0x7F, 0x2A, 0x12, // $
     0x23, 0x13, 0x08, 0x64, 0x62, // %
     0x36, 0x49, 0x55, 0x22, 0x50, // &
     0x00, 0x05, 0x03, 0x00, 0x00, // '
     0x00, 0x1C, 0x22, 0x41, 0x00, // (
     0x00, 0x41, 0x22, 0x1C, 0x00, // )
     0x08, 0x2A, 0x1C, 0x2A, 0x08, // *
     0x08, 0x08, 0x3E, 0x08, 0x08, // +
     0x00, 0x50, 0x30, 0x00, 0x00, // ,
     0x08, 0x08, 0x08, 0x08, 0x08, // -
     0x00, 0x60, 0x60, 0x00, 0x00, // .
     0x20, 0x10, 0x08, 0x04, 0x02, // /
     0x3E, 0x51, 0x49, 0x45, 0x3E, // 0
     0x00, 0x42, 0x7F, 0x40, 0x00, // 1
     0x42, 0x61, 0x51, 0x49, 0x46, // 2
     0x21, 0x41, 0x45, 0x4B, 0x31, // 3
     0x18, 0x14, 0x12, 0x7F, 0x10, // 4
     0x27, 0x45, 0x45, 0x45, 0x39, // 5
     0x3C, 0x4A, 0x49, 0x49, 0x30, // 6
     0x01, 0x71, 0x09, 0x05, 0x03, // 7
     0x36, 0x49, 0x49, 0x49, 0x36, // 8
     0x06, 0x49, 0x49, 0x29, 0x1E, // 9
     0x00, 0x36, 0x36, 0x00, 0x00, // :
     0x00, 0x56, 0x36, 0x00, 0x00, // ;
     0x00, 0x08, 0x14, 0x22, 0x41, // <
     0x14, 0x14, 0x14, 0x14, 0x14, // =
     0x41, 0x22, 0x14, 0x08, 0x00, // >
     0x02, 0x01, 0x51, 0x09, 0x06, // ?
     0x32, 0x49, 0x79, 0x41, 0x3E, // @
     0x7E, 0x11, 0x11, 0x11, 0x7E, // A
     0x7F, 0x49, 0x49, 0x49, 0x36, // B
     0x3E, 0x41, 0x41, 0x41, 0x22, // C
     0x7F, 0x41, 0x41, 0x22, 0x1C, // D
     0x7F, 0x49, 0x49, 0x49, 0x41, // E
     0x7F, 0x09, 0x09, 0x01, 0x01, // F
     0x3E, 0x41, 0x41, 0x49, 0x7A, // G
     0x7F, 0x08, 0x08, 0x08, 0x7F, // H
     0x00, 0x41, 0x7F, 0x41, 0x00, // I
     0x20, 0x40, 0x41, 0x3F, 0x01, // J
     0x7F, 0x08, 0x14, 0x22, 0x41, // K
     0x7F, 0x40, 0x40, 0x40, 0x40, // L
     0x7F, 0x02, 0x04, 0x02, 0x7F, // M
     0x7F, 0x04, 0x08, 0x10, 0x7F, // N
     0x3E, 0x41, 0x41, 0x41, 0x3E, // O
     0x7F, 0x09, 0x09, 0x09, 0x06, // P
     0x3E, 0x41, 0x51, 0x21, 0x5E, // Q
     0x7F, 0x09, 0x19, 0x29, 0x46, // R
     0x46, 0x49, 0x49, 0x49, 0x31, // S
     0x01, 0x01, 0x7F, 0x01, 0x01, // T
     0x3F, 0x40, 0x40, 0x40, 0x3F, // U
     0x1F, 0x20, 0x40, 0x20, 0x1F, // V
     0x7F, 0x20, 0x18, 0x20, 0x7F, // W
     0x63, 0x14, 0x08, 0x14, 0x63, // X
     0x03, 0x04, 0x78, 0x04, 0x03, // Y
     0x61, 0x51, 0x49, 0x45, 0x43, // Z
 };
 
 int main() {
     stdio_init_all();
     sleep_ms(2000);
     
     printf("Environmental Monitoring System\n");
     
     // Initialize the DHT22 pin
     gpio_init(DHT_PIN);
     gpio_set_pulls(DHT_PIN, true, false);  // Enable pull-up
     
     // Initialize ADC for MQ135
     adc_init();
     adc_gpio_init(MQ135_PIN);
     adc_select_input(0);  // ADC0 corresponds to GPIO26
     
     // Initialize LED
     gpio_init(LED_PIN);
     gpio_set_dir(LED_PIN, GPIO_OUT);
     
     // Initialize I2C for OLED display
     i2c_init(i2c0, 400 * 1000);
     gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
     gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
     gpio_pull_up(I2C_SDA_PIN);
     gpio_pull_up(I2C_SCL_PIN);
     
     // Warm-up period for MQ135 sensor
     printf("Warming up MQ135 sensor (30 seconds)...\n");
     for (int i = 0; i < 30; i++) {
         gpio_put(LED_PIN, 1);
         sleep_ms(500);
         gpio_put(LED_PIN, 0);
         sleep_ms(500);
         printf(".");
         if (i % 10 == 9) printf("\n");
     }
     printf("\nSensor warm-up complete!\n\n");
     
     // Initialize OLED display
     oled_found = ssd1306_init();
     if (!oled_found) {
         printf("OLED display not found or not responding!\n");
     }
     
     // Main loop variables
     char line_buffer[32];
     int display_mode = 0;  // 0 = Temperature, 1 = Humidity, 2 = AQI
     uint32_t last_reading_time = 0;
     
     // Initial sensor data
     sensor_data current_data = {
         .dht = {.humidity = 0, .temp = 0, .error = true},
         .co2_ppm = 0,
         .aqi = 0
     };
     
     while (1) {
         // Read from DHT22 (only every 2 seconds as per DHT22 minimum sampling period)
         uint32_t current_time = time_us_32() / 1000000;
         if (current_time - last_reading_time >= 2) {
             // Update time
             last_reading_time = current_time;
             
             // Read temperature and humidity
             dht_reading reading = read_dht22();
             if (!reading.error) {
                 current_data.dht = reading;
             }
             
             // Read air quality data
             uint16_t adc_raw = adc_read();
             float rs = get_resistance(adc_raw);
             float ratio = rs / RZERO;
             float ppm = get_ppm(ratio);
             current_data.co2_ppm = ppm;
             
             // Calculate AQI
             current_data.aqi = calculate_aqi(ppm);
             
             // Blink LED to indicate reading
             gpio_put(LED_PIN, 1);
             sleep_ms(100);
             gpio_put(LED_PIN, 0);
         }
         
         // Display data on OLED
         if (oled_found) {
             // Clear display buffer
             ssd1306_clear();
             
             // Display data depending on display mode
             if (display_mode == 0) {
                 // Temperature display
                 if (!current_data.dht.error) {
                     snprintf(line_buffer, sizeof(line_buffer), "T:%.1fC", current_data.dht.temp);
                 } else {
                     snprintf(line_buffer, sizeof(line_buffer), "T:ERROR");
                 }
                 
                 // Center the text
                 int text_width = strlen(line_buffer) * 12;
                 int x_pos = (OLED_WIDTH - text_width) / 2;
                 if (x_pos < 0) x_pos = 0;
                 
                 draw_string_2x(x_pos, 8, line_buffer);
                 printf("Temperature: %.1f°C\n", current_data.dht.temp);
             } else if (display_mode == 1) {
                 // Humidity display
                 if (!current_data.dht.error) {
                     snprintf(line_buffer, sizeof(line_buffer), "H:%.1f%%", current_data.dht.humidity);
                 } else {
                     snprintf(line_buffer, sizeof(line_buffer), "H:ERROR");
                 }
                 
                 // Center the text
                 int text_width = strlen(line_buffer) * 12;
                 int x_pos = (OLED_WIDTH - text_width) / 2;
                 if (x_pos < 0) x_pos = 0;
                 
                 draw_string_2x(x_pos, 8, line_buffer);
                 printf("Humidity: %.1f%%\n", current_data.dht.humidity);
             } else {
                 // AQI display
                 // Format CO2 value for OLED display
                snprintf(line_buffer, sizeof(line_buffer), "CO2:%d", (int)current_data.co2_ppm);

                // Center the text
                int text_width = strlen(line_buffer) * 12;
                int x_pos = (OLED_WIDTH - text_width) / 2;
                if (x_pos < 0) x_pos = 0;

                // Draw on OLED
                draw_string_2x(x_pos, 8, line_buffer);

                // Get air quality label for serial output
                const char* quality_label = get_air_quality_label(current_data.co2_ppm);

                // Print both CO2 and quality label to serial
                printf("CO2: %.2f ppm - Air Quality: %s\n", current_data.co2_ppm, quality_label);
             }
             
             // Update the display
             ssd1306_display();
         }
         
         // Cycle through display modes every 3 seconds
         display_mode = (display_mode + 1) % 3;
         
         // Wait before updating the display again
         sleep_ms(3000);
     }
     
     return 0;
 }
 
 dht_reading read_dht22() {
     dht_reading result = {0.0f, 0.0f, false};
     uint8_t data[5] = {0, 0, 0, 0, 0};
     uint8_t bit_index = 7;
     uint8_t byte_index = 0;
     
     // Start signal: pull low for at least 1ms, then pull high
     gpio_set_dir(DHT_PIN, GPIO_OUT);
     gpio_put(DHT_PIN, 0);
     sleep_ms(1);               // Pull low for 1ms
     
     // Enable the internal pull-up
     gpio_set_pulls(DHT_PIN, true, false);
     
     // Release the line and switch to input
     gpio_put(DHT_PIN, 1);
     sleep_us(40);              // Wait for 40us
     gpio_set_dir(DHT_PIN, GPIO_IN);
     
     // Wait for DHT22 to pull low (response signal)
     uint64_t timeout_time = time_us_64() + DHT_TIMEOUT_US;
     while (gpio_get(DHT_PIN) == 1) {
         if (time_us_64() > timeout_time) {
             result.error = true;
             return result;
         }
     }
     
     // DHT22 pulls low for 80us, then high for 80us
     timeout_time = time_us_64() + DHT_TIMEOUT_US;
     while (gpio_get(DHT_PIN) == 0) {
         if (time_us_64() > timeout_time) {
             result.error = true;
             return result;
         }
     }
     
     timeout_time = time_us_64() + DHT_TIMEOUT_US;
     while (gpio_get(DHT_PIN) == 1) {
         if (time_us_64() > timeout_time) {
             result.error = true;
             return result;
         }
     }
     
     // Read the 40 bits (5 bytes) of data
     for (int i = 0; i < 40; i++) {
         // Each bit starts with a 50us low pulse
         timeout_time = time_us_64() + DHT_TIMEOUT_US;
         while (gpio_get(DHT_PIN) == 0) {
             if (time_us_64() > timeout_time) {
                 result.error = true;
                 return result;
             }
         }
         
         // Duration of high pulse determines bit value
         // ~26-28us for '0', ~70us for '1'
         uint64_t high_start = time_us_64();
         timeout_time = high_start + DHT_TIMEOUT_US;
         
         while (gpio_get(DHT_PIN) == 1) {
             if (time_us_64() > timeout_time) {
                 result.error = true;
                 return result;
             }
         }
         
         uint64_t high_duration = time_us_64() - high_start;
         
         // If high pulse is longer than ~40us, it's a '1' bit
         if (high_duration > 40) {
             data[byte_index] |= (1 << bit_index);
         }
         
         // Move to next bit
         if (bit_index == 0) {
             bit_index = 7;
             byte_index++;
         } else {
             bit_index--;
         }
     }
     
     // Verify checksum (last byte should equal sum of first 4 bytes)
     if (data[4] != ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) {
         result.error = true;
         return result;
     }
     
     // Convert the data to humidity and temperature values
     result.humidity = ((data[0] << 8) | data[1]) / 10.0f;
     
     // Temperature might be negative
     if (data[2] & 0x80) {
         result.temp = -((((data[2] & 0x7F) << 8) | data[3]) / 10.0f);
     } else {
         result.temp = ((data[2] << 8) | data[3]) / 10.0f;
     }
     
     return result;
 }
 
 float get_resistance(uint16_t adc_value) {
     // Convert ADC to voltage
     float voltage = (adc_value * VOLTAGE_REF) / ADC_RESOLUTION;
     
     // Use voltage divider equation to calculate Rs
     if (voltage < 0.1) {
         return 999999.0; // Avoid division by very small numbers
     }
     
     return R_LOAD * (VOLTAGE_REF - voltage) / voltage;
 }
 
 float get_ppm(float ratio) {
     if (ratio <= 0.01) {
         return 9999.0; // Invalid ratio
     }
     
     // Formula for MQ135 CO2 calculation 
     return PARA * pow(ratio, -1.41);
 }
 
 int calculate_aqi(float ppm) {
    // Adjusted AQI calculation that's more sensitive to lower CO2 levels
    if (ppm < 0) return 0;
    
    // More sensitive scale for indoor environments
    if (ppm < 400) {
        // For very low CO2 levels (exceptional air quality)
        return (int)(ppm * 25 / 400);  // 0-25 range
    } 
    else if (ppm < 600) {
        // Good air quality
        return 25 + (int)((ppm - 400) * 25 / 200);  // 25-50 range
    }
    else if (ppm < 800) {
        // Moderate air quality
        return 50 + (int)((ppm - 600) * 25 / 200);  // 50-75 range
    }
    else if (ppm < 1000) {
        // Acceptable
        return 75 + (int)((ppm - 800) * 25 / 200);  // 75-100 range
    }
    else if (ppm < 1500) {
        // Poor air quality
        return 100 + (int)((ppm - 1000) * 50 / 500);  // 100-150 range
    }
    else if (ppm < 2000) {
        // Very poor air quality
        return 150 + (int)((ppm - 1500) * 50 / 500);  // 150-200 range
    }
    else if (ppm < 5000) {
        // Unhealthy
        return 200 + (int)((ppm - 2000) * 100 / 3000);  // 200-300 range
    }
    else {
        // Hazardous
        return 300 + (int)((ppm - 5000) * 200 / 5000);  // 300-500 range
    }
}
 
 void ssd1306_cmd(uint8_t cmd) {
     uint8_t buf[2] = {OLED_CONTROL_BYTE_CMD, cmd};
     i2c_write_blocking(i2c0, oled_address, buf, 2, false);
 }
 
 bool ssd1306_init() {
     // Check if OLED is responding
     uint8_t buf[2] = {OLED_CONTROL_BYTE_CMD, OLED_CMD_DISPLAY_OFF};
     int result = i2c_write_blocking(i2c0, oled_address, buf, 2, false);
     
     if (result < 0) {
         printf("OLED not responding at address 0x%02X\n", oled_address);
         return false;
     }
     
     printf("OLED responding at address 0x%02X\n", oled_address);
     
     ssd1306_cmd(OLED_CMD_DISPLAY_OFF);
     
     // Basic configuration for 128x32 display
     ssd1306_cmd(OLED_CMD_SET_DISPLAY_CLOCK_DIV);
     ssd1306_cmd(0x80);
     ssd1306_cmd(OLED_CMD_SET_MULTIPLEX);
     ssd1306_cmd(0x1F);  // 32 rows for 0.91" display
     ssd1306_cmd(OLED_CMD_SET_DISPLAY_OFFSET);
     ssd1306_cmd(0x0);
     ssd1306_cmd(OLED_CMD_SET_START_LINE | 0x0);
     
     // Power
     ssd1306_cmd(OLED_CMD_CHARGE_PUMP);
     ssd1306_cmd(0x14);  // Enable charge pump
     
     // Memory mode
     ssd1306_cmd(OLED_CMD_SET_MEMORY_MODE);
     ssd1306_cmd(0x00);  // Horizontal addressing
     
     // Orientation
     ssd1306_cmd(OLED_CMD_SEG_REMAP | 0x1);  // Flip horizontally
     ssd1306_cmd(OLED_CMD_COM_SCAN_DEC);     // Flip vertically
     
     // Hardware configuration
     ssd1306_cmd(OLED_CMD_SET_COM_PINS);
     ssd1306_cmd(0x02);  // For 128x32 display
     ssd1306_cmd(OLED_CMD_SET_CONTRAST);
     ssd1306_cmd(0x8F);  // Medium contrast
     ssd1306_cmd(OLED_CMD_SET_PRECHARGE);
     ssd1306_cmd(0xF1);
     ssd1306_cmd(OLED_CMD_SET_VCOM_DETECT);
     ssd1306_cmd(0x40);
     
     // Turn on
     ssd1306_cmd(OLED_CMD_DISPLAY_NORMAL);
     ssd1306_cmd(OLED_CMD_DISPLAY_ON);
     
     return true;
 }
 
 void ssd1306_clear() {
     memset(buffer, 0, sizeof(buffer));
 }
 
 void ssd1306_display() {
     ssd1306_cmd(OLED_CMD_COLUMN_ADDR);
     ssd1306_cmd(0);
     ssd1306_cmd(OLED_WIDTH - 1);
     
     ssd1306_cmd(OLED_CMD_PAGE_ADDR);
     ssd1306_cmd(0);
     ssd1306_cmd(OLED_PAGES - 1);
     
     // Send in chunks
     for (int i = 0; i < sizeof(buffer); i += 16) {
         uint8_t data[17];
         data[0] = OLED_CONTROL_BYTE_DATA;
         
         int chunk_size = 16;
         if (i + 16 > sizeof(buffer)) {
             chunk_size = sizeof(buffer) - i;
         }
         
         for (int j = 0; j < chunk_size; j++) {
             data[j + 1] = buffer[i + j];
         }
         
         i2c_write_blocking(i2c0, oled_address, data, chunk_size + 1, false);
     }
 }
 
 void draw_char(int x, int y, char c) {
     // Space is ASCII 32, first char in our font
     if (c < 32 || c > 90) {  // Only have space through Z in our font
         c = 32;  // Default to space
     }
     
     c -= 32;  // Adjust for font table
     
     // Each character is 5 pixels wide
     for (int i = 0; i < 5; i++) {
         uint8_t line = font[c * 5 + i];
         for (int j = 0; j < 8; j++) {
             if (line & (1 << j)) {
                 // Calculate which page and bit
                 int page = (y + j) / 8;
                 int bit = (y + j) % 8;
                 
                 // Set the pixel if it's within bounds
                 if (page >= 0 && page < OLED_PAGES && x+i >= 0 && x+i < OLED_WIDTH) {
                     buffer[page * OLED_WIDTH + x + i] |= (1 << bit);
                 }
             }
         }
     }
 }
 
 void draw_string(int x, int y, const char* str) {
     while (*str) {
         draw_char(x, y, *str++);
         x += 6;  // 5 pixels + 1 spacing
     }
 }
 
 void draw_char_2x(int x, int y, char c) {
     // Space is ASCII 32, first char in our font
     if (c < 32 || c > 90) {  // Only have space through Z in our font
         c = 32;  // Default to space
     }
     
     c -= 32;  // Adjust for font table
     
     // Each character is 5 pixels wide
     for (int i = 0; i < 5; i++) {
         uint8_t line = font[c * 5 + i];
         for (int j = 0; j < 8; j++) {
             if (line & (1 << j)) {
                 // Draw 2x2 pixels for each bit that's set
                 for (int dx = 0; dx <= 1; dx++) {
                     for (int dy = 0; dy <= 1; dy++) {
                         int page = (y + j*2 + dy) / 8;
                         int bit = (y + j*2 + dy) % 8;
                         
                         if (page >= 0 && page < OLED_PAGES && 
                             x + i*2 + dx >= 0 && x + i*2 + dx < OLED_WIDTH) {
                             buffer[page * OLED_WIDTH + x + i*2 + dx] |= (1 << bit);
                         }
                     }
                 }
             }
         }
     }
 }
 
 void draw_string_2x(int x, int y, const char* str) {
     while (*str) {
         draw_char_2x(x, y, *str++);
         x += 12;  // 10 pixels (5*2) + 2 spacing
     }
 }

 const char* get_air_quality_label(float ppm) {
    if (ppm < 700) {
        return "GOOD";  // Fresh/Good air
    } 
    else if (ppm < 1000) {
        return "OK";    // Acceptable
    }
    else if (ppm < 2000) {
        return "BAD";   // Poor air quality
    }
    else {
        return "UGLY";  // Very poor/hazardous
    }
}
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"

// Pin definitions
#define LED_PIN PICO_DEFAULT_LED_PIN
#define MQ135_PIN 26
#define I2C_SDA_PIN 4
#define I2C_SCL_PIN 5
#define OLED_ADDR 0x3C

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
#define OLED_CMD_SET_DISPLAY_CLOCK_DIV 0xD5
#define OLED_CMD_SET_MULTIPLEX 0xA8
#define OLED_CMD_SET_DISPLAY_OFFSET 0xD3
#define OLED_CMD_SET_START_LINE 0x40
#define OLED_CMD_CHARGE_PUMP 0x8D
#define OLED_CMD_MEMORY_MODE 0x20
#define OLED_CMD_SEG_REMAP 0xA0
#define OLED_CMD_COM_SCAN_INC 0xC0
#define OLED_CMD_COM_SCAN_DEC 0xC8
#define OLED_CMD_SET_COM_PINS 0xDA
#define OLED_CMD_SET_PRECHARGE 0xD9
#define OLED_CMD_SET_VCOM_DETECT 0xDB
#define OLED_CMD_DISPLAY_NORMAL 0xA6
#define OLED_CMD_COLUMN_ADDR 0x21
#define OLED_CMD_PAGE_ADDR 0x22

// Buffer for the display
uint8_t buffer[OLED_WIDTH * OLED_PAGES];

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
    0x00, 0x08, 0x14, 0x22, 0x41, // 
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

// Function to send command to OLED
void ssd1306_cmd(uint8_t cmd) {
    uint8_t buf[2] = {OLED_CONTROL_BYTE_CMD, cmd};
    i2c_write_blocking(i2c0, OLED_ADDR, buf, 2, false);
}

// Function to initialize OLED
void ssd1306_init() {
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
    ssd1306_cmd(OLED_CMD_MEMORY_MODE);
    ssd1306_cmd(0x00);  // Horizontal addressing
    
    // Orientation - try different combinations if text is mirrored
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
}

// Clear display buffer
void ssd1306_clear() {
    memset(buffer, 0, sizeof(buffer));
}

// Update display with buffer contents
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
        
        i2c_write_blocking(i2c0, OLED_ADDR, data, chunk_size + 1, false);
    }
}

// Draw a character
// Draw a character at 2x size
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

// Draw a string at 2x size
void draw_string_2x(int x, int y, const char* str) {
    while (*str) {
        draw_char_2x(x, y, *str++);
        x += 12;  // 10 pixels (5*2) + 2 spacing
    }
}

// Read MQ135 sensor
int read_mq135() {
    adc_select_input(0);  // ADC0 = GPIO 26
    int raw = adc_read();
    
    // Simple mapping to 0-100 scale
    int aqi = (raw * 100) / 4095;
    return aqi;
}

int main() {
    stdio_init_all();
    sleep_ms(3000);
    
    printf("\nSimplest OLED Test for 0.91 inch display\n");
    
    // Initialize LED
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    
    // Initialize ADC
    adc_init();
    adc_gpio_init(MQ135_PIN);
    
    // Initialize I2C
    i2c_init(i2c0, 100000);  // 100 kHz
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);
    
    // Initialize OLED
    ssd1306_init();
    
    // Simulated values
    int temp = 25;
    int humidity = 50;
    int display_mode = 0;  // 0=Temp, 1=Humidity, 2=AQI
    
    while (1) {
        // Blink LED
        gpio_put(LED_PIN, 1);
        sleep_ms(100);
        gpio_put(LED_PIN, 0);
        
        // Read AQI
        int aqi = read_mq135();
        
        // Clear buffer
        ssd1306_clear();
        
        char line_buffer[32];

// Display value based on current mode
    if (display_mode == 0) {
        // Temperature
        snprintf(line_buffer, sizeof(line_buffer), "TEMP:%d C", temp);
        
        // Calculate center position (each character is 12 pixels wide in 2x mode)
        int text_width = strlen(line_buffer) * 12;
        int x_pos = (OLED_WIDTH - text_width) / 2;
        if (x_pos < 0) x_pos = 0;
        
        draw_string_2x(x_pos, 8, line_buffer);
    } else if (display_mode == 1) {
        // Humidity
        snprintf(line_buffer, sizeof(line_buffer), "HUM:%d%%", humidity);
        
        int text_width = strlen(line_buffer) * 12;
        int x_pos = (OLED_WIDTH - text_width) / 2;
        if (x_pos < 0) x_pos = 0;
        
        draw_string_2x(x_pos, 8, line_buffer);
    } else {
        // AQI
        snprintf(line_buffer, sizeof(line_buffer), "AQI:%d", aqi);
        
        int text_width = strlen(line_buffer) * 12;
        int x_pos = (OLED_WIDTH - text_width) / 2;
        if (x_pos < 0) x_pos = 0;
        
        draw_string_2x(x_pos, 8, line_buffer);
    }
        
        // Update display
        ssd1306_display();
        
        // Wait before changing
        sleep_ms(5000);
        
        // Change mode
        display_mode = (display_mode + 1) % 3;
        
        // Update simulated values
        temp += (rand() % 3) - 1;
        if (temp < 15) temp = 15;
        if (temp > 35) temp = 35;
        
        humidity += (rand() % 5) - 2;
        if (humidity < 30) humidity = 30;
        if (humidity > 80) humidity = 80;
        
        printf("Mode: %d, Temp: %d°C, Humidity: %d%%, AQI: %d\n", 
               display_mode, temp, humidity, aqi);
    }
    
    return 0;
}
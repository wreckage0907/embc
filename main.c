#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"

// Pin definitions - CORRECTED based on actual connections
#define DHT_PIN 16         // Connected to physical pin 21
#define LED_PIN PICO_DEFAULT_LED_PIN
#define MQ135_PIN 26       // Connected to physical pin 31 (ADC0)

// I2C pins for OLED
#define I2C_SDA_PIN 4      // Connected to physical pin 6
#define I2C_SCL_PIN 5      // Connected to physical pin 7
#define OLED_ADDR 0x3C     // SSD1306 address (typical: 0x3C or 0x3D)

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

typedef struct {
    float temp;
    float humidity;
    bool error;
} DHT_reading;

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

// Draw a string
void draw_string(int x, int y, const char* str) {
    while (*str) {
        draw_char(x, y, *str++);
        x += 6;  // 5 pixels + 1 spacing
    }
}

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

// Modified DHT22 reading function with strong internal pull-up
void read_from_dht(DHT_reading *result) {
    uint8_t data[5] = {0};
    uint8_t bytes = 0;
    uint8_t bit = 7;
    
    result->error = true;
    
    // Enable strong internal pull-up
    gpio_set_dir(DHT_PIN, GPIO_OUT);
    gpio_put(DHT_PIN, 1);
    gpio_set_pulls(DHT_PIN, true, false);  // Enable internal pull-up
    sleep_ms(100);  // Give more time for the pull-up to stabilize
    
    // Start signal - more pronounced
    gpio_put(DHT_PIN, 0);
    sleep_ms(30);  // Extended low pulse (30ms)
    
    // Release the line
    gpio_put(DHT_PIN, 1);
    sleep_us(50);
    gpio_set_dir(DHT_PIN, GPIO_IN);
    gpio_set_pulls(DHT_PIN, true, false);
    
    // Wait for DHT response with more flexible timing
    uint32_t timeout = 200; // Increased timeout
    while(gpio_get(DHT_PIN) == 1) {
        if (--timeout == 0) {
            printf("Error: DHT not responding\n");
            return;
        }
        sleep_us(1);
    }
    
    // Measure the response pulses with more flexible timing
    timeout = 200;
    while(gpio_get(DHT_PIN) == 0) {
        if (--timeout == 0) {
            printf("Error: DHT low pulse timeout\n");
            return;
        }
        sleep_us(1);
    }
    
    timeout = 200;
    while(gpio_get(DHT_PIN) == 1) {
        if (--timeout == 0) {
            printf("Error: DHT high pulse timeout\n");
            return;
        }
        sleep_us(1);
    }
    
    // Read the 40 bits of data with more flexible timing
    for(int i = 0; i < 40; i++) {
        // Wait for start of bit (low pulse)
        timeout = 200;
        while(gpio_get(DHT_PIN) == 0) {
            if (--timeout == 0) {
                printf("Error: DHT bit start timeout\n");
                return;
            }
            sleep_us(1);
        }
        
        // Measure high pulse length to determine bit value
        uint32_t start = time_us_32();
        
        timeout = 200;
        while(gpio_get(DHT_PIN) == 1) {
            if (--timeout == 0) {
                printf("Error: DHT bit end timeout\n");
                return;
            }
            sleep_us(1);
        }
        
        uint32_t duration = time_us_32() - start;
        
        // A high pulse longer than ~40us is a '1' bit
        // More flexible threshold (30us instead of 40us)
        if(duration > 30) {
            data[bytes] |= (1 << bit);
        }
        
        bit--;
        if(bit == 255) {
            bit = 7;
            bytes++;
        }
    }
    
    // Verify checksum
    if(bytes != 5) {
        printf("Error: Data length incorrect\n");
        return;
    }
    
    uint8_t checksum = data[0] + data[1] + data[2] + data[3];
    if(checksum != data[4]) {
        printf("Error: Checksum failure (got %d, expected %d)\n", checksum, data[4]);
        return;
    }
    
    // Convert data to temperature and humidity values
    result->humidity = ((data[0] << 8) + data[1]) / 10.0;
    result->temp = ((data[2] << 8) + data[3]) / 10.0;
    
    // Sanity check on values with more flexible ranges
    if(result->humidity > 100 || result->humidity < 0 || 
       result->temp > 85 || result->temp < -45) {
        printf("Error: Values out of range (Temp: %.1f, Humidity: %.1f)\n", 
              result->temp, result->humidity);
        return;
    }
    
    result->error = false;
}

// Improved MQ135 reading function with better AQI calculation and power management
int read_mq135() {
    // Take multiple readings to stabilize
    const int NUM_SAMPLES = 10;
    int sum = 0;
    
    adc_select_input(0);  // ADC0 = GPIO 26
    
    for(int i = 0; i < NUM_SAMPLES; i++) {
        sum += adc_read();
        sleep_ms(10);
    }
    
    int raw = sum / NUM_SAMPLES;
    
    // Convert to voltage (0-3.3V)
    float voltage = (raw * 3.3) / 4095.0;
    
    // Better calibration for MQ135
    // These values need to be adjusted for your specific sensor
    float resistance = ((1023.0 / raw) - 1.0) * 10.0; // Calculate sensor resistance
    
    // Map resistance to ppm (approximate)
    // Adjust these values based on your sensor's datasheet or calibration
    float ppm;
    if (resistance > 20.0) {
        ppm = 400; // Approximately fresh air
    } else if (resistance > 10.0) {
        ppm = 500 + (20.0 - resistance) * 30;
    } else if (resistance > 5.0) {
        ppm = 800 + (10.0 - resistance) * 40;
    } else if (resistance > 3.0) {
        ppm = 1000 + (5.0 - resistance) * 50;
    } else {
        ppm = 1100 + (3.0 - resistance) * 100;
    }
    
    // Convert PPM to AQI (very approximate)
    int aqi;
    if (ppm < 450) {
        aqi = (int)((ppm - 400) / 50.0 * 50); // 0-50 (Good)
    } else if (ppm < 700) {
        aqi = (int)(50 + (ppm - 450) / 250.0 * 50); // 50-100 (Moderate)
    } else if (ppm < 1000) {
        aqi = (int)(100 + (ppm - 700) / 300.0 * 100); // 100-200 (Unhealthy)
    } else {
        aqi = (int)(200 + (ppm - 1000) / 1000.0 * 300); // 200-500 (Very Unhealthy to Hazardous)
    }
    
    // Ensure AQI is within expected range
    if (aqi < 0) aqi = 0;
    if (aqi > 500) aqi = 500;
    
    return aqi;
}

int main() {
    stdio_init_all();
    sleep_ms(3000);  // Wait for serial to initialize
    
    printf("\nEnvironmental Monitoring System Starting...\n");
    printf("Using GPIO %d for DHT22, GPIO %d for MQ135 (ADC0)\n", DHT_PIN, MQ135_PIN);
    printf("Using I2C SDA pin %d, SCL pin %d for OLED display\n", I2C_SDA_PIN, I2C_SCL_PIN);
    
    // Initialize GPIO
    gpio_init(DHT_PIN);
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_set_pulls(DHT_PIN, true, false);  // Pull up

    // Initialize ADC
    adc_init();
    adc_gpio_init(MQ135_PIN);  // GPIO 26 = ADC0
    
    // Initialize I2C for OLED display
    i2c_init(i2c0, 100000);  // 100 kHz for better reliability
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);
    
    // Initialize OLED display
    ssd1306_init();
    ssd1306_clear();
    
    // Show startup message
    draw_string(20, 12, "Initializing...");
    ssd1306_display();
    sleep_ms(2000);

    DHT_reading reading;
    int display_mode = 0;  // 0=Temp, 1=Humidity, 2=AQI
    char line_buffer[32];
    
    // Power management: Longer delay between readings for general loop
    const int MAIN_LOOP_DELAY = 5000;  // 5 seconds between readings
    
    while (1) {
        // Blink LED for activity indicator
        gpio_put(LED_PIN, 1);
        sleep_ms(100);
        gpio_put(LED_PIN, 0);
        
        // Read sensors
        read_from_dht(&reading);
        int aqi = read_mq135();
        
        // Clear display buffer
        ssd1306_clear();
        
        if (display_mode == 0) {
            // Temperature display
            if (!reading.error) {
                snprintf(line_buffer, sizeof(line_buffer), "T:%.1fC", reading.temp);
            } else {
                snprintf(line_buffer, sizeof(line_buffer), "T:ERROR");
            }
            
            // Center the text
            int text_width = strlen(line_buffer) * 12;
            int x_pos = (OLED_WIDTH - text_width) / 2;
            if (x_pos < 0) x_pos = 0;
            
            draw_string_2x(x_pos, 8, line_buffer);
            
            printf("Temperature: %.1f°C\n", reading.temp);
        } else if (display_mode == 1) {
            // Humidity display
            if (!reading.error) {
                snprintf(line_buffer, sizeof(line_buffer), "H:%.1f%%", reading.humidity);
            } else {
                snprintf(line_buffer, sizeof(line_buffer), "H:ERROR");
            }
            
            // Center the text
            int text_width = strlen(line_buffer) * 12;
            int x_pos = (OLED_WIDTH - text_width) / 2;
            if (x_pos < 0) x_pos = 0;
            
            draw_string_2x(x_pos, 8, line_buffer);
            
            printf("Humidity: %.1f%%\n", reading.humidity);
        } else {
            // AQI display
            snprintf(line_buffer, sizeof(line_buffer), "AQI:%d", aqi);
            
            // Center the text
            int text_width = strlen(line_buffer) * 12;
            int x_pos = (OLED_WIDTH - text_width) / 2;
            if (x_pos < 0) x_pos = 0;
            
            draw_string_2x(x_pos, 8, line_buffer);
            
            printf("AQI: %d\n", aqi);
        }
        
        // Update the display
        ssd1306_display();
        
        // Move to next display mode
        display_mode = (display_mode + 1) % 3;
        
        // Wait before next reading
        sleep_ms(MAIN_LOOP_DELAY);
    }
    
    return 0;
}
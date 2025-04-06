#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"

// Pin definitions
#define DHT_PIN 15
#define LED_PIN PICO_DEFAULT_LED_PIN
#define MQ135_PIN 26  // Connected to AOUT of MQ135

// I2C pins for OLED
#define I2C_SDA_PIN 4
#define I2C_SCL_PIN 5
#define OLED_ADDR 0x3C  // SSD1306 address (typical: 0x3C or 0x3D)

// SSD1306 Display Constants
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_PAGE_HEIGHT 8  // 8 pixels per page

// SSD1306 Commands
#define OLED_CONTROL_BYTE_CMD 0x00
#define OLED_CONTROL_BYTE_DATA 0x40
#define OLED_CMD_SET_CONTRAST 0x81
#define OLED_CMD_DISPLAY_RAM 0xA4
#define OLED_CMD_DISPLAY_NORMAL 0xA6
#define OLED_CMD_DISPLAY_OFF 0xAE
#define OLED_CMD_DISPLAY_ON 0xAF
#define OLED_CMD_SET_DISPLAY_OFFSET 0xD3
#define OLED_CMD_SET_COM_PINS 0xDA
#define OLED_CMD_SET_VCOM_DETECT 0xDB
#define OLED_CMD_SET_DISPLAY_CLOCK_DIV 0xD5
#define OLED_CMD_SET_PRECHARGE 0xD9
#define OLED_CMD_SET_MULTIPLEX 0xA8
#define OLED_CMD_SET_LOW_COLUMN 0x00
#define OLED_CMD_SET_HIGH_COLUMN 0x10
#define OLED_CMD_SET_START_LINE 0x40
#define OLED_CMD_MEMORY_MODE 0x20
#define OLED_CMD_COLUMN_ADDR 0x21
#define OLED_CMD_PAGE_ADDR 0x22
#define OLED_CMD_COM_SCAN_INC 0xC0
#define OLED_CMD_COM_SCAN_DEC 0xC8
#define OLED_CMD_SEG_REMAP 0xA0
#define OLED_CMD_CHARGE_PUMP 0x8D

// Buffer for the OLED display
uint8_t buffer[OLED_WIDTH * OLED_HEIGHT / 8];

typedef struct {
    float temp;
    float humidity;
    bool error;
} DHT_reading;

// Font data (5x7 font)
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
    0x00, 0x00, 0x7F, 0x41, 0x41, // [
    0x02, 0x04, 0x08, 0x10, 0x20, // "\"
    0x41, 0x41, 0x7F, 0x00, 0x00, // ]
    0x04, 0x02, 0x01, 0x02, 0x04, // ^
    0x40, 0x40, 0x40, 0x40, 0x40, // _
    0x00, 0x01, 0x02, 0x04, 0x00, // `
    0x20, 0x54, 0x54, 0x54, 0x78, // a
    0x7F, 0x48, 0x44, 0x44, 0x38, // b
    0x38, 0x44, 0x44, 0x44, 0x20, // c
    0x38, 0x44, 0x44, 0x48, 0x7F, // d
    0x38, 0x54, 0x54, 0x54, 0x18, // e
    0x08, 0x7E, 0x09, 0x01, 0x02, // f
    0x08, 0x14, 0x54, 0x54, 0x3C, // g
    0x7F, 0x08, 0x04, 0x04, 0x78, // h
    0x00, 0x44, 0x7D, 0x40, 0x00, // i
    0x20, 0x40, 0x44, 0x3D, 0x00, // j
    0x00, 0x7F, 0x10, 0x28, 0x44, // k
    0x00, 0x41, 0x7F, 0x40, 0x00, // l
    0x7C, 0x04, 0x18, 0x04, 0x78, // m
    0x7C, 0x08, 0x04, 0x04, 0x78, // n
    0x38, 0x44, 0x44, 0x44, 0x38, // o
    0x7C, 0x14, 0x14, 0x14, 0x08, // p
    0x08, 0x14, 0x14, 0x18, 0x7C, // q
    0x7C, 0x08, 0x04, 0x04, 0x08, // r
    0x48, 0x54, 0x54, 0x54, 0x20, // s
    0x04, 0x3F, 0x44, 0x40, 0x20, // t
    0x3C, 0x40, 0x40, 0x20, 0x7C, // u
    0x1C, 0x20, 0x40, 0x20, 0x1C, // v
    0x3C, 0x40, 0x30, 0x40, 0x3C, // w
    0x44, 0x28, 0x10, 0x28, 0x44, // x
    0x0C, 0x50, 0x50, 0x50, 0x3C, // y
    0x44, 0x64, 0x54, 0x4C, 0x44, // z
    0x00, 0x08, 0x36, 0x41, 0x00, // {
    0x00, 0x00, 0x7F, 0x00, 0x00, // |
    0x00, 0x41, 0x36, 0x08, 0x00, // }
    0x08, 0x08, 0x2A, 0x1C, 0x08, // ->
    0x08, 0x1C, 0x2A, 0x08, 0x08  // <-
};

void quick_blink() {
    for(int i = 0; i < 3; i++) {
        gpio_put(LED_PIN, 1);
        sleep_ms(100);
        gpio_put(LED_PIN, 0);
        sleep_ms(100);
    }
}

// Function to send command to OLED
void ssd1306_cmd(uint8_t cmd) {
    uint8_t buf[2] = {OLED_CONTROL_BYTE_CMD, cmd};
    i2c_write_blocking(i2c0, OLED_ADDR, buf, 2, false);
}

// Function to initialize OLED display
void ssd1306_init() {
    ssd1306_cmd(OLED_CMD_DISPLAY_OFF);
    ssd1306_cmd(OLED_CMD_SET_DISPLAY_CLOCK_DIV);
    ssd1306_cmd(0x80);
    ssd1306_cmd(OLED_CMD_SET_MULTIPLEX);
    ssd1306_cmd(OLED_HEIGHT - 1);
    ssd1306_cmd(OLED_CMD_SET_DISPLAY_OFFSET);
    ssd1306_cmd(0x00);
    ssd1306_cmd(OLED_CMD_SET_START_LINE | 0x00);
    ssd1306_cmd(OLED_CMD_CHARGE_PUMP);
    ssd1306_cmd(0x14);
    ssd1306_cmd(OLED_CMD_MEMORY_MODE);
    ssd1306_cmd(0x00);
    ssd1306_cmd(OLED_CMD_SEG_REMAP | 0x01);
    ssd1306_cmd(OLED_CMD_COM_SCAN_DEC);
    ssd1306_cmd(OLED_CMD_SET_COM_PINS);
    ssd1306_cmd(0x12);
    ssd1306_cmd(OLED_CMD_SET_CONTRAST);
    ssd1306_cmd(0xCF);
    ssd1306_cmd(OLED_CMD_SET_PRECHARGE);
    ssd1306_cmd(0xF1);
    ssd1306_cmd(OLED_CMD_SET_VCOM_DETECT);
    ssd1306_cmd(0x40);
    ssd1306_cmd(OLED_CMD_DISPLAY_RAM);
    ssd1306_cmd(OLED_CMD_DISPLAY_NORMAL);
    ssd1306_cmd(OLED_CMD_DISPLAY_ON);
}

// Clear display buffer
void ssd1306_clear() {
    memset(buffer, 0, sizeof(buffer));
}

// Update the display with buffer contents
void ssd1306_display() {
    ssd1306_cmd(OLED_CMD_COLUMN_ADDR);
    ssd1306_cmd(0);
    ssd1306_cmd(OLED_WIDTH - 1);
    ssd1306_cmd(OLED_CMD_PAGE_ADDR);
    ssd1306_cmd(0);
    ssd1306_cmd((OLED_HEIGHT / 8) - 1);

    // Write display buffer in chunks of 16 bytes to avoid I2C buffer limitations
    uint8_t data_array[17];
    data_array[0] = OLED_CONTROL_BYTE_DATA;
    
    for (int i = 0; i < sizeof(buffer); i += 16) {
        for (int j = 0; j < 16 && (i + j) < sizeof(buffer); j++) {
            data_array[j + 1] = buffer[i + j];
        }
        i2c_write_blocking(i2c0, OLED_ADDR, data_array, 17, false);
    }
}

// Function to draw a character on the display
void ssd1306_draw_char(int x, int y, char c) {
    if (c < 32 || c > 127) {
        c = '?';
    }
    
    // Adjust for character index in the font array
    c -= 32;
    
    // Each character is 5 pixels wide
    for (int i = 0; i < 5; i++) {
        uint8_t line = font[c * 5 + i];
        for (int j = 0; j < 8; j++) {
            if (line & (1 << j)) {
                int page = (y + j) / 8;
                int bit = (y + j) % 8;
                buffer[page * OLED_WIDTH + x + i] |= (1 << bit);
            }
        }
    }
}

// Function to display text string on the OLED
void ssd1306_draw_string(int x, int y, const char* str) {
    while (*str) {
        ssd1306_draw_char(x, y, *str++);
        x += 6; // Move cursor to next character position (5 pixels + 1 space)
    }
}

void read_from_dht(DHT_reading *result) {
    uint8_t data[5] = {0};
    uint8_t bytes = 0;
    uint8_t bit = 7;
    
    result->error = true; 
    quick_blink();
    gpio_set_pulls(DHT_PIN, true, false);
    sleep_ms(1);

    gpio_set_dir(DHT_PIN, GPIO_OUT);
    gpio_put(DHT_PIN, 0);
    sleep_ms(20);  

    gpio_set_dir(DHT_PIN, GPIO_IN);
    gpio_set_pulls(DHT_PIN, true, false);

    sleep_us(40);
    if(gpio_get(DHT_PIN)) {
        printf("Error: No response from DHT (1)\n");
        return;
    }
    sleep_us(80);

    if(!gpio_get(DHT_PIN)) {
        printf("Error: No response from DHT (2)\n");
        return;
    }
    sleep_us(80);

    for(int i = 0; i < 40; i++) {
        uint32_t cycles_low = 0;
        while(!gpio_get(DHT_PIN)) {
            cycles_low++;
            sleep_us(1);
            if(cycles_low > 100) {
                printf("Error: Timeout waiting for low\n");
                return;
            }
        }
        
        uint32_t cycles_high = 0;
        while(gpio_get(DHT_PIN)) {
            cycles_high++;
            sleep_us(1);
            if(cycles_high > 100) {
                printf("Error: Timeout waiting for high\n");
                return;
            }
        }
        
        if(cycles_high > cycles_low) {
            data[bytes] |= (1 << bit);
        }
        
        bit--;
        if(bit == 255) {
            bit = 7;
            bytes++;
        }
    }

    if(bytes != 5 || data[4] != ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) {
        printf("Error: Checksum failure\n");
        return;
    }
    
    result->humidity = ((data[0] << 8) + data[1]) / 10.0;
    result->temp = ((data[2] << 8) + data[3]) / 10.0;
    if(result->humidity > 100 || result->temp > 80) {
        printf("Error: Values out of range\n");
        return;
    }
    
    result->error = false;  
}

// Function to read air quality level from MQ135
int read_mq135() {
    adc_select_input(0);  // ADC0 = GPIO 26
    int raw = adc_read();  // 12-bit value (0 - 4095)
    int aqi = (raw * 500) / 4095;
    return aqi;
}

// Function to evaluate environmental conditions
void evaluate_conditions(float temp, float humidity, int aqi, char* temp_status, char* humidity_status, char* aqi_status) {
    // Temperature evaluation
    if (temp < 18) {
        strcpy(temp_status, "Cold");
    } else if (temp < 25) {
        strcpy(temp_status, "Comfortable");
    } else if (temp < 30) {
        strcpy(temp_status, "Warm");
    } else {
        strcpy(temp_status, "Hot");
    }
    
    // Humidity evaluation
    if (humidity < 30) {
        strcpy(humidity_status, "Dry");
    } else if (humidity < 60) {
        strcpy(humidity_status, "Comfortable");
    } else {
        strcpy(humidity_status, "Humid");
    }
    
    // AQI evaluation
    if (aqi < 50) {
        strcpy(aqi_status, "Good");
    } else if (aqi < 100) {
        strcpy(aqi_status, "Moderate");
    } else if (aqi < 200) {
        strcpy(aqi_status, "Poor");
    } else {
        strcpy(aqi_status, "Unhealthy");
    }
}

int main() {
    stdio_init_all();
    sleep_ms(3000);  
    
    printf("\nDHT22 + MQ135 + OLED Test Starting...\n");
    printf("Using GPIO %d for DHT22, GPIO %d for MQ135 (ADC0)\n", DHT_PIN, MQ135_PIN);
    printf("Using I2C SDA pin %d, SCL pin %d for OLED display\n", I2C_SDA_PIN, I2C_SCL_PIN);
    
    // Initialize GPIO
    gpio_init(DHT_PIN);
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_set_pulls(DHT_PIN, true, false);  

    // Initialize ADC
    adc_init();
    adc_gpio_init(MQ135_PIN);  // GPIO 26 = ADC0
    
    // Initialize I2C for OLED display
    i2c_init(i2c0, 400000);  // 400 kHz
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);
    
    // Initialize OLED display
    ssd1306_init();
    ssd1306_clear();
    ssd1306_draw_string(5, 5, "Initializing...");
    ssd1306_display();
    sleep_ms(2000);

    DHT_reading reading;
    bool led_state = false;
    char temp_status[15];
    char humidity_status[15];
    char aqi_status[15];
    char display_buffer[32];
    
    while (1) {
        led_state = !led_state;
        gpio_put(LED_PIN, led_state);
        
        read_from_dht(&reading);
        int aqi = read_mq135();

        // Clear display buffer
        ssd1306_clear();
        
        // Display title
        ssd1306_draw_string(10, 0, "ENVIRONMENT MONITOR");
        
        if (!reading.error) {
            // Evaluate environmental conditions
            evaluate_conditions(reading.temp, reading.humidity, aqi, temp_status, humidity_status, aqi_status);
            
            // Format and display temperature
            snprintf(display_buffer, sizeof(display_buffer), "Temp: %.1fC (%s)", reading.temp, temp_status);
            ssd1306_draw_string(0, 16, display_buffer);
            
            // Format and display humidity
            snprintf(display_buffer, sizeof(display_buffer), "Hum: %.1f%% (%s)", reading.humidity, humidity_status);
            ssd1306_draw_string(0, 26, display_buffer);
            
            // Format and display AQI
            snprintf(display_buffer, sizeof(display_buffer), "AQI: %d (%s)", aqi, aqi_status);
            ssd1306_draw_string(0, 36, display_buffer);
            
            // Display overall assessment
            if (strcmp(aqi_status, "Good") == 0 && 
                (strcmp(temp_status, "Comfortable") == 0 || strcmp(temp_status, "Warm") == 0) && 
                strcmp(humidity_status, "Comfortable") == 0) {
                ssd1306_draw_string(0, 50, "Status: IDEAL");
            } else if (strcmp(aqi_status, "Unhealthy") == 0 || 
                      strcmp(temp_status, "Hot") == 0 || 
                      strcmp(humidity_status, "Humid") == 0) {
                ssd1306_draw_string(0, 50, "Status: POOR");
            } else {
                ssd1306_draw_string(0, 50, "Status: ACCEPTABLE");
            }
            
            // Print to console as well
            printf("Temp: %.1f°C (%s), Humidity: %.1f%% (%s), AQI: %d (%s)\n", 
                   reading.temp, temp_status, reading.humidity, humidity_status, aqi, aqi_status);
        } else {
            // Display error message on OLED
            ssd1306_draw_string(0, 16, "DHT22 read failed!");
            ssd1306_draw_string(0, 36, "AQI: ");
            
            snprintf(display_buffer, sizeof(display_buffer), "%d (%s)", aqi, aqi_status);
            ssd1306_draw_string(30, 36, display_buffer);
            
            // Print to console as well
            printf("DHT22 read failed, AQI: %d (%s)\n", aqi, aqi_status);
        }
        
        // Update the display
        ssd1306_display();
        
        sleep_ms(2500);  
    }
}
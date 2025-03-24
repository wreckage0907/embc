#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

#define DHT_PIN 15
#define LED_PIN PICO_DEFAULT_LED_PIN  // Onboard LED

typedef struct {
    float temp;
    float humidity;
    bool error;
} DHT_reading;

void quick_blink() {
    for(int i = 0; i < 3; i++) {
        gpio_put(LED_PIN, 1);
        sleep_ms(100);
        gpio_put(LED_PIN, 0);
        sleep_ms(100);
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

int main() {
    stdio_init_all();
    sleep_ms(3000);  
    
    printf("\nDHT22 Test Starting...\n");
    printf("Using GPIO %d with internal pull-up\n", DHT_PIN);
    
    // Initialize GPIO
    gpio_init(DHT_PIN);
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_set_pulls(DHT_PIN, true, false);  
    
    DHT_reading reading;
    bool led_state = false;
    
    while (1) {
        led_state = !led_state;
        gpio_put(LED_PIN, led_state);
        
        read_from_dht(&reading);
        if (!reading.error) {
            printf("Temperature: %.1f°C, Humidity: %.1f%%\n", 
                   reading.temp, reading.humidity);
        }
        
        sleep_ms(2500);  
    }
}
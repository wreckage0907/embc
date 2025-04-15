/**
 * DHT22 Sensor Test for Raspberry Pi Pico
 * 
 * Connections:
 * - DHT22 Data pin to GPIO 16 (pin 21)
 * - DHT22 VCC to 3.3V (pin 36)
 * - DHT22 GND to GND (pin 38)
 * 
 * This code uses the internal pull-up resistor.
 */

 #include <stdio.h>
 #include "pico/stdlib.h"
 #include "hardware/gpio.h"
 #include "pico/time.h"
 
 // Define the GPIO pin connected to the DHT22 data line
 #define DHT_PIN 16
 
 // Define timeout values
 #define DHT_TIMEOUT_US 100000
 
 // DHT22 data structure
 typedef struct {
     float humidity;
     float temperature;
     bool error;
 } dht_reading;
 
 // Function to read data from DHT22 sensor
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
     // For DHT22: first two bytes are humidity (integer and decimal parts)
     // next two bytes are temperature (integer and decimal parts)
     result.humidity = ((data[0] << 8) | data[1]) / 10.0f;
     
     // Temperature might be negative
     if (data[2] & 0x80) {
         result.temperature = -((((data[2] & 0x7F) << 8) | data[3]) / 10.0f);
     } else {
         result.temperature = ((data[2] << 8) | data[3]) / 10.0f;
     }
     
     return result;
 }
 
 int main() {
     // Initialize the Pico
     stdio_init_all();
     
     // Wait for serial connection
     sleep_ms(2000);
     
     printf("DHT22 Sensor Test\n");
     
     // Initialize the DHT22 pin
     gpio_init(DHT_PIN);
     
     // Enable the internal pull-up resistor on the data pin
     gpio_set_pulls(DHT_PIN, true, false);
     
     while (1) {
         // Read data from DHT22
         dht_reading reading = read_dht22();
         
         // Print results
         if (reading.error) {
             printf("Error reading from DHT22 sensor\n");
         } else {
             printf("Temperature: %.1f°C, Humidity: %.1f%%\n", 
                    reading.temperature, reading.humidity);
         }
         
         // DHT22 has a minimum sampling period of 2 seconds
         sleep_ms(2500);
     }
     
     return 0;
 }
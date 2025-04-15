/**
 * MQ135 Air Quality Sensor with Raspberry Pi Pico
 * 
 * This program reads values from an MQ135 gas sensor using the Pico's ADC
 * and displays the raw readings and approximate PPM values via UART.
 * 
 * Connections:
 * - MQ135 AO (Analog Output) -> GPIO26 (ADC0) on Pico
 * - MQ135 VCC -> 5V
 * - MQ135 GND -> GND
 */

 #include <stdio.h>
 #include "pico/stdlib.h"
 #include "hardware/adc.h"
 #include "hardware/gpio.h"
 
 // Pin definitions
 #define MQ135_PIN 26       // Connected to ADC0
 #define LED_PIN 25         // Onboard LED
 
 // MQ135 parameters
 #define VOLTAGE_REF 3.3    // ADC reference voltage
 #define ADC_RESOLUTION 4095 // 12-bit ADC (2^12 - 1)
 #define R_LOAD 10.0        // Load resistor value (kOhms)
 #define RZERO 76.63        // Calibration resistance at atmospheric CO2 (kOhms)
 #define PARA 116.6020682   // Atmospheric CO2 level parameter
 
 // Function prototypes
 float get_resistance(uint16_t adc_value);
 float get_ppm(float ratio);
 
 int main() {
     // Initialize standard I/O
     stdio_init_all();
     
     // Initialize ADC
     adc_init();
     adc_gpio_init(MQ135_PIN);
     adc_select_input(0);  // ADC0 corresponds to GPIO26
     
     // Initialize LED
     gpio_init(LED_PIN);
     gpio_set_dir(LED_PIN, GPIO_OUT);
     
     printf("\nMQ135 Gas Sensor Reading Program\n");
     printf("--------------------------------\n");
     
     // Warm-up delay (sensor needs time to heat up)
     printf("Warming up MQ135 sensor (60 seconds)...\n");
     for (int i = 0; i < 60; i++) {
         gpio_put(LED_PIN, 1);
         sleep_ms(500);
         gpio_put(LED_PIN, 0);
         sleep_ms(500);
         printf(".");
         if (i % 10 == 9) printf("\n");
     }
     printf("\nSensor ready!\n\n");
     
     while (1) {
         // Read ADC value
         uint16_t adc_raw = adc_read();
         
         // Convert ADC reading to voltage
         float voltage = (adc_raw * VOLTAGE_REF) / ADC_RESOLUTION;
         
         // Calculate sensor resistance
         float rs = get_resistance(adc_raw);
         
         // Calculate Rs/R0 ratio
         float ratio = rs / RZERO;
         
         // Calculate approximate PPM
         float ppm = get_ppm(ratio);
         
         // Print data
         printf("ADC Raw: %d, Voltage: %.2f V, Rs: %.2f kOhm, Rs/R0: %.2f, CO2 PPM: %.2f\n", 
                adc_raw, voltage, rs, ratio, ppm);
         
         // Blink LED to indicate reading
         gpio_put(LED_PIN, 1);
         sleep_ms(100);
         gpio_put(LED_PIN, 0);
         
         // Wait before next reading
         sleep_ms(1900);  // Total 2 seconds between readings
     }
     
     return 0;
 }
 
 /**
  * Calculate the resistance of the sensor based on the ADC value
  * 
  * @param adc_value The raw ADC reading
  * @return The calculated resistance in kOhms
  */
 float get_resistance(uint16_t adc_value) {
     // Convert ADC to voltage
     float voltage = (adc_value * VOLTAGE_REF) / ADC_RESOLUTION;
     
     // Use voltage divider equation to calculate Rs
     // Rs = R_LOAD * (VOLTAGE_REF - voltage) / voltage
     if (voltage == 0) {
         return 999999.0; // Avoid division by zero
     }
     
     return R_LOAD * (VOLTAGE_REF - voltage) / voltage;
 }
 
 /**
  * Convert the Rs/R0 ratio to PPM
  * 
  * @param ratio The Rs/R0 ratio
  * @return The estimated CO2 PPM value
  */
 float get_ppm(float ratio) {
     if (ratio <= 0) {
         return 999999.0; // Invalid ratio
     }
     
     // Formula for MQ135 CO2 calculation 
     // This is an approximation and should be calibrated for your specific sensor
     return PARA * pow(ratio, -1.41);
 }
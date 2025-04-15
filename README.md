# ENV MONITORING SYSTEM 

## COMPONENTS 
1. [Raspberry Pi Pico](https://robu.in/product/raspberry-pi-pico/)
2. [DHT22 Sensor](https://robu.in/product/dht22-am2302-digital-temperature-humidity-sensor/)
3. [MQ135  Sensor](https://robu.in/product/waveshare-mq-135-gas-sensor/)
4. [SSD1306 Display (0.91"/0.96")](https://robu.in/product/oled-display-white-color-screen/)
5. Bread Board
6. Wires 

## TO RUN  

1. Clone the repository
```bash
git clone https://github.com/wreckage0907/embc.git
cd embc
```

2. Make BUILD Directory 
```bash
mkdir build
cd build
```
3. RUN the CMAKE FILE 
```bash
cmake ..
make
```

## HARDWARE CONNECTIONS
1. DHT22 Temperature Sensor
- Data - Pin 21 (GPIO 16)
- VCC - Pin 36
- GND - Pin 38

2. MQ135 Air Quality Sensor
- AOUT - Pin 31 (GPIO 26)
- VCC - Pin 39
- GND - Pin 38

3. OLED Display
- SCL - Pin 2 (GPIO 1)
- SDA - Pin 1 (GPIO 0)
- VCC - Pin 36
- GND - Pin 38

## PIN DIAGRAM 
![image](https://github.com/user-attachments/assets/143e8923-cbce-4066-80ba-4512d939ae48)

## ACKNOWLEDGEMENT 
> [!IMPORTANT]
> This was done for the course **BECE320E : Embedded C Programming**  


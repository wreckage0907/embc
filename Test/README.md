# Test Files

This folder contains various test programs for different sensors and peripherals:

## Test Programs

- **dht.c**: Tests the DHT22 temperature and humidity sensor.
- **gas.c**: Tests the MQ135 gas sensor.
- **i2c.c**: Verifies if the I2C peripheral is working properly on the configured pins.
- **oled.c**: Displays test content on an OLED screen to verify its functionality.

## How to Test

To properly test any of these components:

1. Navigate to the main project folder:
    ```
    cd ../
    ```

2. Modify the CMakeLists.txt file to change the executable target to the specific test file you want to run.

3. Build and run the project following the standard build process:
    ```
    mkdir build
    cd build
    cmake ..
    make
    ```

4. Run the compiled executable to test your selected component.

After testing, remember to revert the CMakeLists.txt file to its original configuration if needed.
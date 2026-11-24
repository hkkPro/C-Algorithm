\# ESP32 MPU6050 Reader



An educational ESP-IDF project that reads raw accelerometer

and gyroscope data from an MPU6050 over I2C.



The program also calculates approximate tilt angles from

accelerometer readings and estimates rotation around the Z axis

by integrating gyroscope measurements.





\# Hardware and Configuration



| Signal | ESP32 GPIO |

|--------|------------|

| SDA | GPIO8 |

| SCL | GPIO9 |



\- I2C address: `0x68`.

\- I2C clock: 400 kHz.

\- Connect the sensor and ESP32 to a common ground.

\- Use suitable I2C pull-up resistors, either on the module

&#x20; or externally, with signal levels compatible with ESP32.

\- Power the sensor module according to its specifications.



The GPIO assignments and device address can be changed

in `main/main.cpp` to match your wiring.



\# Running the Project



1\. Open this project directory in an ESP-IDF environment.

2\. Select the target matching your ESP32 board.

3\. Build and flash the firmware.

4\. Open the serial monitor.



The program prints raw acceleration, raw angular velocity,

and estimated angles, with a one-second delay between iterations.



\# Limitations



\- The X/Y angles are estimates based on acceleration and

&#x20; are affected by motion and vibration.

\- The Z angle is relative to the initial state, not an absolute

&#x20; heading, and accumulates drift.

\- No sensor calibration or sensor fusion is implemented.

\- The gyroscope conversion uses a sensitivity of 131 LSB/(degree/s),

&#x20; assuming the default ±250 degree/s range. The code does not

&#x20; explicitly configure this range.

\- The slow sampling loop is intended for learning and basic

&#x20; observation rather than accurate tracking of fast motion.



\## Learning Context



The project was developed to practice I2C communication,

register access, and sensor data processing.

AI assistance was used for the angle-calculation section.




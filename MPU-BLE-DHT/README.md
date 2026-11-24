# ESP32 DHT11 Reader with BLE

An educational ESP-IDF project that reads temperature and
humidity from a DHT11 sensor and makes the measurements
available over Bluetooth Low Energy using NimBLE.

The DHT11 communication routine is implemented directly
with GPIO operations and pulse-duration measurements,
without a dedicated DHT sensor library.

# Hardware and Configuration

- Target used: ESP32-S3.
- DHT11 data line: GPIO4.
- Connect the sensor and ESP32 to a common ground.
- Power the sensor or module according to its specifications.
- Use either a DHT11 module with a built-in pull-up resistor
  or a standalone sensor with an external data-line pull-up.
- Ensure the data-line voltage is compatible with ESP32's
  3.3 V GPIO.

The data pin can be changed through the `DHT_IO` definition
in `main/main.cpp`.

# BLE Interface

Device name: `ESP32`

| Item | UUID |
|------|------|
| Environmental Sensing service | `0x181A` |
| Temperature characteristic | `0x2A6E` |
| Humidity characteristic | `0x2A6F` |

Both characteristics support reading and notifications.
A client must subscribe to receive notifications.

Values are sent as two-byte little-endian integers:

- Temperature: signed integer, divided by 100 to obtain °C.
- Humidity: unsigned integer, divided by 100 to obtain %RH.

For example, a temperature value of `2350` represents `23.50°C`.
This encoding does not imply that the DHT11 has 0.01°C accuracy.

# Running the Project

1. Open this project directory in an ESP-IDF environment.
2. Select the target matching your board.
3. Enable Bluetooth with the NimBLE host in the project configuration.
4. Build and flash the firmware.
5. Open the serial monitor to inspect measurements and errors.
6. Connect using a BLE client that supports GATT reads
   and notification subscriptions.
7. Read or subscribe to the temperature and humidity characteristics.

# Limitations

- DHT11 timing is measured through software polling and
  may be affected by task scheduling and interrupts.
- Until the first valid measurement, BLE values are zero.
- If a measurement fails, BLE values retain the last valid
  readings; no separate data-validity characteristic is provided.
- Error handling is basic and requires further development.


# Learning Context

The DHT11 communication routine was written as a hands-on
learning exercise. The BLE section was developed with
tutorials and AI assistance while studying the code.


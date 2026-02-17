# ESP32 Room Climate Station

### Monitoring with Telegram Integration & Persistent Web Dashboard

An IoT project that monitors ambient temperature and humidity using an ESP32 and SHTC3 sensor, displays data on an I2C LCD, and sends periodic reports via a Telegram Bot.

## Features

- **Data Persistence**: Uses **SPIFFS** to store 24 hours of sensor history. If the device restarts, the data is reloaded from the internal flash memory rather than starting from zero.
- **Wireless Maintenance**: Integrated **ElegantOTA** allows for firmware updates via the browser, removing the need for USB cables once the device is deployed.
- **mDNS Support**: Accessible locally via `http://climate.local` instead of a shifting IP address.

## User Interfaces

- **Telegram Bot**: Sends a formatted report every 15 minutes. It accepts commands to toggle the LCD (/light_on, /light_off), check status (/status), or reboot the device (/restart).
- **Web Dashboard**: A Highcharts-powered interface that visualizes live trends and 24-hour historical data snapshots.
- **Hardware Toggle**: A physical button on GPIO 4 allows turning the display on and off. (In my setup a small analog joystick module was used).

## Technical Details

- **Hardware**: ESP32, SHTC3 Sensor, 16x2 I2C LCD.
- **Software**: Circular data buffers, non-blocking timers, and a protocol to clear the Telegram message queue on boot to prevent infinite restart loops.

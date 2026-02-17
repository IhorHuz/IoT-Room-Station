# ESP32 Climate Monitor & Telegram Notifier

An IoT project that monitors ambient temperature and humidity using an ESP32 and SHTC3 sensor, displays data on an I2C LCD, and sends periodic reports via a Telegram Bot.

## Features

- **Interactive Web Dashboard**: Live graphing of temperature and humidity using Highcharts.
- **Hands-Free Updates (OTA)**: Wireless firmware updates via the ElegantOTA web portal.
- **Telegram Bot Integration**: Remote commands (/status, /light_on, /light_off, /restart) and periodic automated reports.
- **mDNS Support**: Access the device locally via `http://climate.local`.
- **Hardware Privacy Toggle**: Physical button to instantly turn the LCD display and backlight on/off.
- **Data Persistence (Upcoming)**: Built-in recovery from power loss and reboot loops.

## Components

- ESP32 Development Board
- SHTC3 Temperature & Humidity Sensor
- 16x2 I2C LCD Display
- Push Button(In my setup a small analog joystick module was used)

## Setup

1. Create a file named `env.h`.
2. Define your WiFi and Telegram credentials (SSID, PASSWORD, BOT_TOKEN, CHAT_ID).
3. Install required libraries: `UniversalTelegramBot`, `Adafruit_SHTC3`, `LiquidCrystal_I2C`.

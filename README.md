# ESP32 Climate Monitor & Telegram Notifier

An IoT project that monitors ambient temperature and humidity using an ESP32 and SHTC3 sensor, displays data on an I2C LCD, and sends periodic reports via a Telegram Bot.

## Features

- **Real-time Monitoring**: Displays Temp/Hum on a 16x2 LCD.
- **Privacy Mode**: Physical button to toggle LCD backlight and display on/off.
- **Telegram Integration**: Sends room reports every 15 minutes to a private chat.
- **Non-blocking Code**: Uses `millis()` timers for responsive button handling.

## Components

- ESP32 Development Board
- SHTC3 Temperature & Humidity Sensor
- 16x2 I2C LCD Display
- Push Button(In my setup a small analog joystick module was used)

## Setup

1. Create a file named `env.h`.
2. Define your WiFi and Telegram credentials (SSID, PASSWORD, BOT_TOKEN, CHAT_ID).
3. Install required libraries: `UniversalTelegramBot`, `Adafruit_SHTC3`, `LiquidCrystal_I2C`.

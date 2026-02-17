#include <Wire.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <Adafruit_SHTC3.h>
#include <LiquidCrystal_I2C.h>
#include "env.h"

const char *ssid = SECRET_SSID;
const char *password = SECRET_PASS;
#define BOT_TOKEN SECRET_BOT_TOKEN
#define CHAT_ID SECRET_CHAT_ID

#define BUTTON_PIN 4
#define BUILTIN_LED 2

WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);
Adafruit_SHTC3 shtc3 = Adafruit_SHTC3();
LiquidCrystal_I2C lcd(0x27, 16, 2);

float temperature = 0;
float humidity = 0;
bool displayOn = true;
unsigned long lastTelegram = 0;
unsigned long telegramInterval = 15 * 60 * 1000;
unsigned long lastSensorRead = 0;

void setup()
{
  Serial.begin(115200);
  Wire.begin(21, 22);

  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, LOW);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  lcd.init();
  lcd.backlight();
  lcd.clear();

  if (!shtc3.begin())
  {
    lcd.setCursor(0, 0);
    lcd.print("Sensor error!");
    while (1)
      ;
  }

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
  }

  lcd.clear();
  client.setInsecure();
}

void loop()
{
  if (digitalRead(BUTTON_PIN) == LOW)
  {
    delay(50);
    displayOn = !displayOn;

    if (displayOn)
    {
      lcd.backlight();
      lcd.display();
    }
    else
    {
      lcd.noBacklight();
      lcd.noDisplay();
    }

    while (digitalRead(BUTTON_PIN) == LOW)
      ;
    delay(50);
  }

  if (millis() - lastSensorRead > 2000)
  {
    lastSensorRead = millis();
    sensors_event_t humidity_event, temp_event;
    shtc3.getEvent(&humidity_event, &temp_event);
    temperature = temp_event.temperature;
    humidity = humidity_event.relative_humidity;

    if (displayOn)
    {
      lcd.setCursor(0, 0);
      lcd.print("Temp: ");
      lcd.print(temperature);
      lcd.print(" C   ");
      lcd.setCursor(0, 1);
      lcd.print("Hum:  ");
      lcd.print(humidity);
      lcd.print(" %   ");
    }
  }

  if (millis() - lastTelegram > telegramInterval)
  {
    lastTelegram = millis();
    String message = "Room Report:\n";
    message += "🌡️Temperature: " + String(temperature) + " °C\n";
    message += "💦Humidity: " + String(humidity) + " %";
    bot.sendMessage(CHAT_ID, message, "");
  }
}
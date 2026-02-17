#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>
#include <Wire.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <Adafruit_SHTC3.h>
#include <LiquidCrystal_I2C.h>
#include <ESPmDNS.h>
#include "env.h"

// Hardware and Network Setup from env.h
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
AsyncWebServer server(80);

float temperature = 0, humidity = 0;
bool displayOn = true;
unsigned long lastTelegram = 0, lastSensorRead = 0, lastBotCheck = 0;
const unsigned long telegramInterval = 15 * 60 * 1000;
const unsigned long botCheckInterval = 1000;

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<script src="https://code.highcharts.com/highcharts.js"></script>
<title>Climate Dashboard</title>
<style>body{font-family:Arial;text-align:center;background:#f4f4f4;} .chart{height:300px;width:90%;margin:auto;}</style>
</head><body><h2>Live Climate Data</h2><div id="t" class="chart"></div><div id="h" class="chart"></div>
<script>
var chartT=new Highcharts.Chart({chart:{renderTo:'t'},title:{text:'Temp'},series:[{data:[]}],xAxis:{type:'datetime'},yAxis:{title:{text:'C'}},credits:{enabled:false}});
var chartH=new Highcharts.Chart({chart:{renderTo:'h'},title:{text:'Hum'},series:[{data:[]}],xAxis:{type:'datetime'},yAxis:{title:{text:'%'}},credits:{enabled:false}});
setInterval(function(){
  fetch('/temperature').then(r=>r.text()).then(v=>{chartT.series[0].addPoint([(new Date()).getTime(),parseFloat(v)],true,chartT.series[0].data.length>20);});
  fetch('/humidity').then(r=>r.text()).then(v=>{chartH.series[0].addPoint([(new Date()).getTime(),parseFloat(v)],true,chartH.series[0].data.length>20);});
},5000);
</script></body></html>)rawliteral";

void handleNewMessages(int numNewMessages)
{
  for (int i = 0; i < numNewMessages; i++)
  {
    String chat_id = String(bot.messages[i].chat_id);
    if (chat_id != CHAT_ID)
      continue;

    String text = bot.messages[i].text;
    if (text == "/status")
    {
      bot.sendMessage(CHAT_ID, "🌡️ Temp: " + String(temperature) + "°C\n💦 Hum: " + String(humidity) + "%", "");
    }
    else if (text == "/light_on")
    {
      displayOn = true;
      lcd.backlight();
      lcd.display();
      bot.sendMessage(CHAT_ID, "LCD ON", "");
    }
    else if (text == "/light_off")
    {
      displayOn = false;
      lcd.noBacklight();
      lcd.noDisplay();
      bot.sendMessage(CHAT_ID, "LCD OFF", "");
    }
    else if (text == "/restart")
    {
      bot.sendMessage(CHAT_ID, "Rebooting now...", "");

      bot.getUpdates(bot.last_message_received + 1);

      delay(500);
      ESP.restart();
    }
  }
}

void setup()
{
  Serial.begin(115200);
  Wire.begin(21, 22);
  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, LOW);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  lcd.init();
  lcd.backlight();
  if (!shtc3.begin())
    while (1)
      ;

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
    delay(500);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connected! IP:");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());
  delay(5000);
  lcd.clear();

  if (MDNS.begin("climate"))
    Serial.println("mDNS: http://climate.local");

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *f)
            { f->send_P(200, "text/html", index_html); });
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *f)
            { f->send(200, "text/plain", String(temperature)); });
  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *f)
            { f->send(200, "text/plain", String(humidity)); });

  ElegantOTA.begin(&server);
  server.begin();
  client.setInsecure();
}

void loop()
{
  if (digitalRead(BUTTON_PIN) == LOW)
  {
    delay(50); // Debounce
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
      ; // Wait for release
    delay(50);
  }

  ElegantOTA.loop();

  if (millis() - lastSensorRead > 2000)
  {
    lastSensorRead = millis();
    sensors_event_t h, t;
    shtc3.getEvent(&h, &t);
    temperature = t.temperature;
    humidity = h.relative_humidity;
    if (displayOn)
    {
      lcd.setCursor(0, 0);
      lcd.print("Temp: ");
      lcd.print(temperature);
      lcd.print(" C  ");
      lcd.setCursor(0, 1);
      lcd.print("Hum: ");
      lcd.print(humidity);
      lcd.print(" %  ");
    }
  }

  if (millis() - lastBotCheck > botCheckInterval)
  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages)
    {
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastBotCheck = millis();
  }

  if (millis() - lastTelegram > telegramInterval)
  {
    lastTelegram = millis();
    bot.sendMessage(CHAT_ID, "Room Report:\n"
                             "🌡️ Temp: " +
                                 String(temperature) + "°C\n💦 Hum: " + String(humidity) + "%",
                    "");
  }
}
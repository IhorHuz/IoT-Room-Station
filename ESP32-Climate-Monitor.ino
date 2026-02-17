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
#include "SPIFFS.h"
#include "env.h"

// Hardware and Network Setup from env.h
const char *ssid = SECRET_SSID;
const char *password = SECRET_PASS;
#define BOT_TOKEN SECRET_BOT_TOKEN
#define CHAT_ID SECRET_CHAT_ID

#define BUTTON_PIN 4
#define BUILTIN_LED 2

#define MAX_READINGS 288
float tempHistory[MAX_READINGS];
float humHistory[MAX_READINGS];
int historyIndex = 0;
unsigned long lastHistorySave = 0;

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

void saveHistory()
{
  File f = SPIFFS.open("/history.bin", FILE_WRITE);
  if (f)
  {
    f.write((const uint8_t *)tempHistory, sizeof(tempHistory));
    f.write((const uint8_t *)humHistory, sizeof(humHistory));
    f.close();
    Serial.println("Data saved to SPIFFS");
  }
}

void loadHistory()
{
  if (SPIFFS.exists("/history.bin"))
  {
    File f = SPIFFS.open("/history.bin", FILE_READ);
    if (f)
    {
      f.read((uint8_t *)tempHistory, sizeof(tempHistory));
      f.read((uint8_t *)humHistory, sizeof(humHistory));
      f.close();
      Serial.println("Data loaded from SPIFFS");
    }
  }
}

// Web Dashboard
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <script src="https://code.highcharts.com/highcharts.js"></script>
  <title>24h Climate Dashboard</title>
  <style>
    body{font-family:Arial;text-align:center;background:#f4f4f4;margin:0;padding:20px;}
    .chart{height:300px;width:95%;max-width:800px;margin:20px auto;background:white;border-radius:8px;padding:10px;box-shadow:0 4px 6px rgba(0,0,0,0.1);}
  </style>
</head><body>
  <h2>24-Hour Climate Trend</h2>
  <div id="t" class="chart"></div><div id="h" class="chart"></div>
<script>
var chartT = new Highcharts.Chart({chart:{renderTo:'t',type:'line'},title:{text:'Temperature (24h)'},xAxis:{type:'datetime'},yAxis:{title:{text:'Celsius'}},series:[{name:'Temp',data:[],color:'#ff4444'}],credits:{enabled:false}});
var chartH = new Highcharts.Chart({chart:{renderTo:'h',type:'line'},title:{text:'Humidity (24h)'},xAxis:{type:'datetime'},yAxis:{title:{text:'%'}},series:[{name:'Hum',data:[],color:'#4444ff'}],credits:{enabled:false}});

// Fix: Improved History Plotting logic
fetch('/history').then(r=>r.json()).then(data=>{
  const now = new Date().getTime();
  const interval = 5 * 60 * 1000; 
  data.temp.forEach((v, i) => { 
    if(v > 0) chartT.series[0].addPoint([now - (data.temp.length - i) * interval, v], false); 
  });
  data.hum.forEach((v, i) => { 
    if(v > 0) chartH.series[0].addPoint([now - (data.hum.length - i) * interval, v], false); 
  });
  chartT.redraw(); chartH.redraw();
});

setInterval(function(){
  fetch('/temperature').then(r=>r.text()).then(v=>{chartT.series[0].addPoint([(new Date()).getTime(),parseFloat(v)],true,chartT.series[0].data.length>300);});
  fetch('/humidity').then(r=>r.text()).then(v=>{chartH.series[0].addPoint([(new Date()).getTime(),parseFloat(v)],true,chartH.series[0].data.length>300);});
},30000);
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
      String msg = "Current Status:\n🌡️Temperature: " + String(temperature) + " °C\n💦Humidity: " + String(humidity) + " %";
      bot.sendMessage(CHAT_ID, msg, "");
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

  if (!SPIFFS.begin(true))
    Serial.println("SPIFFS Mount Failed");
  else
    loadHistory();

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

  server.on("/history", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    String json = "{\"temp\":[";
    for(int i=0; i<MAX_READINGS; i++){
      json += String(tempHistory[(historyIndex + i) % MAX_READINGS]);
      if(i < MAX_READINGS-1) json += ",";
    }
    json += "],\"hum\":[";
    for(int i=0; i<MAX_READINGS; i++){
      json += String(humHistory[(historyIndex + i) % MAX_READINGS]);
      if(i < MAX_READINGS-1) json += ",";
    }
    json += "]}";
    request->send(200, "application/json", json); });

  ElegantOTA.begin(&server);
  server.begin();
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

  ElegantOTA.loop();

  if (millis() - lastSensorRead > 2000)
  {
    lastSensorRead = millis();
    sensors_event_t h_ev, t_ev;
    shtc3.getEvent(&h_ev, &t_ev);
    temperature = t_ev.temperature;
    humidity = h_ev.relative_humidity;
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

  // Save every 5 minutes
  if (millis() - lastHistorySave > 300000)
  {
    lastHistorySave = millis();
    tempHistory[historyIndex] = temperature;
    humHistory[historyIndex] = humidity;
    historyIndex = (historyIndex + 1) % MAX_READINGS;
    saveHistory();
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
    String message = "Room Report:\n";
    message += "🌡️Temperature: " + String(temperature) + " °C\n";
    message += "💦Humidity: " + String(humidity) + " %";
    bot.sendMessage(CHAT_ID, message, "");
  }
}
#include <ArduinoJson.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

#define ENABLE_GxEPD2_GFX 0
#include <GxEPD2_3C.h>
#include <Fonts/FreeMonoBold24pt7b.h>


GxEPD2_3C<GxEPD2_750c, GxEPD2_750c::HEIGHT / 4> display(GxEPD2_750c(/*CS=D8*/ 15, /*DC=D3*/ 4, /*RST=D4*/ 2, /*BUSY=D2*/ 5));

ESP8266WiFiMulti WiFiMulti;
DynamicJsonDocument doc(521);

WiFiClient client;
HTTPClient http;
void setup() {
  Serial.begin(115200);
  // Serial.setDebugOutput(true);
  Serial.println();

  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP("4G-Gateway", "21078256");

  display.init(115200);
  display.setFont(&FreeMonoBold24pt7b);
  display.setTextColor(GxEPD_BLACK);
  int16_t tbx, tby; uint16_t tbw, tbh;
  display.getTextBounds("Hello World", 0, 0, &tbx, &tby, &tbw, &tbh);
  uint16_t x = ((display.width() - tbw) / 2) - tbx;
  uint16_t y = ((display.height() - tbh) / 2) - tby;
  display.setFullWindow();
  display.firstPage();

}

String getWeather() {
  if ((WiFiMulti.run() == WL_CONNECTED)) {
    if (http.begin(client, "http://api.openweathermap.org/data/2.5/weather?q=Oslo,Norway&appid=61616020863f09e0a462ea300d512a56")) {
      int httpCode = http.GET();
      if (httpCode > 0) {
        if (httpCode = HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          return http.getString();
        }
      } 
      http.end();
    }
  }
}

void draw(String weather, String desc, double temp, double windSpeed, double windDir) {
  do {
    display.fillScreen(GxEPD_WHITE);
    display.setCursor(0, 50);
    display.print(weather);
    display.setCursor(0, 100);
    display.print(desc);  
    display.setCursor(0, 150);
    display.print(temp);
    display.setCursor(0, 200);
    display.print(windSpeed);
    display.setCursor(0, 250);
    display.print(windDir);
  }

  while (display.nextPage());
}

void updateWeather() {
  String payload = getWeather();
  deserializeJson(doc, payload);
  JsonObject obj = doc.as<JsonObject>();

  String weather = obj["weather"][0]["main"]; // Weather one word
  String desc = obj["weather"][0]["description"]; // Description
  double temp = ((double) obj["main"]["temp"]) - 273.15; // Temperature
  double windSpeed = obj["wind"]["speed"];
  double windDir = obj["wind"]["deg"];

  draw(weather, desc, temp, windSpeed, windDir);
}

void loop() {
  updateWeather();

  delay(10000);
}



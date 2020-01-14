// WiFi and json parsing
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <Arduino.h>

// Display drivers
#define ENABLE_GxEPD2_GFX 0
#include <GxEPD2_3C.h>
#include <Fonts/FreeMonoBold24pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSans9pt7b.h>

GxEPD2_3C<GxEPD2_750c, GxEPD2_750c::HEIGHT / 4> display(GxEPD2_750c(/*CS=D8*/ 15, /*DC=D3*/ 4, /*RST=D4*/ 2, /*BUSY=D2*/ 5));

// Globals
//DynamicJsonDocument weatherDoc(521);
//DynamicJsonDocument forecastDoc(17375);

struct Event {
  const char* title;
  const char* description;
  const char* timeInterval;
};

struct Data {
  const char* currentWeather;
  const char* weatherDescription;
  const char* icon;
  const char* city;
  float temperature;
  int windVelocity;
  int windDirection;
  Event events[4];
};

struct WeatherInstance {
  const char* main;
  const char* description;
  const char* icon;
  float temperature;
  float rain;
  const char* date;
};

Data data;
WeatherInstance forecast[9];


void setup() {
  Serial.begin(115200);
  WiFi.begin("4G-Gateway", "21078256");

  // Wait for wifi connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting...");
  }
  
  display.init(115200);

  updateData(getWeather("weather"));
  updateForecast(getWeather("forecast"));
  draw();
}

void loop() {}


void draw() {
  // Init screen
  display.setRotation(0);
  display.setFullWindow();
  display.firstPage();

  // Do drawing
  do {
    // Clear screen
    display.fillScreen(GxEPD_WHITE);

    
    // General UI
    display.fillRect(384, 0, 256, 384, GxEPD_BLACK);
    display.setCursor(0, 0);
    display.setTextColor(GxEPD_BLACK);
    display.setFont(&FreeSans9pt7b);

    // Graph
    uint16_t verticalPadding = 72;
    uint16_t horizontalPadding = 44;
    uint16_t graphHeight = display.height() - 2 * verticalPadding;
    uint16_t graphWidth = display.width() - 256 - 2 * horizontalPadding;
    int hGuidelines = 4;
    int vGuidelines = 9;
    
    for (int i = 0; i < 4; i++) {
      uint16_t y = verticalPadding + i * graphHeight / (hGuidelines - 1);
      display.drawFastHLine(horizontalPadding, y, graphWidth, GxEPD_BLACK);
      display.setCursor(10, y + 5);
      display.println(minTemp() - 85 - i * 5);
    }
    
    for (int i = 0; i <= 8; i++) {
      uint16_t x = horizontalPadding + i * graphWidth / (vGuidelines - 1);
      display.drawFastVLine(x, graphHeight + verticalPadding, 10, GxEPD_BLACK);
      display.setCursor(x, graphHeight + verticalPadding);
      display.println(((String) forecast[i].date).substring(11, 15));
    }

    uint16_t oldX = horizontalPadding;
    uint16_t oldY = verticalPadding + map(forecast[0].temperature, -5, 10, graphHeight, 0);
    for (int i = 0; i <= 8; i++) {
      uint16_t x = horizontalPadding + i * graphWidth / (vGuidelines - 1);
      uint16_t y = verticalPadding + map(forecast[i].temperature, -5.0, 10.0, graphHeight, 0.0);
      display.drawLine(oldX, oldY, x, y, GxEPD_BLACK);
      display.fillRoundRect(x - 2, y - 2, 5, 5, 3, GxEPD_RED);
      oldX = x;
      oldY = y;
    }

    /*
    display.setCursor(10, 100);
    display.println(data.currentWeather);
    display.println(data.weatherDescription);
    display.println(data.city);
    display.println(data.temperature);
    display.println(data.windVelocity);
    display.println(data.windDirection);
    
    // Events
    int eventCount = 4;
    int eventPadding = 16;
    int eventHeight = (display.height() - (eventPadding * (eventCount + 1))) / eventCount;
    
    for (int i = 0; i < 4; i++) {
      if (i == 0) { display.drawRect(display.height() + eventPadding - 2, eventPadding + i * (eventHeight + eventPadding) - 2, display.width() - display.height() - 2 * eventPadding + 4, eventHeight + 4, GxEPD_RED); }
      display.fillRect(display.height() + eventPadding, eventPadding + i * (eventHeight + eventPadding), 256 - 2 * eventPadding, eventHeight, GxEPD_WHITE);
      display.setFont(&FreeSansBold18pt7b);
      display.setCursor(display.height() + eventPadding + 8, eventPadding + i * (eventHeight + eventPadding) + 30);
      display.println(data.events[i].title);
      display.setFont(&FreeSans9pt7b);
      display.setCursor(display.height() + eventPadding + 10, eventPadding + i * (eventHeight + eventPadding) + 50);
      display.println(data.events[i].description);
      display.setCursor(display.height() + eventPadding + 10, eventPadding + i * (eventHeight + eventPadding) + 70);
      display.println(data.events[i].timeInterval);
    }
    */
  }
  while (display.nextPage());
}


String getWeather(String type) {
  String key = "&appid=61616020863f09e0a462ea300d512a56";
  String host = "http://api.openweathermap.org/data/2.5/";
  String location = "?q=oslo,norway";
  String count = "&cnt=9";
  
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(host + type + location + key + count);
    int httpCode = http.GET();
    if (httpCode > 0) {
      return http.getString();
    }
    http.end();
  }
}


void updateForecast(String payload) {
  JsonArray list = parseJson(payload)["list"];
  
  for (int i = 0; i < 9; i++) {
    forecast[i].temperature = ((float) list[i]["main"]["temp"]) - 273.15;
    forecast[i].main = list[i]["weather"][0]["main"];
    forecast[i].description = list[i]["weather"][0]["description"];
    forecast[i].icon = list[i]["weather"][0]["icon"];
    forecast[i].rain = list[i]["rain"]["3h"];
    forecast[i].date = list[i]["dt_txt"];
  } 
}


void updateData(String payload) {
  JsonObject root = parseJson(payload);

  data.currentWeather = root["weather"][0]["main"];
  data.weatherDescription = root["weather"][0]["description"];
  data.icon = root["weather"][0]["icon"];
  data.city = root["name"];
  data.temperature = ((float) root["main"]["temp"]) - 273.15;
  data.windVelocity = root["wind"]["speed"];
  data.windDirection = root["wind"]["deg"];

  // Mock
  data.events[0] = {"DATA2500", "Undervisning", "08:30 - 10.15"};
  data.events[1] = {"DATA3705", "Undervisning", "10:30 - 12.15"};
  data.events[2] = {"DAPA2101", "Lab", "12:30 - 14.15"};
  data.events[3] = {"DATA2410", "Lab", "14:30 - 16.15"};
}

JsonObject parseJson(String payload) {
  DynamicJsonDocument jsonBuffer(6164);
  deserializeJson(jsonBuffer, payload);
  return jsonBuffer.as<JsonObject>();
}

int minTemp() {
  int minTemp = 100;
  for (int i = 0; 9 <= 8; i++) {
    float t = forecast[i].temperature;
    if (t < minTemp) {
      minTemp = t;
    }
  }
  return round5(minTemp);
}

int round5 (int a)
{
  return a >= 0 ? (a+2)/5*5 : (a-2)/5*5;
}



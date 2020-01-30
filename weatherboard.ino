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
#include <Fonts/Org_01.h>
#include <Fonts/FreeMono12pt7b.h>

GxEPD2_3C<GxEPD2_750c, GxEPD2_750c::HEIGHT / 4> display(GxEPD2_750c(/*CS=D8*/ 15, /*DC=D3*/ 4, /*RST=D4*/ 2, /*BUSY=D2*/ 5));

// Globals

enum alignment {LEFT, RIGHT, CENTER};

struct Segment {
  int lower;
  int upper;
};

struct Data {
  const char* currentWeather;
  const char* weatherDescription;
  const char* icon;
  const char* city;
  int updateTime;
  double temperature;
  double windVelocity;
  double windDirection;
};

struct WeatherInstance {
  const char* main;
  const char* description;
  const char* icon;
  double temperature;
  double rain;
  const char* date;
};

Data data;
WeatherInstance forecast[8];


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
    display.setTextColor(GxEPD_BLACK);

    display.drawLine(0, 44, 640, 44, GxEPD_BLACK);
    display.drawLine(430, 0, 430, 384, GxEPD_BLACK);
    display.drawLine(430, 44 + 200, 640, 44 + 200, GxEPD_BLACK);
    display.setFont(&FreeMono12pt7b);
    drawString(display.width() - 100, 15, convertUnixTime(data.updateTime), CENTER);
    drawString(25, 15, "Oslo", LEFT);

    display.setFont(&FreeSans9pt7b);
    

    drawGraph(0, 0, 352, 240);
    
    drawWind(430 + 200 / 2, 44 + 200 / 2, data.windDirection, data.windVelocity, 80);
    
  }
  while (display.nextPage());
}

String convertUnixTime(int unixTime) {
  time_t tm = (unixTime + 60 * 60);
  struct tm *now_tm = localtime(&tm);
  char output[40];
  strftime(output, sizeof(output), "%H:%M %d/%m/%y", now_tm);
  return output;
}

String getTime() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin("http://worldclockapi.com/api/json/cet/now");
    int httpCode = http.GET();
    if (httpCode > 0) {
      return http.getString();
    }
    http.end();
  }
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


void updateData(String payload) {
  JsonObject root = parseJson(payload);

  data.currentWeather = root["weather"][0]["main"];
  data.weatherDescription = root["weather"][0]["description"];
  data.icon = root["weather"][0]["icon"];
  data.city = root["name"];
  data.temperature = ((float) root["main"]["temp"]) - 273.15;
  data.windVelocity = root["wind"]["speed"];
  data.windDirection = root["wind"]["deg"];
  data.updateTime = root["dt"];
}

void updateForecast(String payload) {
  JsonArray list = parseJson(payload)["list"];
  
  for (int i = 0; i < 8; i++) {
    forecast[i].temperature = ((double) list[i]["main"]["temp"]) - 273.15;
    forecast[i].main = list[i]["weather"][0]["main"];
    forecast[i].description = list[i]["weather"][0]["description"];
    forecast[i].icon = list[i]["weather"][0]["icon"];
    forecast[i].rain = list[i]["rain"]["3h"];
    forecast[i].date = list[i]["dt_txt"];
  } 
}

JsonObject parseJson(String payload) {
  DynamicJsonDocument jsonBuffer(6164);
  deserializeJson(jsonBuffer, payload);
  return jsonBuffer.as<JsonObject>();
}

Segment getRange(double range[8]) {
  double mi = range[0];
  double ma = range[0];

  for (int i = 1; i < 8; i++) {
    double t = range[i];
    
    if (t < mi) {
      mi = t;
    } else if (t > ma) {
      ma = t;
    }
  }
  
  int a = floor(mi);
  int b = ceil(ma);

  // finds closest multiple of three outside the range
  int lower = (a >= 0) ? a - (a % 3) : a - 3 - (a % 3);
  int upper = (b >= 0) ? b + (3 - (b % 3)) : b - (b % 3);

  return {lower, upper};
}

String WindDegToDirection(float winddirection) {
  if (winddirection >= 348.75 || winddirection < 11.25)  return "N";
  if (winddirection >=  11.25 && winddirection < 33.75)  return "NNE";
  if (winddirection >=  33.75 && winddirection < 56.25)  return "NE";
  if (winddirection >=  56.25 && winddirection < 78.75)  return "ENE";
  if (winddirection >=  78.75 && winddirection < 101.25) return "E";
  if (winddirection >= 101.25 && winddirection < 123.75) return "ESE";
  if (winddirection >= 123.75 && winddirection < 146.25) return "SE";
  if (winddirection >= 146.25 && winddirection < 168.75) return "SSE";
  if (winddirection >= 168.75 && winddirection < 191.25) return "S";
  if (winddirection >= 191.25 && winddirection < 213.75) return "SSW";
  if (winddirection >= 213.75 && winddirection < 236.25) return "SW";
  if (winddirection >= 236.25 && winddirection < 258.75) return "WSW";
  if (winddirection >= 258.75 && winddirection < 281.25) return "W";
  if (winddirection >= 281.25 && winddirection < 303.75) return "WNW";
  if (winddirection >= 303.75 && winddirection < 326.25) return "NW";
  if (winddirection >= 326.25 && winddirection < 348.75) return "NNW";
  return "?";
}

int roundedMap(double x, double inMin, double inMax, double outMin, double outMax) {
  return round((x - inMin) * (outMax - outMin) / (inMax - inMin) + outMin);
}

void drawArrow(int x, int y, int asize, float aangle, int pwidth, int plength) {
  float pointX = asize * cos(radians(aangle - 90)) + x; // calculate X position
  float pointY = asize * sin(radians(aangle - 90)) + y; // calculate Y position

  float inner1X = asize * 0.7 * cos(radians(aangle - 80)) + x;
  float inner1Y = asize * 0.7 * sin(radians(aangle - 80)) + y;

  float inner2X = asize * 0.7 * cos(radians(aangle - 100)) + x;
  float inner2Y = asize * 0.7 * sin(radians(aangle - 100)) + y;

  display.fillTriangle(pointX, pointY, inner1X, inner1Y, inner2X, inner2Y, GxEPD_RED);
}

void drawString(int x, int y, String text, alignment align) {
  int16_t x1, y1;
  uint16_t w, h;
  display.setTextWrap(false);
  display.getTextBounds(text, x, y, &x1, &y1, &w, &h);
  if (align == RIGHT) x = x - w;
  if (align == CENTER) x = x - w / 2;
  display.setCursor(x, y + h);
  display.print(text);
}

void drawWind(int x, int y, float angle, float windspeed, int compassRadius) {
  drawArrow(x, y, compassRadius, angle, 15, 72);
  
  int dxo, dyo, dxi, dyi;
  display.drawCircle(x, y, compassRadius, GxEPD_BLACK);     // Draw compass circle
  display.drawCircle(x, y, compassRadius + 1, GxEPD_BLACK); // Draw compass circle
  display.drawCircle(x, y, compassRadius * 0.7, GxEPD_BLACK); // Draw compass inner circle

   for (float a = 0; a < 360; a = a + 22.5) {
    dxo = compassRadius * cos((a - 90) * PI / 180);
    dyo = compassRadius * sin((a - 90) * PI / 180);
    if (a == 45)  drawString(dxo + x + 10, dyo + y - 10, "NE", CENTER);
    if (a == 135) drawString(dxo + x + 7,  dyo + y + 5,  "SE", CENTER);
    if (a == 225) drawString(dxo + x - 15, dyo + y,      "SW", CENTER);
    if (a == 315) drawString(dxo + x - 15, dyo + y - 10, "NW", CENTER);
    dxi = dxo * 0.9;
    dyi = dyo * 0.9;
    display.drawLine(dxo + x, dyo + y, dxi + x, dyi + y, GxEPD_BLACK);
    dxo = dxo * 0.7;
    dyo = dyo * 0.7;
    dxi = dxo * 0.9;
    dyi = dyo * 0.9;
    display.drawLine(dxo + x, dyo + y, dxi + x, dyi + y, GxEPD_BLACK);
   }

  drawString(x, y - compassRadius - 10, "N", CENTER);
  drawString(x, y + compassRadius + 5, "S", CENTER);
  drawString(x - compassRadius - 10, y - 3, "W", CENTER);
  drawString(x + compassRadius + 8,  y - 3, "E", CENTER);
  drawString(x + 5, y + 24, String(angle, 0) + "°", CENTER);
  display.setFont(&FreeSansBold18pt7b);
  drawString(x - 10, y - 14, String(windspeed, 1), CENTER);
  display.setFont(&FreeSans9pt7b);
  drawString(x - 5, y - 35, WindDegToDirection(angle), CENTER);
  drawString(x, y + 10, "m/s", CENTER);
}

//#######################################################################

void drawGraph(int x, int y, int w, int h) {

  // Graph
  int hGuidelines = 4;
  int vGuidelines = 9;
  uint16_t verticalPadding = (display.height() - h) / 2;
  uint16_t horizontalPadding = (display.width() - w - 200) / 2;

  // Temperature and rain readable
  double temps[8];
  double rains[8];
  
  for (int i = 0; i < 8; i++) {
    temps[i] = forecast[i].temperature;
    rains[i] = forecast[i].rain;
  }

  Segment tempRange = getRange(temps);
  Segment rainRange = getRange(rains);
  rainRange.lower = 0;

  int tempStep = (tempRange.upper - tempRange.lower) / 3;
  int rainStep = (rainRange.upper - rainRange.lower) / 3;

  // Horizontal lines and y axis label
  for (int i = 0; i <= hGuidelines; i++) {
    uint16_t y = verticalPadding + i * h / (hGuidelines - 1);
    if (i == hGuidelines) {
      drawString(35, y + 12, "C", LEFT);
      drawString(w + horizontalPadding + 7, y + 12, "mm", RIGHT);
    }
    
    display.drawFastHLine(horizontalPadding, y, w, GxEPD_BLACK);
    drawString(35, y - 6, String(tempRange.upper - i * tempStep), RIGHT);
    drawString(w + horizontalPadding + 7, y - 6, String(rainRange.upper - i * rainStep), LEFT);
  }
  
  // Vertical lines
  display.setFont(&Org_01);
  for (int i = 0; i <= 8; i++) {
    uint16_t x = horizontalPadding + i * w / (vGuidelines - 1);
    display.drawFastVLine(x, h + verticalPadding, 14, GxEPD_BLACK);
  }

  // Graph temp/rain and lines and x axis label
  uint16_t oldX = horizontalPadding + 0.5 * w / (vGuidelines - 1);
  uint16_t oldTemp = (h + verticalPadding) - roundedMap(temps[0], tempRange.lower, tempRange.upper, 0.0, h);
  uint16_t oldRain = (h + verticalPadding) - roundedMap(rains[0], rainRange.lower, rainRange.upper, 0.0, h);
  
  for (int i = 0; i < 8; i++) {
    uint16_t x = horizontalPadding + (i + 0.5) * w / (vGuidelines - 1);
    uint16_t temp = (h + verticalPadding) - roundedMap(temps[i], tempRange.lower, tempRange.upper, 0.0, h);
    uint16_t rain = (h + verticalPadding) - roundedMap(rains[i], rainRange.lower, rainRange.upper, 0.0, h);
   
    // Hour text
    display.setCursor(x - 10, h + verticalPadding + 11);
    display.println(((String) forecast[i].date).substring(11, 16));
    displayWeatherIcon(x, h + verticalPadding + 40, forecast[i].icon, 5);

    // Points on graph
    for (int i = -1; i < 2; i++) {
      display.drawLine(oldX, oldTemp + i, x, temp + i, GxEPD_BLACK);
      display.drawLine(oldX, oldRain + i, x, rain + i, GxEPD_RED);
    }

    display.fillRoundRect(x - 4, temp - 4, 9, 9, 5, GxEPD_BLACK);
    display.fillRoundRect(x - 4, rain - 4, 9, 9, 5, GxEPD_RED);
    
    oldX = x;
    oldTemp = temp;
    oldRain = rain;
  }
}


void displayWeatherIcon(int x, int y, String iconName, int scale) {
  int linesize = scale / 5;
  if      (iconName == "01d" || iconName == "01n") sunny(x, y, iconName, scale, linesize);
  else if (iconName == "02d" || iconName == "02n") mostlySunny(x, y, iconName, scale, linesize);
  else if (iconName == "03d" || iconName == "03n") cloudy(x, y, iconName, scale, linesize);
  else if (iconName == "04d" || iconName == "04n") mostlySunny(x, y, iconName, scale, linesize);
  else if (iconName == "09d" || iconName == "09n") chanceRain(x, y, iconName, scale, linesize);
  else if (iconName == "10d" || iconName == "10n") rain(x, y, iconName, scale, linesize);
  else if (iconName == "11d" || iconName == "11n") tStorm(x, y, iconName, scale, linesize);
  else if (iconName == "13d" || iconName == "13n") snow(x, y, iconName, scale, linesize);
  else if (iconName == "50d")                      haze(x, y, iconName, scale, linesize);
  else if (iconName == "50n")                      fog(x, y, iconName, scale, linesize);
}

void addSun(int x, int y, int scale, int linesize) {
  display.fillRect(x - scale * 2, y, scale * 4, linesize, GxEPD_BLACK);
  display.fillRect(x, y - scale * 2, linesize, scale * 4, GxEPD_BLACK);
  display.drawLine(x - scale * 1.3, y - scale * 1.3, x + scale * 1.3, y + scale * 1.3, GxEPD_BLACK);
  display.drawLine(x - scale * 1.3, y + scale * 1.3, x + scale * 1.3, y - scale * 1.3, GxEPD_BLACK);
  display.fillCircle(x, y, scale * 1.3, GxEPD_WHITE);
  display.fillCircle(x, y, scale, GxEPD_BLACK);
  display.fillCircle(x, y, scale - linesize, GxEPD_WHITE);
}

void addFog(int x, int y, int scale, int linesize) {
  for (int i = 0; i < 6; i++) {
    display.fillRect(x - scale * 3, y + scale * 1.5, scale * 6, linesize, GxEPD_BLACK);
    display.fillRect(x - scale * 3, y + scale * 2.0, scale * 6, linesize, GxEPD_BLACK);
    display.fillRect(x - scale * 3, y + scale * 2.5, scale * 6, linesize, GxEPD_BLACK);
  }
}

void addTStorm(int x, int y, int scale) {
  y = y + scale / 2;
  for (int i = 0; i < 5; i++) {
    display.drawLine(x - scale * 4 + scale * i * 1.5 + 0, y + scale * 1.5, x - scale * 3.5 + scale * i * 1.5 + 0, y + scale, GxEPD_BLACK);
    display.drawLine(x - scale * 4 + scale * i * 1.5, y + scale * 1.5 + 0, x - scale * 3 + scale * i * 1.5 + 0, y + scale * 1.5 + 0, GxEPD_BLACK);
    display.drawLine(x - scale * 3.5 + scale * i * 1.4 + 0, y + scale * 2.5, x - scale * 3 + scale * i * 1.5 + 0, y + scale * 1.5, GxEPD_BLACK);
  }
}

void addSnow(int x, int y, int scale) {
  int dxo, dyo, dxi, dyi;
  for (int flakes = 0; flakes < 5; flakes++) {
    for (int i = 0; i < 360; i = i + 45) {
      dxo = 0.5 * scale * cos((i - 90) * 3.14 / 180); dxi = dxo * 0.1;
      dyo = 0.5 * scale * sin((i - 90) * 3.14 / 180); dyi = dyo * 0.1;
      display.drawLine(dxo + x + flakes * 1.5 * scale - scale * 3, dyo + y + scale * 2, dxi + x + 0 + flakes * 1.5 * scale - scale * 3, dyi + y + scale * 2, GxEPD_BLACK);
    }
  }
}

void addRain(int x, int y, int scale) {
  for (int d = 0; d < 4; d++) {
    addRaindrop(x + scale * (7.8 - d * 1.95) - scale * 5.2, y + scale * 2.1 - scale / 6, scale / 1.6);
  }
}

void addRaindrop(int x, int y, int scale) {
  display.fillCircle(x, y, scale / 2, GxEPD_BLACK);
  display.fillTriangle(x - scale / 2, y, x, y - scale * 1.2, x + scale / 2, y , GxEPD_BLACK);
  x = x + scale * 1.6; y = y + scale / 3;
  display.fillCircle(x, y, scale / 2, GxEPD_BLACK);
  display.fillTriangle(x - scale / 2, y, x, y - scale * 1.2, x + scale / 2, y , GxEPD_BLACK);
}

void addCloud(int x, int y, int scale, int linesize) {
  //Draw cloud outer
  display.fillCircle(x - scale * 3, y, scale, GxEPD_BLACK);                // Left most circle
  display.fillCircle(x + scale * 3, y, scale, GxEPD_BLACK);                // Right most circle
  display.fillCircle(x - scale, y - scale, scale * 1.4, GxEPD_BLACK);    // left middle upper circle
  display.fillCircle(x + scale * 1.5, y - scale * 1.3, scale * 1.75, GxEPD_BLACK); // Right middle upper circle
  display.fillRect(x - scale * 3 - 1, y - scale, scale * 6, scale * 2 + 1, GxEPD_BLACK); // Upper and lower lines
  //Clear cloud inner
  display.fillCircle(x - scale * 3, y, scale - linesize, GxEPD_WHITE);            // Clear left most circle
  display.fillCircle(x + scale * 3, y, scale - linesize, GxEPD_WHITE);            // Clear right most circle
  display.fillCircle(x - scale, y - scale, scale * 1.4 - linesize, GxEPD_WHITE);  // left middle upper circle
  display.fillCircle(x + scale * 1.5, y - scale * 1.3, scale * 1.75 - linesize, GxEPD_WHITE); // Right middle upper circle
  display.fillRect(x - scale * 3 + 2, y - scale + linesize - 1, scale * 5.9, scale * 2 - linesize * 2 + 2, GxEPD_WHITE); // Upper and lower lines
}

void addMoon(int x, int y, int scale) {
  display.fillCircle(x - 20, y - 12, scale, GxEPD_BLACK);
  display.fillCircle(x - 15, y - 12, scale * 1.6, GxEPD_WHITE);
}

void sunny(int x, int y, String iconName, int scale, int linesize) {
  if (iconName.endsWith("n")) addMoon(x, y + 3, scale);
  scale = scale * 1.6;
  addSun(x, y, scale, linesize);
}

void mostlySunny(int x, int y, String iconName, int scale, int linesize) {
  int offset = 5;
  if (iconName.endsWith("n")) addMoon(x, y + offset, scale);
  addCloud(x, y + offset, scale, linesize);
  addSun(x - scale * 1.8, y - scale * 1.8 + offset, scale, linesize);
}

void mostlyCloudy(int x, int y, String iconName, int scale, int linesize) {
  if (iconName.endsWith("n")) addMoon(x, y, scale);
  addCloud(x, y, scale, linesize);
  addSun(x - scale * 1.8, y - scale * 1.8, scale, linesize);
  addCloud(x, y, scale, linesize);
}

void cloudy(int x, int  y, String iconName, int scale, int linesize) {
  if (iconName.endsWith("n")) addMoon(x, y, scale);
  addCloud(x, y, scale, linesize);
}

void rain(int x, int y, String iconName, int scale, int linesize) {
  if (iconName.endsWith("n")) addMoon(x, y, scale);
  addCloud(x, y, scale, linesize);
  addRain(x, y, scale);
}

void expectRain(int x, int y, String iconName, int scale, int linesize) {
  if (iconName.endsWith("n")) addMoon(x, y, scale);
  addSun(x - scale * 1.8, y - scale * 1.8, scale, linesize);
  addCloud(x, y, scale, linesize);
  addRain(x, y, scale);
}

void chanceRain(int x, int y, String iconName, int scale, int linesize) {
  if (iconName.endsWith("n")) addMoon(x, y, scale);
  addSun(x - scale * 1.8, y - scale * 1.8, scale, linesize);
  addCloud(x, y, scale, linesize);
  addRain(x, y, scale);
}

void tStorm(int x, int y, String iconName, int scale, int linesize) {
  if (iconName.endsWith("n")) addMoon(x, y, scale);
  addCloud(x, y, scale, linesize);
  addTStorm(x, y, scale);
}

void snow(int x, int y, String iconName, int scale, int linesize) {
  if (iconName.endsWith("n")) addMoon(x, y, scale);
  addCloud(x, y, scale, linesize);
  addSnow(x, y, scale);
}

void fog(int x, int y, String iconName, int scale, int linesize) {
  if (iconName.endsWith("n")) addMoon(x, y, scale);
  addCloud(x, y - 5, scale, linesize);
  addFog(x, y - 5, scale, linesize);
}

void haze(int x, int y, String iconName, int scale, int linesize) {
  if (iconName.endsWith("n")) addMoon(x, y, scale);
  addSun(x, y - 5, scale * 1.4, linesize);
  addFog(x, y - 5, scale * 1.4, linesize);
}
















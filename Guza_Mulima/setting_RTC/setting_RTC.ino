#include <M5Stack.h>
#include <WiFi.h>
#include "time.h"
#include <RTClib.h>

RTC_DS3231 rtc;

// WiFi credentials
const char* ssid = "chiri";
const char* password = "dython_66";

// NTP server for Nairobi time (UTC+3)
const char* ntpServer = "ke.pool.ntp.org";
const long gmtOffset_sec = 3 * 3600; // UTC+3 for Nairobi
const int daylightOffset_sec = 0;     // No daylight saving in Kenya

void setup() {
  M5.begin();
  Serial.begin(115200);
  
  // Initialize RTC
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while(1);
  }

  // Connect to WiFi
  WiFi.begin(ssid, password);
  M5.Lcd.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    M5.Lcd.print(".");
  }
  M5.Lcd.println("\nConnected!");

  // Configure and get NTP time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    M5.Lcd.println("Failed to get NTP time");
    return;
  }

  // Set RTC to NTP time
  rtc.adjust(DateTime(
    timeinfo.tm_year + 1900,
    timeinfo.tm_mon + 1,
    timeinfo.tm_mday,
    timeinfo.tm_hour,
    timeinfo.tm_min,
    timeinfo.tm_sec
  ));

  // Display confirmation
  M5.Lcd.println("RTC Set to Nairobi Time:");
  printRTCTime();
  
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}

void loop() {
  // Display current RTC time every second
  printRTCTime();
  delay(1000);
}

void printRTCTime() {
  DateTime now = rtc.now();
  M5.Lcd.setCursor(0, 50);
  M5.Lcd.fillRect(0, 50, 320, 30, BLACK);
  M5.Lcd.printf("%04d-%02d-%02d %02d:%02d:%02d",
               now.year(), now.month(), now.day(),
               now.hour(), now.minute(), now.second());
}
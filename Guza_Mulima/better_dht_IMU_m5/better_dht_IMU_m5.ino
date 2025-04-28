#include <M5Stack.h>
#include <SD.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP085_U.h>
#include <RTClib.h>

#define DHTPIN 5
#define DHTTYPE DHT11

// Sensor objects
DHT dht(DHTPIN, DHTTYPE);
Adafruit_BMP085_Unified bmp(18001);
RTC_DS3231 rtc;

// SD Card
const int SD_CS = 4;
String basePath = "/hiking_data/";

// Display layout
const int headerHeight = 30;
const int sectionHeight = 70;
const int footerHeight = 30;

// Colors
const uint16_t bgColor = BLACK;
const uint16_t headerColor = NAVY;
const uint16_t dhtColor = CYAN;
const uint16_t bmpColor = YELLOW;
const uint16_t atmColor = GREEN;
const uint16_t timeColor = MAROON;
const uint16_t errorColor = RED;

void setup() {
  M5.begin();
  Serial.begin(115200);
  SPI.begin();

  // Initialize sensors
  dht.begin();
  if(!bmp.begin() || !rtc.begin()) {
    showFatalError("Sensor Init Failed!");
  }

  // Initialize SD card with retries
  if(!initSDCard()) {
    showFatalError("SD Card Failed!");
  }

  // Draw static UI elements
  drawUI();
}

bool initSDCard() {
  for(int i = 0; i < 3; i++) { // 3 retries
    if(SD.begin(SD_CS)) {
      return true;
    }
    delay(500);
  }
  return false;
}

void loop() {
  // Read all sensors
  float dht_temp = dht.readTemperature();
  float dht_humid = dht.readHumidity();
  
  sensors_event_t bmp_event;
  bmp.getEvent(&bmp_event);
  float altitude = bmp.pressureToAltitude(1013.25, bmp_event.pressure);
  float bmp_temp;
  bmp.getTemperature(&bmp_temp);

  // Get timestamp
  DateTime now = rtc.now();
  String timestamp = getTimestamp(now);

  // Update display with colors
  updateDisplay(dht_temp, dht_humid, bmp_temp, bmp_event.pressure, altitude, timestamp);

  // Save data with error feedback
  bool saveSuccess = logHikingData(
    isnan(dht_temp) ? NULL : dht_temp,
    isnan(dht_humid) ? NULL : dht_humid,
    bmp_temp,
    bmp_event.pressure,
    altitude,
    timestamp
  );

  // Show save status
  M5.Lcd.fillRect(0, 240 - footerHeight, 320, footerHeight, bgColor);
  M5.Lcd.setCursor(10, 240 - footerHeight + 10);
  if(saveSuccess) {
    M5.Lcd.setTextColor(GREEN);
    M5.Lcd.print("Data Saved!");
  } else {
    M5.Lcd.setTextColor(errorColor);
    M5.Lcd.print("Save Failed!");
  }

  delay(5000); // Log every 5 seconds
}

void drawUI() {
  // Header
  M5.Lcd.fillRect(0, 0, 320, headerHeight, headerColor);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(100, 10);
  M5.Lcd.print("HIKING LOGGER");

  // Section dividers
  M5.Lcd.drawFastHLine(0, headerHeight, 320, WHITE);
  M5.Lcd.drawFastHLine(0, headerHeight + sectionHeight, 320, WHITE);
  M5.Lcd.drawFastHLine(0, headerHeight + 2*sectionHeight, 320, WHITE);
  M5.Lcd.drawFastVLine(160, headerHeight, 2*sectionHeight, WHITE);
}

void updateDisplay(float dht_temp, float dht_humid, float bmp_temp, float pressure, float altitude, String timestamp) {
  // Clear content areas
  M5.Lcd.fillRect(0, headerHeight + 1, 320, sectionHeight - 1, bgColor);
  M5.Lcd.fillRect(0, headerHeight + sectionHeight + 1, 320, sectionHeight - 1, bgColor);

  // DHT11 Data (Top-left)
  M5.Lcd.setTextColor(dhtColor);
  M5.Lcd.setCursor(10, headerHeight + 15);
  M5.Lcd.printf("DHT11 Sensor:");
  M5.Lcd.setCursor(10, headerHeight + 35);
  M5.Lcd.printf("Temp: %.1f C", dht_temp);
  M5.Lcd.setCursor(10, headerHeight + 55);
  M5.Lcd.printf("Humid: %.1f %%", dht_humid);

  // BMP180 Data (Top-right)
  M5.Lcd.setTextColor(bmpColor);
  M5.Lcd.setCursor(170, headerHeight + 15);
  M5.Lcd.printf("BMP180 Sensor:");
  M5.Lcd.setCursor(170, headerHeight + 35);
  M5.Lcd.printf("Temp: %.1f C", bmp_temp);

  // Atmospheric Data (Bottom-left)
  M5.Lcd.setTextColor(atmColor);
  M5.Lcd.setCursor(10, headerHeight + sectionHeight + 15);
  M5.Lcd.printf("Atmospheric:");
  M5.Lcd.setCursor(10, headerHeight + sectionHeight + 35);
  M5.Lcd.printf("Pres: %.2f hPa", pressure);
  M5.Lcd.setCursor(10, headerHeight + sectionHeight + 55);
  M5.Lcd.printf("Alt: %.2f m", altitude);

  // Time (Bottom-right)
  M5.Lcd.setTextColor(timeColor);
  M5.Lcd.setCursor(170, headerHeight + sectionHeight + 15);
  M5.Lcd.printf("Time:");
  M5.Lcd.setCursor(170, headerHeight + sectionHeight + 35);
  M5.Lcd.printf("%s", timestamp.substring(11,19).c_str());
  M5.Lcd.setCursor(170, headerHeight + sectionHeight + 55);
  M5.Lcd.printf("%s", timestamp.substring(0,10).c_str());
}

bool logHikingData(float dht_temp, float dht_humid, float bmp_temp, float pressure, float altitude, String timestamp) {
  String datePath = basePath + timestamp.substring(0, 10) + "/";
  String filename = datePath + "hike_data.json";

  // Create directory if needed (with retries)
  if(!SD.exists(datePath)) {
    for(int i = 0; i < 3; i++) {
      if(SD.mkdir(datePath)) break;
      if(i == 2) return false;
      delay(100);
    }
  }

  File file = SD.open(filename, FILE_APPEND);
  if(!file) return false;

  StaticJsonDocument<256> doc;
  doc["timestamp"] = timestamp;
  
  JsonObject temps = doc.createNestedObject("temperature");
  temps["dht11"] = dht_temp;
  temps["bmp180"] = bmp_temp;
  
  doc["humidity"] = dht_humid;
  
  JsonObject atm = doc.createNestedObject("atmospheric");
  atm["pressure"] = pressure;
  atm["altitude"] = altitude;

  if(serializeJson(doc, file) == 0) {
    file.close();
    return false;
  }
  
  file.println();
  file.close();
  return true;
}

String getTimestamp(DateTime now) {
  char timestamp[20];
  snprintf(timestamp, sizeof(timestamp), "%04d-%02d-%02dT%02d:%02d:%02d",
          now.year(), now.month(), now.day(),
          now.hour(), now.minute(), now.second());
  return String(timestamp);
}

void showFatalError(const char* message) {
  M5.Lcd.fillScreen(RED);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(10, 10);
  M5.Lcd.println(message);
  while(1);
}

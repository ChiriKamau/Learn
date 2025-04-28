#include <M5Stack.h>
#include <SD.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP085_U.h>
#include <RTClib.h>

#define DHTPIN 35         // DHT11 data pin
#define DHTTYPE DHT11    // DHT 11

// Sensor objects
DHT dht(DHTPIN, DHTTYPE);
Adafruit_BMP085_Unified bmp(18001);
RTC_DS3231 rtc;

// SD Card
const int SD_CS = 4;
String basePath = "/hiking_data/";

void setup() {
  M5.begin();
  Serial.begin(115200);
  SPI.begin();

  // Initialize sensors
  dht.begin();
  if(!bmp.begin() || !rtc.begin()) {
    M5.Lcd.println("Sensor Init Failed!");
    while(1);
  }

  // Initialize SD card
  if(!SD.begin(SD_CS)) {
    M5.Lcd.println("SD Card Failed!");
    return;
  }

  M5.Lcd.setTextSize(2);
  M5.Lcd.println("Hiking Logger Ready");
}

void loop() {
  // Read all sensors
  float dht_temp = dht.readTemperature();     // DHT11 temperature
  float dht_humid = dht.readHumidity();       // DHT11 humidity
  
  sensors_event_t bmp_event;
  bmp.getEvent(&bmp_event);
  float altitude = bmp.pressureToAltitude(1013.25, bmp_event.pressure);
  float bmp_temp;
  bmp.getTemperature(&bmp_temp);              // BMP180 temperature

  // Get timestamp
  DateTime now = rtc.now();
  String timestamp = getTimestamp(now);

  // Display values
  updateDisplay(dht_temp, dht_humid, bmp_temp, bmp_event.pressure, altitude, timestamp);

  // Save to SD card (including null checks)
  logHikingData(
    isnan(dht_temp) ? NULL : dht_temp,
    isnan(dht_humid) ? NULL : dht_humid,
    bmp_temp,
    bmp_event.pressure,
    altitude,
    timestamp
  );

  delay(5000); // Log every 5 seconds
}

void logHikingData(float dht_temp, float dht_humid, float bmp_temp, float pressure, float altitude, String timestamp) {
  String datePath = basePath + timestamp.substring(0, 10) + "/";
  String filename = datePath + "hike_data.json";

  // Create directory if needed
  if(!SD.exists(datePath)) {
    SD.mkdir(datePath);
  }

  File file = SD.open(filename, FILE_APPEND);
  if(file) {
    StaticJsonDocument<256> doc;
    doc["timestamp"] = timestamp;
    
    JsonObject temps = doc.createNestedObject("temperature");
    temps["dht11"] = dht_temp;
    temps["bmp180"] = bmp_temp;
    
    doc["humidity"] = dht_humid;
    
    JsonObject atm = doc.createNestedObject("atmospheric");
    atm["pressure"] = pressure;
    atm["altitude"] = altitude;

    serializeJson(doc, file);
    file.println();
    file.close();
    M5.Lcd.println("Data Saved!");
  } else {
    M5.Lcd.println("Save Failed!");
  }
}

String getTimestamp(DateTime now) {
  char timestamp[20];
  snprintf(timestamp, sizeof(timestamp), "%04d-%02d-%02dT%02d:%02d:%02d",
          now.year(), now.month(), now.day(),
          now.hour(), now.minute(), now.second());
  return String(timestamp);
}

void updateDisplay(float dht_temp, float dht_humid, float bmp_temp, float pressure, float altitude, String timestamp) {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0);
  
  M5.Lcd.printf("DHT11: %.1fC %.1f%%\n", dht_temp, dht_humid);
  M5.Lcd.printf("BMP180: %.1fC\n", bmp_temp);
  M5.Lcd.printf("Alt: %.2fm\n", altitude);
  M5.Lcd.printf("Pres: %.2fhPa\n", pressure);
  M5.Lcd.printf("Time: %s\n", timestamp.substring(11,19).c_str());
}

#include <M5Stack.h>
#include <SD.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP085_U.h>
#include <RTClib.h>

#define DHTPIN 5
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);
Adafruit_BMP085_Unified bmp(18001);
RTC_DS3231 rtc;

const int SD_CS = 4;
String basePath = "/hiking_data/";

// Display layout
const int headerHeight = 30;
const int valueHeight = 80;
const int statusHeight = 20;

// Colors
const uint16_t bgColor = BLACK;
const uint16_t headerColor = NAVY;
const uint16_t dhtColor = CYAN;
const uint16_t bmpColor = YELLOW;
const uint16_t atmColor = GREEN;
const uint16_t timeColor = MAROON;
const uint16_t successColor = GREEN;
const uint16_t errorColor = RED;

void setup() {
  M5.begin();
  Serial.begin(115200);
  
  // Initialize sensors
  dht.begin();
  if(!bmp.begin() || !rtc.begin()) {
    showFatalError("SENSOR FAIL");
  }

  // Initialize SD card with more robust checking
  if(!initSDCard()) {
    showFatalError("SD FAIL");
  }

  drawUI();
}

bool initSDCard() {
  for(int i = 0; i < 5; i++) { // More retries
    if(SD.begin(SD_CS)) {
      // Verify we can actually write
      if(testWrite()) return true;
    }
    delay(300);
  }
  return false;
}

bool testWrite() {
  File testFile = SD.open("/test.txt", FILE_WRITE);
  if(!testFile) return false;
  
  if(testFile.println("TEST") <= 0) {
    testFile.close();
    return false;
  }
  testFile.close();
  
  SD.remove("/test.txt");
  return true;
}

void loop() {
  // Read sensors
  float dht_temp = dht.readTemperature();
  float dht_humid = dht.readHumidity();
  
  sensors_event_t bmp_event;
  bmp.getEvent(&bmp_event);
  float altitude = bmp.pressureToAltitude(1013.25, bmp_event.pressure);
  float bmp_temp;
  bmp.getTemperature(&bmp_temp);

  DateTime now = rtc.now();
  String timestamp = getTimestamp(now);

  // Update display
  updateDisplay(dht_temp, dht_humid, bmp_temp, bmp_event.pressure, altitude, now);

  // Save data with retry
  bool saveSuccess = false;
  for(int i = 0; i < 3; i++) { // Retry up to 3 times
    saveSuccess = logData(dht_temp, dht_humid, bmp_temp, bmp_event.pressure, altitude, timestamp);
    if(saveSuccess) break;
    delay(200);
  }
  showStatus(saveSuccess);

  delay(5000); // 5 second interval
}

void drawUI() {
  // Header
  M5.Lcd.fillRect(0, 0, 320, headerHeight, headerColor);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(80, 10);
  M5.Lcd.print("GUZA MULIMA");

  // Section dividers
  M5.Lcd.drawFastHLine(0, headerHeight, 320, WHITE);
  M5.Lcd.drawFastVLine(80, headerHeight, valueHeight, WHITE);
  M5.Lcd.drawFastVLine(160, headerHeight, valueHeight, WHITE);
  M5.Lcd.drawFastVLine(240, headerHeight, valueHeight, WHITE);
}

void updateDisplay(float dht_temp, float dht_humid, float bmp_temp, float pressure, float altitude, DateTime now) {
  M5.Lcd.setTextSize(3); // Larger text size
  
  // Column 1: DHT values
  M5.Lcd.setTextColor(dhtColor);
  M5.Lcd.setCursor(10, headerHeight + 20);
  M5.Lcd.printf("dT:%.1f", dht_temp);
  M5.Lcd.setCursor(10, headerHeight + 50);
  M5.Lcd.printf("H:%.0f", dht_humid);

  // Column 2: BMP values
  M5.Lcd.setTextColor(bmpColor);
  M5.Lcd.setCursor(90, headerHeight + 20);
  M5.Lcd.printf("bT:%.1f", bmp_temp);

  // Column 3: Atmospheric
  M5.Lcd.setTextColor(atmColor);
  M5.Lcd.setCursor(170, headerHeight + 20);
  M5.Lcd.printf("P:%.0f", pressure);
  M5.Lcd.setCursor(170, headerHeight + 50);
  M5.Lcd.printf("AT:%.0f", altitude);

  // Column 4: Time (Hr:Min:Sec)
  M5.Lcd.setTextColor(timeColor);
  M5.Lcd.setCursor(250, headerHeight + 20);
  M5.Lcd.printf("%02d:%02d", now.hour(), now.minute());
  M5.Lcd.setCursor(250, headerHeight + 50);
  M5.Lcd.printf(":%02d", now.second());
}

bool logData(float dht_temp, float dht_humid, float bmp_temp, float pressure, float altitude, String timestamp) {
  String datePath = basePath + timestamp.substring(0, 10);
  String filename = datePath + "/data.json";

  // Create directory if needed
  if(!SD.exists(datePath)) {
    if(!SD.mkdir(datePath)) {
      Serial.println("Failed to create directory");
      return false;
    }
  }

  File file = SD.open(filename, FILE_WRITE);
  if(!file) {
    Serial.println("Failed to open file");
    return false;
  }

  StaticJsonDocument<256> doc;
  doc["time"] = timestamp;
  doc["dht_temp"] = isnan(dht_temp) ? NULL : dht_temp;
  doc["dht_humid"] = isnan(dht_humid) ? NULL : dht_humid;
  doc["bmp_temp"] = bmp_temp;
  doc["pressure"] = pressure;
  doc["altitude"] = altitude;

  size_t bytesWritten = serializeJson(doc, file);
  file.println(); // Add newline
  file.close();

  if(bytesWritten == 0) {
    Serial.println("Failed to write data");
    return false;
  }

  return true;
}

void showStatus(bool success) {
  M5.Lcd.fillRect(0, headerHeight + valueHeight, 320, statusHeight, 
                 success ? successColor : errorColor);
}

void showFatalError(const char* message) {
  M5.Lcd.fillScreen(RED);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(3);
  M5.Lcd.setCursor(80, 100);
  M5.Lcd.println(message);
  while(1);
}

String getTimestamp(DateTime now) {
  char timestamp[20];
  snprintf(timestamp, sizeof(timestamp), "%04d-%02d-%02dT%02d:%02d:%02d",
          now.year(), now.month(), now.day(),
          now.hour(), now.minute(), now.second());
  return String(timestamp);
}
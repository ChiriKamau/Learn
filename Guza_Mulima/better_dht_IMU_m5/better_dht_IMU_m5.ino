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
const int dataHeight = 180;  // Top area for data
const int statusHeight = 60; // Bottom status area

// Colors
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

  // Initialize SD card with verification
  if(!initSDCard()) {
    showFatalError("SD CARD FAIL");
  }

  drawUI();
}

bool initSDCard() {
  for(int i = 0; i < 5; i++) {
    if(SD.begin(SD_CS)) {
      // Verify write capability
      File testFile = SD.open("/test.txt", FILE_WRITE);
      if(testFile) {
        testFile.println("TEST");
        testFile.close();
        SD.remove("/test.txt");
        return true;
      }
    }
    delay(300);
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

  DateTime now = rtc.now();

  // Update display (all data at top)
  updateDisplay(dht_temp, dht_humid, bmp_temp, bmp_event.pressure, altitude, now);

  // Save data with retry
  bool saveSuccess = saveToSD(dht_temp, dht_humid, bmp_temp, bmp_event.pressure, altitude, now);
  showStatus(saveSuccess);

  delay(5000); // 5 second interval
}

void drawUI() {
  // Clear screen
  M5.Lcd.fillScreen(BLACK);
  
  // Header
  M5.Lcd.fillRect(0, 0, 320, 30, headerColor);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(80, 10);
  M5.Lcd.print("GUZA MULIMA");
}

void updateDisplay(float dht_temp, float dht_humid, float bmp_temp, float pressure, float altitude, DateTime now) {
  M5.Lcd.setTextSize(3);
  
  // Clear only the data area
  M5.Lcd.fillRect(0, 30, 320, dataHeight-30, BLACK);

  // Row 1: Time and DHT Temp
  M5.Lcd.setTextColor(timeColor);
  M5.Lcd.setCursor(10, 40);
  M5.Lcd.printf("%02d:%02d:%02d", now.hour(), now.minute(), now.second());
  
  M5.Lcd.setTextColor(dhtColor);
  M5.Lcd.setCursor(200, 40);
  M5.Lcd.printf("dT:%.1fC", dht_temp);

  // Row 2: DHT Humidity and BMP Temp
  M5.Lcd.setTextColor(dhtColor);
  M5.Lcd.setCursor(10, 80);
  M5.Lcd.printf("H:%.0f%%", dht_humid);
  
  M5.Lcd.setTextColor(bmpColor);
  M5.Lcd.setCursor(200, 80);
  M5.Lcd.printf("bT:%.1fC", bmp_temp);

  // Row 3: Pressure and Altitude
  M5.Lcd.setTextColor(atmColor);
  M5.Lcd.setCursor(10, 120);
  M5.Lcd.printf("P:%.0fhPa", pressure);
  
  M5.Lcd.setCursor(200, 120);
  M5.Lcd.printf("AT:%.0fm", altitude);

  // Row 4: Date
  M5.Lcd.setTextColor(timeColor);
  M5.Lcd.setCursor(10, 160);
  M5.Lcd.printf("%04d-%02d-%02d", now.year(), now.month(), now.day());
}

bool saveToSD(float dht_temp, float dht_humid, float bmp_temp, float pressure, float altitude, DateTime now) {
  String datePath = basePath + String(now.year()) + "/" + String(now.month()) + "/" + String(now.day()) + "/";
  String filename = datePath + "data.json";

  // Create directory structure
  if(!createPath(datePath)) {
    Serial.println("Path creation failed");
    return false;
  }

  // Open file with retry
  File file;
  for(int i = 0; i < 3; i++) {
    file = SD.open(filename, FILE_WRITE);
    if(file) break;
    delay(100);
  }
  
  if(!file) {
    Serial.println("File open failed");
    return false;
  }

  // Create JSON data
  StaticJsonDocument<256> doc;
  doc["timestamp"] = getTimestamp(now);
  doc["dht_temp"] = isnan(dht_temp) ? NULL : dht_temp;
  doc["dht_humid"] = isnan(dht_humid) ? NULL : dht_humid;
  doc["bmp_temp"] = bmp_temp;
  doc["pressure"] = pressure;
  doc["altitude"] = altitude;

  // Write data
  size_t bytesWritten = serializeJson(doc, file);
  file.println(); // Add newline
  file.close();

  return bytesWritten > 0;
}

bool createPath(String path) {
  String currentPath = "";
  int startIdx = 0;
  int endIdx = path.indexOf('/', startIdx);
  
  while(endIdx > 0) {
    currentPath = path.substring(0, endIdx);
    if(!SD.exists(currentPath)) {
      if(!SD.mkdir(currentPath)) {
        return false;
      }
    }
    startIdx = endIdx + 1;
    endIdx = path.indexOf('/', startIdx);
  }
  return true;
}

void showStatus(bool success) {
  M5.Lcd.fillRect(0, dataHeight, 320, statusHeight, success ? successColor : errorColor);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(120, dataHeight + 20);
  M5.Lcd.print(success ? "SAVED" : "ERROR");
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
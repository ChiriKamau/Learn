#include <M5Stack.h>
#include <SD.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP085_U.h>
#include <RTClib.h>
#include <Wire.h>

#define DHTPIN 5
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);
Adafruit_BMP085_Unified bmp(18001);
RTC_DS3231 rtc;

const int SD_CS = 4;
String basePath = "/hiking_data/";

// Display layout
const int headerHeight = 40;
const int dataHeight = 200;
const int statusHeight = 20;

// Colors
const uint16_t titleColor = WHITE;
const uint16_t dhtColor = GREEN;
const uint16_t bmpColor = YELLOW;
const uint16_t successColor = GREEN;
const uint16_t errorColor = RED;

void setup() {
  M5.begin(true, true, false); // Disable IMU
  Serial.begin(115200);
  Wire.begin();
  
  // Initialize sensors
  dht.begin();
  if (!bmp.begin()) {
    showFatalError("BMP FAIL");
  }
  if (!rtc.begin()) {
    showFatalError("RTC FAIL");
  }
  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // Set to compile time
    Serial.println("RTC lost power, time reset to compile time");
  }

  // Initialize SD card
  if (!initSDCard()) {
    showFatalError("SD FAIL");
  }
}

bool initSDCard() {
  for (int i = 0; i < 5; i++) {
    if (SD.begin(SD_CS)) {
      File testFile = SD.open("/test.txt", FILE_WRITE);
      if (testFile) {
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
  // Read sensors
  float dht_temp = dht.readTemperature();
  float dht_humid = dht.readHumidity();
  
  sensors_event_t bmp_event;
  bmp.getEvent(&bmp_event);
  float altitude = bmp.pressureToAltitude(1013.25, bmp_event.pressure);
  float bmp_temp;
  bmp.getTemperature(&bmp_temp);

  DateTime now = rtc.now();
  if (now.year() < 2020 || now.year() > 2030) {
    Serial.println("Invalid RTC time!");
    showFatalError("RTC TIME ERR");
  }
  Serial.printf("RTC Time: %04d-%02d-%02d %02d:%02d:%02d\n", 
                now.year(), now.month(), now.day(), 
                now.hour(), now.minute(), now.second());

  // Update display
  updateDisplay(dht_temp, dht_humid, bmp_temp, bmp_event.pressure, altitude, now);

  // Save data
  bool saveSuccess = saveToSD(dht_temp, dht_humid, bmp_temp, bmp_event.pressure, altitude, now);
  showStatus(saveSuccess);

  delay(5000);
}

void updateDisplay(float dht_temp, float dht_humid, float bmp_temp, float pressure, float altitude, DateTime now) {
  M5.Lcd.fillScreen(BLACK);

  // Header bar with blue background
  M5.Lcd.fillRect(0, 0, 320, headerHeight, BLUE);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(20, 10);
  M5.Lcd.print("    Guza Mulima");

  // Altitude on top-left
  M5.Lcd.setTextColor(RED);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(10, 55);
  M5.Lcd.print("A:");
  M5.Lcd.setTextSize(3);
  M5.Lcd.setCursor(40, 55);
  M5.Lcd.printf("%.0fm", altitude);

  // Time on top-right
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(3);
  M5.Lcd.setCursor(200, 55);
  M5.Lcd.printf("%02d:%02d", now.hour(), now.minute());

  // Boxes layout
  M5.Lcd.drawRect(10, 90, 300, 120, WHITE);
  M5.Lcd.drawLine(160, 90, 160, 210, WHITE);

  // Left column - DHT
  M5.Lcd.setTextColor(dhtColor);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(20, 110);
  M5.Lcd.print("T:");
  M5.Lcd.setTextSize(3);
  M5.Lcd.setCursor(50, 105);
  M5.Lcd.printf("%.1fC", dht_temp);

  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(20, 150);
  M5.Lcd.print("H:");
  M5.Lcd.setTextSize(3);
  M5.Lcd.setCursor(50, 145);
  M5.Lcd.printf("%.0f%%", dht_humid);

  // Right column - BMP
  M5.Lcd.setTextColor(bmpColor);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(170, 110);
  M5.Lcd.print("T:");
  M5.Lcd.setTextSize(3);
  M5.Lcd.setCursor(200, 105);
  M5.Lcd.printf("%.1fC", bmp_temp);

  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(170, 150);
  M5.Lcd.print("P:");
  M5.Lcd.setTextSize(3);
  M5.Lcd.setCursor(200, 145);
  M5.Lcd.printf("%.0fh", pressure);
}

bool saveToSD(float dht_temp, float dht_humid, float bmp_temp, float pressure, float altitude, DateTime now) {
  String datePath = basePath + String(now.year()) + "/" + String(now.month()) + "/" + String(now.day()) + "/";
  String filename = datePath + "data.json";

  if (!createPath(datePath)) return false;

  File file = SD.open(filename, FILE_WRITE);
  if (!file) return false;

  StaticJsonDocument<256> doc;
  doc["timestamp"] = getTimestamp(now);
  doc["dht_temp"] = isnan(dht_temp) ? NULL : dht_temp;
  doc["dht_humid"] = isnan(dht_humid) ? NULL : dht_humid;
  doc["bmp_temp"] = bmp_temp;
  doc["pressure"] = pressure;
  doc["altitude"] = altitude;

  size_t bytesWritten = serializeJson(doc, file);
  file.println();
  file.close();

  return bytesWritten > 0;
}

bool createPath(String path) {
  String currentPath = "";
  int startIdx = 0;
  int endIdx = path.indexOf('/', startIdx);
  
  while (endIdx > 0) {
    currentPath = path.substring(0, endIdx);
    if (!SD.exists(currentPath)) {
      if (!SD.mkdir(currentPath)) return false;
    }
    startIdx = endIdx + 1;
    endIdx = path.indexOf('/', startIdx);
  }
  return true;
}

void showStatus(bool success) {
  M5.Lcd.fillRect(10, 230, 300, 10, success ? successColor : errorColor);
}

void showFatalError(const char* message) {
  M5.Lcd.fillScreen(RED);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(80, 110);
  M5.Lcd.println(message);
  while (1);
}

String getTimestamp(DateTime now) {
  char timestamp[20];
  snprintf(timestamp, sizeof(timestamp), "%04d-%02d-%02dT%02d:%02d:%02d",
          now.year(), now.month(), now.day(),
          now.hour(), now.minute(), now.second());
  return String(timestamp);
}
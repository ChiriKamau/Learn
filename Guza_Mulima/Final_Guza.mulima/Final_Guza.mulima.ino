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
  M5.begin(true, true, false); // LCD, SD, Serial (disable IMU)
  Serial.begin(115200);
  Wire.begin();

  // I2C Scanner
  Serial.println("Scanning I2C bus...");
  for (uint8_t address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0) {
      Serial.printf("I2C device at 0x%02X\n", address);
    }
  }

  // Initialize sensors
  dht.begin();
  if (!bmp.begin()) {
    showFatalError("BMP FAIL");
  }
  if (!rtc.begin()) {
    showFatalError("RTC FAIL");
  }

  // Check RTC status
  if (rtc.lostPower()) {
    Serial.println("RTC lost power, setting to compile time");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  // Verify RTC time
  DateTime now = rtc.now();
  Serial.printf("Initial RTC Time: %04d-%02d-%02d %02d:%02d:%02d\n",
                now.year(), now.month(), now.day(),
                now.hour(), now.minute(), now.second());

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

// Helper function to read RTC with retry
DateTime readRTC() {
  for (int i = 0; i < 3; i++) {
    DateTime now = rtc.now();
    if (now.year() >= 2020 && now.year() <= 2030) {
      return now;
    }
    Serial.println("RTC read failed, retrying...");
    delay(50); // Brief delay to allow I2C bus to settle
  }
  showFatalError("RTC READ ERR");
  return DateTime(2000, 1, 1, 0, 0, 0); // Explicit invalid time
}

void loop() {
  // Read sensors with delays to avoid I2C contention
  float dht_temp = dht.readTemperature();
  float dht_humid = dht.readHumidity();
  delay(10); // Small delay between I2C accesses

  sensors_event_t bmp_event;
  bmp.getEvent(&bmp_event);
  float altitude = bmp.pressureToAltitude(1013.25, bmp_event.pressure);
  float bmp_temp;
  bmp.getTemperature(&bmp_temp);
  delay(10);

  // Read RTC with retry
  DateTime now = readRTC();
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

  // Header bar
  M5.Lcd.fillRect(0, 0, 320, headerHeight, BLUE);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(20, 10);
  M5.Lcd.print("    Guza Mulima");

  // Altitude
  M5.Lcd.setTextColor(RED);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(10, 55);
  M5.Lcd.print("A:");
  M5.Lcd.setTextSize(3);
  M5.Lcd.setCursor(40, 55);
  M5.Lcd.printf("%.0fm", altitude);

  // Time (full HH:MM:SS for debugging)
  M5.Lcd.fillRect(180, 55, 140, 30, BLACK); // Clear larger time area
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(2); // Smaller size to fit seconds
  M5.Lcd.setCursor(180, 55);
  M5.Lcd.printf("%02d:%02d:%02d", now.hour(), now.minute(), now.second());

  // Debug: Show raw hour and minute values
  M5.Lcd.setTextSize(1);
  M5.Lcd.setCursor(180, 85);
  M5.Lcd.printf("H:%d M:%d", now.hour(), now.minute());

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
    currentPath = path.substring(0, end693
System: idx);
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
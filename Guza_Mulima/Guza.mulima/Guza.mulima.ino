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
  M5.begin();
  Serial.begin(115200);
  
  // Initialize sensors
  dht.begin();
  if(!bmp.begin() || !rtc.begin()) {
    showFatalError("SENSOR FAIL");
  }

  // Initialize SD card
  if(!initSDCard()) {
    showFatalError("SD FAIL");
  }
}

bool initSDCard() {
  for(int i = 0; i < 5; i++) {
    if(SD.begin(SD_CS)) {
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
  // Read sensors
  float dht_temp = dht.readTemperature();
  float dht_humid = dht.readHumidity();
  
  sensors_event_t bmp_event;
  bmp.getEvent(&bmp_event);
  float altitude = bmp.pressureToAltitude(1013.25, bmp_event.pressure);
  float bmp_temp;
  bmp.getTemperature(&bmp_temp);

  DateTime now = rtc.now();

  // Update display
  updateDisplay(dht_temp, dht_humid, bmp_temp, bmp_event.pressure, altitude, now);

  // Save data
  bool saveSuccess = saveToSD(dht_temp, dht_humid, bmp_temp, bmp_event.pressure, altitude, now);
  showStatus(saveSuccess);

  delay(5000);
}

void updateDisplay(float dht_temp, float dht_humid, float bmp_temp, float pressure, float altitude, DateTime now) {
  M5.Lcd.fillScreen(BLACK);
  
  // Display headers
  M5.Lcd.setTextColor(titleColor);
  M5.Lcd.setTextSize(2);
  
  // Draw title headers
  M5.Lcd.setCursor(20, 10);
  M5.Lcd.print("GW20 MoTiMa");
  
  M5.Lcd.setCursor(220, 10);
  M5.Lcd.print("BTOP");
  
  // Draw grid lines
  M5.Lcd.drawRect(10, 40, 300, 200, WHITE);  // Main border
  M5.Lcd.drawLine(160, 40, 160, 175, WHITE); // Vertical divider - stops before AT/P row
  
  // Draw the date/time in the first row with larger text
  M5.Lcd.setTextSize(3);
  
  // Date (left column) - without year
  M5.Lcd.setCursor(20, 50);
  M5.Lcd.printf("%02d-%02d", now.month(), now.day());
  
  // Time (right column)
  M5.Lcd.setCursor(180, 50);
  M5.Lcd.printf("%02d:%02d", now.hour(), now.minute());
  
  // Draw horizontal line after date/time
  M5.Lcd.drawLine(10, 85, 310, 85, WHITE);
  
  // DHT and BMU values
  // Left column
  M5.Lcd.setTextColor(dhtColor);
  M5.Lcd.setCursor(20, 95);
  M5.Lcd.printf("T: %.1fC", dht_temp);
  
  M5.Lcd.setCursor(20, 135);
  M5.Lcd.printf("H: %.0f%%", dht_humid);
  
  // Right column
  M5.Lcd.setTextColor(bmpColor);
  M5.Lcd.setCursor(180, 95);
  M5.Lcd.printf("T: %.1fC", bmp_temp);
  
  // Draw horizontal line before the AT/P row
  M5.Lcd.drawLine(10, 175, 310, 175, WHITE);
  
  // AT and P in one box that stretches across - larger values
  M5.Lcd.setTextSize(3);
  M5.Lcd.setTextColor(dhtColor);
  M5.Lcd.setCursor(20, 190);
  M5.Lcd.print("AT:");
  M5.Lcd.printf("%.0fm", altitude);
  
  M5.Lcd.setTextColor(bmpColor);
  M5.Lcd.setCursor(180, 190);
  M5.Lcd.print("P:");
  M5.Lcd.printf("%.0fhPa", pressure);
}

bool saveToSD(float dht_temp, float dht_humid, float bmp_temp, float pressure, float altitude, DateTime now) {
  String datePath = basePath + String(now.year()) + "/" + String(now.month()) + "/" + String(now.day()) + "/";
  String filename = datePath + "data.json";

  if(!createPath(datePath)) return false;

  File file = SD.open(filename, FILE_WRITE);
  if(!file) return false;

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
  
  while(endIdx > 0) {
    currentPath = path.substring(0, endIdx);
    if(!SD.exists(currentPath)) {
      if(!SD.mkdir(currentPath)) return false;
    }
    startIdx = endIdx + 1;
    endIdx = path.indexOf('/', startIdx);
  }
  return true;
}

void showStatus(bool success) {
  // Show red/green line at bottom as per sketch
  M5.Lcd.fillRect(10, 242, 300, 5, success ? successColor : errorColor);
}

void showFatalError(const char* message) {
  M5.Lcd.fillScreen(RED);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(80, 110);
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
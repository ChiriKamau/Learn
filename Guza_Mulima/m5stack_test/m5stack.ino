#include <M5Stack.h>
#include <SD.h>
#include <ArduinoJson.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP085_U.h>
#include <Adafruit_LSM303_U.h>
#include <RTClib.h>

// Sensor objects
Adafruit_BMP085_Unified bmp(18001);
Adafruit_LSM303_Accel_Unified accel(30301);
Adafruit_LSM303_Mag_Unified mag(30302);
RTC_DS3231 rtc;

// SD Card
const int SD_CS = 4;
String basePath = "/sensor_data/";

void setup() {
  M5.begin();
  Serial.begin(115200);
  SPI.begin();

  // Initialize RTC
  if (!rtc.begin()) {
    showFatalError("RTC Init Failed!");
  }

  // Initialize sensors
  if(!bmp.begin() || !accel.begin() || !mag.begin()) {
    showFatalError("Sensor Init Failed!");
  }

  // Initialize SD with debugging
  initSDCard();

  M5.Lcd.setTextSize(2);
  M5.Lcd.println("System Ready");
}

void loop() {
  // Read sensors
  sensors_event_t bmp_event, accel_event, mag_event;
  bmp.getEvent(&bmp_event);
  accel.getEvent(&accel_event);
  mag.getEvent(&mag_event);

  float altitude = bmp.pressureToAltitude(1013.25, bmp_event.pressure);
  DateTime now = rtc.now();
  String timestamp = getTimestamp(now);

  updateDisplay(altitude, accel_event, mag_event, timestamp);
  logToSD(altitude, bmp_event, accel_event, mag_event, timestamp);

  delay(5000);
}

// Improved SD Card Initialization
void initSDCard() {
  M5.Lcd.print("Initializing SD...");
  
  if (!SD.begin(SD_CS)) {
    String error = "SD Init Failed! ";
    error += "Type: ";
    switch (SD.cardType()) {
      case CARD_NONE: error += "No card"; break;
      case CARD_MMC: error += "MMC"; break;
      case CARD_SD: error += "SDSC"; break;
      case CARD_SDHC: error += "SDHC"; break;
      default: error += "Unknown";
    }
    showFatalError(error);
  }

  M5.Lcd.println("OK");
  M5.Lcd.print("Card Size: ");
  M5.Lcd.print(SD.cardSize() / (1024 * 1024));
  M5.Lcd.println("MB");
}

// Simplified SD Logging without getLastError()
// Replace the logToSD function with this improved version
void logToSD(float altitude, sensors_event_t bmp_event, 
            sensors_event_t accel_event, sensors_event_t mag_event, 
            String timestamp) {
  String datePath = basePath + timestamp.substring(0, 10); // Without trailing slash
  String filename = datePath + "/data.json"; // Add slash here

  // Debug output
  Serial.print("Attempting path: ");
  Serial.println(datePath);
  Serial.print("Full path: ");
  Serial.println(filename);

  // Create directory structure (modified)
  if (!SD.exists(datePath)) {
    Serial.println("Directory doesn't exist, creating...");
    
    // Create each directory level separately
    String currentPath = "";
    int startIdx = 0;
    int endIdx = datePath.indexOf('/', startIdx);
    
    while (endIdx >= 0) {
      currentPath = datePath.substring(0, endIdx);
      if (!SD.exists(currentPath)) {
        Serial.print("Creating subdir: ");
        Serial.println(currentPath);
        if (!SD.mkdir(currentPath)) {
          showSDError("Failed to create: " + currentPath);
          return;
        }
        // Small delay between directory creations
        delay(10);
      }
      startIdx = endIdx + 1;
      endIdx = datePath.indexOf('/', startIdx);
    }
    
    // Create the final directory
    if (!SD.mkdir(datePath)) {
      showSDError("Final dir create fail");
      return;
    }
  }

  // Now try to open the file
  File file = SD.open(filename, FILE_WRITE);
  if (!file) {
    showSDError("File open failed");
    // Additional debug
    Serial.print("File size: ");
    Serial.println(file.size());
    return;
  }

  // Create JSON
  StaticJsonDocument<256> doc;
  // ... [rest of your JSON creation code remains the same] ...

  if (serializeJson(doc, file) == 0) {
    showSDError("JSON write failed");
  } else {
    file.println(); // Newline
    M5.Lcd.println("Write OK");
    Serial.println("Write successful");
  }
  
  file.close();
}

// Helper Functions
String getTimestamp(DateTime now) {
  char timestamp[20];
  snprintf(timestamp, sizeof(timestamp), "%04d-%02d-%02dT%02d:%02d:%02d",
          now.year(), now.month(), now.day(),
          now.hour(), now.minute(), now.second());
  return String(timestamp);
}

void updateDisplay(float altitude, sensors_event_t accel, 
                  sensors_event_t mag, String timestamp) {
  M5.Lcd.fillRect(0, 0, 320, 180, BLACK);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.printf("Alt: %.2fm\n", altitude);
  M5.Lcd.printf("Accel: %.2f,%.2f,%.2f\n", 
               accel.acceleration.x,
               accel.acceleration.y,
               accel.acceleration.z);
  M5.Lcd.printf("Mag: %.2f,%.2f,%.2f\n",
               mag.magnetic.x,
               mag.magnetic.y,
               mag.magnetic.z);
  M5.Lcd.printf("Time: %s\n", timestamp.c_str());
}

void showSDError(String message) {
  M5.Lcd.setCursor(0, 180);
  M5.Lcd.print("SD Status: ");
  M5.Lcd.setTextColor(RED);
  M5.Lcd.println(message);
  M5.Lcd.setTextColor(WHITE);
  Serial.println("SD Error: " + message);
}

void showFatalError(String message) {
  M5.Lcd.fillScreen(RED);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(3);
  M5.Lcd.setCursor(10, 10);
  M5.Lcd.println(message);
  while(1);
}

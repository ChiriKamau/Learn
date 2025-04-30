#include <M5Stack.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include <SD.h>

TinyGPSPlus gps;
HardwareSerial GPS_Serial(2);

// Track if it's the first log entry for the day (to write [ ])
bool newFileToday = false;
String currentDate = "";

void setup() {
  M5.begin();
  M5.Lcd.setTextSize(2);
  M5.Lcd.println("Starting GPS...");

  GPS_Serial.begin(9600, SERIAL_8N1, 16, 17);

  if (!SD.begin()) {
    M5.Lcd.println("SD init failed!");
    while (1);
  } else {
    M5.Lcd.println("SD OK");
  }
}

void loop() {
  while (GPS_Serial.available()) {
    gps.encode(GPS_Serial.read());
  }

  if (gps.location.isUpdated() && gps.time.isUpdated() && gps.date.isUpdated()) {
    int hour = (gps.time.hour() + 3) % 24;  // UTC+3
    int minute = gps.time.minute();
    int second = gps.time.second();
    int year = gps.date.year();
    int month = gps.date.month();
    int day = gps.date.day();

    // Display
    M5.Lcd.clear();
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.printf("Time: %02d:%02d:%02d\n", hour, minute, second);
    M5.Lcd.printf("Lat: %.6f\n", gps.location.lat());
    M5.Lcd.printf("Lng: %.6f\n", gps.location.lng());

    // Format timestamp and date
    char timestamp[25];
    sprintf(timestamp, "%04d-%02d-%02dT%02d:%02d:%02d", year, month, day, hour, minute, second);
    char dateFolder[11];
    sprintf(dateFolder, "%04d-%02d-%02d", year, month, day);

    String folderPath = String("/") + dateFolder;
    String filePath = folderPath + "/gps_log.json";

    // If date changes, reset flag
    if (currentDate != String(dateFolder)) {
      currentDate = String(dateFolder);
      newFileToday = true;
    }

    // Create folder if it doesn't exist
    if (!SD.exists(folderPath)) {
      SD.mkdir(folderPath);
    }

    // Write JSON entry
    File file;
    if (newFileToday) {
      file = SD.open(filePath, FILE_WRITE);
      if (file) {
        file.println("[");
        file.close();
        newFileToday = false;
      }
    }

    // Append data
    file = SD.open(filePath, FILE_APPEND);
    if (file) {
      // Remove last ] if exists (for clean append)
      file.seek(file.size() - 2); // Remove newline and ]
      file.print(",\n");

      // JSON object
      String jsonData = String("  {\"timestamp\": \"") + timestamp +
                        "\", \"lat\": " + String(gps.location.lat(), 6) +
                        ", \"lng\": " + String(gps.location.lng(), 6) + "}";

      file.print(jsonData);
      file.println("\n]");
      file.close();
    } else {
      M5.Lcd.println("Failed to write file");
    }
  }

  delay(5000);  // Log every 5 seconds
}
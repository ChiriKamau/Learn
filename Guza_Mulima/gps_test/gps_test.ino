#include <M5Stack.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>

// Create GPS instance
TinyGPSPlus gps;

// Use UART2 on M5Stack (GPIO 16 = RX, GPIO 17 = TX)
HardwareSerial GPS_Serial(2);

void setup() {
  M5.begin();
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.println("Initializing GPS...");

  GPS_Serial.begin(9600, SERIAL_8N1, 16, 17);  // Baud, config, RX, TX
}

void loop() {
  while (GPS_Serial.available()) {
    gps.encode(GPS_Serial.read());
  }

  if (gps.location.isUpdated() && gps.time.isUpdated()) {
    M5.Lcd.clear();
    
    // Time in UTC
    int hour = gps.time.hour();
    int minute = gps.time.minute();
    int second = gps.time.second();

    // Adjust time to local time zone (e.g., +3 for East Africa)
    hour = (hour + 3) % 24;

    M5.Lcd.setCursor(0, 0);
    M5.Lcd.printf("Time: %02d:%02d:%02d\n", hour, minute, second);

    // Coordinates
    M5.Lcd.printf("Lat: %.6f\n", gps.location.lat());
    M5.Lcd.printf("Lng: %.6f\n", gps.location.lng());
  }

  delay(1000);
}
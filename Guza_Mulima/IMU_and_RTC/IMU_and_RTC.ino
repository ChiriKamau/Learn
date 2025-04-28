#include <Wire.h>
#include <RTClib.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_LSM303_U.h>
#include <Adafruit_L3GD20_U.h>
#include <Adafruit_BMP085_U.h>
#include <LowPower.h>

// Sensor objects
Adafruit_LSM303_Accel_Unified accel = Adafruit_LSM303_Accel_Unified(30301);
Adafruit_LSM303_Mag_Unified mag = Adafruit_LSM303_Mag_Unified(30302);
Adafruit_L3GD20_Unified gyro = Adafruit_L3GD20_Unified(20);
Adafruit_BMP085_Unified bmp = Adafruit_BMP085_Unified(18001);

RTC_DS3231 rtc;
const int SQW_PIN = 2;

void setup() {
  Serial.begin(9600);  // Increased baud rate
  while (!Serial) {
    delay(10);  // Wait for serial port to connect
  }
  Serial.println("Starting setup...");

  // Initialize RTC
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC!");
    while (1);
  } else {
    Serial.println("RTC initialized");
  }

  // Initialize sensors
  if (!accel.begin()) Serial.println("Accel failed");
  if (!mag.begin()) Serial.println("Mag failed");
  if (!gyro.begin()) Serial.println("Gyro failed");
  if (!bmp.begin()) Serial.println("BMP failed");

  // Configure SQW pin
  rtc.writeSqwPinMode(DS3231_SquareWave1Hz);
  pinMode(SQW_PIN, INPUT_PULLUP);
  
  Serial.println("Setup complete. Starting loop...");
}

void loop() {
  // First test - just show we're alive
  Serial.println("Loop running");
  
  // Get and display time
  DateTime now = rtc.now();
  Serial.print("Current time: ");
  Serial.print(now.year()); Serial.print("-");
  Serial.print(now.month()); Serial.print("-");
  Serial.print(now.day()); Serial.print(" ");
  Serial.print(now.hour()); Serial.print(":");
  Serial.print(now.minute()); Serial.print(":");
  Serial.println(now.second());

  // Simple sensor test
  sensors_event_t event;
  accel.getEvent(&event);
  Serial.print("Accel X: "); Serial.print(event.acceleration.x);
  
  delay(2000);  // Short delay for testing
}
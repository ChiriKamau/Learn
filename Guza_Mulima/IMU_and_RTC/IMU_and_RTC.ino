#include <Wire.h>
#include <RTClib.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_LSM303_U.h>
#include <Adafruit_L3GD20_U.h>
#include <Adafruit_BMP085_U.h>
#include <Adafruit_10DOF.h>
#include <LowPower.h> // For AVR deep sleep

RTC_DS3231 rtc;

// Sensor objects
Adafruit_LSM303_Accel_Unified accel = Adafruit_LSM303_Accel_Unified(30301);
Adafruit_LSM303_Mag_Unified mag = Adafruit_LSM303_Mag_Unified(30302);
Adafruit_L3GD20_Unified gyro = Adafruit_L3GD20_Unified(20);
Adafruit_BMP085_Unified bmp = Adafruit_BMP085_Unified(18001);
Adafruit_10DOF dof;

// Configuration
const int WAKE_INTERVAL_MINUTES = 10;
const int SQW_PIN = 2; // Connected to SQW output

// Variables for pulse counting
volatile unsigned long pulseCount = 0;
const unsigned long PULSES_NEEDED = WAKE_INTERVAL_MINUTES * 60; // 600 for 10 mins

// Function declarations (prototypes)
void countPulse();
void enterDeepSleep();
bool initializeSensors();
void readAndDisplaySensors(const char* timestamp);

void setup() {
  Serial.begin(9600);
  while (!Serial); // Wait for serial
  
  // Initialize RTC
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  // Configure SQW to 1Hz output
  rtc.writeSqwPinMode(DS3231_SquareWave1Hz);
  
  // Initialize sensors
  if (!initializeSensors()) {
    Serial.println("Sensor init failed!");
    while (1);
  }

  // Set up interrupt on falling edge
  pinMode(SQW_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(SQW_PIN), countPulse, FALLING);

  Serial.println("Setup complete. Starting main operation.");
}

void loop() {
  // Put Arduino to sleep (will wake on interrupt)
  enterDeepSleep();
  
  // After wake-up:
  if (pulseCount >= PULSES_NEEDED) {
    pulseCount = 0; // Reset counter
    
    // Get timestamp
    DateTime now = rtc.now();
    char timestamp[20];
    sprintf(timestamp, "%04d-%02d-%02d %02d:%02d:%02d", 
            now.year(), now.month(), now.day(), 
            now.hour(), now.minute(), now.second());
    
    Serial.print("\n10-minute wake at: ");
    Serial.println(timestamp);
    
    // Read and display sensor data
    readAndDisplaySensors(timestamp);
    
    // Here you would add your data saving logic
  }
}

// Interrupt Service Routine (keep it minimal)
void countPulse() {
  pulseCount++;
}

void enterDeepSleep() {
  // For AVR Arduinos:
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
}

bool initializeSensors() {
  return accel.begin() && 
         mag.begin() && 
         gyro.begin() && 
         bmp.begin();
}

void readAndDisplaySensors(const char* timestamp) {
  sensors_event_t accel_event, mag_event, gyro_event;
  float pressure;
  
  // Read sensors
  accel.getEvent(&accel_event);
  mag.getEvent(&mag_event);
  gyro.getEvent(&gyro_event);
  bmp.getPressure(&pressure);

  // Calculate altitude
  float altitude = bmp.pressureToAltitude(1013.25, pressure);
  Serial.print(timestamp);
  Serial.print(" - Altitude: ");
  Serial.print(altitude);
  Serial.println(" m");

  // Calculate orientation
  sensors_vec_t orientation;
  if (dof.accelGetOrientation(&accel_event, &orientation)) {
    Serial.print("Roll: ");
    Serial.print(orientation.roll);
    Serial.print("°, Pitch: ");
    Serial.print(orientation.pitch);
    Serial.println("°");
  }

  // Add gyro data
  Serial.print("Gyro X: ");
  Serial.print(gyro_event.gyro.x);
  Serial.print(" Y: ");
  Serial.print(gyro_event.gyro.y);
  Serial.print(" Z: ");
  Serial.println(gyro_event.gyro.z);
  
  // Add magnetic field data
  Serial.print("Mag X: ");
  Serial.print(mag_event.magnetic.x);
  Serial.print(" Y: ");
  Serial.print(mag_event.magnetic.y);
  Serial.print(" Z: ");
  Serial.println(mag_event.magnetic.z);
}
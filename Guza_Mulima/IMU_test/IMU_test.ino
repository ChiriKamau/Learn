#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_LSM303_U.h>
#include <Adafruit_L3GD20_U.h>
#include <Adafruit_BMP085_U.h>
#include <Adafruit_10DOF.h>

Adafruit_LSM303_Accel_Unified accel = Adafruit_LSM303_Accel_Unified(30301);
Adafruit_LSM303_Mag_Unified mag = Adafruit_LSM303_Mag_Unified(30302);
Adafruit_L3GD20_Unified gyro = Adafruit_L3GD20_Unified(20);
Adafruit_BMP085_Unified bmp = Adafruit_BMP085_Unified(18001);
Adafruit_10DOF dof;

void setup() {
  Serial.begin(9600);
  
  // Initialize sensors
  if (!accel.begin() || !mag.begin() || !gyro.begin() || !bmp.begin()) {
    Serial.println("Sensor init failed!");
    while (1);
  }
}

void loop() {
  sensors_event_t accel_event, mag_event, gyro_event;
  float pressure;  // Store pressure as float
  
  // Read sensors
  accel.getEvent(&accel_event);
  mag.getEvent(&mag_event);
  gyro.getEvent(&gyro_event);
  bmp.getPressure(&pressure);  // Correct method for BMP180

  // Calculate altitude (BMP180)
  float altitude = bmp.pressureToAltitude(1013.25, pressure);  // Use pressure value
  Serial.print("Altitude: "); Serial.print(altitude); Serial.println(" m");

  // Calculate orientation (LSM303)
  sensors_vec_t orientation;
  if (dof.accelGetOrientation(&accel_event, &orientation)) {
    Serial.print("Roll: "); Serial.print(orientation.roll);
    Serial.print(", Pitch: "); Serial.println(orientation.pitch);
  }

  delay(2000);
}
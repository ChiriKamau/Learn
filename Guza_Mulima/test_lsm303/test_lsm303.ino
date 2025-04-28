#include <Wire.h>
#include <Adafruit_BMP3XX.h>

Adafruit_BMP3XX bmp;
#define SEALEVEL_PRESSURE_HPA 1013.25 // Adjust for your local pressure

void setup() {
  Serial.begin(9600);
  if (!bmp.begin_I2C()) { // Default I2C address (0x77)
    Serial.println("BMP160 not found!");
    while (1);
  }
  // Configure sensor
  bmp.setTemperatureOversampling(BMP3_OVERSAMPLING_8X);
  bmp.setPressureOversampling(BMP3_OVERSAMPLING_4X);
}

void loop() {
  if (!bmp.performReading()) {
    Serial.println("Failed to read BMP160!");
    return;
  }
  float altitude = bmp.readAltitude(SEALEVEL_PRESSURE_HPA);
  Serial.print("Altitude (m): "); 
  Serial.println(altitude);
  delay(500);
}
#include <M5Stack.h>
#include "skubi_img.h"  // Include the generated header file

void setup() {
  // Initialize with optimized settings
  M5.begin(true, false, true);  // Enable LCD, disable SD card for speed, enable Serial
  
  // Set orientation if needed (0-3)
  M5.Lcd.setRotation(1);  // 0=portrait, 1=landscape, 2=portrait flipped, 3=landscape flipped
  
  // Set screen brightness (0-255)
  M5.Lcd.setBrightness(220);
  
  // Apply display settings for better image quality
  M5.Lcd.setSwapBytes(true);  // Required for correct color byte order
  
  // Clear screen
  M5.Lcd.fillScreen(TFT_BLACK);
  
  // Calculate center position for the image
  int x = (M5.Lcd.width() - IMG_DATA_WIDTH) / 2;
  int y = (M5.Lcd.height() - IMG_DATA_HEIGHT) / 2;
  
  // Draw image - using pushImage for best performance
  M5.Lcd.pushImage(x, y, IMG_DATA_WIDTH, IMG_DATA_HEIGHT, img_data);
  
  // Optional: Add a border
  M5.Lcd.drawRect(x-1, y-1, IMG_DATA_WIDTH+2, IMG_DATA_HEIGHT+2, TFT_WHITE);
  
  Serial.println("Image displayed");
}

void loop() {
  M5.update();
  
  // Button A: Toggle image color enhancement
  if (M5.BtnA.wasPressed()) {
    static bool enhanced = false;
    enhanced = !enhanced;
    
    if (enhanced) {
      // Apply color enhancement directly on the display
      M5.Lcd.invertDisplay(false);
      M5.Lcd.setTextColor(TFT_GREEN);
      M5.Lcd.setCursor(10, 10);
      M5.Lcd.print("Enhanced");
      // Increase contrast/brightness via display commands
      M5.Lcd.writecommand(0x55); // CABC Control
      M5.Lcd.writedata(0x02);    // Medium enhancement
    } else {
      // Normal display
      M5.Lcd.invertDisplay(false);
      M5.Lcd.setTextColor(TFT_WHITE);
      M5.Lcd.setCursor(10, 10);
      M5.Lcd.print("Normal   ");
      // Default settings
      M5.Lcd.writecommand(0x55);
      M5.Lcd.writedata(0x00);
    }
  }
  
  // Button B: Refresh image
  if (M5.BtnB.wasPressed()) {
    M5.Lcd.fillScreen(TFT_BLACK);
    int x = (M5.Lcd.width() - IMG_DATA_WIDTH) / 2;
    int y = (M5.Lcd.height() - IMG_DATA_HEIGHT) / 2;
    M5.Lcd.pushImage(x, y, IMG_DATA_WIDTH, IMG_DATA_HEIGHT, img_data);
    M5.Lcd.drawRect(x-1, y-1, IMG_DATA_WIDTH+2, IMG_DATA_HEIGHT+2, TFT_WHITE);
  }
  
  // Button C: Cycle through gamma settings
  if (M5.BtnC.wasPressed()) {
    static uint8_t gamma = 0;
    gamma = (gamma + 1) % 4;
    
    // Apply different gamma correction
    M5.Lcd.writecommand(0x26); // Gamma Set
    M5.Lcd.writedata(gamma);   // 0-3 different gamma curves
    
    M5.Lcd.setTextColor(TFT_YELLOW);
    M5.Lcd.setCursor(M5.Lcd.width() - 70, 10);
    M5.Lcd.printf("Gamma: %d", gamma);
  }
  
  delay(10);
}

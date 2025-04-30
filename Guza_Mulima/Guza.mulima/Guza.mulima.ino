void updateDisplay(float dht_temp, float dht_humid, float bmp_temp, float pressure, float altitude, DateTime now) {
  M5.Lcd.fillScreen(BLACK);

  // Header bar with blue background
  M5.Lcd.fillRect(0, 0, 320, headerHeight, BLUE);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(20, 10);
  M5.Lcd.print("Guza Mulima");

  // Altitude on top-left
  M5.Lcd.setTextColor(dhtColor);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(10, 45);
  M5.Lcd.print("A:");
  M5.Lcd.setTextSize(3);  // Larger value
  M5.Lcd.setCursor(40, 40);
  M5.Lcd.printf("%.0fm", altitude);

  // Time on top-right
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(3);
  M5.Lcd.setCursor(200, 40);
  M5.Lcd.printf("%02d:%02d", now.hour(), now.minute());

  // Boxes layout (only two columns now)
  M5.Lcd.drawRect(10, 90, 300, 120, WHITE);   // Main box
  M5.Lcd.drawLine(160, 90, 160, 210, WHITE);  // Middle divider

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
  M5.Lcd.printf("%.0fh", pressure);  // Remove 'Pa' to fit better
}


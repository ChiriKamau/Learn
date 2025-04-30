#include <M5Stack.h>
#include <DHT.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP085_U.h>
#include <TinyGPS++.h>
#include <SD.h>

#define DHTPIN 26
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);
Adafruit_BMP085_Unified bmp(18001);

TinyGPSPlus gps;
HardwareSerial GPSserial(2); // RX=16, TX=17

const String basePath = "/";  // Base path for SD

// Track timing for 1-minute data storage
unsigned long previousMillis = 0;
const long storageInterval = 60000; // 60000 milliseconds = 1 minute

// Display state
bool displayOn = false;
unsigned long displayOffTime = 0;
const long displayTimeout = 10000; // Display timeout after 10 seconds of inactivity

// 🛠 Forward declarations
void updateDisplay(float dht_temp, float dht_humid, float bmp_temp, float pressure, float altitude, int hour, int minute, int second, double lat, double lng);
void saveToJson(float lat, float lng, float altitude, float bmp_temp, float dht_temp, float dht_humid, float pressure, String timestamp, String filePath);

void setup() {
  M5.begin(true, true, true);
  Serial.begin(115200);
  GPSserial.begin(9600, SERIAL_8N1, 16, 17);

  dht.begin();
  if (!bmp.begin()) {
    M5.Lcd.fillScreen(RED);
    M5.Lcd.setTextColor(WHITE); M5.Lcd.setTextSize(3);
    M5.Lcd.setCursor(10, 100); M5.Lcd.print("BMP FAIL");
    while (true);
  }

  if (!SD.begin()) {
    M5.Lcd.fillScreen(RED);
    M5.Lcd.setCursor(10, 140); M5.Lcd.print("SD FAIL");
    while (true);
  }
  
  // Initialize the previousMillis
  previousMillis = millis();
  
  // Show initial message
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.fillRect(0, 0, 320, 40, BLUE);
  M5.Lcd.setTextColor(WHITE); M5.Lcd.setTextSize(3);
  M5.Lcd.setCursor(20, 10); M5.Lcd.print("  Guza Mulima");
  M5.Lcd.setTextColor(GREEN); M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(20, 120); M5.Lcd.print("Press any button");
  M5.Lcd.setCursor(20, 150); M5.Lcd.print("to view data");
  
  // Display initial message for 5 seconds then sleep
  delay(5000);
  M5.Lcd.sleep();
  M5.Lcd.setBrightness(0); // Turn off backlight completely
  displayOn = false;
}

void loop() {
  // Update M5Stack button states
  M5.update();
  
  // Process GPS data
  while (GPSserial.available()) {
    gps.encode(GPSserial.read());
  }

  // Read sensor data
  float dht_temp = dht.readTemperature();
  float dht_humid = dht.readHumidity();

  sensors_event_t bmp_event;
  bmp.getEvent(&bmp_event);
  float altitude = bmp.pressureToAltitude(1013.25, bmp_event.pressure);
  float bmp_temp;
  bmp.getTemperature(&bmp_temp);

  int hour = gps.time.isValid() ? gps.time.hour() : 0;
  int minute = gps.time.isValid() ? gps.time.minute() : 0;
  int second = gps.time.isValid() ? gps.time.second() : 0;
  double lat = gps.location.isValid() ? gps.location.lat() : 0.0;
  double lng = gps.location.isValid() ? gps.location.lng() : 0.0;

  // Check if any button is pressed
  if (M5.BtnA.wasPressed() || M5.BtnB.wasPressed() || M5.BtnC.wasPressed()) {
    // Wake up the display if it was sleeping
    if (!displayOn) {
      M5.Lcd.wakeup();
      M5.Lcd.setBrightness(100); // Restore backlight to full brightness
    }
    
    displayOn = true;
    displayOffTime = millis() + displayTimeout;
    updateDisplay(dht_temp, dht_humid, bmp_temp, bmp_event.pressure, altitude, hour, minute, second, lat, lng);
  }
  
  // Turn off display after timeout
  if (displayOn && millis() > displayOffTime) {
    displayOn = false;
    // Turn off display completely to save power
    M5.Lcd.sleep();
    M5.Lcd.setBrightness(0); // Turn off backlight completely
  }

  // Check if it's time to store data (every minute)
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= storageInterval) {
    previousMillis = currentMillis; // Reset the timer
    
    String timestamp = String(hour) + ":" + String(minute) + ":" + String(second);

    // Get date from GPS and format it as DDMMYYYY
    String date = String(gps.date.day()) + String(gps.date.month()) + String(gps.date.year());

    // Use date as the filename
    String filePath = basePath + date + ".json";

    saveToJson(lat, lng, altitude, bmp_temp, dht_temp, dht_humid, bmp_event.pressure, timestamp, filePath);
    
    // Visual indicator that data was saved (only show if display is on)
    if (displayOn) {
      M5.Lcd.fillRect(0, 230, 320, 10, GREEN);
      delay(500); // Flash green for half a second
      M5.Lcd.fillRect(0, 230, 320, 10, BLACK);
      
      // Refresh the display after showing the indicator
      updateDisplay(dht_temp, dht_humid, bmp_temp, bmp_event.pressure, altitude, hour, minute, second, lat, lng);
    }
  }

  // Short delay to prevent CPU hogging
  delay(100);
}

void updateDisplay(float dht_temp, float dht_humid, float bmp_temp, float pressure, float altitude, int hour, int minute, int second, double lat, double lng) {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.fillRect(0, 0, 320, 40, BLUE);
  M5.Lcd.setTextColor(WHITE); M5.Lcd.setTextSize(3);
  M5.Lcd.setCursor(20, 10); M5.Lcd.print("  Guza Mulima");

  M5.Lcd.setTextColor(RED); M5.Lcd.setTextSize(2.85);
  M5.Lcd.setCursor(10, 45); M5.Lcd.printf("A:%.0fm", altitude);
  M5.Lcd.setTextColor(GREEN); M5.Lcd.setTextSize(3);
  M5.Lcd.setCursor(150, 45); M5.Lcd.printf("|%02d:%02d:%02d", hour, minute, second);

  M5.Lcd.drawRect(10, 70, 300, 120, WHITE);
  M5.Lcd.drawLine(160, 70, 160, 190, WHITE);

  M5.Lcd.setTextColor(GREEN); M5.Lcd.setCursor(20, 90); M5.Lcd.setTextSize(3);
  M5.Lcd.printf("T:%.1fC", dht_temp);
  M5.Lcd.setCursor(20, 130); M5.Lcd.printf("H:%.0f%%", dht_humid);

  M5.Lcd.setTextColor(YELLOW); M5.Lcd.setCursor(170, 90); M5.Lcd.setTextSize(3);
  M5.Lcd.printf("T:%.1fC", bmp_temp);
  M5.Lcd.setCursor(170, 130); M5.Lcd.printf("P:%.0fh", pressure);

  M5.Lcd.setTextColor(WHITE); M5.Lcd.setCursor(10, 200); M5.Lcd.setTextSize(2);
  M5.Lcd.printf("L:%.5f", lat);
  M5.Lcd.setCursor(160, 200);
  M5.Lcd.printf("Ln:%.5f", lng);
}

void saveToJson(float lat, float lng, float altitude, float bmp_temp, float dht_temp, float dht_humid, float pressure, String timestamp, String filePath) {
  File file = SD.open(filePath, FILE_READ);
  String content = "";

  if (file) {
    while (file.available()) {
      content += (char)file.read();
    }
    file.close();
  }

  if (content.endsWith("]")) {
    content.remove(content.length() - 1); // remove last ']'
    content += ",\n";
  }

  content += "  {\"timestamp\": \"" + timestamp + "\", ";
  content += "\"lat\": " + String(lat, 6) + ", \"lng\": " + String(lng, 6) + ", ";
  content += "\"altitude\": " + String(altitude, 2) + ", ";
  content += "\"pressure\": " + String(pressure, 2) + ", ";
  content += "\"bmp_temp\": " + String(bmp_temp, 2) + ", ";
  content += "\"dht_temp\": " + String(dht_temp, 1) + ", ";
  content += "\"dht_humidity\": " + String(dht_humid, 1) + "}\n]";

  File writeFile = SD.open(filePath, FILE_WRITE);
  if (writeFile) {
    writeFile.print(content);
    writeFile.close();
  }
}
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

const String filePath = "/data.json";

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

  if (!SD.exists(filePath)) {
    File file = SD.open(filePath, FILE_WRITE);
    if (file) {
      file.println("[");
      file.println("]");
      file.close();
    }
  }
}

void loop() {
  while (GPSserial.available()) {
    gps.encode(GPSserial.read());
  }

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

  updateDisplay(dht_temp, dht_humid, bmp_temp, bmp_event.pressure, altitude, hour, minute, second, lat, lng);

  String timestamp = String(hour) + ":" + String(minute) + ":" + String(second);
  saveToJson(lat, lng, altitude, bmp_temp, dht_temp, dht_humid, bmp_event.pressure, timestamp, filePath);

  delay(5000);
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

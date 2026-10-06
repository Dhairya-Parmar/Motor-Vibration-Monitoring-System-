#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_ADXL345_U.h>
#include <WiFi.h>
#include "ThingSpeak.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);

// Motor pins
#define IN1 26
#define IN2 27
#define ENA 25


// Wi-Fi & ThingSpeak
const char* ssid = "Dhairya's Wifi";
const char* password = "123456789";
WiFiClient client;
unsigned long myChannelNumber = 1234567;
const char* myWriteAPIKey = "1234567890";

float baseline = 0;
unsigned long lastAlertTime = 0;
bool alertActive = false;

void setup() {
  Serial.begin(115200);

  // OLED setup
  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Vibration Monitor");
  display.display();

  // ADXL345
  if (!accel.begin()) {
    Serial.println("No ADXL345 detected!");
    while (1);
  }
  accel.setRange(ADXL345_RANGE_16_G);

  // Motor pins
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENA, OUTPUT);

  // WiFi
  WiFi.begin(ssid, password);
  display.setCursor(0, 10);
  display.println("Connecting WiFi...");
  display.display();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  ThingSpeak.begin(client);

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("WiFi Connected!");
  display.display();
  delay(1000);

  // Calibrate baseline
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Calibrating...");
  display.display();

  float sum = 0;
  for (int i = 0; i < 100; i++) {
    sensors_event_t event;
    accel.getEvent(&event);
    float mag = sqrt(event.acceleration.x * event.acceleration.x +
                     event.acceleration.y * event.acceleration.y +
                     event.acceleration.z * event.acceleration.z);
    sum += mag;
    delay(20);
  }
  baseline = sum / 100.0;
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Baseline: ");
  display.println(baseline);
  display.display();
  delay(1000);
}

void loop() {
  sensors_event_t event;
  accel.getEvent(&event);

  float mag = sqrt(event.acceleration.x * event.acceleration.x +
                   event.acceleration.y * event.acceleration.y +
                   event.acceleration.z * event.acceleration.z);

  float vibration = fabs(mag - baseline);

  // OLED display
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.print("Vibration: ");
  display.println(vibration, 2);

  // ---- ALERT TRIGGER ----
  if (vibration > 2.5) {
    display.setCursor(0, 20);
    display.setTextSize(1);
    display.println("ALERT! High vibration");
    display.display();

    // MOTOR ON (Forward)
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    digitalWrite(ENA, HIGH);  // Enable

    alertActive = true;
    lastAlertTime = millis();
  } 
  
  // ---- END ALERT AFTER 5 SEC ----
  else if (alertActive && millis() - lastAlertTime > 5000) {

    // MOTOR OFF
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    digitalWrite(ENA, LOW);

    alertActive = false;
  }

  display.display();

  // SEND TO THINGSPEAK
  ThingSpeak.setField(1, vibration);
  ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);

  delay(1000);
}

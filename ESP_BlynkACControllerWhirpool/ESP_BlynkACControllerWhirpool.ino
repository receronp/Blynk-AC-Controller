/*
   Author: Raul Eugenio Ceron Pineda
   Institutional ID: A00823906
*/

// Template ID, Device Name and Auth Token are provided by the Blynk.Cloud
// See the Device Info tab, or Template settings
#define BLYNK_TEMPLATE_ID "TMPLq329pej2"
#define BLYNK_DEVICE_NAME "ESP32 AC Controller"
#define BLYNK_AUTH_TOKEN "oRDe49AxZyCincI8do325X79GiBrQQIm"

// Header file with WiFi Configuration
#include <WifiConfig.h>

// Comment this out to disable prints and save space
#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <LiquidCrystal_I2C.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <ir_Whirlpool.h>
#include <DHT.h>

char auth[] = BLYNK_AUTH_TOKEN;

// Your WiFi credentials
char ssid[] = HOME_SSID2;
char pass[] = HOME_PASS2;

// Set digital pin for DHT Sensor connection
#define DHTPIN 16

// Set the LCD number of columns and rows
int lcdColumns = 16;
int lcdRows = 2;

bool acStatus = false;
uint8_t goalTemp = 25;
const uint16_t kIrLed = 4;

// set LCD address, number of columns and rows
// if you don't know your display address, run an I2C scanner sketch
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);
DHT dht(DHTPIN, DHT11);
IRWhirlpoolAc ac(kIrLed);
BlynkTimer timer;

void printState() {
  // Display the settings.
  Serial.println("A/C remote is in the following state:");
  Serial.printf("  %s\n", ac.toString().c_str());
}

void printAcStatus() {
  lcd.setCursor(0, 0);
  if (acStatus == false) {
    lcd.print("OFF");
  } else {
    lcd.printf("ON - Set: %d%cC", goalTemp, 223);  // char(223) = degree symbol (°)
  }
}

void sendSensor() {
  float t = dht.readTemperature();

  if (isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  lcd.clear();
  printAcStatus();
  lcd.setCursor(0, 1);
  lcd.printf("Temp: %.2f%cC", t, 223);  // char(223) = degree symbol (°)
  Blynk.virtualWrite(V1, t);
}

BLYNK_WRITE(V0) {
  if (param.asInt() == 1) {
    Serial.println("Turn on the A/C ...");
    acStatus = true;
    ac.setPowerToggle(acStatus);
    ac.send();
  } else {
    Serial.println("Turn off the A/C ...");
    acStatus = false;
    ac.setPowerToggle(true);
    ac.send();
    ac.setPowerToggle(acStatus);
  }
  printState();
}

BLYNK_WRITE(V2) {
  goalTemp = param.asInt();
  Serial.printf("Setting new goal temp: %d°C\n", goalTemp);
  ac.setTemp(goalTemp);
  if (acStatus) {
    ac.send();
    ac.setPowerToggle(true);
    ac.send();
  }
  printState();
}

void setup() {
  ac.begin();
  Serial.begin(115200);
  delay(200);

  // initialize LCD
  lcd.init();
  // turn on LCD backlight
  lcd.backlight();

  Blynk.begin(auth, ssid, pass);
  dht.begin();

  // Set up what we want to send.
  Serial.println("Default state of the remote.");
  printState();
  Serial.println("Setting initial state for A/C.");
  ac.setPowerToggle(acStatus);
  ac.setFan(1);
  ac.setMode(kWhirlpoolAcCool);
  ac.setSwing(false);
  ac.setLight(true);
  ac.setTemp(goalTemp);
  Blynk.virtualWrite(V0, acStatus);
  Blynk.virtualWrite(V2, goalTemp);
  printState();

  // Setup a function to be called every second
  timer.setInterval(1000L, sendSensor);
}

void loop() {
  Blynk.run();
  timer.run();
}

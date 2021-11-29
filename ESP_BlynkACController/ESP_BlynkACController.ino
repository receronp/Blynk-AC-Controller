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
#include <ir_Kelvinator.h>
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

const uint16_t kIrLed = 4;
bool acTraneSelected = true;
String acTraneStatus = "OFF";
uint8_t acTraneGoalTemp = 25;
String acWhirlpoolStatus = "OFF";
uint8_t acWhirlpoolGoalTemp = 25;

// set LCD address, number of columns and rows
// if you don't know your display address, run an I2C scanner sketch
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);
DHT dht(DHTPIN, DHT11);
IRKelvinatorAC acTrane(kIrLed);
IRWhirlpoolAc acWhirlpool(kIrLed);
BlynkTimer timer;

void printAcStatus(float temp) {
  lcd.clear();
  lcd.setCursor(0, 0);
  // char(223) = degree symbol (°)
  lcd.printf("1:%s|2:%s|T:%d%cC", acTraneStatus, acWhirlpoolStatus, (uint8_t)temp, 223);
  lcd.setCursor(0, 1);
  lcd.printf("S1:%d%cC|S2:%d%cC", acTraneGoalTemp, 223, acWhirlpoolGoalTemp, 223);
}

void sendSensor() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (isnan(t) | isnan(h)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  printAcStatus(t);
  Blynk.virtualWrite(V3, t);
  Blynk.virtualWrite(V4, h);
}

BLYNK_CONNECTED() {
  Blynk.virtualWrite(V0, 0);    // Trane AC Select
  Blynk.virtualWrite(V1, 0);    // Set to OFF
  Blynk.virtualWrite(V2, acTraneGoalTemp);
}

BLYNK_WRITE(V0) {
  uint8_t recoverStatus;
  if (param.asInt() == 1) {
    Serial.println("Setting Whirlpool A/C to Active...");
    acTraneSelected = false;
    recoverStatus = acWhirlpoolStatus == "ON" ? 1 : 0;
    Blynk.virtualWrite(V1, recoverStatus);
    Blynk.virtualWrite(V2, acWhirlpoolGoalTemp);
  } else {
    Serial.println("Setting Trane A/C to Active...");
    acTraneSelected = true;
    recoverStatus = acTraneStatus == "ON" ? 1 : 0;
    Blynk.virtualWrite(V1, recoverStatus);
    Blynk.virtualWrite(V2, acTraneGoalTemp);
  }
}

BLYNK_WRITE(V1) {
  if (acTraneSelected) {
    if (param.asInt() == 1) {
      Serial.println("Turn on the Trane A/C ...");
      acTraneStatus = "ON";
      acTrane.on();
    } else {
      Serial.println("Turn off the Trane A/C ...");
      acTraneStatus = "OFF";
      acTrane.off();
    }
    acTrane.send();
  } else {
    if (param.asInt() == 1) {
      Serial.println("Turn on the Whirlpool A/C ...");
      acWhirlpoolStatus = "ON";
      acWhirlpool.setPowerToggle(true);
      acWhirlpool.send();
    } else {
      Serial.println("Turn off the Whirlpool A/C ...");
      acWhirlpoolStatus = "OFF";
      acWhirlpool.setPowerToggle(true);
      acWhirlpool.send();
      acWhirlpool.setPowerToggle(false);
    }
  }
}

BLYNK_WRITE(V2) {
  if (acTraneSelected) {
    acTraneGoalTemp = param.asInt();
    Serial.printf("Setting Trane goal temp: %d°C\n", acTraneGoalTemp);
    acTrane.setTemp(acTraneGoalTemp);
    acTrane.send();
  } else {
    acWhirlpoolGoalTemp = param.asInt();
    if (acWhirlpoolGoalTemp < 18) {
      acWhirlpoolGoalTemp =  18;
      Blynk.virtualWrite(V2, 18);
    }
    Serial.printf("Setting Whirlpool goal temp: %d°C\n", acWhirlpoolGoalTemp);
    acWhirlpool.setTemp(acWhirlpoolGoalTemp);
    if (acWhirlpoolStatus == "ON") {
      acWhirlpool.send();
      acWhirlpool.setPowerToggle(true);
      acWhirlpool.send();
    }
  }
}

void setup() {
  acTrane.begin();
  acWhirlpool.begin();
  Serial.begin(115200);
  delay(200);

  // initialize DHT and LCD
  dht.begin();
  lcd.init();
  lcd.backlight();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting to");
  lcd.setCursor(0, 1);
  lcd.print("Blynk...");
  Blynk.begin(auth, ssid, pass);

  // Set up Trane AC
  acTrane.off();
  acTrane.setFan(1);
  acTrane.setMode(kKelvinatorCool);
  acTrane.setSwingVertical(false);
  acTrane.setLight(true);
  acTrane.setTemp(acTraneGoalTemp);

  // Set up Whirlpool AC
  acWhirlpool.setPowerToggle(false);
  acWhirlpool.setFan(1);
  acWhirlpool.setMode(kWhirlpoolAcCool);
  acWhirlpool.setSwing(false);
  acWhirlpool.setLight(true);
  acWhirlpool.setTemp(acWhirlpoolGoalTemp);

  // Setup a function to be called every second
  timer.setInterval(1000L, sendSensor);
}

void loop() {
  Blynk.run();
  timer.run();
}

/*
 * heart_sensor_BMI
 *
 * Arduino Uno R3 firmware:
 *   - MAX30102 pulse-oximeter / heart-rate sensor (I2C, address 0x57)
 *   - SSD1306 0.96" 128x64 OLED display (I2C, address 0x3C)
 *   - Heartbeat indicator LED on D9 (through 220 ohm resistor)
 *
 * Libraries (Arduino IDE -> Tools -> Manage Libraries...):
 *   - SparkFun MAX3010x Sensor     (by SparkFun Electronics)
 *   - Adafruit SSD1306             (by Adafruit)
 *   - Adafruit GFX Library         (by Adafruit)
 *
 * Wiring:
 *   MAX30102 VIN -> 3.3V,  GND -> GND,  SDA -> A4,  SCL -> A5
 *   SSD1306  VCC -> 3.3V (or 5V per module), GND -> GND,
 *             SDA -> A4,  SCL -> A5   (shared I2C bus)
 *   LED: D9 -> 220 ohm -> LED anode, LED cathode -> GND
 *
 * Open the Serial Monitor at 115200 baud for the plotter-ready output.
 */

#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

// ---------- Pins ---------------------------------------------------------
const uint8_t LED_PIN = 9;            // heartbeat indicator LED

// ---------- Display ------------------------------------------------------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET   -1               // shared with Arduino reset
#define OLED_ADDR    0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ---------- MAX30102 -----------------------------------------------------
MAX30105 particleSensor;

// ---------- Beat detection ----------------------------------------------
const uint8_t  BEAT_THRESHOLD     = 125;  // IR intensity delta that counts as a beat
const uint16_t BEAT_TIMEOUT_MS    = 2000; // no beat for 2 s -> reset average
const uint8_t  AVG_WINDOW         = 8;    // beats averaged for BPM

byte         heartRateArray[AVG_WINDOW];
uint8_t      heartRateIndex  = 0;
bool         beatArrived     = false;
long         lastBeatMillis  = 0;
float        beatsPerMinute  = 0.0;
int          beatAverage     = 0;

uint32_t     irValue         = 0;
bool         fingerDetected   = false;

// ---------- Display refresh ---------------------------------------------
const uint16_t DISPLAY_PERIOD_MS = 200;
uint32_t       lastDisplayMillis = 0;

// ---------- Serial log --------------------------------------------------
const uint16_t SERIAL_PERIOD_MS = 500;
uint32_t       lastSerialMillis = 0;

// ========================================================================

void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Serial.begin(115200);
  while (!Serial) { ; }               // wait for USB serial on native-USB boards
  Serial.println(F("heart_sensor_BMI starting..."));

  // --- OLED -------------------------------------------------------------
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println(F("SSD1306 not found on I2C bus (0x3C). Check wiring."));
    while (true) { digitalWrite(LED_PIN, !digitalRead(LED_PIN)); delay(150); }
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(F("heart_sensor_BMI"));
  display.println(F(""));
  display.println(F("Warming up MAX30102..."));
  display.display();
  delay(500);

  // --- MAX30102 ---------------------------------------------------------
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println(F("MAX30102 not found on I2C bus (0x57). Check wiring."));
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println(F("ERROR"));
    display.println(F("MAX30102 not found"));
    display.display();
    while (true) { digitalWrite(LED_PIN, !digitalRead(LED_PIN)); delay(150); }
  }

  // Config tuned for finger-based heart-rate on a 5 V Uno:
  //  sample average 4, 400 samples/s, ADC range 4096, LED current 0x24 (~7 mA)
  particleSensor.setup(BEAT_THRESHOLD, 4, 2, 400, 411, 4096);
  particleSensor.setPulseAmplitudeRed(0x0A);
  particleSensor.setPulseAmplitudeIR(0x1F);   // IR drives the beat detection

  Serial.println(F("Place your fingertip lightly over the MAX30102 sensor."));
}

// ========================================================================

void loop() {
  irValue = particleSensor.getIR();

  // --- finger detection --------------------------------------------------
  fingerDetected = (irValue > 50000);

  // --- beat detection ----------------------------------------------------
  beatArrived = checkForBeat(irValue);

  if (beatArrived && fingerDetected) {
    // Flash the indicator LED and measure the inter-beat interval.
    digitalWrite(LED_PIN, HIGH);

    long interval = millis() - lastBeatMillis;
    lastBeatMillis = millis();

    if (interval > 300 && interval < 2000) {          // 30-200 BPM plausibility
      byte bpm = (byte)(60.0 / (interval / 1000.0));
      heartRateArray[heartRateIndex++] = bpm;
      heartRateIndex %= AVG_WINDOW;

      beatAverage = 0;
      for (uint8_t i = 0; i < AVG_WINDOW; i++) beatAverage += heartRateArray[i];
      beatAverage /= AVG_WINDOW;
      beatsPerMinute = beatAverage;
    }
  } else {
    digitalWrite(LED_PIN, LOW);
  }

  // --- BPM decay when finger is removed ---------------------------------
  if (!fingerDetected || (millis() - lastBeatMillis) > BEAT_TIMEOUT_MS) {
    beatsPerMinute    = 0;
    beatAverage       = 0;
    memset(heartRateArray, 0, sizeof(heartRateArray));
    heartRateIndex    = 0;
  }

  // --- OLED refresh ------------------------------------------------------
  if (millis() - lastDisplayMillis >= DISPLAY_PERIOD_MS) {
    lastDisplayMillis = millis();
    renderDisplay();
  }

  // --- Serial log (Serial Plotter friendly) ------------------------------
  if (millis() - lastSerialMillis >= SERIAL_PERIOD_MS) {
    lastSerialMillis = millis();
    Serial.print(F("IR="));
    Serial.print(irValue);
    Serial.print(F(" BPM="));
    Serial.println((int)beatsPerMinute);
  }
}

// ========================================================================

void renderDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(F("  heart_sensor_BMI"));
  display.drawLine(0, 10, SCREEN_WIDTH - 1, 10, SSD1306_WHITE);

  if (!fingerDetected) {
    display.setCursor(0, 24);
    display.setTextSize(1);
    display.println(F("Place finger on"));
    display.setCursor(0, 36);
    display.println(F("the MAX30102 sensor"));
  } else if (beatsPerMinute > 0) {
    display.setCursor(0, 16);
    display.setTextSize(1);
    display.println(F("Heart rate"));

    display.setTextSize(3);
    display.setCursor(10, 30);
    display.print((int)beatsPerMinute);
    display.setTextSize(2);
    display.print(F(" BPM"));

    // simple heartbeat glyph that tracks the LED
    display.setTextSize(1);
    display.setCursor(100, 56);
    display.print(digitalRead(LED_PIN) ? F("<3 <3") : F("  <3 "));
  } else {
    display.setCursor(0, 24);
    display.setTextSize(1);
    display.println(F("Measuring..."));
  }

  display.display();
}

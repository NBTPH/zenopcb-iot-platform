/**
 * @file 04_ldr_light_sensor.ino
 * @brief Measure relative brightness with an LDR voltage divider, publish 0..100 % to Z0.
 *
 * What you'll learn:
 *   - How an LDR (light-dependent resistor) changes its resistance with light
 *   - How to build a voltage divider so the ADC can read that change
 *   - How to normalise an ADC value to 0..100 % across different boards
 *
 * Hardware needed:
 *   - Any supported board with an ADC pin
 *   - LDR (any GL55xx series is fine)
 *   - 10 kOhm fixed resistor
 *   - Jumper wires, breadboard
 *
 * Wiring (voltage divider):
 *   - 3.3 V    -> one leg of the LDR
 *   - Other leg of LDR -> LDR_PIN (ADC) AND -> 10 kOhm -> GND
 *   (Bright light lowers the LDR resistance, raising the ADC reading.)
 *
 * Cloud dashboard setup:
 *   - Create Z0 of type Float — brightness, expressed as 0..100 %
 *
 * How to use:
 *   1. Replace WIFI_SSID / WIFI_PASS / DEVICE_ID / DEVICE_TOKEN below.
 *   2. Open Tools > Partition Scheme > "Minimal SPIFFS (1.9MB APP)" (ESP32 only).
 *   3. Flash and open Serial Monitor at 115200 baud.
 *   4. Cover the LDR with your hand vs. shine a phone torch — Z0 should swing.
 *
 * Tips & common mistakes:
 *   - The percentage is a relative figure, not a calibrated lux value.
 *   - Ambient temperature and your supply voltage both shift the divider —
 *     if you need real lux, switch to a digital sensor like BH1750.
 */

#include <ZenoPCBMain.h>

using namespace ZenoPCB;

#define WIFI_SSID "REPLACE_ME"
#define WIFI_PASS "REPLACE_ME"
#define DEVICE_ID "REPLACE_ME"
#define DEVICE_TOKEN "REPLACE_AT_PROVISIONING"

// ADC full-scale varies per board (see 05_analog_read for a fuller explanation).
#if defined(ESP32)
  #define LDR_PIN 34
  #define ADC_FULL_SCALE 4095.0f
#elif defined(ESP8266)
  #define LDR_PIN A0
  #define ADC_FULL_SCALE 1023.0f
#elif defined(ARDUINO_UNOR4_WIFI)
  #define LDR_PIN A0
  #define ADC_FULL_SCALE 16383.0f
#elif defined(STM32F4)
  #define LDR_PIN PA0
  #define ADC_FULL_SCALE 4095.0f
#elif defined(STM32F1)
  #define LDR_PIN PA0
  #define ADC_FULL_SCALE 4095.0f
#endif

Zeno zeno;

// Device -> Cloud: sample brightness every 0.5 seconds.
// setZPublishInterval(1000) below publishes dirty Z values every 1 second.
ZENO_EVERY(500)
{
    const float raw = (float)analogRead(LDR_PIN);
    const float pct = (raw / ADC_FULL_SCALE) * 100.0f;  // normalise to 0..100 %
    DEVICE_TO_CLOUD(Z0, pct);
    Serial.printf("[LDR] brightness = %.1f%%\n", pct);
}

void setup()
{
    Serial.begin(115200);

    zeno.wifi(WIFI_SSID, WIFI_PASS)
        .device(DEVICE_ID, DEVICE_TOKEN)
        .enableZKeys()
        .setZPublishInterval(1000)
        .begin();
}

void loop()
{
    zeno.loop();
}

/**
 * @file 03_ultrasonic_distance.ino
 * @brief HC-SR04 ultrasonic distance sensor -> Z0 in centimetres.
 *
 * @category Sensors
 * @level Beginner
 *
 * @hardware
 * - Any supported board.
 * - HC-SR04 ultrasonic (5 V; echo line may need divider on 3.3 V boards).
 *
 * @wiring
 * - HC-SR04 VCC -> 5 V.
 * - HC-SR04 GND -> GND.
 * - HC-SR04 TRIG -> TRIG_PIN.
 * - HC-SR04 ECHO -> ECHO_PIN (use 1 kOhm + 2 kOhm divider on 3.3 V MCUs).
 *
 * @usage
 * 1. Set credentials.
 * 2. Distance sampled + published to Z0 every 2 s via ZENO_EVERY(2000).
 */

#include <ZenoPCBMain.h>

using namespace ZenoPCB;

#define WIFI_SSID "REPLACE_ME"
#define WIFI_PASS "REPLACE_ME"
#define DEVICE_ID "REPLACE_ME"
#define DEVICE_TOKEN "REPLACE_AT_PROVISIONING"

#if defined(ESP32)
  #define TRIG_PIN 5
  #define ECHO_PIN 18
#elif defined(ESP8266)
  #define TRIG_PIN 5    // D1
  #define ECHO_PIN 4    // D2
#elif defined(ARDUINO_UNOR4_WIFI)
  #define TRIG_PIN 7
  #define ECHO_PIN 8
#elif defined(STM32F4)
  #define TRIG_PIN PB4
  #define ECHO_PIN PB5
#elif defined(STM32F1)
  #define TRIG_PIN PB4
  #define ECHO_PIN PB5
#endif

Zeno zeno;

static float measureDistanceCm()
{
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    const uint32_t durUs = pulseIn(ECHO_PIN, HIGH, 30000UL); // 30 ms timeout
    if (durUs == 0) return -1.0f;
    // Speed of sound 343 m/s -> 0.0343 cm/us; round trip /2
    return (float)durUs * 0.0343f * 0.5f;
}

// Ping + publish distance every 2 s.
ZENO_EVERY(2000)
{
    const float d = measureDistanceCm();
    if (d > 0.0f)
    {
        DEVICE_TO_CLOUD(Z0, d);
        Serial.printf("[HC-SR04] d=%.1f cm\n", d);
    }
}

void setup()
{
    Serial.begin(115200);
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    digitalWrite(TRIG_PIN, LOW);

    zeno.wifi(WIFI_SSID, WIFI_PASS)
        .device(DEVICE_ID, DEVICE_TOKEN)
        .enableZKeys()
        .begin();
}

void loop()
{
    zeno.loop();
}

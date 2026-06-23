/**
 * @file 03_ultrasonic_distance.ino
 * @brief Measure distance with an HC-SR04 ultrasonic sensor and publish centimetres to Z0.
 *
 * What you'll learn:
 *   - How the HC-SR04 uses a trigger pulse and echo pulse to measure distance
 *   - How pulseIn() times an incoming pulse in microseconds
 *   - How to convert echo time to centimetres using the speed of sound
 *
 * Hardware needed:
 *   - Any supported board
 *   - HC-SR04 ultrasonic distance sensor (5 V supply)
 *   - For 3.3 V boards: voltage divider on ECHO (e.g. 1 kOhm + 2 kOhm) — the
 *     HC-SR04 outputs 5 V on ECHO which can damage 3.3 V GPIOs over time.
 *   - Jumper wires, breadboard
 *
 * Wiring:
 *   - HC-SR04 VCC  -> 5 V
 *   - HC-SR04 GND  -> GND
 *   - HC-SR04 TRIG -> TRIG_PIN
 *   - HC-SR04 ECHO -> ECHO_PIN  (through divider on 3.3 V MCUs)
 *
 * Cloud dashboard setup:
 *   - Create Z0 of type Float — distance in centimetres
 *
 * How to use:
 *   1. Replace WIFI_SSID / WIFI_PASS / DEVICE_ID / DEVICE_TOKEN below.
 *   2. Open Tools > Partition Scheme > "Minimal SPIFFS (1.9MB APP)" (ESP32 only).
 *   3. Flash and open Serial Monitor at 115200 baud.
 *   4. Move your hand 5..200 cm in front of the sensor — Z0 should track it
 *      every 2 s.
 *
 * Tips & common mistakes:
 *   - The HC-SR04 is useful from about 2 cm to 4 m. Outside that range pulseIn
 *     either times out or returns junk; we treat 0 (timeout) as "no reading".
 *   - Hard, flat surfaces echo well. Soft fabric, fur, and angled walls do not.
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

// Standard HC-SR04 ping: send a 10 us pulse on TRIG, then time the echo pulse on ECHO.
static float measureDistanceCm()
{
    // Ensure TRIG starts LOW, then send a clean 10 us HIGH pulse.
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    // Time how long ECHO stays HIGH, in microseconds. 30 ms timeout ~ 5 m max range.
    const uint32_t durUs = pulseIn(ECHO_PIN, HIGH, 30000UL);
    if (durUs == 0) return -1.0f;        // timeout = no echo received
    // Speed of sound ~343 m/s = 0.0343 cm/us. The echo round-trips, so divide by 2.
    return (float)durUs * 0.0343f * 0.5f;
}

// Device -> Cloud: ping and publish distance every 2 seconds.
ZENO_EVERY(2000)
{
    const float d = measureDistanceCm();
    if (d > 0.0f)    // skip if the measurement timed out
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
    digitalWrite(TRIG_PIN, LOW);   // make sure no stray ping happens at boot

    zeno.wifi(WIFI_SSID, WIFI_PASS)
        .device(DEVICE_ID, DEVICE_TOKEN)
        .enableZKeys()
        .begin();
}

void loop()
{
    zeno.loop();
}

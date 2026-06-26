/**
 * @file 02_pir_motion_detect.ino
 * @brief Publish a motion / no-motion event from an HC-SR501 PIR sensor to Z0.
 *
 * What you'll learn:
 *   - How a PIR motion sensor exposes a simple digital HIGH/LOW signal
 *   - Why PIR modules need a power-on warm-up before they behave correctly
 *   - How to publish only on state change (no spam while motion lingers)
 *
 * Hardware needed:
 *   - Any supported board
 *   - HC-SR501 PIR motion module (5 V supply; output swings 0 / 3.3 V — safe for 3.3 V MCUs)
 *   - Jumper wires, breadboard
 *
 * Wiring:
 *   - PIR VCC -> 5 V
 *   - PIR GND -> GND
 *   - PIR OUT -> PIR_PIN
 *
 * Cloud dashboard setup:
 *   - Create Z0 of type Bool — true = motion detected, false = quiet
 *
 * How to use:
 *   1. Replace WIFI_SSID / WIFI_PASS / DEVICE_ID / DEVICE_TOKEN below.
 *   2. Open Tools > Partition Scheme > "Minimal SPIFFS (1.9MB APP)" (ESP32 only).
 *   3. Flash and open Serial Monitor at 115200 baud.
 *   4. Stand still for ~30 s so the PIR can calibrate, then walk through its cone.
 *   5. Z0 should flip true on motion and back to false after the hold-time elapses.
 *
 * Tips & common mistakes:
 *   - The two trimpots on the HC-SR501 set sensitivity and hold-time. Turn them
 *     fully counter-clockwise for shortest hold while testing.
 *   - The PIR needs roughly 30 s of stable ambient IR after power-on. Spurious
 *     triggers during that window are normal.
 */

#include <ZenoPCBMain.h>

using namespace ZenoPCB;

#define WIFI_SSID "REPLACE_ME"
#define WIFI_PASS "REPLACE_ME"
#define DEVICE_ID "REPLACE_ME"
#define DEVICE_TOKEN "REPLACE_AT_PROVISIONING"

#if defined(ESP32)
  #define PIR_PIN 13
#elif defined(ESP8266)
  #define PIR_PIN 14   // D5
#elif defined(ARDUINO_UNOR4_WIFI)
  #define PIR_PIN 5
#elif defined(STM32F4)
  #define PIR_PIN PB10
#elif defined(STM32F1)
  #define PIR_PIN PB10
#endif

Zeno zeno;
static int s_lastState = LOW;       // PIR idles LOW, so start there

void setup()
{
    Serial.begin(115200);
    // Plain INPUT — the HC-SR501 actively drives its output, no pull-up needed.
    pinMode(PIR_PIN, INPUT);

    zeno.wifi(WIFI_SSID, WIFI_PASS)
        .device(DEVICE_ID, DEVICE_TOKEN)
        .enableZKeys()
        .setZInstantPublish(true)
        .begin();
}

void loop()
{
    const int s = digitalRead(PIR_PIN);
    if (s != s_lastState)                 // only act on a real change
    {
        s_lastState = s;
        const bool motion = (s == HIGH);  // PIR is active-HIGH: HIGH = motion
        DEVICE_TO_CLOUD(Z0, motion);      // Device -> Cloud: send new state
        Serial.printf("[PIR] %s\n", motion ? "MOTION" : "quiet");
    }
    zeno.loop();
}

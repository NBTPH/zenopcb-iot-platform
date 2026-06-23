/**
 * @file 03_button_debounce.ino
 * @brief Software-debounced push button — only publish a press/release after the signal is stable.
 *
 * What you'll learn:
 *   - What "switch bounce" is and why it produces phantom button presses
 *   - The standard non-blocking debounce pattern (millis() based, no delay)
 *   - The difference between the "raw" reading and the "stable" reading
 *
 * Hardware needed:
 *   - Any supported board
 *   - One momentary push button
 *   - Jumper wires, breadboard
 *
 * Wiring:
 *   - BUTTON_PIN -> button -> GND   (internal pull-up handles the resistor)
 *
 * Cloud dashboard setup:
 *   - Create Z1 of type Bool — true = pressed (after debounce), false = released
 *
 * How to use:
 *   1. Replace WIFI_SSID / WIFI_PASS / DEVICE_ID / DEVICE_TOKEN below.
 *   2. Open Tools > Partition Scheme > "Minimal SPIFFS (1.9MB APP)" (ESP32 only).
 *   3. Flash and open Serial Monitor at 115200 baud.
 *   4. Mash the button quickly. Compare with 02_button_read — this version
 *      should give exactly one event per real press.
 *
 * Tips & common mistakes:
 *   - 30 ms is a safe default for tactile switches. Older / dirtier switches
 *     may need 50 ms; bigger arcade buttons can use 10 ms.
 *   - Never use delay() to debounce — it would block WiFi + MQTT.
 */

#include <ZenoPCBMain.h>

using namespace ZenoPCB;

#define WIFI_SSID "REPLACE_ME"
#define WIFI_PASS "REPLACE_ME"
#define DEVICE_ID "REPLACE_ME"
#define DEVICE_TOKEN "REPLACE_AT_PROVISIONING"

#if defined(ESP32)
  #define BUTTON_PIN 0
#elif defined(ESP8266)
  #define BUTTON_PIN 0
#elif defined(ARDUINO_UNOR4_WIFI)
  #define BUTTON_PIN 2
#elif defined(STM32F4)
  #define BUTTON_PIN PC13
#elif defined(STM32F1)
  #define BUTTON_PIN PA0
#endif

Zeno zeno;

static const uint32_t DEBOUNCE_MS = 30;    // ignore bounces shorter than 30 ms
static int s_lastRaw    = HIGH;            // most recent raw read (idles HIGH via pull-up)
static int s_stableRaw  = HIGH;            // last value we treated as "real"
static uint32_t s_lastChangeMs = 0;        // millis() of the last raw change

void setup()
{
    Serial.begin(115200);
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    zeno.wifi(WIFI_SSID, WIFI_PASS)
        .device(DEVICE_ID, DEVICE_TOKEN)
        .enableZKeys()
        .begin();
}

void loop()
{
    const int raw = digitalRead(BUTTON_PIN);
    const uint32_t now = millis();

    // Step 1: any raw change resets the debounce timer.
    if (raw != s_lastRaw)
    {
        s_lastChangeMs = now;
        s_lastRaw      = raw;
    }

    // Step 2: only accept the new value once it has held steady for DEBOUNCE_MS.
    if ((now - s_lastChangeMs) >= DEBOUNCE_MS && raw != s_stableRaw)
    {
        s_stableRaw = raw;
        const bool pressed = (s_stableRaw == LOW);   // active-LOW button
        DEVICE_TO_CLOUD(Z1, pressed);                // Device -> Cloud: send debounced state
        Serial.printf("[Z1] debounced %s\n", pressed ? "PRESSED" : "RELEASED");
    }

    zeno.loop();
}

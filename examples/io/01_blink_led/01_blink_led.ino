/**
 * @file 01_blink_led.ino
 * @brief Blink the on-board LED and mirror its state to the cloud via ZSignal Z0.
 *
 * What you'll learn:
 *   - How to blink an LED without using delay() (so other code keeps running)
 *   - How to publish a boolean to the cloud each time it changes
 *   - Why the same pin behaves differently on different boards (active-LOW LEDs)
 *
 * Hardware needed:
 *   - Any supported board (ESP32 / ESP8266 / UNO R4 WiFi / STM32 F1 / STM32 F4)
 *   - Built-in LED is enough. External LED + 220 Ohm resistor works too.
 *   - Jumper wires, breadboard (only if using an external LED)
 *
 * Wiring:
 *   - LED_PIN -> LED anode -> 220 Ohm -> GND   (skip if your board has an on-board LED)
 *
 * Cloud dashboard setup:
 *   - Create Z0 of type Bool — mirrors the local LED state each blink
 *
 * How to use:
 *   1. Replace WIFI_SSID / WIFI_PASS / DEVICE_ID / DEVICE_TOKEN below.
 *   2. Open Tools > Partition Scheme > "Minimal SPIFFS (1.9MB APP)" (ESP32 only).
 *   3. Flash to the board.
 *   4. Open Serial Monitor at 115200 baud.
 *   5. The LED toggles every 500 ms; watch Z0 flip on the dashboard.
 *
 * Tips & common mistakes:
 *   - Using delay() inside loop() would block the WiFi / MQTT stack. This
 *     sketch uses millis() instead, which is the standard non-blocking pattern.
 *   - On the STM32F1 Blue Pill, the on-board LED is active-LOW (LOW = on).
 */

#include <ZenoPCBMain.h>

using namespace ZenoPCB;

// ============================================
// Credentials — replace before flashing
// ============================================
#define WIFI_SSID "REPLACE_ME"
#define WIFI_PASS "REPLACE_ME"
#define DEVICE_ID "REPLACE_ME"
#define DEVICE_TOKEN "REPLACE_AT_PROVISIONING"

// ============================================
// LED pin per board — picks the on-board LED where there is one.
// ============================================
#if defined(ESP32)
  #define LED_PIN 2                // Built-in LED on most ESP32 dev boards
#elif defined(ESP8266)
  #define LED_PIN LED_BUILTIN
#elif defined(ARDUINO_UNOR4_WIFI)
  #define LED_PIN LED_BUILTIN
#elif defined(STM32F4)
  #define LED_PIN PA5              // Nucleo-F429ZI green user LED
#elif defined(STM32F1)
  #define LED_PIN PC13             // Blue Pill on-board LED — active-LOW
#endif

// ============================================
// Globals
// ============================================
Zeno zeno;

static const uint32_t BLINK_PERIOD_MS = 500;   // half-second per toggle
static uint32_t s_lastToggle = 0;              // millis() of last toggle
static bool s_ledState = false;                // current logical state (true = on)

void setup()
{
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);    // start in a known OFF state

    zeno.wifi(WIFI_SSID, WIFI_PASS)
        .device(DEVICE_ID, DEVICE_TOKEN)
        .enableZKeys()
        .begin();
}

void loop()
{
    const uint32_t now = millis();
    // Non-blocking timer: only act when enough time has passed.
    if (now - s_lastToggle >= BLINK_PERIOD_MS)
    {
        s_lastToggle = now;
        s_ledState   = !s_ledState;

#if defined(STM32F1)
        // Blue Pill LED is active-LOW, so invert the level.
        digitalWrite(LED_PIN, s_ledState ? LOW : HIGH);
#else
        digitalWrite(LED_PIN, s_ledState ? HIGH : LOW);
#endif

        // Device -> Cloud: publish the new boolean to Z0.
        DEVICE_TO_CLOUD(Z0, s_ledState);
    }

    zeno.loop();   // keep WiFi + MQTT alive; required every loop iteration
}

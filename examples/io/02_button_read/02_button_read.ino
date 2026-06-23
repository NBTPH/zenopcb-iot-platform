/**
 * @file 02_button_read.ino
 * @brief Read a push button with INPUT_PULLUP and publish each press/release to Z1.
 *
 * What you'll learn:
 *   - How INPUT_PULLUP wires a button without an external resistor
 *   - Why "active-LOW" means pressed = LOW, released = HIGH
 *   - How to publish only on state change instead of spamming on every loop
 *
 * Hardware needed:
 *   - Any supported board
 *   - One momentary push button (4-pin tactile switch is typical)
 *   - Jumper wires, breadboard
 *
 * Wiring:
 *   - BUTTON_PIN -> one leg of the button
 *   - Other leg of the button -> GND
 *   (INPUT_PULLUP enables the MCU's internal pull-up — no external resistor needed.)
 *
 * Cloud dashboard setup:
 *   - Create Z1 of type Bool — true = pressed, false = released
 *
 * How to use:
 *   1. Replace WIFI_SSID / WIFI_PASS / DEVICE_ID / DEVICE_TOKEN below.
 *   2. Open Tools > Partition Scheme > "Minimal SPIFFS (1.9MB APP)" (ESP32 only).
 *   3. Flash and open Serial Monitor at 115200 baud.
 *   4. Press the button — you should see a PRESSED/RELEASED line, and Z1 should
 *      flip on the dashboard.
 *
 * Tips & common mistakes:
 *   - Without INPUT_PULLUP the pin would "float" and produce random reads.
 *   - This sketch does NOT debounce — a fast press can fire several events.
 *     See 03_button_debounce.ino for the fix.
 */

#include <ZenoPCBMain.h>

using namespace ZenoPCB;

#define WIFI_SSID "REPLACE_ME"
#define WIFI_PASS "REPLACE_ME"
#define DEVICE_ID "REPLACE_ME"
#define DEVICE_TOKEN "REPLACE_AT_PROVISIONING"

// ============================================
// Button pin per board — picks the on-board button where there is one.
// ============================================
#if defined(ESP32)
  #define BUTTON_PIN 0           // BOOT button on most ESP32 dev boards
#elif defined(ESP8266)
  #define BUTTON_PIN 0           // FLASH button on NodeMCU
#elif defined(ARDUINO_UNOR4_WIFI)
  #define BUTTON_PIN 2
#elif defined(STM32F4)
  #define BUTTON_PIN PC13        // Nucleo-F429ZI blue user button
#elif defined(STM32F1)
  #define BUTTON_PIN PA0
#endif

Zeno zeno;

static int s_lastReading = HIGH;   // start in "released" state (HIGH thanks to pull-up)

void setup()
{
    Serial.begin(115200);
    pinMode(BUTTON_PIN, INPUT_PULLUP);   // enable internal pull-up — pin idles HIGH

    zeno.wifi(WIFI_SSID, WIFI_PASS)
        .device(DEVICE_ID, DEVICE_TOKEN)
        .enableZKeys()
        .begin();
}

void loop()
{
    const int reading = digitalRead(BUTTON_PIN);
    // Edge detection: only act when the level actually changes.
    if (reading != s_lastReading)
    {
        s_lastReading = reading;
        const bool pressed = (reading == LOW);   // active-LOW: LOW means pressed
        DEVICE_TO_CLOUD(Z1, pressed);            // Device -> Cloud: send new state
        Serial.printf("[Z1] button %s\n", pressed ? "PRESSED" : "RELEASED");
    }

    zeno.loop();
}

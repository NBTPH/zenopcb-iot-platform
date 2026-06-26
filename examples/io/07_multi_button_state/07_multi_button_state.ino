/**
 * @file 07_multi_button_state.ino
 * @brief Three independent push buttons, each mapped to its own ZSignal (Z1 / Z2 / Z3).
 *
 * What you'll learn:
 *   - How to group repeated hardware (3 buttons) into a small struct + array
 *   - How to detect edges on each button independently in a single loop
 *   - How DEVICE_TO_CLOUD(Zn, ...) requires a literal channel name — hence the switch
 *
 * Hardware needed:
 *   - Any supported board
 *   - 3 momentary push buttons
 *   - Jumper wires, breadboard
 *
 * Wiring:
 *   - Each button: BTN_x_PIN -> button -> GND
 *   (INPUT_PULLUP again — no external resistors needed.)
 *
 * Cloud dashboard setup:
 *   - Create Z1 of type Bool — button A state
 *   - Create Z2 of type Bool — button B state
 *   - Create Z3 of type Bool — button C state
 *
 * How to use:
 *   1. Replace WIFI_SSID / WIFI_PASS / DEVICE_ID / DEVICE_TOKEN below.
 *   2. Open Tools > Partition Scheme > "Minimal SPIFFS (1.9MB APP)" (ESP32 only).
 *   3. Flash and open Serial Monitor at 115200 baud.
 *   4. Press each button — only the matching Z signal should flip.
 *
 * Tips & common mistakes:
 *   - DEVICE_TO_CLOUD is a macro that needs the literal Z name at compile time,
 *     so we can't loop over channels directly — the switch is the canonical pattern.
 *   - Not debounced; see 03_button_debounce.ino if you need clean edges.
 */

#include <ZenoPCBMain.h>

using namespace ZenoPCB;

#define WIFI_SSID "REPLACE_ME"
#define WIFI_PASS "REPLACE_ME"
#define DEVICE_ID "REPLACE_ME"
#define DEVICE_TOKEN "REPLACE_AT_PROVISIONING"

#if defined(ESP32)
  #define BTN_A_PIN 25
  #define BTN_B_PIN 26
  #define BTN_C_PIN 27
#elif defined(ESP8266)
  #define BTN_A_PIN 12  // D6
  #define BTN_B_PIN 13  // D7
  #define BTN_C_PIN 14  // D5
#elif defined(ARDUINO_UNOR4_WIFI)
  #define BTN_A_PIN 2
  #define BTN_B_PIN 3
  #define BTN_C_PIN 4
#elif defined(STM32F4)
  #define BTN_A_PIN PG0
  #define BTN_B_PIN PG1
  #define BTN_C_PIN PG2
#elif defined(STM32F1)
  #define BTN_A_PIN PA0
  #define BTN_B_PIN PA1
  #define BTN_C_PIN PA2
#endif

Zeno zeno;

// One small record per button keeps the loop body short.
struct Btn {
    uint8_t pin;     // GPIO pin
    int     last;    // last reading (HIGH/LOW)
    ZKey    key;     // which ZSignal this button publishes to
};

static Btn s_btns[3] = {
    { BTN_A_PIN, HIGH, ZKey::Z1 },
    { BTN_B_PIN, HIGH, ZKey::Z2 },
    { BTN_C_PIN, HIGH, ZKey::Z3 },
};

void setup()
{
    Serial.begin(115200);
    for (int i = 0; i < 3; ++i)
        pinMode(s_btns[i].pin, INPUT_PULLUP);    // all buttons share the same wiring scheme

    zeno.wifi(WIFI_SSID, WIFI_PASS)
        .device(DEVICE_ID, DEVICE_TOKEN)
        .enableZKeys()
        .setZPublishInterval(500)
        .begin();
}

void loop()
{
    for (int i = 0; i < 3; ++i)
    {
        const int r = digitalRead(s_btns[i].pin);
        if (r != s_btns[i].last)        // edge detected on this button
        {
            s_btns[i].last = r;
            const bool pressed = (r == LOW);   // active-LOW (pull-up)
            // DEVICE_TO_CLOUD needs the literal Zn name, so we switch by stored key.
            switch (s_btns[i].key)
            {
                case ZKey::Z1: DEVICE_TO_CLOUD(Z1, pressed); break;
                case ZKey::Z2: DEVICE_TO_CLOUD(Z2, pressed); break;
                case ZKey::Z3: DEVICE_TO_CLOUD(Z3, pressed); break;
                default: break;
            }
            Serial.printf("[btn %d] %s\n", i, pressed ? "PRESS" : "RELEASE");
        }
    }
    zeno.loop();
}

/**
 * @file 01_relay_single.ino
 * @brief Turn one relay ON or OFF from the cloud dashboard.
 *
 * What you'll learn:
 *   - How a cloud value (Z0) drives a physical pin via CLOUD_TO_DEVICE().
 *   - Why "active-LOW" matters when wiring most cheap relay modules.
 *   - How to echo state back to the cloud so the dashboard stays in sync.
 *
 * Hardware needed:
 *   - Any supported board (ESP32, ESP8266, UNO R4 WiFi, STM32 F4/F1).
 *   - 1-channel 5 V relay module (opto-isolated recommended).
 *   - Jumper wires.
 *
 * Wiring:
 *   - Relay VCC -> 5 V on the board (or 3.3 V if the module supports it).
 *   - Relay GND -> board GND.
 *   - Relay IN  -> RELAY_PIN (see board map below).
 *
 * Cloud dashboard setup:
 *   - Create Z0 of type Bool — toggles the relay ON/OFF.
 *
 * @category Actuation
 * @level Beginner
 *
 * @hardware
 * - Any supported board.
 * - 1-channel 5 V relay module (opto-isolated recommended).
 *
 * @wiring
 * - Relay VCC -> 5 V.
 * - Relay GND -> GND.
 * - Relay IN  -> RELAY_PIN.
 *
 * @usage
 * 1. Replace WIFI_SSID / WIFI_PASS / DEVICE_ID / DEVICE_TOKEN below.
 * 2. Arduino IDE: Tools > Partition Scheme > "Minimal SPIFFS (1.9MB APP)" on ESP32.
 * 3. Flash the board.
 * 4. Open Serial Monitor at 115200 baud.
 * 5. Cloud-write Z0 = true to close the relay (you should hear a click and the
 *    Serial log prints "[Z0] relay ON"). Z0 = false opens it again.
 *
 * Tips & common mistakes:
 *   - Most opto-isolated relay modules are ACTIVE-LOW: writing LOW closes the
 *     relay, writing HIGH opens it. If your module behaves backward, flip the
 *     RELAY_ACTIVE_LOW constant.
 *   - During boot a pin can briefly float HIGH. We drive the pin to the OFF
 *     level BEFORE switching it to OUTPUT, so the relay doesn't glitch on for
 *     a few microseconds at power-up.
 *   - Never switch mains voltage unless your relay module and wiring are
 *     rated for it and you understand the safety implications.
 *
 * @safety
 * This sketch drives a relay. Read CLAUDE.md actuation safety note:
 *   - Always failsafe-init outputs LOW (OFF) in setup().
 *   - Do not latch the relay across boots unless explicitly designed for it.
 */

#include <ZenoPCBMain.h>

using namespace ZenoPCB;

// ---- Credentials ------------------------------------------------------------
// Get DEVICE_ID + DEVICE_TOKEN from the ZenoPCB Cloud after provisioning.
#define WIFI_SSID "REPLACE_ME"
#define WIFI_PASS "REPLACE_ME"
#define DEVICE_ID "REPLACE_ME"
#define DEVICE_TOKEN "REPLACE_AT_PROVISIONING"

// ---- Pin map ----------------------------------------------------------------
// Each board names its GPIOs differently. The block below selects a safe
// general-purpose output pin per supported platform.
#if defined(ESP32)
  #define RELAY_PIN 26
#elif defined(ESP8266)
  #define RELAY_PIN 5    // labelled D1 on NodeMCU/Wemos D1 boards
#elif defined(ARDUINO_UNOR4_WIFI)
  #define RELAY_PIN 7
#elif defined(STM32F4)
  #define RELAY_PIN PE0
#elif defined(STM32F1)
  #define RELAY_PIN PB12
#endif

// Most cheap opto-isolated relay boards are active-LOW: writing LOW turns the
// relay ON. Set to false if your specific module is active-HIGH.
static const bool RELAY_ACTIVE_LOW = true;

Zeno zeno;

// Translate a logical on/off into the correct electrical level for this module.
static inline void writeRelay(bool on)
{
    const int v = RELAY_ACTIVE_LOW ? (on ? LOW : HIGH) : (on ? HIGH : LOW);
    digitalWrite(RELAY_PIN, v);
}

// Cloud -> Device: dashboard writes Z0 (bool) to toggle the relay.
CLOUD_TO_DEVICE(Z0)
{
    const bool on = param.toBool();
    writeRelay(on);
    // Echo the applied state back so the cloud dashboard shows the real
    // current state. Without this echo, the server keeps polling because
    // its cached Z0 stays "unknown" until we publish something.
    DEVICE_TO_CLOUD(Z0, on);
    Serial.printf("[Z0] relay %s\n", on ? "ON" : "OFF");
}

void setup()
{
    Serial.begin(115200);

    // Failsafe boot sequence: drive the pin to the OFF level BEFORE we set
    // pinMode(OUTPUT). Otherwise the pin floats during the mode-switch and
    // can briefly turn the relay on at power-up.
    digitalWrite(RELAY_PIN, RELAY_ACTIVE_LOW ? HIGH : LOW);
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, RELAY_ACTIVE_LOW ? HIGH : LOW);

    zeno.wifi(WIFI_SSID, WIFI_PASS)
        .device(DEVICE_ID, DEVICE_TOKEN)
        .enableZKeys()
        .setZPublishInterval(1000)
        .begin();

    // Seed Z0 with the initial OFF state right after connect. This makes the
    // dashboard show the correct value immediately, instead of "no data yet"
    // until the user first toggles the switch.
    DEVICE_TO_CLOUD(Z0, false);
}

void loop()
{
    zeno.loop();
}

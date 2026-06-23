/**
 * @file 02_relay_4ch.ino
 * @brief Control four independent relays from the cloud using Z0..Z3.
 *
 * What you'll learn:
 *   - How to bind multiple CLOUD_TO_DEVICE handlers (one per channel).
 *   - Using an array of pins for clean per-channel control.
 *   - Bringing up many outputs safely at boot.
 *
 * Hardware needed:
 *   - Any supported board.
 *   - 4-channel 5 V relay module.
 *   - Jumper wires.
 *
 * Wiring:
 *   - Relay VCC -> 5 V on the board.
 *   - Relay GND -> board GND.
 *   - Relay IN1..IN4 -> the pins listed in RELAY_PINS for your board.
 *
 * Cloud dashboard setup:
 *   - Create Z0 of type Bool — channel 1.
 *   - Create Z1 of type Bool — channel 2.
 *   - Create Z2 of type Bool — channel 3.
 *   - Create Z3 of type Bool — channel 4.
 *
 * @category Actuation
 * @level Beginner
 *
 * @hardware
 * - Any supported board.
 * - 4-channel 5 V relay module.
 *
 * @wiring
 * - Relay VCC -> 5 V.
 * - Relay GND -> GND.
 * - Relay IN1..IN4 -> RELAY_PINS array entries.
 *
 * @usage
 * 1. Replace WIFI_SSID / WIFI_PASS / DEVICE_ID / DEVICE_TOKEN below.
 * 2. ESP32 only: Tools > Partition Scheme > "Minimal SPIFFS (1.9MB APP)".
 * 3. Flash the board.
 * 4. Open Serial Monitor at 115200 baud.
 * 5. Cloud-write Z0..Z3 = true/false to drive each channel independently.
 *
 * Tips & common mistakes:
 *   - All four channels init OFF in setup() — see the active-LOW failsafe
 *     pattern from sketch 01_relay_single for the why.
 *   - Cheap 4-channel modules often need an external 5 V supply for the relay
 *     coils — the MCU's 5 V regulator can brown out when more than one or two
 *     coils energize at once.
 *
 * @safety
 * Every channel failsafe-initialised OFF in setup() per CLAUDE.md actuation
 * safety note.
 */

#include <ZenoPCBMain.h>

using namespace ZenoPCB;

// ---- Credentials ------------------------------------------------------------
#define WIFI_SSID "REPLACE_ME"
#define WIFI_PASS "REPLACE_ME"
#define DEVICE_ID "REPLACE_ME"
#define DEVICE_TOKEN "REPLACE_AT_PROVISIONING"

// ---- Pin map: 4 outputs per platform ----------------------------------------
#if defined(ESP32)
  static const uint8_t RELAY_PINS[4] = { 26, 27, 32, 33 };
#elif defined(ESP8266)
  static const uint8_t RELAY_PINS[4] = { 5, 4, 14, 12 }; // labelled D1, D2, D5, D6
#elif defined(ARDUINO_UNOR4_WIFI)
  static const uint8_t RELAY_PINS[4] = { 4, 5, 6, 7 };
#elif defined(STM32F4)
  static const uint8_t RELAY_PINS[4] = { PE0, PE1, PE2, PE3 };
#elif defined(STM32F1)
  static const uint8_t RELAY_PINS[4] = { PB12, PB13, PB14, PB15 };
#endif

// Active-LOW = LOW closes the relay. Flip for active-HIGH modules.
static const bool RELAY_ACTIVE_LOW = true;

Zeno zeno;

// Translate logical on/off into the right electrical level for channel idx.
static inline void writeRelay(uint8_t idx, bool on)
{
    const int v = RELAY_ACTIVE_LOW ? (on ? LOW : HIGH) : (on ? HIGH : LOW);
    digitalWrite(RELAY_PINS[idx], v);
}

// Cloud -> Device: one handler per channel. Each takes a bool from the cloud
// and forwards it to the matching relay index.
CLOUD_TO_DEVICE(Z0) { bool on = param.toBool(); writeRelay(0, on); Serial.printf("[Z0] CH1 %s\n", on ? "ON" : "OFF"); }
CLOUD_TO_DEVICE(Z1) { bool on = param.toBool(); writeRelay(1, on); Serial.printf("[Z1] CH2 %s\n", on ? "ON" : "OFF"); }
CLOUD_TO_DEVICE(Z2) { bool on = param.toBool(); writeRelay(2, on); Serial.printf("[Z2] CH3 %s\n", on ? "ON" : "OFF"); }
CLOUD_TO_DEVICE(Z3) { bool on = param.toBool(); writeRelay(3, on); Serial.printf("[Z3] CH4 %s\n", on ? "ON" : "OFF"); }

void setup()
{
    Serial.begin(115200);

    // Same failsafe-OFF pattern as the 1-channel sketch, applied to all four
    // pins: pre-drive the OFF level, THEN switch to OUTPUT, then re-assert OFF.
    // This avoids a brief "all relays click on" glitch at power-up.
    for (int i = 0; i < 4; ++i)
    {
        digitalWrite(RELAY_PINS[i], RELAY_ACTIVE_LOW ? HIGH : LOW);
        pinMode(RELAY_PINS[i], OUTPUT);
        digitalWrite(RELAY_PINS[i], RELAY_ACTIVE_LOW ? HIGH : LOW);
    }

    zeno.wifi(WIFI_SSID, WIFI_PASS)
        .device(DEVICE_ID, DEVICE_TOKEN)
        .enableZKeys()
        .begin();
}

void loop()
{
    zeno.loop();
}

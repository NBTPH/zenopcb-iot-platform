/**
 * @file 05_solenoid_valve.ino
 * @brief Open a 12 V solenoid valve from the cloud, with a hard 60 s safety
 *        auto-close that runs even if WiFi drops.
 *
 * What you'll learn:
 *   - Driving an inductive load (solenoid coil) safely via a relay.
 *   - Why a flyback diode is non-negotiable on coils.
 *   - Defensive programming: never trust the cloud to send you "OFF" — own
 *     the auto-off locally.
 *
 * Hardware needed:
 *   - Any supported board.
 *   - 1-channel relay rated for the solenoid coil current (typically 5..10 A
 *     inrush on a 12 V irrigation valve).
 *   - 12 V solenoid valve.
 *   - 12 V DC supply for the valve.
 *   - Flyback diode (1N4007 or similar) across the solenoid coil.
 *   - Jumper wires.
 *
 * Wiring:
 *   - Relay IN  -> VALVE_PIN.
 *   - Relay COM -> 12 V +.
 *   - Relay NO  -> solenoid + terminal.
 *   - Solenoid - terminal -> 12 V GND.
 *   - Flyback diode: cathode (striped end) to solenoid +, anode to solenoid -.
 *     Reverse-biased across the coil so it shunts the back-EMF spike when the
 *     coil de-energizes — without it, the spike can fry the relay contacts or
 *     anything else on the same supply.
 *
 * Cloud dashboard setup:
 *   - Create Z0 of type Bool — true opens the valve, false closes it.
 *
 * @category Actuation
 * @level Intermediate
 *
 * @hardware
 * - Any supported board.
 * - 1-channel relay (rated for solenoid current — typically 5..10 A coil
 *   inrush on a 12 V irrigation solenoid).
 * - 12 V solenoid valve + external 12 V supply.
 * - **Flyback diode (1N4007 or similar)** across the solenoid coil.
 *
 * @wiring
 * - Relay IN  -> VALVE_PIN.
 * - Relay COM -> 12 V +.
 * - Relay NO  -> solenoid + terminal.
 * - Solenoid - terminal -> 12 V GND.
 * - Flyback diode reverse-biased across solenoid coil terminals.
 *
 * @usage
 * 1. Replace WIFI_SSID / WIFI_PASS / DEVICE_ID / DEVICE_TOKEN below.
 * 2. ESP32 only: Tools > Partition Scheme > "Minimal SPIFFS (1.9MB APP)".
 * 3. Flash the board.
 * 4. Open Serial Monitor at 115200 baud.
 * 5. Cloud-write Z0 = true to open the valve, false to close it. After
 *    MAX_ON_MS the valve auto-closes even if Z0 is still true.
 *
 * Tips & common mistakes:
 *   - The flyback diode is mandatory on inductive loads. Skipping it is the
 *     #1 cause of "the relay worked for a day, then died" in DIY irrigation.
 *   - Prefer a mechanical normally-closed valve over normally-open: if the
 *     board loses power, the valve closes on its own.
 *   - The 60 s ceiling here is conservative for irrigation. Tune MAX_ON_MS to
 *     match the longest legitimate run you ever want; keep it short enough
 *     that a runaway valve can't flood the area.
 *
 * @safety
 * - **Auto-off after MAX_ON_MS = 60 s** even if cloud Z0 stays true.
 *   Prevents a stuck valve flooding the irrigation zone if the cloud
 *   connection drops mid-cycle.
 * - Cloud writes false to Z0 cancel the auto-off and close the valve
 *   immediately.
 * - CLAUDE.md actuation safety: explicitly LOW-initialise the relay BEFORE
 *   setting pinMode OUTPUT, and document that the OSS user is responsible
 *   for fail-safe hardware (mechanical normally-closed valve preferred).
 */

#include <ZenoPCBMain.h>

using namespace ZenoPCB;

// ---- Credentials ------------------------------------------------------------
#define WIFI_SSID "REPLACE_ME"
#define WIFI_PASS "REPLACE_ME"
#define DEVICE_ID "REPLACE_ME"
#define DEVICE_TOKEN "REPLACE_AT_PROVISIONING"

// ---- Pin map ----------------------------------------------------------------
#if defined(ESP32)
  #define VALVE_PIN 26
#elif defined(ESP8266)
  #define VALVE_PIN 5   // D1
#elif defined(ARDUINO_UNOR4_WIFI)
  #define VALVE_PIN 7
#elif defined(STM32F4)
  #define VALVE_PIN PE0
#elif defined(STM32F1)
  #define VALVE_PIN PB12
#endif

// Most cheap relay modules are active-LOW. Flip for active-HIGH boards.
static const bool VALVE_ACTIVE_LOW = true;

// Hard safety ceiling: the valve auto-closes after this many ms regardless
// of cloud state. Protects against runaway flooding if WiFi/MQTT drops
// after the cloud sent OPEN but before it ever sends CLOSE.
static const uint32_t MAX_ON_MS = 60000UL; // 60 seconds

Zeno zeno;
static bool     s_valveOn       = false;
static uint32_t s_valveStartMs  = 0;

static void writeValve(bool on)
{
    const int v = VALVE_ACTIVE_LOW ? (on ? LOW : HIGH) : (on ? HIGH : LOW);
    digitalWrite(VALVE_PIN, v);
    s_valveOn      = on;
    // Capture the moment the valve opened so the safety timer can measure
    // elapsed time. Reset to 0 when closing so a stale timestamp can't trip.
    s_valveStartMs = on ? millis() : 0;
    // Echo state back so the dashboard always shows the real position,
    // including AFTER a safety auto-close fires.
    DEVICE_TO_CLOUD(Z0, on);
    Serial.printf("[VALVE] %s\n", on ? "OPEN" : "CLOSED");
}

// Cloud -> Device: dashboard writes Z0 (bool) to open/close the valve.
CLOUD_TO_DEVICE(Z0)
{
    writeValve(param.toBool());
}

void setup()
{
    Serial.begin(115200);
    // Failsafe-OFF dance: drive OFF level before mode-switch, then again
    // after, so the relay never glitches open at power-up (mandatory for
    // anything wired to a water/irrigation valve).
    digitalWrite(VALVE_PIN, VALVE_ACTIVE_LOW ? HIGH : LOW);
    pinMode(VALVE_PIN, OUTPUT);
    digitalWrite(VALVE_PIN, VALVE_ACTIVE_LOW ? HIGH : LOW);

    zeno.wifi(WIFI_SSID, WIFI_PASS)
        .device(DEVICE_ID, DEVICE_TOKEN)
        .enableZKeys()
        .setZPublishInterval(1000)
        .begin();
}

void loop()
{
    // Safety auto-off — runs locally, no cloud round-trip required. This is
    // the whole point of this sketch: even if WiFi or the broker dies right
    // after we received OPEN, the valve still closes on time.
    if (s_valveOn && (millis() - s_valveStartMs >= MAX_ON_MS))
    {
        Serial.printf("[VALVE] safety auto-off after %lu ms\n",
                       (unsigned long)MAX_ON_MS);
        writeValve(false);
    }
    zeno.loop();
}

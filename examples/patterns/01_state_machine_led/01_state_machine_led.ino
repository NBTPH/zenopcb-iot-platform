/**
 * @file 01_state_machine_led.ino
 * @brief Three-state machine (IDLE / RUNNING / PAUSED) driven by a single
 *        cloud value (Z0), with one LED per state and the active state name
 *        reported back to the cloud (Z1).
 *
 * What you'll learn:
 *   - Modeling device behavior as an explicit Finite State Machine (FSM).
 *   - Using an enum class instead of magic numbers for state.
 *   - Centralizing transitions in one function so logging + I/O stay in sync.
 *   - The "one LED per state" pattern (only one lit at a time).
 *
 * Hardware needed:
 *   - Any supported board (works on F103 — no cloud subsystem dependencies).
 *   - 3 LEDs (or 1 RGB) on LED_IDLE / LED_RUN / LED_PAUSE. On-board single
 *     LED + Serial log also works for first-time learners.
 *
 * Wiring:
 *   - Each LED: anode -> board pin via 220 ohm resistor, cathode -> GND.
 *
 * Cloud dashboard setup:
 *   - Create Z0 of type Int — controls the state (0 = IDLE, 1 = RUNNING, 2 = PAUSED).
 *   - Create Z1 of type String — reports the current state name after each
 *     transition (one-shot per transition, not periodic).
 *
 * @category Patterns
 * @level Intermediate
 *
 * @hardware
 * - Any supported board.
 * - 3 LEDs (or single RGB) on LED_IDLE / LED_RUN / LED_PAUSE. On-board
 *   single LED + Serial logging also works for first-time learners.
 *
 * @platform_notes
 * Pure local state machine — works on every supported port including F103.
 *
 * @usage
 * 1. Replace WIFI_SSID / WIFI_PASS / DEVICE_ID / DEVICE_TOKEN below.
 * 2. ESP32 only: Tools > Partition Scheme > "Minimal SPIFFS (1.9MB APP)".
 * 3. Flash the board.
 * 4. Open Serial Monitor at 115200 baud.
 * 5. Cloud-write Z0 to transition the FSM:
 *      Z0 = 0 -> IDLE.
 *      Z0 = 1 -> RUNNING.
 *      Z0 = 2 -> PAUSED.
 *    Z1 reports the new state name back to the cloud after each transition.
 *
 * Tips & common mistakes:
 *   - Values outside 0..2 are silently ignored — an FSM should never enter
 *     an undefined state.
 *   - Writing the same Z0 twice is a no-op (transition() short-circuits if
 *     the target state already matches). This avoids redundant log spam.
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
  #define LED_IDLE 25
  #define LED_RUN  26
  #define LED_PAUSE 27
#elif defined(ESP8266)
  #define LED_IDLE 5
  #define LED_RUN  4
  #define LED_PAUSE 14
#elif defined(ARDUINO_UNOR4_WIFI)
  #define LED_IDLE 4
  #define LED_RUN  5
  #define LED_PAUSE 6
#elif defined(STM32F4)
  #define LED_IDLE PB0
  #define LED_RUN  PB7
  #define LED_PAUSE PB14
#elif defined(STM32F1)
  #define LED_IDLE PB12
  #define LED_RUN  PB13
  #define LED_PAUSE PB14
#endif

// Strong-typed states: prevents accidentally passing a plain int where a
// State is expected, and makes the source readable.
enum class State : uint8_t { IDLE = 0, RUNNING = 1, PAUSED = 2 };

Zeno  zeno;
static State s_state = State::IDLE;

static const char* stateName(State s)
{
    switch (s)
    {
        case State::IDLE:    return "IDLE";
        case State::RUNNING: return "RUNNING";
        case State::PAUSED:  return "PAUSED";
    }
    return "?";
}

// Drive the three LEDs so exactly ONE matches the current state.
static void applyStateLeds(State s)
{
    digitalWrite(LED_IDLE,  s == State::IDLE   ? HIGH : LOW);
    digitalWrite(LED_RUN,   s == State::RUNNING ? HIGH : LOW);
    digitalWrite(LED_PAUSE, s == State::PAUSED  ? HIGH : LOW);
}

// All transitions go through here — single funnel guarantees the LEDs, the
// log, and the cloud echo always agree on the current state.
static void transition(State next)
{
    // No-op if already in target state — keeps cloud + log quiet on repeats.
    if (next == s_state) return;
    Serial.printf("[FSM] %s -> %s\n", stateName(s_state), stateName(next));
    s_state = next;
    applyStateLeds(s_state);
    // Device -> Cloud: report the new state name to Z1 (one-shot per transition).
    DEVICE_TO_CLOUD(Z1, String(stateName(s_state)));
}

// Cloud -> Device: dashboard writes an int 0/1/2 to Z0 to request a transition.
CLOUD_TO_DEVICE(Z0)
{
    const long v = param.toInt();
    // Bounds check — anything outside 0..2 is silently ignored so a typo on
    // the dashboard can't push the FSM into an undefined state.
    if (v < 0 || v > 2) return;
    transition(static_cast<State>(v));
}

void setup()
{
    Serial.begin(115200);
    pinMode(LED_IDLE, OUTPUT);
    pinMode(LED_RUN,  OUTPUT);
    pinMode(LED_PAUSE, OUTPUT);
    // Show the initial state (IDLE) on the LEDs at boot.
    applyStateLeds(s_state);

    zeno.wifi(WIFI_SSID, WIFI_PASS)
        .device(DEVICE_ID, DEVICE_TOKEN)
        .enableZKeys()
        .begin();
}

void loop()
{
    zeno.loop();
}

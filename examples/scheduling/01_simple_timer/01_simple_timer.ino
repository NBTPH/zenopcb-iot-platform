/**
 * @file 01_simple_timer.ino
 * @brief Local "every 5 seconds" timer — toggles the LED and publishes the
 *        boolean state as a heartbeat on Z0.
 *
 * What you'll learn:
 *   - The simplest scheduling primitive: a millis()-based timer in loop().
 *   - Why this runs on EVERY board, even tiny ones with no cloud Schedule.
 *   - The difference between LOCAL scheduling (this sketch) and CLOUD-driven
 *     scheduling (see 02_daily_schedule).
 *
 * Hardware needed:
 *   - Any supported board (including STM32F103 Blue Pill).
 *   - On-board LED (no external parts required).
 *
 * Wiring:
 *   - None — uses the board's built-in LED.
 *
 * Cloud dashboard setup:
 *   - Create Z0 of type Bool — just for visualizing the heartbeat (optional).
 *
 * @category Scheduling
 * @level Beginner
 *
 * @hardware
 * - Any supported board.
 * - Built-in LED on LED_PIN.
 *
 * @platform_notes
 * STM32F103 Blue Pill uses ZENOPCB_MICRO_BASIC profile — `enableSchedule()`
 * is compile-stripped on F103. This sketch does NOT call enableSchedule()
 * so it runs identically on every supported port. The "timer" here is a
 * plain millis() comparison in loop(), which is the lightest scheduling
 * primitive the maker needs.
 *
 * @usage
 * 1. Replace WIFI_SSID / WIFI_PASS / DEVICE_ID / DEVICE_TOKEN below.
 * 2. ESP32 only: Tools > Partition Scheme > "Minimal SPIFFS (1.9MB APP)".
 * 3. Flash the board.
 * 4. Open Serial Monitor at 115200 baud.
 * 5. The LED toggles every 5 s; Z0 publishes the boolean state at the same
 *    beat. Compare with sketch 02_daily_schedule for a CLOUD-driven schedule.
 *
 * Tips & common mistakes:
 *   - Always use `now - s_lastFireMs >= PERIOD` (subtraction), not
 *     `now >= s_lastFireMs + PERIOD` (addition). The subtraction handles the
 *     millis() rollover at ~49 days correctly.
 *   - Don't use delay() to time things — it blocks zeno.loop() and you'll
 *     lose MQTT messages.
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
  #define LED_PIN 2
#elif defined(ESP8266)
  #define LED_PIN LED_BUILTIN
#elif defined(ARDUINO_UNOR4_WIFI)
  #define LED_PIN LED_BUILTIN
#elif defined(STM32F4)
  #define LED_PIN PA5
#elif defined(STM32F1)
  #define LED_PIN PC13
#endif

static const uint32_t TIMER_PERIOD_MS = 5000; // toggle every 5 seconds
static uint32_t s_lastFireMs = 0;
static bool     s_ledState   = false;

Zeno zeno;

// Wrap digitalWrite to absorb the F103 "LED is active LOW" quirk so the
// rest of the code can speak in logical on/off.
static inline void writeLed(bool on)
{
#if defined(STM32F1)
    // Blue Pill onboard LED (PC13) is wired to VCC — LOW lights it up.
    digitalWrite(LED_PIN, on ? LOW : HIGH);
#else
    digitalWrite(LED_PIN, on ? HIGH : LOW);
#endif
}

void setup()
{
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);
    writeLed(false);

    zeno.wifi(WIFI_SSID, WIFI_PASS)
        .device(DEVICE_ID, DEVICE_TOKEN)
        .enableZKeys()
        .begin();
}

void loop()
{
    // Classic "non-blocking interval" pattern. The subtraction form is
    // rollover-safe — see the Tips section in the header.
    const uint32_t now = millis();
    if (now - s_lastFireMs >= TIMER_PERIOD_MS)
    {
        s_lastFireMs = now;
        s_ledState   = !s_ledState;
        writeLed(s_ledState);
        // Device -> Cloud: heartbeat the new state to Z0 every tick.
        DEVICE_TO_CLOUD(Z0, s_ledState);
        Serial.printf("[Timer] tick, LED %s\n", s_ledState ? "ON" : "OFF");
    }
    zeno.loop();
}

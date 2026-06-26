/**
 * @file 03_cron_pattern.ino
 * @brief Cron-style cloud schedule — fires every 15 minutes on weekdays,
 *        pulses an output for 2 s, and counts firings on Z0.
 *
 * What you'll learn:
 *   - Driving an output from a CRON-style cloud schedule.
 *   - Pulse pattern: turn ON, remember when, turn OFF later in loop() without
 *     using delay().
 *   - Counting events and publishing the running tally to a cloud key.
 *
 * Hardware needed:
 *   - Any supported board EXCEPT STM32F103.
 *   - LED or relay on OUTPUT_PIN (optional — Serial output alone is enough to
 *     watch the cron fire).
 *
 * Wiring:
 *   - LED + 220 ohm resistor on OUTPUT_PIN, or relay IN -> OUTPUT_PIN.
 *
 * Cloud dashboard setup:
 *   - Create Z0 of type Int — running fire count.
 *   - Create Z1 of type Bool — pulse on/off indicator.
 *   - In the Schedule editor, create a CRON schedule (Mon-Fri, every 15 min).
 *     The exact cron syntax depends on the dashboard editor; this sketch
 *     only relies on the ScheduleExecuted callback firing.
 *
 * @category Scheduling
 * @level Intermediate
 *
 * @hardware
 * - Any supported board EXCEPT STM32F103.
 * - LED or relay on OUTPUT_PIN (optional — sketch is also useful for testing
 *   the cron pattern in Serial output alone).
 *
 * @platform_notes
 * F103 NOT SUPPORTED — Schedule is compile-stripped under MICRO_BASIC.
 * See sketch 01_simple_timer for an F103-friendly local timer.
 *
 * @usage
 * 1. Replace WIFI_SSID / WIFI_PASS / DEVICE_ID / DEVICE_TOKEN below.
 * 2. ESP32 only: Tools > Partition Scheme > "Minimal SPIFFS (1.9MB APP)".
 * 3. Create the CRON schedule on the cloud dashboard.
 * 4. Flash the board.
 * 5. Open Serial Monitor at 115200 baud.
 * 6. Each fire pulses OUTPUT_PIN for 2 s, increments Z0, and toggles Z1.
 *
 * Tips & common mistakes:
 *   - Don't use delay(2000) for the pulse — it would block zeno.loop() and
 *     drop MQTT keepalives. The millis()-based pattern below is the right
 *     way to do "ON now, OFF later".
 *   - If the schedule doesn't fire, verify the rule is enabled on the
 *     dashboard and your timezone is set correctly.
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
  #define OUTPUT_PIN 26
#elif defined(ESP8266)
  #define OUTPUT_PIN 5
#elif defined(ARDUINO_UNOR4_WIFI)
  #define OUTPUT_PIN 7
#elif defined(STM32F4)
  #define OUTPUT_PIN PE0
#elif defined(STM32F1)
  #define OUTPUT_PIN PB12 // unused; F103 early-returns in setup()
#endif

Zeno     zeno;
static uint32_t s_fireCount    = 0;
static uint32_t s_pulseStartMs = 0;
static bool     s_pulseActive  = false;
static const uint32_t PULSE_MS = 2000; // pulse output HIGH for 2 seconds

static void startPulse()
{
    s_pulseActive  = true;
    s_pulseStartMs = millis();
    digitalWrite(OUTPUT_PIN, HIGH);
}

// Schedule callback — fires on each cron tick (see file 02 for parameter
// details).
void onScheduleExecuted(const String &id,
                        ExecutionStatus status,
                        int64_t value,
                        const String &error)
{
    if (status != ExecutionStatus::SUCCESS)
    {
        Serial.printf("[Cron] %s FAIL: %s\n", id.c_str(), error.c_str());
        return;
    }
    ++s_fireCount;
    Serial.printf("[Cron] %s fire #%lu (value=%lld)\n",
                   id.c_str(), (unsigned long)s_fireCount, (long long)value);
    // Device -> Cloud: publish the new count + start the pulse indicator.
    DEVICE_TO_CLOUD(Z0, (int32_t)s_fireCount);
    DEVICE_TO_CLOUD(Z1, true);
    startPulse();
}

void setup()
{
    Serial.begin(115200);

#if defined(STM32F1)
    Serial.println(F("[INFO] cron Schedule not available on STM32F103. "
                     "See basics/05_scheduling/01_simple_timer."));
    return;
#endif

    pinMode(OUTPUT_PIN, OUTPUT);
    digitalWrite(OUTPUT_PIN, LOW);

    zeno.wifi(WIFI_SSID, WIFI_PASS)
        .device(DEVICE_ID, DEVICE_TOKEN)
        .enableZKeys()
        .setZInstantPublish(true)
        .enableSchedule()
        .onScheduleExecuted(onScheduleExecuted)
        .begin();
}

void loop()
{
#if defined(STM32F1)
    return;
#else
    // End the pulse once PULSE_MS has elapsed since startPulse(). Tells the
    // dashboard the pulse is over by writing Z1 back to false.
    if (s_pulseActive && (millis() - s_pulseStartMs >= PULSE_MS))
    {
        s_pulseActive = false;
        digitalWrite(OUTPUT_PIN, LOW);
        DEVICE_TO_CLOUD(Z1, false);
    }
    zeno.loop();
#endif
}

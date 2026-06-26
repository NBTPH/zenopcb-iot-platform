/**
 * @file 02_daily_schedule.ino
 * @brief Cloud-driven daily schedule — relay ON at 06:00, OFF at 18:00.
 *
 * What you'll learn:
 *   - How the cloud Schedule subsystem fires a callback on the device at
 *     a specific time-of-day.
 *   - The event model: what's in `id`, `status`, `value`, `error`.
 *   - Why schedules keep working even if WiFi briefly drops mid-day.
 *
 * Hardware needed:
 *   - Any supported board EXCEPT STM32F103 Blue Pill.
 *   - 1-channel relay (or just an LED + 220 ohm resistor for testing).
 *
 * Wiring:
 *   - Relay VCC -> 5 V; Relay GND -> board GND; Relay IN -> RELAY_PIN.
 *   - Or: LED + resistor on RELAY_PIN if you're just testing the timing.
 *
 * Cloud dashboard setup:
 *   - Create Z0 of type Int — reflects current relay state (0/1).
 *   - In the Schedule editor, create two daily schedules:
 *       06:00 every day -> value = 1 (turn relay ON).
 *       18:00 every day -> value = 0 (turn relay OFF).
 *     Schedules sync to the device on first MQTT connect and the firmware
 *     evaluates them locally — they fire even if the internet drops later.
 *
 * @category Scheduling
 * @level Intermediate
 *
 * @hardware
 * - Any supported board EXCEPT STM32F103 Blue Pill.
 * - 1-channel relay (or LED + resistor for testing) on RELAY_PIN.
 *
 * @platform_notes
 * **STM32 F103 (MICRO_BASIC profile) NOT SUPPORTED** — Schedule subsystem is
 * compile-stripped to fit the 64KB Flash budget per Plan 07-06.6. On F103
 * this sketch falls through to a Serial.print warning + early return in
 * setup() so it still cross-compiles cleanly. Use 01_simple_timer or build
 * your own millis-based timer on F103.
 *
 * @usage
 * 1. Replace WIFI_SSID / WIFI_PASS / DEVICE_ID / DEVICE_TOKEN below.
 * 2. ESP32 only: Tools > Partition Scheme > "Minimal SPIFFS (1.9MB APP)".
 * 3. Create both daily schedules on the cloud dashboard (see above).
 * 4. Flash the board.
 * 5. Open Serial Monitor at 115200 baud.
 * 6. Watch the Serial log around 06:00 / 18:00 — the schedule callback fires
 *    and the relay flips. Z0 echoes back the current state.
 *
 * Tips & common mistakes:
 *   - The device uses the time pushed by the cloud (it does not need an RTC).
 *     Make sure your DEVICE_ID's timezone is configured correctly on the
 *     dashboard or the schedule will fire at the wrong wall-clock time.
 *   - Schedules are cached on the device, so once they sync they fire even
 *     during a temporary internet outage.
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
  #define RELAY_PIN 26
#elif defined(ESP8266)
  #define RELAY_PIN 5
#elif defined(ARDUINO_UNOR4_WIFI)
  #define RELAY_PIN 7
#elif defined(STM32F4)
  #define RELAY_PIN PE0
#elif defined(STM32F1)
  #define RELAY_PIN PB12 // unused on F103 (see early-return in setup())
#endif

Zeno zeno;
static bool s_relayState = false;

static void setRelay(bool on)
{
    s_relayState = on;
    digitalWrite(RELAY_PIN, on ? HIGH : LOW);
    // Echo state back so the dashboard reflects reality, even if the
    // schedule fired hours after the last user interaction.
    DEVICE_TO_CLOUD(Z0, (int32_t)(on ? 1 : 0));
    Serial.printf("[Relay] %s\n", on ? "ON" : "OFF");
}

// Schedule callback — fires when one of YOUR cloud-defined schedules hits.
// Parameters:
//   id     = the schedule's ID from the dashboard.
//   status = SUCCESS or a failure code (e.g. expired, invalid).
//   value  = the int payload you configured for the schedule (here 0 or 1).
//   error  = human-readable failure reason (empty on SUCCESS).
// What to DO here: branch on the value/id and drive your hardware.
void onScheduleExecuted(const String &id,
                        ExecutionStatus status,
                        int64_t value,
                        const String &error)
{
    if (status == ExecutionStatus::SUCCESS)
    {
        Serial.printf("[Schedule] %s fired value=%lld\n",
                       id.c_str(), (long long)value);
        setRelay(value != 0);
    }
    else
    {
        Serial.printf("[Schedule] %s FAILED: %s\n",
                       id.c_str(), error.c_str());
    }
}

void setup()
{
    Serial.begin(115200);

#if defined(STM32F1)
    // Blue Pill has no Schedule engine in the MICRO_BASIC profile.
    Serial.println(F("[INFO] Schedule not available on STM32F103 (MICRO_BASIC). "
                     "Use basics/05_scheduling/01_simple_timer instead."));
    return; // skip Zeno init; loop() is a no-op
#endif

    pinMode(RELAY_PIN, OUTPUT);
    setRelay(false); // failsafe-OFF at boot

    zeno.wifi(WIFI_SSID, WIFI_PASS)
        .device(DEVICE_ID, DEVICE_TOKEN)
        .enableZKeys()
        .setZPublishInterval(1000)
        .enableSchedule()                          // turn on schedule engine
        .onScheduleExecuted(onScheduleExecuted)    // wire our callback
        .begin();
}

void loop()
{
#if defined(STM32F1)
    return;
#else
    zeno.loop();
#endif
}

/**
 * @file 01_threshold_high.ino
 * @brief Light an LED when a cloud-side alarm rule on Z2 trips ("Z2 > 80").
 *
 * What you'll learn:
 *   - How the cloud Alarm subsystem fires a callback on the device.
 *   - The event model: what's in `ruleId`, `key`, `value`, `severity`.
 *   - How to drive a local indicator from an alarm event.
 *   - Why some sketches early-return on STM32F103 (limited Flash).
 *
 * Hardware needed:
 *   - Any supported board EXCEPT STM32F103 (the Alarm engine is stripped
 *     from the MICRO_BASIC build profile on F103 to fit Flash).
 *   - Potentiometer (or LDR) wired between 3V3, GND and SENSOR_PIN.
 *   - LED on LED_PIN (built-in LED works).
 *
 * Wiring:
 *   - Pot: ends to 3V3 and GND, wiper to SENSOR_PIN.
 *   - LED + 220 ohm resistor to LED_PIN (skip if using the built-in LED).
 *
 * Cloud dashboard setup:
 *   - Create Z2 of type Float — sensor reading in percent (0..100).
 *   - In the Alarm rule editor, create a rule on key Z2 with condition `> 80`.
 *     The cloud evaluates this rule on every Z2 telemetry update and pushes
 *     a trigger back to this device when it matches.
 *
 * @category Alarm
 * @level Beginner
 *
 * @hardware
 * - Any supported board EXCEPT STM32F103 (alarm subsystem stripped under
 *   MICRO_BASIC).
 * - Pot or LDR on SENSOR_PIN.
 * - LED on LED_PIN (built-in is fine).
 *
 * @platform_notes
 * F103 NOT SUPPORTED — Alarm subsystem is compile-stripped in MICRO_BASIC.
 * Sketch early-returns on F103 with a Serial notice.
 *
 * @usage
 * 1. Replace WIFI_SSID / WIFI_PASS / DEVICE_ID / DEVICE_TOKEN below.
 * 2. ESP32 only: Tools > Partition Scheme > "Minimal SPIFFS (1.9MB APP)".
 * 3. On the cloud dashboard, create the Z2 key + alarm rule (see above).
 * 4. Flash the board.
 * 5. Open Serial Monitor at 115200 baud.
 * 6. Turn the pot up past 80% — within a few seconds the LED lights and the
 *    Serial log prints "[ALARM] ...". The LED auto-clears 5 s later.
 *
 * Tips & common mistakes:
 *   - The alarm rule lives on the CLOUD, not in this sketch. If nothing
 *     fires, double-check the rule exists and is enabled in the dashboard.
 *   - The 5 s "lit" window is a UX trick — without it, brief alarms would
 *     flash the LED too fast to notice.
 */

#include <ZenoPCBMain.h>

using namespace ZenoPCB;

// ---- Credentials ------------------------------------------------------------
#define WIFI_SSID "REPLACE_ME"
#define WIFI_PASS "REPLACE_ME"
#define DEVICE_ID "REPLACE_ME"
#define DEVICE_TOKEN "REPLACE_AT_PROVISIONING"

// ---- Pin map + ADC scale ----------------------------------------------------
// Different boards have different ADC bit depths. ADC_FULL is the max raw
// reading, used to normalize to 0..100% for the cloud key.
#if defined(ESP32)
  #define SENSOR_PIN 34
  #define LED_PIN 2
  #define ADC_FULL 4095.0f   // 12-bit ADC
#elif defined(ESP8266)
  #define SENSOR_PIN A0
  #define LED_PIN LED_BUILTIN
  #define ADC_FULL 1023.0f   // 10-bit ADC
#elif defined(ARDUINO_UNOR4_WIFI)
  #define SENSOR_PIN A0
  #define LED_PIN LED_BUILTIN
  #define ADC_FULL 16383.0f  // 14-bit ADC by default on UNO R4
#elif defined(STM32F4)
  #define SENSOR_PIN PA0
  #define LED_PIN PA5
  #define ADC_FULL 4095.0f   // 12-bit ADC
#elif defined(STM32F1)
  #define SENSOR_PIN PA0
  #define LED_PIN PC13
  #define ADC_FULL 4095.0f
#endif

Zeno zeno;
// Timestamp at which the alarm LED should be turned back off.
// 0 means "no active lit window".
static uint32_t s_alarmLitUntilMs = 0;
static const uint32_t LIT_MS = 5000; // hold the LED on for 5 s after a trip

// Device -> Cloud: publish the sensor reading every 3 seconds so the cloud
// can evaluate its alarm rule against it.
ZENO_EVERY(3000)
{
    const float pct = (float)analogRead(SENSOR_PIN) / ADC_FULL * 100.0f;
    DEVICE_TO_CLOUD(Z2, pct);
}

// Alarm callback — fires when the cloud-side rule matches a telemetry value.
// Parameters:
//   ruleId   = the rule ID you set on the dashboard (use it to disambiguate
//              when you have multiple rules — see sketch 02_threshold_range).
//   key      = the ZKey that crossed the threshold (here always "Z2").
//   value    = the actual value that tripped the rule.
//   severity = 0..N from the rule definition (higher = more urgent).
// What to DO here: drive your local indicator/actuator; the cloud has
// already done the threshold math for you.
void onAlarmTriggered(const String &ruleId, const String &key,
                      double value, uint8_t severity)
{
    Serial.printf("[ALARM] %s %s=%.2f sev=%u\n",
                   ruleId.c_str(), key.c_str(), value, (unsigned)severity);
    digitalWrite(LED_PIN, HIGH);
    s_alarmLitUntilMs = millis() + LIT_MS;
}

void setup()
{
    Serial.begin(115200);

#if defined(STM32F1)
    // Blue Pill ships in MICRO_BASIC profile — the Alarm engine is excluded
    // to fit in 64 KB of Flash. Skip the rest so the sketch still compiles
    // cleanly across all boards.
    Serial.println(F("[INFO] Alarm not available on STM32F103 (MICRO_BASIC)."));
    return;
#endif

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    zeno.wifi(WIFI_SSID, WIFI_PASS)
        .device(DEVICE_ID, DEVICE_TOKEN)
        .enableZKeys()
        .enableAlarm()                       // turn on alarm subsystem
        .onAlarmTriggered(onAlarmTriggered)  // wire our callback
        .begin();
}

void loop()
{
#if defined(STM32F1)
    return;
#else
    // Auto-clear the LED after LIT_MS so a single trip flashes briefly
    // instead of latching forever.
    if (s_alarmLitUntilMs != 0 && millis() >= s_alarmLitUntilMs)
    {
        s_alarmLitUntilMs = 0;
        digitalWrite(LED_PIN, LOW);
    }
    zeno.loop();
#endif
}

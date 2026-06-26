/**
 * @file 03_buzzer_alarm.ino
 * @brief Sound an active buzzer for 3 s when a cloud alarm rule on Z2 trips.
 *
 * What you'll learn:
 *   - Driving an audible alert (active buzzer) from an alarm callback.
 *   - Using a millis() timer to auto-silence a pulsed actuator without delay().
 *   - The difference between "active" and "passive" buzzers (see Tips).
 *
 * Hardware needed:
 *   - Any supported board EXCEPT STM32F103.
 *   - Active buzzer (3.3 V or 5 V) on BUZZER_PIN.
 *   - Potentiometer or LDR on SENSOR_PIN to drive Z2.
 *
 * Wiring:
 *   - Buzzer + -> BUZZER_PIN.
 *   - Buzzer - -> GND.
 *   - Pot wiper -> SENSOR_PIN; pot ends -> 3V3 and GND.
 *
 * Cloud dashboard setup:
 *   - Create Z2 of type Float — sensor reading (0..100%).
 *   - Create an alarm rule on Z2 (e.g. condition `> 70`).
 *
 * @category Alarm
 * @level Beginner
 *
 * @hardware
 * - Any supported board EXCEPT STM32F103.
 * - Active buzzer (3.3 V or 5 V) on BUZZER_PIN.
 * - Pot or LDR on SENSOR_PIN (or any analog source to drive Z2).
 *
 * @platform_notes
 * F103 NOT SUPPORTED (Alarm subsystem stripped under MICRO_BASIC).
 *
 * @usage
 * 1. Replace WIFI_SSID / WIFI_PASS / DEVICE_ID / DEVICE_TOKEN below.
 * 2. ESP32 only: Tools > Partition Scheme > "Minimal SPIFFS (1.9MB APP)".
 * 3. Create the Z2 key + alarm rule on the cloud dashboard.
 * 4. Flash the board.
 * 5. Open Serial Monitor at 115200 baud.
 * 6. When the rule trips, the buzzer sounds for 3 s and then auto-silences.
 *
 * Tips & common mistakes:
 *   - This sketch assumes an ACTIVE buzzer (one that beeps when you just
 *     give it DC). A PASSIVE buzzer needs a square wave (use tone() instead
 *     of digitalWrite()).
 *   - The 3 s pulse uses a millis() compare in loop(), NOT delay() — using
 *     delay() would block zeno.loop() and you'd miss MQTT keepalives.
 */

#include <ZenoPCBMain.h>

using namespace ZenoPCB;

// ---- Credentials ------------------------------------------------------------
#define WIFI_SSID "REPLACE_ME"
#define WIFI_PASS "REPLACE_ME"
#define DEVICE_ID "REPLACE_ME"
#define DEVICE_TOKEN "REPLACE_AT_PROVISIONING"

// ---- Pin map + ADC scale ----------------------------------------------------
#if defined(ESP32)
  #define SENSOR_PIN 34
  #define BUZZER_PIN 25
  #define ADC_FULL 4095.0f
#elif defined(ESP8266)
  #define SENSOR_PIN A0
  #define BUZZER_PIN 5
  #define ADC_FULL 1023.0f
#elif defined(ARDUINO_UNOR4_WIFI)
  #define SENSOR_PIN A0
  #define BUZZER_PIN 8
  #define ADC_FULL 16383.0f
#elif defined(STM32F4)
  #define SENSOR_PIN PA0
  #define BUZZER_PIN PB0
  #define ADC_FULL 4095.0f
#elif defined(STM32F1)
  #define SENSOR_PIN PA0
  #define BUZZER_PIN PB12
  #define ADC_FULL 4095.0f
#endif

Zeno zeno;
// Timestamp at which the buzzer should be silenced. 0 = idle / silent.
static uint32_t s_buzzerOffMs = 0;
static const uint32_t PULSE_MS = 3000; // 3-second beep

// Device -> Cloud: sample sensor readings to Z2 every 0.5 s for the rule
// engine to evaluate. Publish cadence is 1 s below.
ZENO_EVERY(500)
{
    const float pct = (float)analogRead(SENSOR_PIN) / ADC_FULL * 100.0f;
    DEVICE_TO_CLOUD(Z2, pct);
}

// Alarm callback — fires when the cloud-side rule matches.
void onAlarmTriggered(const String &ruleId, const String &key,
                      double value, uint8_t severity)
{
    Serial.printf("[ALARM->BUZZER] %s %s=%.2f\n",
                   ruleId.c_str(), key.c_str(), value);
    // Start the buzz NOW and remember when to stop. We do not call delay()
    // here because the main loop must keep running for MQTT keepalive.
    digitalWrite(BUZZER_PIN, HIGH);
    s_buzzerOffMs = millis() + PULSE_MS;
}

void setup()
{
    Serial.begin(115200);

#if defined(STM32F1)
    Serial.println(F("[INFO] Alarm not available on STM32F103 (MICRO_BASIC)."));
    return;
#endif

    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW); // start silent

    zeno.wifi(WIFI_SSID, WIFI_PASS)
        .device(DEVICE_ID, DEVICE_TOKEN)
        .enableZKeys()
        .setZPublishInterval(1000)
        .enableAlarm()
        .onAlarmTriggered(onAlarmTriggered)
        .begin();
}

void loop()
{
#if defined(STM32F1)
    return;
#else
    // Non-blocking auto-silence: when the deadline arrives, drop the pin.
    if (s_buzzerOffMs != 0 && millis() >= s_buzzerOffMs)
    {
        s_buzzerOffMs = 0;
        digitalWrite(BUZZER_PIN, LOW);
    }
    zeno.loop();
#endif
}

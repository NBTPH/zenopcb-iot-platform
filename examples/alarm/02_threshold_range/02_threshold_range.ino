/**
 * @file 02_threshold_range.ino
 * @brief Two cloud rules ("Z2 < 10" and "Z2 > 90") light different LEDs so you
 *        can tell WHICH boundary tripped.
 *
 * What you'll learn:
 *   - How to distinguish between multiple cloud alarm rules in one callback.
 *   - Using `ruleId` to branch (the cloud-set string is your handle).
 *   - Why naming rules clearly ("low", "high") matters for device code.
 *
 * Hardware needed:
 *   - Any supported board EXCEPT STM32F103.
 *   - Analog sensor (pot or LDR) on SENSOR_PIN.
 *   - Two LEDs (LOW_LED_PIN, HIGH_LED_PIN) — single on-board LED also fine,
 *     the Serial log still tells you which boundary fired.
 *
 * Wiring:
 *   - Pot wiper -> SENSOR_PIN; pot ends -> 3V3 and GND.
 *   - LED + 220 ohm resistor on each of LOW_LED_PIN and HIGH_LED_PIN.
 *
 * Cloud dashboard setup:
 *   - Create Z2 of type Float — sensor reading (0..100%).
 *   - Create alarm rule with ID containing "low"  and condition `Z2 < 10`.
 *   - Create alarm rule with ID containing "high" and condition `Z2 > 90`.
 *     The sketch matches on substrings of the rule ID, so the exact name is
 *     up to you (e.g. "low-water", "highTemp" both work).
 *
 * @category Alarm
 * @level Beginner
 *
 * @hardware
 * - Any supported board EXCEPT STM32F103.
 * - Analog sensor (pot/LDR) on SENSOR_PIN.
 * - Two LEDs (LOW_LED_PIN, HIGH_LED_PIN) to indicate which boundary tripped.
 *   On-board single LED is fine — the sketch logs which side fired anyway.
 *
 * @platform_notes
 * F103 NOT SUPPORTED (Alarm subsystem stripped).
 *
 * @usage
 * 1. Replace WIFI_SSID / WIFI_PASS / DEVICE_ID / DEVICE_TOKEN below.
 * 2. ESP32 only: Tools > Partition Scheme > "Minimal SPIFFS (1.9MB APP)".
 * 3. Create both rules on the cloud dashboard (see above).
 * 4. Flash the board.
 * 5. Open Serial Monitor at 115200 baud.
 * 6. Spin the pot to either extreme — the corresponding LED lights and the
 *    Serial log identifies which rule triggered.
 *
 * Tips & common mistakes:
 *   - The LEDs LATCH ON here (no auto-clear). Add a timer like sketch 01 if
 *     you want them to clear automatically.
 *   - If neither LED ever lights, your rules probably aren't enabled on the
 *     dashboard, or the rule IDs don't contain "low"/"high".
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
  #define LOW_LED_PIN 25
  #define HIGH_LED_PIN 26
  #define ADC_FULL 4095.0f    // 12-bit
#elif defined(ESP8266)
  #define SENSOR_PIN A0
  #define LOW_LED_PIN 5
  #define HIGH_LED_PIN 4
  #define ADC_FULL 1023.0f    // 10-bit
#elif defined(ARDUINO_UNOR4_WIFI)
  #define SENSOR_PIN A0
  #define LOW_LED_PIN 2
  #define HIGH_LED_PIN 3
  #define ADC_FULL 16383.0f   // 14-bit
#elif defined(STM32F4)
  #define SENSOR_PIN PA0
  #define LOW_LED_PIN PB0
  #define HIGH_LED_PIN PB1
  #define ADC_FULL 4095.0f
#elif defined(STM32F1)
  #define SENSOR_PIN PA0
  #define LOW_LED_PIN PB12
  #define HIGH_LED_PIN PB13
  #define ADC_FULL 4095.0f
#endif

Zeno zeno;

// Device -> Cloud: publish the sensor value every 3 s so the cloud rule
// engine has something to evaluate against.
ZENO_EVERY(3000)
{
    const float pct = (float)analogRead(SENSOR_PIN) / ADC_FULL * 100.0f;
    DEVICE_TO_CLOUD(Z2, pct);
}

// Alarm callback — fires once per rule match (see file 01 for parameter
// details). We use ruleId to figure out WHICH boundary tripped: the cloud
// owns the threshold value, the device just reacts to the rule by name.
void onAlarmTriggered(const String &ruleId, const String &key,
                      double value, uint8_t severity)
{
    Serial.printf("[ALARM] rule=%s key=%s value=%.2f sev=%u\n",
                   ruleId.c_str(), key.c_str(), value, (unsigned)severity);
    // Substring match so users can name rules naturally ("low-water" etc.).
    if (ruleId.indexOf("low") >= 0)
    {
        digitalWrite(LOW_LED_PIN, HIGH);
    }
    else if (ruleId.indexOf("high") >= 0)
    {
        digitalWrite(HIGH_LED_PIN, HIGH);
    }
}

void setup()
{
    Serial.begin(115200);

#if defined(STM32F1)
    // Alarm subsystem is excluded from the MICRO_BASIC build for Blue Pill.
    Serial.println(F("[INFO] Alarm not available on STM32F103 (MICRO_BASIC)."));
    return;
#endif

    pinMode(LOW_LED_PIN, OUTPUT);
    pinMode(HIGH_LED_PIN, OUTPUT);
    digitalWrite(LOW_LED_PIN, LOW);
    digitalWrite(HIGH_LED_PIN, LOW);

    zeno.wifi(WIFI_SSID, WIFI_PASS)
        .device(DEVICE_ID, DEVICE_TOKEN)
        .enableZKeys()
        .enableAlarm()
        .onAlarmTriggered(onAlarmTriggered)
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

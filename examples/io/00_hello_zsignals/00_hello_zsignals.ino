/**
 * @file 00_hello_zsignals.ino
 * @brief Hello-world for ZenoPCB ZSignals — read one analog input, drive one LED, talk to the cloud both ways.
 *
 * What you'll learn:
 *   - What a ZSignal is (a named slot on the ZenoPCB Cloud, called Z0..Zn)
 *   - How to publish a sensor value to the cloud on a timer (Device -> Cloud)
 *   - How to receive a command from the cloud and act on it (Cloud -> Device)
 *   - Why ADC code looks different on each board (resolution varies)
 *
 * Hardware needed:
 *   - Any supported board (ESP32 / ESP8266 / UNO R4 WiFi / STM32 F1 / STM32 F4)
 *   - LED + 220 Ohm resistor on LED_PIN (or just use the on-board LED)
 *   - Optional: potentiometer or analog sensor wired to SENSOR_PIN.
 *     With nothing wired you will still see noisy "floating" readings — that
 *     is fine for confirming the cloud link works end-to-end.
 *   - Jumper wires, breadboard
 *
 * Wiring:
 *   - LED_PIN  -> LED anode -> 220 Ohm -> GND
 *   - SENSOR_PIN -> potentiometer wiper (outer pins to 3.3 V and GND)
 *
 * Cloud dashboard setup:
 *   - Create Z0 of type Float — sensor reading expressed as 0..100 percent
 *   - Create Z1 of type Bool  — turn the LED on/off from the dashboard
 *
 * How to use:
 *   1. Replace WIFI_SSID / WIFI_PASS / DEVICE_ID / DEVICE_TOKEN below.
 *   2. Open Tools > Partition Scheme > "Minimal SPIFFS (1.9MB APP)" (ESP32 only).
 *   3. Flash to the board.
 *   4. Open Serial Monitor at 115200 baud — you should see a Z0 line every 5 seconds.
 *   5. On the dashboard, watch Z0 update every 5 s and toggle Z1 to control the LED.
 *
 * Tips & common mistakes:
 *   - The MQTT broker hostname is built into the library — you do NOT need to
 *     run your own broker.
 *   - On the STM32F1 Blue Pill, the on-board LED is active-LOW (LOW = on).
 *     The code handles this for you; remember it if you swap pins.
 *   - ESP32 GPIO 34 is input-only — you cannot use it as an output.
 */

#include <ZenoPCBMain.h>

using namespace ZenoPCB;

// ============================================
// Credentials — replace before flashing
// ============================================
#define WIFI_SSID "REPLACE_ME"
#define WIFI_PASS "REPLACE_ME"
#define DEVICE_ID "REPLACE_ME"
#define DEVICE_TOKEN "REPLACE_AT_PROVISIONING"

// ============================================
// Pin + ADC settings per board.
// ADCs have different bit-widths, so the "full scale" raw value differs:
//   ESP32 / STM32 = 12-bit (0..4095), ESP8266 = 10-bit (0..1023),
//   UNO R4 WiFi = 14-bit (0..16383). We use SENSOR_MAX below to normalise.
// ============================================
#if defined(ESP32)
  #define SENSOR_PIN 34       // ESP32 GPIO 34 — input-only ADC pin
  #define SENSOR_MAX 4095.0f  // 12-bit ADC
  #define LED_PIN    2        // Built-in LED on most ESP32 dev boards
#elif defined(ESP8266)
  #define SENSOR_PIN A0       // ESP8266 only has one ADC pin (A0)
  #define SENSOR_MAX 1023.0f  // 10-bit ADC
  #define LED_PIN    LED_BUILTIN
#elif defined(ARDUINO_UNOR4_WIFI)
  #define SENSOR_PIN A0
  #define SENSOR_MAX 16383.0f // 14-bit ADC (default on UNO R4)
  #define LED_PIN    LED_BUILTIN
#elif defined(STM32F4)
  #define SENSOR_PIN PA0      // Nucleo-F429ZI Arduino A0
  #define SENSOR_MAX 4095.0f  // 12-bit ADC
  #define LED_PIN    PA5      // Nucleo-F429ZI green user LED
#elif defined(STM32F1)
  #define SENSOR_PIN PA0      // Blue Pill PA0
  #define SENSOR_MAX 4095.0f  // 12-bit ADC
  #define LED_PIN    PC13     // Blue Pill on-board LED — note: active-LOW
#endif

// ============================================
// Globals
// ============================================
Zeno zeno;

// ============================================
// Device -> Cloud: sample the sensor and publish Z0 every 5 seconds.
// ZENO_EVERY(N) runs the body roughly every N milliseconds, without using
// delay() — so the rest of the loop keeps running.
// ============================================
ZENO_EVERY(5000)
{
    float raw = (float)analogRead(SENSOR_PIN);
    float scaled = raw / SENSOR_MAX * 100.0f; // normalise to 0..100 across all boards
    DEVICE_TO_CLOUD(Z0, scaled);
    Serial.print("[Z0] sensor = ");
    Serial.println(scaled);
}

// ============================================
// Cloud -> Device: triggered whenever the dashboard writes Z1.
// `param.toBool()` converts the incoming value to a boolean.
// ============================================
CLOUD_TO_DEVICE(Z1)
{
    bool on = param.toBool();
#if defined(STM32F1)
    // Blue Pill LED is active-LOW, so invert the level.
    digitalWrite(LED_PIN, on ? LOW : HIGH);
#else
    digitalWrite(LED_PIN, on ? HIGH : LOW);
#endif
    Serial.print("[Z1] LED ");
    Serial.println(on ? "ON" : "OFF");
}

// ============================================
// Setup
// ============================================
void setup()
{
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);    // start OFF so we don't glitch on boot

    // Fluent chain: connect to WiFi, identify to the cloud, enable the ZSignal
    // (Z0..Zn) feature, then start the background loop.
    zeno.wifi(WIFI_SSID, WIFI_PASS)
        .device(DEVICE_ID, DEVICE_TOKEN)
        .enableZKeys()
        .begin();
}

// ============================================
// Loop
// ============================================
void loop()
{
    zeno.loop();    // services WiFi + MQTT + ZENO_EVERY timers — must be called every loop
}

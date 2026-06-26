/**
 * @file 00_hello_zsignals.ino
 * @brief Hello-world for ZenoPCB ZSignals — send one local value and receive one cloud value.
 *
 * What you'll learn:
 *   - What a ZSignal is (a named slot on the ZenoPCB Cloud, called Z0..Zn)
 *   - How to update a local ZSignal value in code (Device -> Cloud)
 *   - How to receive a ZSignal value written from the dashboard (Cloud -> Device)
 *   - How ZENO_EVERY sets the local update rate separately from the MQTT publish rate
 *
 * Hardware needed:
 *   - Any supported board (ESP32 / ESP8266 / UNO R4 WiFi / STM32 F1 / STM32 F4)
 *
 * Wiring:
 *   - None, only the board connected to computer via usb to see print outputs
 *
 * Cloud dashboard setup:
 *   - Create Z0 of type Integer — device-generated value sent to the cloud
 *   - Create Z1 of type Integer — dashboard value sent back to the device
 *
 * How to use:
 *   1. Replace WIFI_SSID / WIFI_PASS / DEVICE_ID / DEVICE_TOKEN below.
 *   2. Open Tools > Partition Scheme > "Minimal SPIFFS (1.9MB APP)" (ESP32 only).
 *   3. Flash to the board.
 *   4. Open Serial Monitor at 115200 baud — you should see local Z0 updates every 0.5 s.
 *   5. On the dashboard, watch Z0 publish every 1 s and write a value to Z1.
 *
 * Tips & common mistakes:
 *   - The MQTT broker hostname is built into the library — you do NOT need to
 *     run your own broker.
 *   - This example intentionally avoids ADC, buttons, LEDs, and other hardware
 *     details so the ZSignal send/receive flow is easy to see first.
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
// Globals
// ============================================
Zeno zeno;
int random_number;
static const uint32_t RND_PERIOD_MS = 500;  // half-second per generated value
static uint32_t s_lastRnd = 0;              // millis() of last value update
// ============================================
// Device -> Cloud: send the current local value every 0.5 seconds.
// setZPublishInterval(1000) below publishes dirty Z values every 1 second.
// ZENO_EVERY(N) runs the body roughly every N milliseconds, without using
// delay() — so the rest of the loop keeps running.
// ============================================
ZENO_EVERY(500)
{
    DEVICE_TO_CLOUD(Z0, random_number);
    Serial.print("[Z0] Sending = ");
    Serial.println(random_number);
}

// ============================================
// Cloud -> Device: triggered whenever the dashboard writes Z1.
// `param.toInt()` converts the incoming value to an integer.
// ============================================
CLOUD_TO_DEVICE(Z1)
{
    const int received_value = param.toInt();
    Serial.print("[Z1] Cloud received: ");
    Serial.println(received_value);
}

// ============================================
// Setup
// ============================================
void setup()
{
    random_number = 0; // start from a known value before the first generated update
    Serial.begin(115200);
    randomSeed(micros());

    // Fluent chain: connect to WiFi, identify to the cloud, enable the ZSignal
    // (Z0..Zn) feature, then start the background loop.
    zeno.wifi(WIFI_SSID, WIFI_PASS)
        .device(DEVICE_ID, DEVICE_TOKEN)
        .enableZKeys()
        .setZPublishInterval(1000)
        .begin();
}

// ============================================
// Loop
// ============================================
void loop()
{
    const uint32_t now = millis();
    // Non-blocking timer: generate a new local value on a simple interval.
    if (now - s_lastRnd >= RND_PERIOD_MS)
    {
      s_lastRnd = now;
      random_number = random(10000); // generate a value from 0 to 9999
    }
    zeno.loop();    // services WiFi + MQTT + ZENO_EVERY timers — must be called every loop
}

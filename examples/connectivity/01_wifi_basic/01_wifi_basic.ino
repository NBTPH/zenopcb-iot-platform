/**
 * @file 01_wifi_basic.ino
 * @brief Connect to the ZenoPCB Cloud over WiFi and publish a heartbeat counter.
 *
 * What you'll learn:
 *   - How to connect an Arduino-compatible board to WiFi
 *   - How to register a device with the ZenoPCB Cloud (DEVICE_ID + TOKEN)
 *   - How to send a value to the cloud every N seconds using ZENO_EVERY
 *
 * Hardware needed:
 *   - Any supported board with WiFi, for example:
 *       * ESP32 dev board (built-in WiFi)
 *       * ESP8266 dev board (built-in WiFi)
 *       * Arduino UNO R4 WiFi (uses on-board ESP32-C3 through WiFiS3)
 *       * STM32 Blue Pill F103 plus external ESP-AT module (through WiFiEspAT)
 *   - USB cable to flash and power the board
 *
 * Wiring:
 *   - None. The sketch uses each board's default WiFi peripheral.
 *
 * Cloud dashboard setup:
 *   - Create Z0 of type Int — receives the heartbeat counter
 *
 * @category Connectivity
 * @level Beginner
 *
 * @hardware
 *   - Any supported board with WiFi (see "Hardware needed" above).
 *
 *   Note: STM32 Nucleo-F429ZI ships with on-board Ethernet (RMII PHY) and is the
 *   default target for the `02_ethernet_w5500` sketch rather than this one. If
 *   you want WiFi on the Nucleo, attach an ESP-AT module and the `WiFiEspAT`
 *   library will work the same as on the Blue Pill.
 *
 * @wiring
 *   - None. Each board uses its built-in (or on-board) WiFi.
 *
 * @usage
 *   1. Replace WIFI_SSID, WIFI_PASS, DEVICE_ID, and DEVICE_TOKEN below with
 *      values from your ZenoPCB dashboard.
 *   2. Flash the sketch and open the Serial Monitor at 115200 baud.
 *   3. Watch the device connect to WiFi, then the cloud. A counter samples
 *      every 0.5 s and publishes to Z0 every 1 s.
 *
 * Tips & common mistakes:
 *   - WIFI_PASS is case-sensitive. ESP32/ESP8266 only support 2.4 GHz WiFi —
 *     5 GHz networks won't be visible to the chip.
 *   - If the dashboard never sees a heartbeat, double-check DEVICE_ID and
 *     DEVICE_TOKEN — they come from the ZenoPCB Cloud's device-provisioning
 *     page, not from your WiFi router.
 */

#include <ZenoPCBMain.h>

using namespace ZenoPCB;

// ============================================
// Credentials — replace these four lines before flashing
// ============================================
#define WIFI_SSID "REPLACE_ME"            // your 2.4 GHz WiFi network name
#define WIFI_PASS "REPLACE_ME"            // WiFi password (case-sensitive)
#define DEVICE_ID "REPLACE_ME"            // from the ZenoPCB dashboard
#define DEVICE_TOKEN "REPLACE_AT_PROVISIONING"  // device secret, also from the dashboard

// ============================================
// Globals — `zeno` must live for the lifetime of the program
// ============================================
Zeno zeno;

static uint32_t s_beatCount = 0;          // counter incremented on each heartbeat

// Device -> Cloud: sample a heartbeat every 0.5 seconds.
// ZENO_EVERY auto-registers this block at boot — no need to call it from loop().
ZENO_EVERY(500)
{
    s_beatCount++;
    DEVICE_TO_CLOUD(Z0, (int32_t)s_beatCount);  // send counter to dashboard slot Z0
    Serial.printf("[01_wifi_basic] heartbeat %lu\n", (unsigned long)s_beatCount);
}

void setup()
{
    Serial.begin(115200);                 // USB serial for debug output
    delay(200);                           // give the USB-CDC port a moment to enumerate
    Serial.println();
    Serial.println(F("[01_wifi_basic] boot"));

    // Fluent builder: configure WiFi, identify the device, enable the
    // dashboard-key system (Z0..Z39), wire a connect callback, then start.
    zeno.wifi(WIFI_SSID, WIFI_PASS)
        .device(DEVICE_ID, DEVICE_TOKEN)
        .enableZKeys()                    // turns on the Z0..Z39 cloud-key system
        .setZPublishInterval(1000)        // publish dirty values every 1 second
        .onConnected([]()
                     { Serial.println(F("[01_wifi_basic] cloud connected")); })
        .begin();
}

void loop()
{
    zeno.loop();                          // pumps WiFi, MQTT, and the ZENO_EVERY scheduler
}

/**
 * @file 02_diagnostics.ino
 * @brief Auto-publish device health (free heap, uptime, RSSI, ...) to the cloud diagnostics widget.
 *
 * What you'll learn:
 *   - How to enable the built-in diagnostics reporter with one builder call
 *   - Which health metrics the library bundles automatically (no custom code)
 *   - How to tune the reporting cadence between dev (fast feedback) and
 *     production (data-friendly)
 *
 * Hardware needed:
 *   - Any supported board whose HAL exposes the CAP_DIAGNOSTICS bit:
 *     ESP32, ESP8266, UNO R4 WiFi, STM32 Nucleo-F429ZI
 *     (STM32 Blue Pill F103 in MICRO_BASIC profile drops Diagnostics by design)
 *   - USB cable to flash and power the board
 *
 * Wiring:
 *   - None — diagnostics piggyback on the existing MQTT connection.
 *
 * Cloud dashboard setup:
 *   - Nothing manual. The diagnostics widget on the ZenoPCB dashboard auto-
 *     populates with each report. Fields: freeHeap, totalHeap, maxAllocHeap,
 *     uptimeMs, wifiRSSI, fwVersion, ipAddr.
 *
 * @category Maintenance
 * @level Beginner
 *
 * @hardware
 *   - Any supported board with CAP_DIAGNOSTICS — see list above.
 *
 * @wiring
 *   - None — diagnostics piggyback on the existing MQTT connection.
 *
 * @usage
 *   1. Replace credentials below.
 *   2. Flash and open the Serial Monitor at 115200 baud.
 *   3. Watch the dashboard's diagnostics widget — a record arrives every
 *      60 seconds. The interval is set short so the demo is visible within
 *      a minute; the production default is 600000 ms (10 minutes).
 *
 * Tips & common mistakes:
 *   - 60 s is intentionally aggressive for a demo. In production, 600000 ms
 *     (10 minutes) is plenty and uses far less bandwidth.
 *   - `.setConnectionType("WIFI")` is a label sent with each record so the
 *     dashboard can filter by transport. Use "ETHERNET" or "CELLULAR" if you
 *     swap network providers.
 *   - On boards without CAP_DIAGNOSTICS (e.g. F103 MICRO_BASIC) the call is
 *     compiled out — the sketch boots fine but no records are sent.
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

// ============================================
// Setup
// ============================================
void setup()
{
    Serial.begin(115200);

    zeno.wifi(WIFI_SSID, WIFI_PASS)
        .device(DEVICE_ID, DEVICE_TOKEN)
        // Report health every 60 s for demo visibility.
        // Production default: 600000 ms (10 minutes) — far less bandwidth.
        .enableDiagnostics(60000)
        // Tag the records so the dashboard can filter by transport type.
        .setConnectionType("WIFI")
        .begin();
}

// ============================================
// Loop
// ============================================
void loop()
{
    zeno.loop();                          // pumps WiFi + MQTT + diagnostics scheduler
}

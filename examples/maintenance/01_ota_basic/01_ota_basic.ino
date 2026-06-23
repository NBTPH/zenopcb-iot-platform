/**
 * @file 01_ota_basic.ino
 * @brief Receive a new firmware image Over-The-Air from the ZenoPCB Cloud (ESP32).
 *
 * What you'll learn:
 *   - What "OTA" means and why you'd push a build to a deployed device
 *   - How to subscribe to progress / success / error callbacks during an OTA
 *   - How to also trigger an OTA from your own code (Pattern G) — useful when
 *     you have a custom MQTT command or a maintenance button
 *
 * Hardware needed:
 *   - ESP32 dev board with at least 4 MB flash (most do)
 *   - An OTA-aware partition table (Arduino ESP32 default works)
 *   (UNO R4 needs `-DZENOPCB_ENABLE_UNOR4_OTA`. STM32 OTA is off by default.)
 *
 * Wiring:
 *   - None beyond USB power — OTA travels over WiFi.
 *
 * Cloud dashboard setup:
 *   - No ZSignal channels required. OTA is triggered from the firmware
 *     manager / device console on the dashboard.
 *
 * How to use:
 *   1. Replace WIFI_SSID / WIFI_PASS / DEVICE_ID / DEVICE_TOKEN below and
 *      set FIRMWARE_VER to your current build string.
 *   2. Open Tools > Partition Scheme > "Minimal SPIFFS (1.9MB APP)" (ESP32).
 *   3. Flash this sketch.
 *   4. From the dashboard, upload a new build (with FIRMWARE_VER bumped) —
 *      the device should download it, print progress, and reboot.
 *   5. After reboot, the new FIRMWARE_VER prints on the Serial Monitor.
 *
 * Tips & common mistakes:
 *   - Always bump FIRMWARE_VER between builds — otherwise the device cannot
 *     tell whether an offered image is newer.
 *   - If your sketch is large, use the "Huge APP 3MB No OTA" partition for
 *     development only. For production OTA you need a partition layout that
 *     reserves space for both the running image and the incoming one.
 *   - Don't call delay() inside the OTA callbacks — they fire frequently and
 *     blocking them can stall the download.
 */

#include <ZenoPCBMain.h>

using namespace ZenoPCB;

// ============================================
// Credentials — replace before flashing.
// FIRMWARE_VER must change every build so the cloud can compare versions.
// ============================================
#define WIFI_SSID "REPLACE_ME"
#define WIFI_PASS "REPLACE_ME"
#define DEVICE_ID "REPLACE_ME"
#define DEVICE_TOKEN "REPLACE_AT_PROVISIONING"
#define FIRMWARE_VER "1.0.0"

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
    Serial.printf("[OTA] Boot, firmware v%s\n", FIRMWARE_VER);

    // Fluent setup: WiFi, identify to cloud, enable OTA, register three callbacks.
    // The callbacks are tiny lambdas — keep them short so they don't slow the download.
    zeno.wifi(WIFI_SSID, WIFI_PASS)
        .device(DEVICE_ID, DEVICE_TOKEN)
        .enableOTA()
        .onOTAProgress([](float pct)
                       { Serial.printf("[OTA] progress %.1f%%\n", pct); })
        .onOTAComplete([](const String &ver)
                       { Serial.printf("[OTA] complete, now v%s rebooting\n", ver.c_str()); })
        .onOTAError([](OTAError code, const String &msg)
                    { Serial.printf("[OTA] error %d: %s\n", (int)code, msg.c_str()); })
        .begin();
}

// ============================================
// Loop
// ============================================
void loop()
{
    zeno.loop();    // services WiFi + MQTT + OTA — required every loop iteration

    // ============================================
    // Pattern G — trigger an OTA from your own code path.
    //
    // The fluent enableOTA() chain above already handles cloud-pushed OTAs.
    // This block is for the other direction: you want YOUR code (an MQTT
    // command handler, a maintenance button, a scheduled task) to kick off
    // an OTA from a URL it computed.
    //
    // `mqttCommand` is a static placeholder so the example compiles without
    // an MQTT handler attached. In production you'd populate it from your
    // own command path.
    // ============================================
    static String mqttCommand = "";   // wire this to your real MQTT handler
    if (mqttCommand.startsWith("OTA:"))
    {
        String urlStr = mqttCommand.substring(4);   // strip the "OTA:" prefix
        // zeno.ota(url) returns a ZenoCapability indicating whether the OTA
        // started, isn't available on this platform, failed immediately, or
        // is already in progress. Handle each so users get a clear log line.
        switch (zeno.ota(urlStr.c_str()))
        {
            case ZenoCapability::OK:
                Serial.println(F("[OTA] download started"));
                break;
            case ZenoCapability::Unavailable:
                Serial.println(F("[OTA] not available on this platform"));
                break;
            case ZenoCapability::Error:
                Serial.println(F("[OTA] download failed check URL or network"));
                break;
            case ZenoCapability::Pending:
                Serial.println(F("[OTA] already in progress, ignoring duplicate request"));
                break;
        }
        mqttCommand = "";   // clear the slot so we don't re-trigger next loop
    }
}

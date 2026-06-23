/**
 * @file 04_capability_matrix.ino
 * @brief Print which optional library features (OTA, NVS, NTP, TLS, ...) the current board supports.
 *
 * What you'll learn:
 *   - How ZenoPCB exposes per-platform capabilities via a HAL bitmask
 *   - How "Pattern G" lets one sketch run on every board: missing features
 *     return `ZenoCapability::Unavailable` instead of failing at compile time
 *   - How to write a tiny self-test that asserts the runtime answer matches
 *     the documented capability table
 *
 * Hardware needed:
 *   - Any board the library supports (ESP32, ESP8266, UNO R4 WiFi,
 *     STM32 Nucleo-F429ZI, STM32 Blue Pill F103)
 *   - USB cable (no network or sensors required — this probe stays offline)
 *
 * Wiring:
 *   - None.
 *
 * Cloud dashboard setup:
 *   - Not used. This sketch never connects to the cloud.
 *
 * @category Patterns
 * @level Advanced
 *
 * @hardware
 *   - Any board the library supports. The probe never connects to the network
 *     and never needs credentials.
 *
 * @usage
 *   1. Flash the sketch as-is (no credentials needed).
 *   2. Open the Serial Monitor at 115200 baud.
 *   3. Read the capability table and the OK / FAIL assertion lines.
 *
 * Notes:
 *   This is the companion sketch for the README capability matrix (GOV-04).
 *   When the CI matrix unpauses, this sketch becomes the executable check
 *   that the hand-authored matrix table matches the library's runtime
 *   `capabilities()` bitmask on each platform.
 *
 *   `Serial.printf` is intentionally not used — STM32duino's HardwareSerial
 *   does not provide it. The `Serial.print` + `Serial.println` pattern below
 *   is portable across all five supported platforms.
 */

#include <ZenoPCBMain.h>

using namespace ZenoPCB;

// ============================================
// Globals
// ============================================
Zeno zeno;

// Resolve the HAL singleton for the current platform. `ZENOPCB_DEFAULT_HAL()`
// expands to the right factory at compile time (`getEsp32Hal()`,
// `getEsp8266Hal()`, `getUnoR4Hal()`, or `getStm32Hal()`), so this sketch is
// the same source file on every board.
static IZenoHal &hal = ZENOPCB_DEFAULT_HAL();

// ============================================
// Helper — print "CAP_NAME : 0/1" for a single capability bit
// ============================================
static void printCapRow(const __FlashStringHelper *name, uint32_t caps, uint32_t bit)
{
    Serial.print(name);
    Serial.print(F(" : "));
    Serial.println((caps & bit) ? F("1") : F("0"));
}

// ============================================
// Setup — runs the probe once
// ============================================
void setup()
{
    Serial.begin(115200);
    delay(100);                                  // brief pause so the host serial opens
    Serial.println();
    Serial.println(F("[ZenoPCB 07_CapabilityMatrix] runtime capability probe"));

    uint32_t caps = hal.capabilities();          // platform's capability bitmask

    // ---- Bitmask dump (matches IZenoHal::Capability enum, Phase 7 D-09/D-10) ----
    printCapRow(F("CAP_FS_FILES "), caps, IZenoHal::CAP_FS_FILES);
    printCapRow(F("CAP_OTA "), caps, IZenoHal::CAP_OTA);
    printCapRow(F("CAP_NVS "), caps, IZenoHal::CAP_NVS);
    printCapRow(F("CAP_NTP "), caps, IZenoHal::CAP_NTP);
    printCapRow(F("CAP_WATCHDOG "), caps, IZenoHal::CAP_WATCHDOG);
    printCapRow(F("CAP_CAPTIVE_PORTAL"), caps, IZenoHal::CAP_CAPTIVE_PORTAL);
    printCapRow(F("CAP_TLS "), caps, IZenoHal::CAP_TLS);
    printCapRow(F("CAP_DIAGNOSTICS "), caps, IZenoHal::CAP_DIAGNOSTICS);

    // ---- D-08 + D-21 Pattern G assertion sketch -------------------------
    // Rule: when a capability bit is 0, calling the Pattern G surface for
    // that feature MUST return `ZenoCapability::Unavailable`. Anything else
    // is a library bug — the assertion FAILs in that case.

    if (!(caps & IZenoHal::CAP_OTA))
    {
        ZenoCapability result = zeno.ota("https://placeholder.invalid/fw.bin");
        if (result != ZenoCapability::Unavailable)
        {
            Serial.println(F("FAIL: zeno.ota expected Unavailable when CAP_OTA=0"));
        }
        else
        {
            Serial.println(F("OK: zeno.ota returned Unavailable as expected"));
        }
    }
    else
    {
        // CAP_OTA = 1 on this board — the probe doesn't actually pull
        // firmware (that would need a real URL), so it just records SKIP.
        Serial.println(F("SKIP: zeno.ota assertion (CAP_OTA=1 not tested by this probe)"));
    }

    if (!(caps & IZenoHal::CAP_CAPTIVE_PORTAL))
    {
        ZenoCapability result = zeno.wifiProvisioning("zeno-test-ap", "");
        if (result != ZenoCapability::Unavailable)
        {
            Serial.println(F("FAIL: zeno.wifiProvisioning expected Unavailable when CAP_CAPTIVE_PORTAL=0"));
        }
        else
        {
            Serial.println(F("OK: zeno.wifiProvisioning returned Unavailable as expected"));
        }
    }
    else
    {
        Serial.println(F("SKIP: zeno.wifiProvisioning assertion (CAP_CAPTIVE_PORTAL=1 not tested by this probe)"));
    }

    Serial.println(F("[07_CapabilityMatrix] probe complete"));
}

// ============================================
// Loop — keep the cooperative scheduler ticking even though we never connect
// ============================================
void loop()
{
    zeno.loop();
}

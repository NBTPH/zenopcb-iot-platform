/**
 * @file 04_multi_failover.ino
 * @brief Prioritized network failover (Ethernet -> WiFi -> 4G) with the active link published to Z0.
 *
 * What you'll learn:
 *   - How to compose multiple network providers under one ZenoMultiConnectProvider
 *   - Why priority order matters (first added = highest priority)
 *   - How to publish runtime connectivity state to the dashboard
 *
 * Hardware needed:
 *   - ESP32 dev board (failover is ESP32-only in v0.3.0)
 *   - WIZnet W5500 Ethernet module (wired link)
 *   - SIM7600-family 4G modem + active SIM (fallback link)
 *   - WiFi access point (middle-priority fallback)
 *   - External 5 V supply for the modem (peaks of ~2 A)
 *   - Jumper wires, breadboard
 *
 * Wiring (same as 02_ethernet_w5500.ino + 03_4g_sim7600.ino combined):
 *   - W5500 CS    -> GPIO 5    SIM7600 TX  -> GPIO 16 (ESP32 RX2)
 *   - W5500 RST   -> GPIO 26   SIM7600 RX  <- GPIO 17 (ESP32 TX2)
 *   - W5500 SCK   -> GPIO 18   SIM7600 PWR -> GPIO 4
 *   - W5500 MISO  -> GPIO 19
 *   - W5500 MOSI  -> GPIO 23
 *
 * Cloud dashboard setup:
 *   - Create Z0 of type String — active provider name ("Ethernet"/"WiFi"/"4G")
 *   - Create Z1 of type String — current IP address
 *
 * Build flags (required):
 *   - -DZENOPCB_ENABLE_ETHERNET
 *   - -DZENOPCB_ENABLE_CELLULAR
 *   - -DTINY_GSM_MODEM_SIM7600   (or your modem's macro)
 *   How to set:
 *     PlatformIO:  build_flags = -DZENOPCB_ENABLE_ETHERNET -DZENOPCB_ENABLE_CELLULAR -DTINY_GSM_MODEM_SIM7600
 *
 * @category Connectivity
 * @level Advanced
 *
 * @hardware
 *   - ESP32 (Ethernet + Cellular providers are ESP32-only in v0.3.0).
 *     Non-ESP32 boards fall through with a Serial notice.
 *   - WiFi (built into ESP32), W5500, SIM7600 modem — see "Wiring" above.
 *
 * @wiring
 *   See "Wiring" section above (combination of 02 + 03 sketches).
 *
 * @lib_deps (PlatformIO)
 *   build_flags = -DZENOPCB_ENABLE_ETHERNET -DZENOPCB_ENABLE_CELLULAR -DTINY_GSM_MODEM_SIM7600
 *
 *   No extra lib_deps — `ETH.h` is in Arduino Core 3.x, TinyGSM is vendored.
 *
 * @usage
 *   1. Wire all three peripherals: W5500 + SIM7600 + a known WiFi network.
 *   2. Build with the three flags above.
 *   3. Replace credentials below.
 *   4. Flash and open the Serial Monitor at 115200 baud. The active provider
 *      name is published to Z0 every 10 seconds.
 *   5. Try unplugging the Ethernet cable: Z0 should switch to "WiFi".
 *      Disable the WiFi network: Z0 should switch to "4G".
 *
 * Priority order (first added = highest priority):
 *   1. Ethernet (fastest, most deterministic — added first)
 *   2. WiFi     (free, low-latency when working — handled by Zeno core)
 *   3. 4G       (last-resort fallback — added second, runs only if Ethernet is down)
 *
 * Tips & common mistakes:
 *   - All three providers must be DECLARED at file scope (not in setup()) so
 *     they live for the entire program. Local providers would go out of scope.
 *   - WiFi is NOT added to `multiProvider` here — the Zeno core handles WiFi
 *     internally via `.wifi(...)`. Adding the same provider twice would loop.
 *   - Failover takes 5-15 s because each provider needs to time out before
 *     the next one is tried. That's by design — flapping links shouldn't
 *     cause constant switching.
 */

#include <ZenoPCBMain.h>

using namespace ZenoPCB;

// Warn if either build flag is missing — the failover code paths get
// compiled out without them.
#if defined(ESP32)
  #if !defined(ZENOPCB_ENABLE_ETHERNET) || !defined(ZENOPCB_ENABLE_CELLULAR)
    #warning "Build with -DZENOPCB_ENABLE_ETHERNET -DZENOPCB_ENABLE_CELLULAR for full failover"
  #endif
#endif

// ============================================
// Credentials — replace before flashing
// ============================================
#define WIFI_SSID "REPLACE_ME"
#define WIFI_PASS "REPLACE_ME"
#define DEVICE_ID "REPLACE_ME"
#define DEVICE_TOKEN "REPLACE_AT_PROVISIONING"

// ============================================
// Hardware pins (ESP32) — match files 02 and 03
// ============================================
#if defined(ESP32)
  #define ETH_CS_PIN 5
  #define ETH_RST_PIN 26
  #define MODEM_TX_PIN 17
  #define MODEM_RX_PIN 16
  #define MODEM_PWR_PIN 4
#endif

// ============================================
// Globals — providers MUST live for the lifetime of the program
// ============================================
Zeno zeno;

#if defined(ESP32) && defined(ZENOPCB_ENABLE_ETHERNET) && defined(ZENOPCB_ENABLE_CELLULAR)
  ZenoEthernetProvider     ethProvider(ETH_CS_PIN, ETH_RST_PIN);
  Zeno4GProvider           cellProvider(MODEM_TX_PIN, MODEM_RX_PIN, MODEM_PWR_PIN);
  ZenoMultiConnectProvider multiProvider;             // composes the others
#endif

#if defined(ESP32) && defined(ZENOPCB_ENABLE_ETHERNET) && defined(ZENOPCB_ENABLE_CELLULAR)
// Device -> Cloud: every 10 seconds, publish the name + IP of the link that
// is currently carrying traffic. The dashboard can then plot link state.
ZENO_EVERY(10000)
{
    const char *activeName = multiProvider.getName();    // "Ethernet" / "WiFi" / "4G"
    String      activeIP   = multiProvider.getLocalIP();
    DEVICE_TO_CLOUD(Z0, String(activeName));
    DEVICE_TO_CLOUD(Z1, activeIP);
    Serial.printf("[04_multi_failover] active=%s ip=%s\n",
                   activeName, activeIP.c_str());
}
#endif

void setup()
{
    Serial.begin(115200);
    delay(200);
    Serial.println();
    Serial.println(F("[04_multi_failover] boot"));

#if defined(ESP32) && defined(ZENOPCB_ENABLE_ETHERNET) && defined(ZENOPCB_ENABLE_CELLULAR)
    // First-added provider = highest priority. WiFi is NOT added here — the
    // Zeno core's built-in WiFi path is used as the middle fallback whenever
    // both external providers are down.
    multiProvider.addProvider(&ethProvider);    // priority 1 (preferred)
    multiProvider.addProvider(&cellProvider);   // priority 2 (after WiFi falls back)

    zeno.wifi(WIFI_SSID, WIFI_PASS)             // priority 3 (core handles it)
        .device(DEVICE_ID, DEVICE_TOKEN)
        .setNetworkProvider(&multiProvider)
        .enableZKeys()
        .onConnected([]()
                     { Serial.println(F("[04_multi_failover] cloud connected (some link)")); })
        .begin();
#else
    Serial.println(F("[04_multi_failover] Multi-connectivity not available on this platform."));
    Serial.println(F(" Ethernet + Cellular providers are ESP32-only in v0.3.0."));
    Serial.println(F(" Use connectivity/01_wifi_basic for plain WiFi."));
#endif
}

void loop()
{
#if defined(ESP32) && defined(ZENOPCB_ENABLE_ETHERNET) && defined(ZENOPCB_ENABLE_CELLULAR)
    zeno.loop();                                // pumps every provider, MQTT, ZENO_EVERY
#else
    delay(1000);
#endif
}

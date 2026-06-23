/**
 * @file 02_ethernet_w5500.ino
 * @brief Connect to the ZenoPCB Cloud over wired Ethernet (W5500) and publish a heartbeat.
 *
 * What you'll learn:
 *   - How to use a wired network instead of WiFi (more reliable in industrial setups)
 *   - How a NetworkProvider plugs into the Zeno builder via .setNetworkProvider()
 *   - Why the same sketch works on ESP32 + W5500 and on Nucleo-F429ZI native PHY
 *
 * Hardware needed:
 *   - ESP32 dev board (canonical Ethernet path), OR
 *     STM32 Nucleo-F429ZI (uses on-board RMII PHY — no extra module needed)
 *   - WIZnet W5500 Ethernet module (for the ESP32 path)
 *   - Cat5/Cat6 Ethernet cable + a router with DHCP (any home router will do)
 *   - Jumper wires (ESP32 path only)
 *
 * Wiring (ESP32 + W5500):
 *   - W5500 CS    -> ESP32 GPIO 5    (SPI chip-select)
 *   - W5500 RST   -> ESP32 GPIO 26   (optional reset — set ETH_RST_PIN = -1 to skip)
 *   - W5500 SCK   -> ESP32 GPIO 18   (SPI clock)
 *   - W5500 MISO  -> ESP32 GPIO 19   (SPI data, slave -> master)
 *   - W5500 MOSI  -> ESP32 GPIO 23   (SPI data, master -> slave)
 *   - W5500 VCC   -> ESP32 3V3
 *   - W5500 GND   -> ESP32 GND
 *
 * Wiring (STM32 Nucleo-F429ZI):
 *   - Plug an Ethernet cable into the on-board RJ-45 jack. That's it — the
 *     RMII PHY is already wired on the Nucleo PCB.
 *
 * Cloud dashboard setup:
 *   - Create Z0 of type Int — receives the heartbeat counter
 *
 * Build flags (required):
 *   - -DZENOPCB_ENABLE_ETHERNET   turns on the W5500 driver inside the library.
 *   How to set:
 *     Arduino IDE: Tools > Build Options > Extra flags (if your board core
 *                  exposes this option) — or edit `platform.local.txt`.
 *     PlatformIO:  build_flags = -DZENOPCB_ENABLE_ETHERNET   under [env:esp32dev]
 *
 * @category Connectivity
 * @level Intermediate
 *
 * @hardware
 *   - ESP32 + WIZnet W5500 SPI module (canonical ZenoPCB Ethernet path)
 *   - STM32 Nucleo-F429ZI on-board Ethernet (RMII PHY, no W5500 needed)
 *   - Other boards: this sketch prints a Serial notice and stops — use
 *     `connectivity/01_wifi_basic` instead.
 *
 * @wiring
 *   See "Wiring" section above.
 *
 * @lib_deps (PlatformIO)
 *   - ESP32 path: uses Arduino Core 3.x built-in `ETH.h` (W5500 native driver).
 *     No extra lib_deps. Requires build flag `-DZENOPCB_ENABLE_ETHERNET`.
 *   - STM32 path: requires `stm32duino/STM32Ethernet` (BSD-3, Arduino official).
 *
 * @usage
 *   1. Wire the W5500 to the ESP32 (or plug a cable into the Nucleo).
 *   2. Replace DEVICE_ID and DEVICE_TOKEN below.
 *   3. Add `-DZENOPCB_ENABLE_ETHERNET` to your build flags (ESP32 only).
 *   4. Flash and open the Serial Monitor at 115200 baud.
 *   5. The router's DHCP assigns an IP; a heartbeat then publishes to Z0
 *      every 30 seconds.
 *
 * How to use:
 *   1. Replace DEVICE_ID / DEVICE_TOKEN below.
 *   2. Add the build flag, then flash the board.
 *   3. Open the Serial Monitor at 115200 baud — you should see "cloud
 *      connected via Ethernet" then a periodic heartbeat line.
 *   4. Confirm the counter updating on dashboard slot Z0.
 *
 * Tips & common mistakes:
 *   - Forgetting `-DZENOPCB_ENABLE_ETHERNET` leaves the Ethernet code paths
 *     out of the build — the sketch then prints a warning and falls through.
 *   - The W5500 needs SPI on pins 18/19/23. If you already use those pins for
 *     a sensor, pick different ones and update ETH_CS_PIN + the SPI.begin()
 *     call inside ZenoEthernetProvider accordingly.
 *   - The board pulls a DHCP lease — if your network has no DHCP server,
 *     you'll need a static-IP variant (see the library docs).
 */

#include <ZenoPCBMain.h>

using namespace ZenoPCB;

// Compile-time check: warn (don't fail) when the user forgets the build flag.
#if defined(ESP32)
  #ifndef ZENOPCB_ENABLE_ETHERNET
    #warning "Build with -DZENOPCB_ENABLE_ETHERNET to enable W5500 Ethernet"
  #endif
#endif

// ============================================
// Credentials — replace before flashing
// ============================================
#define DEVICE_ID "REPLACE_ME"
#define DEVICE_TOKEN "REPLACE_AT_PROVISIONING"

// ============================================
// Hardware pins (ESP32 + W5500)
// ============================================
#if defined(ESP32)
  #define ETH_CS_PIN 5                    // W5500 chip-select on SPI bus
  #define ETH_RST_PIN 26                  // W5500 hardware reset (use -1 if not wired)
#endif

// ============================================
// Globals — providers must live for the lifetime of the program
// ============================================
Zeno zeno;

#if defined(ESP32) && defined(ZENOPCB_ENABLE_ETHERNET)
  // The Ethernet provider owns the SPI driver. Declared at file scope so it
  // outlives setup() and stays alive while loop() runs.
  ZenoEthernetProvider ethProvider(ETH_CS_PIN, ETH_RST_PIN);
#endif

static uint32_t s_beatCount = 0;

#if defined(ESP32) && defined(ZENOPCB_ENABLE_ETHERNET)
// Device -> Cloud: publish a heartbeat every 30 seconds, plus the current IP.
ZENO_EVERY(30000)
{
    s_beatCount++;
    DEVICE_TO_CLOUD(Z0, (int32_t)s_beatCount);
    Serial.printf("[02_ethernet_w5500] heartbeat %lu (IP=%s)\n",
                   (unsigned long)s_beatCount, ethProvider.getLocalIP().c_str());
}
#endif

void setup()
{
    Serial.begin(115200);
    delay(200);
    Serial.println();
    Serial.println(F("[02_ethernet_w5500] boot"));

#if defined(ESP32) && defined(ZENOPCB_ENABLE_ETHERNET)
    // Note: no .wifi() call — `.setNetworkProvider(&ethProvider)` tells the
    // library to use Ethernet instead. The rest of the API is identical.
    zeno.device(DEVICE_ID, DEVICE_TOKEN)
        .setNetworkProvider(&ethProvider)
        .enableZKeys()
        .onConnected([]()
                     { Serial.println(F("[02_ethernet_w5500] cloud connected via Ethernet")); })
        .begin();
#elif defined(STM32F4)
    // STM32 Nucleo-F429ZI has a wired PHY but the v0.3.0 release does not yet
    // ship a built-in STM32Ethernet provider. Until that lands you have two
    // options: use WiFi via an ESP-AT module, or wire a W5500.
    Serial.println(F("[02_ethernet_w5500] STM32 F4 native Ethernet provider"));
    Serial.println(F(" is not bundled in v0.3.0 see backlog."));
    Serial.println(F(" Use connectivity/01_wifi_basic instead"));
    Serial.println(F(" or wire a W5500 module if you need wired."));
#else
    // ESP8266 / UNO R4 / STM32 F1 — no Ethernet support in v0.3.0.
    Serial.println(F("[02_ethernet_w5500] Ethernet not available on this platform."));
    Serial.println(F(" Build for ESP32 (-DZENOPCB_ENABLE_ETHERNET)"));
    Serial.println(F(" or use connectivity/01_wifi_basic."));
#endif
}

void loop()
{
#if defined(ESP32) && defined(ZENOPCB_ENABLE_ETHERNET)
    zeno.loop();                          // pumps Ethernet, MQTT, and ZENO_EVERY
#else
    // No Ethernet path — just yield so the watchdog stays happy.
    delay(1000);
#endif
}

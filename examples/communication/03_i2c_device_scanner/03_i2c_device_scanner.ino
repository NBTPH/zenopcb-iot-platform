/**
 * @file 03_i2c_device_scanner.ino
 * @brief Scan the I2C bus, publish discovered addresses as a CSV string to Z0.
 *
 * What you'll learn:
 *   - How I2C devices respond to "address probes" so you can discover them
 *   - The standard I2C 7-bit address range (0x03 .. 0x77)
 *   - How to trigger a one-shot action from the cloud (CLOUD_TO_DEVICE)
 *
 * Hardware needed:
 *   - Any supported board with WiFi and I2C
 *   - One or more I2C peripherals (sensors, OLEDs, RTCs — anything with SDA/SCL)
 *   - 4.7 kOhm pull-up resistors on SDA and SCL to 3.3 V
 *     (most breakout boards already include them)
 *
 * Wiring:
 *   - I2C device SDA -> board default SDA pin (GPIO 21 on ESP32, A4 on UNO R4)
 *   - I2C device SCL -> board default SCL pin (GPIO 22 on ESP32, A5 on UNO R4)
 *   - VCC -> 3.3 V (some 5 V-only modules need 5 V — check yours)
 *   - GND -> GND
 *
 * Cloud dashboard setup:
 *   - Create Z0 of type String — receives the CSV of found addresses
 *   - Create Z1 of type Bool   — write true to trigger a re-scan
 *
 * @category Communication
 * @level Beginner
 *
 * @hardware
 *   - Any supported board with I2C, plus at least one I2C peripheral.
 *
 * @wiring
 *   Use the board's default Wire SDA / SCL pins.
 *   4.7 kOhm pull-ups on SDA + SCL to 3.3 V (most modules already include them).
 *
 * @usage
 *   1. Replace credentials below.
 *   2. On boot the sketch scans the bus once and prints the list of
 *      responding addresses to Serial. It also publishes the same list as
 *      a CSV string to Z0 (e.g. "0x3C, 0x68, 0x76").
 *   3. To re-scan at runtime, write true to Z1 from the dashboard.
 *
 * Tips & common mistakes:
 *   - I2C addresses are 7 bits (0x00 .. 0x7F) but addresses 0x00 .. 0x02 and
 *     0x78 .. 0x7F are reserved by the I2C spec — scanning from 0x03 to 0x77
 *     skips them.
 *   - "(none)" in the output usually means missing pull-ups, wrong voltage,
 *     or SDA/SCL swapped.
 *   - Two devices sharing the same address cause read errors on both. Use
 *     a device with a configurable address jumper, or an I2C multiplexer.
 */

#include <ZenoPCBMain.h>
#include <Wire.h>

using namespace ZenoPCB;

#define WIFI_SSID "REPLACE_ME"
#define WIFI_PASS "REPLACE_ME"
#define DEVICE_ID "REPLACE_ME"
#define DEVICE_TOKEN "REPLACE_AT_PROVISIONING"

Zeno zeno;

// Walk every valid I2C address. A device that ACKs the probe is "present".
static void scanAndPublish()
{
    String csv;
    uint8_t found = 0;
    for (uint8_t addr = 0x03; addr <= 0x77; ++addr)
    {
        Wire.beginTransmission(addr);
        // endTransmission returns 0 on success — meaning the slave ACK'd.
        // Non-zero codes mean NACK, timeout, or other bus errors.
        const uint8_t err = Wire.endTransmission();
        if (err == 0)
        {
            if (found > 0) csv += ", ";
            csv += "0x";
            if (addr < 0x10) csv += "0";   // pad single-digit hex for readability
            csv += String(addr, HEX);
            ++found;
        }
    }
    if (found == 0) csv = "(none)";
    // Device -> Cloud: publish the CSV of discovered addresses
    DEVICE_TO_CLOUD(Z0, csv);
    Serial.printf("[I2C] scan: %u device(s) -> %s\n",
                   (unsigned)found, csv.c_str());
}

// Cloud -> Device: dashboard wrote true to Z1, kick off a fresh scan.
CLOUD_TO_DEVICE(Z1)
{
    if (param.toBool())
    {
        Serial.printf("[I2C] rescan triggered by cloud\n");
        scanAndPublish();
    }
}

void setup()
{
    Serial.begin(115200);
    Wire.begin();                          // default SDA/SCL: GPIO 21/22 on ESP32

    zeno.wifi(WIFI_SSID, WIFI_PASS)
        .device(DEVICE_ID, DEVICE_TOKEN)
        .enableZKeys()
        .setZPublishInterval(1000)
        .begin();

    delay(500);                            // let MQTT connect before first publish
                                           // (otherwise Z0 has no destination yet)
    scanAndPublish();
}

void loop()
{
    zeno.loop();                           // keeps WiFi + MQTT + Z1 callback responsive
}

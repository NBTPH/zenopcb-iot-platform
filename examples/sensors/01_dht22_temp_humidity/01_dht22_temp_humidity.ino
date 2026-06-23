/**
 * @file 01_dht22_temp_humidity.ino
 * @brief Read DHT22 temperature + humidity every 5 s, publish to Z0 / Z1.
 *
 * @category Sensors
 * @level Beginner
 *
 * @hardware
 * - Any supported board.
 * - DHT22 (AM2302) sensor — 3.3 V or 5 V tolerant, 4.7 kOhm pull-up between
 *   data pin and VCC.
 *
 * @wiring
 * - DHT22 VCC -> 3.3 V (or 5 V).
 * - DHT22 GND -> GND.
 * - DHT22 DATA -> DHT_PIN, with 4.7 kOhm to VCC.
 *
 * @lib_deps
 * Add to platformio.ini: `adafruit/DHT sensor library @ ^1.4.6`.
 * Arduino IDE: Library Manager -> install "DHT sensor library" by Adafruit.
 *
 * @usage
 * 1. Install the DHT library (see @lib_deps).
 * 2. Set credentials.
 * 3. Z0 = temperature in degC, Z1 = humidity in %RH. Published every 5 s.
 */

#include <ZenoPCBMain.h>
#include <DHT.h>

using namespace ZenoPCB;

#define WIFI_SSID "REPLACE_ME"
#define WIFI_PASS "REPLACE_ME"
#define DEVICE_ID "REPLACE_ME"
#define DEVICE_TOKEN "REPLACE_AT_PROVISIONING"

#if defined(ESP32)
  #define DHT_PIN 4
#elif defined(ESP8266)
  #define DHT_PIN 4    // D2
#elif defined(ARDUINO_UNOR4_WIFI)
  #define DHT_PIN 2
#elif defined(STM32F4)
  #define DHT_PIN PA8
#elif defined(STM32F1)
  #define DHT_PIN PA8
#endif

#define DHT_TYPE DHT22

Zeno zeno;
DHT  dht(DHT_PIN, DHT_TYPE);

// Read DHT22 and publish T/H every 5 s.
ZENO_EVERY(5000)
{
    const float t = dht.readTemperature();
    const float h = dht.readHumidity();
    if (!isnan(t) && !isnan(h))
    {
        DEVICE_TO_CLOUD(Z0, t);
        DEVICE_TO_CLOUD(Z1, h);
        Serial.printf("[DHT22] T=%.1fC H=%.1f%%\n", t, h);
    }
    else
    {
        Serial.printf("[DHT22] read failed\n");
    }
}

void setup()
{
    Serial.begin(115200);
    dht.begin();

    zeno.wifi(WIFI_SSID, WIFI_PASS)
        .device(DEVICE_ID, DEVICE_TOKEN)
        .enableZKeys()
        .begin();
}

void loop()
{
    zeno.loop();
}

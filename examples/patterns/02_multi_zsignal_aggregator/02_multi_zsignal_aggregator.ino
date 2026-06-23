/**
 * @file 02_multi_zsignal_aggregator.ino
 * @brief Aggregate 5 analog channels into 5 ZSignals + one JSON summary.
 *
 * @category Patterns
 * @level Intermediate
 *
 * @hardware
 * - Any supported board.
 * - Up to 5 analog inputs on A0..A4 (or equivalents).
 *
 * @platform_notes
 * Pure ZSignals — works on every supported port including F103.
 *
 * @usage
 * 1. Set credentials.
 * 2. Each loop iteration reads 5 analog channels and writes them to
 *    Z0..Z4 (raw 0..100% normalised across each board's ADC bit depth).
 * 3. A combined JSON summary is also written to Z5 once every 10 s, so
 *    the cloud receives a single packet with all readings — useful when
 *    downstream consumers prefer one event over five.
 */

#include <ZenoPCBMain.h>

using namespace ZenoPCB;

#define WIFI_SSID "REPLACE_ME"
#define WIFI_PASS "REPLACE_ME"
#define DEVICE_ID "REPLACE_ME"
#define DEVICE_TOKEN "REPLACE_AT_PROVISIONING"

#if defined(ESP32)
  static const int   ADC_PINS[5] = { 32, 33, 34, 35, 36 };
  #define ADC_FULL 4095.0f
#elif defined(ESP8266)
  static const int   ADC_PINS[5] = { A0, A0, A0, A0, A0 }; // ESP8266 has only A0; same pin sampled 5x for demo
  #define ADC_FULL 1023.0f
#elif defined(ARDUINO_UNOR4_WIFI)
  static const int   ADC_PINS[5] = { A0, A1, A2, A3, A4 };
  #define ADC_FULL 16383.0f
#elif defined(STM32F4)
  static const int   ADC_PINS[5] = { PA0, PA1, PA2, PA3, PA4 };
  #define ADC_FULL 4095.0f
#elif defined(STM32F1)
  static const int   ADC_PINS[5] = { PA0, PA1, PA2, PA3, PA4 };
  #define ADC_FULL 4095.0f
#endif

Zeno zeno;
static const uint32_t SAMPLE_MS    = 2000;
// Summary is emitted every SUMMARY_EVERY_N samples (so 5 * 2000 ms = 10 s).
// Library limitation: only one ZENO_EVERY block per translation unit, so the
// summary cadence is sub-divided inside the same block rather than declared
// as a second block.
static const uint8_t  SUMMARY_EVERY_N = 5;

static float   s_pct[5]      = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
static uint8_t s_sampleCount = 0;

ZENO_EVERY(SAMPLE_MS)
{
    for (int i = 0; i < 5; ++i)
    {
        s_pct[i] = (float)analogRead(ADC_PINS[i]) / ADC_FULL * 100.0f;
    }
    DEVICE_TO_CLOUD(Z0, s_pct[0]);
    DEVICE_TO_CLOUD(Z1, s_pct[1]);
    DEVICE_TO_CLOUD(Z2, s_pct[2]);
    DEVICE_TO_CLOUD(Z3, s_pct[3]);
    DEVICE_TO_CLOUD(Z4, s_pct[4]);

    if (++s_sampleCount >= SUMMARY_EVERY_N)
    {
        s_sampleCount = 0;
        String json = "{\"ch\":[";
        for (int i = 0; i < 5; ++i)
        {
            if (i > 0) json += ",";
            json += String(s_pct[i], 1);
        }
        json += "]}";
        DEVICE_TO_CLOUD(Z5, json);
        Serial.printf("[Aggregator] %s\n", json.c_str());
    }
}

void setup()
{
    Serial.begin(115200);

    zeno.wifi(WIFI_SSID, WIFI_PASS)
        .device(DEVICE_ID, DEVICE_TOKEN)
        .enableZKeys()
        .begin();
}

void loop()
{
    zeno.loop();
}

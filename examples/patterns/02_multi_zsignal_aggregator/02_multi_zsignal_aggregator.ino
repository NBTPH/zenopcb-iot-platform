/**
 * @file 02_multi_zsignal_aggregator.ino
 * @brief Read 5 analog channels every 2 s, publish each as Z0..Z4, and every
 *        10 s also publish a combined JSON summary on Z5.
 *
 * What you'll learn:
 *   - Publishing multiple independent values from a single ZENO_EVERY block.
 *   - Building a small JSON string by hand for compact transport.
 *   - The "sub-cadence" pattern: do A every N samples, B every M*N samples,
 *     all inside one block (library limit: one ZENO_EVERY per translation unit).
 *
 * Hardware needed:
 *   - Any supported board.
 *   - Up to 5 analog inputs on A0..A4 (or the per-board equivalents).
 *
 * Wiring:
 *   - Each analog source (pot, sensor) -> matching pin in ADC_PINS[].
 *   - On ESP8266 (only one ADC pin, A0) all five entries point to A0 — that's
 *     a demo workaround so the sketch still compiles; in real use you'd add
 *     a multiplexer for true 5-channel reading.
 *
 * Cloud dashboard setup:
 *   - Create Z0..Z4 of type Float — one per channel, value in percent.
 *   - Create Z5 of type String — receives the combined JSON summary every 10 s.
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
 * 1. Replace WIFI_SSID / WIFI_PASS / DEVICE_ID / DEVICE_TOKEN below.
 * 2. ESP32 only: Tools > Partition Scheme > "Minimal SPIFFS (1.9MB APP)".
 * 3. Flash the board.
 * 4. Open Serial Monitor at 115200 baud.
 * 5. Each loop iteration reads 5 analog channels and writes them to Z0..Z4
 *    (normalised to 0..100% using the board's ADC bit depth). Every 10 s
 *    (every 5 samples at 2 s cadence) the JSON summary is also written to
 *    Z5 — useful when downstream consumers prefer one event over five.
 *
 * Tips & common mistakes:
 *   - Only ONE ZENO_EVERY block is allowed per sketch. To get multiple
 *     cadences, sub-divide inside that block using a sample counter — that's
 *     what SUMMARY_EVERY_N does here.
 *   - The JSON summary is built by string concatenation for clarity. For
 *     production-grade JSON, use the ZenoJson helpers in the library.
 */

#include <ZenoPCBMain.h>

using namespace ZenoPCB;

// ---- Credentials ------------------------------------------------------------
#define WIFI_SSID "REPLACE_ME"
#define WIFI_PASS "REPLACE_ME"
#define DEVICE_ID "REPLACE_ME"
#define DEVICE_TOKEN "REPLACE_AT_PROVISIONING"

// ---- Pin map + ADC scale ----------------------------------------------------
#if defined(ESP32)
  static const int   ADC_PINS[5] = { 32, 33, 34, 35, 36 };
  #define ADC_FULL 4095.0f
#elif defined(ESP8266)
  // ESP8266 has only one ADC pin (A0); we sample it 5x for demo purposes.
  // In real use, add an analog multiplexer for true 5-channel reading.
  static const int   ADC_PINS[5] = { A0, A0, A0, A0, A0 };
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
static const uint32_t SAMPLE_MS    = 2000; // sample all 5 channels every 2 s
// Summary is emitted every SUMMARY_EVERY_N samples (so 5 * 2000 ms = 10 s).
// Library limitation: only one ZENO_EVERY block per translation unit, so the
// summary cadence is sub-divided inside the same block rather than declared
// as a second block.
static const uint8_t  SUMMARY_EVERY_N = 5;

static float   s_pct[5]      = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
static uint8_t s_sampleCount = 0;

// Device -> Cloud: every SAMPLE_MS, read all 5 channels and publish them.
ZENO_EVERY(SAMPLE_MS)
{
    // 1) Sample all channels and convert raw ADC counts to a percentage.
    for (int i = 0; i < 5; ++i)
    {
        s_pct[i] = (float)analogRead(ADC_PINS[i]) / ADC_FULL * 100.0f;
    }
    // 2) Stream each channel out on its own key.
    DEVICE_TO_CLOUD(Z0, s_pct[0]);
    DEVICE_TO_CLOUD(Z1, s_pct[1]);
    DEVICE_TO_CLOUD(Z2, s_pct[2]);
    DEVICE_TO_CLOUD(Z3, s_pct[3]);
    DEVICE_TO_CLOUD(Z4, s_pct[4]);

    // 3) Every Nth sample, also emit a combined JSON summary on Z5.
    if (++s_sampleCount >= SUMMARY_EVERY_N)
    {
        s_sampleCount = 0;
        String json = "{\"ch\":[";
        for (int i = 0; i < 5; ++i)
        {
            if (i > 0) json += ",";
            json += String(s_pct[i], 1); // one decimal place
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

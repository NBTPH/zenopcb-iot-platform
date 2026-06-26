/**
 * @file 04_serial_passthrough.ino
 * @brief Bidirectional UART bridge: Z0 -> external UART, external UART -> Z1.
 *
 * What you'll learn:
 *   - How to drive a second UART so the USB-debug Serial stays usable
 *   - How to assemble incoming serial bytes into newline-delimited lines
 *   - How to expose a hardware UART to the cloud (both directions)
 *
 * Hardware needed:
 *   - Any supported board with a spare UART
 *   - An external serial device to bridge (sensor, GPS, modem, another MCU)
 *   - 3.3 V / 5 V level-shifter if your external device runs at a different
 *     logic level than the board (most ESP32 pins are NOT 5 V tolerant)
 *   - Jumper wires
 *
 * Wiring:
 *   - DEV_RX_PIN -> external device TX (board listens for incoming bytes)
 *   - DEV_TX_PIN -> external device RX (board sends bytes out)
 *   - Common GND (mandatory)
 *
 * Cloud dashboard setup:
 *   - Create Z0 of type String — write here to send a string out the UART
 *   - Create Z1 of type String — receives one line per newline from the UART
 *
 * @category Communication
 * @level Intermediate
 *
 * @hardware
 *   - Any supported board with a spare UART, plus a serial device to talk to.
 *
 * @wiring
 *   See "Wiring" section above (TX <-> RX cross + common GND).
 *
 * @usage
 *   1. Replace credentials below.
 *   2. Cloud-write any String to Z0 — the sketch writes it verbatim out the UART.
 *   3. The sketch buffers bytes received from the external device into a
 *      128-byte buffer; it flushes that buffer to Z1 on a newline or when full.
 *
 * Tips & common mistakes:
 *   - 5 V devices (e.g. an Arduino UNO) will damage ESP32 GPIOs. Use a
 *     bidirectional level shifter or a TTL-friendly module.
 *   - Without a common ground the line voltages are undefined — RX will see
 *     garbage even though the wiring "looks right".
 *   - This bridge expects newline-delimited lines from the external device.
 *     For binary protocols, replace the '\n' framing with a length-prefixed
 *     or COBS-style decoder.
 */

#include <ZenoPCBMain.h>

using namespace ZenoPCB;

#define WIFI_SSID "REPLACE_ME"
#define WIFI_PASS "REPLACE_ME"
#define DEVICE_ID "REPLACE_ME"
#define DEVICE_TOKEN "REPLACE_AT_PROVISIONING"

// Pick the second UART each board exposes so the USB-CDC debug Serial stays
// free for `Serial.printf`. Pin names + numbers differ by family:
//   - ESP32 / STM32: full hardware UART with explicit RX/TX pin args
//   - ESP8266: only Serial1 TX is available; for true full duplex you'd need
//     SoftwareSerial on a free GPIO
//   - UNO R4: Serial1 maps to fixed pins D0/D1
#if defined(ESP32)
  #define DEV_SERIAL Serial2
  #define DEV_RX_PIN 16
  #define DEV_TX_PIN 17
#elif defined(ESP8266)
  // ESP8266 has only Serial (UART0) and Serial1 (TX only). Use SoftwareSerial
  // on a free GPIO if you need RX too. For simplicity:
  #define DEV_SERIAL Serial1
  #define DEV_RX_PIN 0  // unused on ESP8266 Serial1
  #define DEV_TX_PIN 2
#elif defined(ARDUINO_UNOR4_WIFI)
  // UNO R4 Serial1 is on pin D0 (RX) and D1 (TX) — fixed by the board layout.
  #define DEV_SERIAL Serial1
  #define DEV_RX_PIN 0
  #define DEV_TX_PIN 1
#elif defined(STM32F4)
  #define DEV_SERIAL Serial2
  #define DEV_RX_PIN PA3
  #define DEV_TX_PIN PA2
#elif defined(STM32F1)
  #define DEV_SERIAL Serial2
  #define DEV_RX_PIN PA3
  #define DEV_TX_PIN PA2
#endif

Zeno zeno;
static char     s_rxBuf[128];              // line buffer for incoming bytes
static uint16_t s_rxLen = 0;               // current length, in bytes

// Cloud -> Device: dashboard wrote a String to Z0; send it out the UART.
CLOUD_TO_DEVICE(Z0)
{
    const String out = param.toString();
    DEV_SERIAL.print(out);
    Serial.printf("[Serial] tx (%u bytes)\n", (unsigned)out.length());
}

void setup()
{
    Serial.begin(115200);                  // USB-CDC debug serial

#if defined(ESP32)
    // ESP32 lets us remap UART pins at .begin(): 9600 baud, 8N1, custom RX/TX.
    DEV_SERIAL.begin(9600, SERIAL_8N1, DEV_RX_PIN, DEV_TX_PIN);
#else
    // Other cores use fixed pins for each UART; just pass the baud rate.
    DEV_SERIAL.begin(9600);
#endif

    zeno.wifi(WIFI_SSID, WIFI_PASS)
        .device(DEVICE_ID, DEVICE_TOKEN)
        .enableZKeys()
        .setZPublishInterval(1000)
        .begin();
}

void loop()
{
    // Drain whatever is sitting in the UART receive buffer this iteration.
    while (DEV_SERIAL.available() > 0 && s_rxLen < sizeof(s_rxBuf) - 1)
    {
        const char c = (char)DEV_SERIAL.read();
        if (c == '\n')
        {
            // Newline = end of line. NUL-terminate, publish, reset.
            s_rxBuf[s_rxLen] = '\0';
            DEVICE_TO_CLOUD(Z1, String(s_rxBuf));
            Serial.printf("[Serial] rx -> Z1: %s\n", s_rxBuf);
            s_rxLen = 0;
        }
        else if (c != '\r')                // ignore '\r' so "\r\n" still works
        {
            s_rxBuf[s_rxLen++] = c;
        }
    }
    // Buffer-full guard: flush whatever we have to avoid losing bytes.
    if (s_rxLen >= sizeof(s_rxBuf) - 1)
    {
        s_rxBuf[s_rxLen] = '\0';
        DEVICE_TO_CLOUD(Z1, String(s_rxBuf));
        s_rxLen = 0;
    }
    zeno.loop();
}

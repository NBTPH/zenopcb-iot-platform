/**
 * @file 03_seven_segment_4digit.ino
 * @brief Show a cloud-driven integer (Z0) on a TM1637 4-digit 7-segment display.
 *
 * What you'll learn:
 *   - How to use the TM1637 2-wire protocol (CLK + DIO — not I2C, not SPI)
 *   - How to clamp incoming cloud values to a hardware-safe range
 *   - How pin assignments differ per board family (ESP32 vs STM32 vs UNO R4)
 *
 * Hardware needed:
 *   - Any supported board with WiFi
 *   - TM1637 4-digit 7-segment display module
 *   - Jumper wires
 *
 * Wiring:
 *   - TM1637 CLK -> CLK_PIN (board-specific, see #if block below)
 *   - TM1637 DIO -> DIO_PIN (board-specific)
 *   - TM1637 VCC -> 3.3 V or 5 V (most modules accept either)
 *   - TM1637 GND -> GND
 *
 * Cloud dashboard setup:
 *   - Create Z0 of type Int — value shown on the display (-999 .. 9999)
 *
 * @category Display
 * @level Beginner
 *
 * @hardware
 *   - Any supported board with WiFi.
 *   - TM1637 4-digit 7-segment display module.
 *
 * @wiring
 *   See "Wiring" section above (CLK/DIO pins depend on your board).
 *
 * @lib_deps
 *   `avishorp/TM1637 @ ^1.2.0`
 *
 * @usage
 *   1. Install the TM1637 library.
 *   2. Replace credentials below.
 *   3. Cloud-write an integer to Z0 in the range -999 .. 9999.
 *      Values outside that range are clamped (this display only has 4 digits).
 *
 * Tips & common mistakes:
 *   - TM1637 is NOT I2C even though it uses 2 wires. It is a custom protocol
 *     spoken by the library — don't try to share the bus with I2C devices.
 *   - Negative values use one digit for the minus sign, so the smallest
 *     negative value shown is -999, not -9999.
 *   - Brightness is 0x00 (dim) to 0x0f (max). Lower values save current.
 */

#include <ZenoPCBMain.h>
#include <TM1637Display.h>

using namespace ZenoPCB;

#define WIFI_SSID "REPLACE_ME"
#define WIFI_PASS "REPLACE_ME"
#define DEVICE_ID "REPLACE_ME"
#define DEVICE_TOKEN "REPLACE_AT_PROVISIONING"

// Pick reasonable default pins per board. Override here if those pins are
// already used by another peripheral on your hardware.
#if defined(ESP32)
  #define CLK_PIN 18
  #define DIO_PIN 19
#elif defined(ESP8266)
  #define CLK_PIN 5  // D1 on a NodeMCU pinout
  #define DIO_PIN 4  // D2 on a NodeMCU pinout
#elif defined(ARDUINO_UNOR4_WIFI)
  #define CLK_PIN 6
  #define DIO_PIN 7
#elif defined(STM32F4)
  #define CLK_PIN PB6
  #define DIO_PIN PB7
#elif defined(STM32F1)
  #define CLK_PIN PB6
  #define DIO_PIN PB7
#endif

Zeno          zeno;
TM1637Display display(CLK_PIN, DIO_PIN);

// Cloud -> Device: dashboard pushed a new integer to Z0; refresh the display.
CLOUD_TO_DEVICE(Z0)
{
    long v = (long)param.toInt();
    // Clamp to the displayable range: 4 digits, with a leading "-" eating
    // one digit for negative numbers.
    if (v >  9999) v =  9999;
    if (v < -999)  v = -999;
    display.showNumberDec((int)v, false);  // false = no leading-zero padding
    Serial.printf("[7SEG] Z0 -> %ld\n", v);
}

void setup()
{
    Serial.begin(115200);
    display.setBrightness(0x0f);          // 0x00 (dim) .. 0x0f (max brightness)
    display.clear();

    zeno.wifi(WIFI_SSID, WIFI_PASS)
        .device(DEVICE_ID, DEVICE_TOKEN)
        .enableZKeys()
        .begin();
}

void loop()
{
    zeno.loop();
}

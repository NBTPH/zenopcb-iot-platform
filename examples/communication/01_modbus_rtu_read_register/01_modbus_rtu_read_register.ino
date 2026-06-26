/**
 * @file 01_modbus_rtu_read_register.ino
 * @brief Poll one Modbus RTU holding register over RS-485 and publish the value to Z0.
 *
 * What you'll learn:
 *   - How Modbus RTU works: master/slave addresses, holding registers, baud rate
 *   - How to wire a MAX485 transceiver to an ESP32 UART
 *   - How to poll a register on a fixed cadence and publish results to the cloud
 *
 * Hardware needed:
 *   - ESP32 dev board (Modbus subsystem is ESP32-only in v0.3.0)
 *   - MAX485 / SP485 RS-485 transceiver module (TTL <-> differential pair)
 *   - A Modbus RTU slave device: PLC, energy meter, temperature sensor, etc.
 *   - Twisted-pair cable for the A/B differential lines
 *   - Two 120 ohm termination resistors (one at each end of the bus)
 *   - Jumper wires
 *
 * Wiring (ESP32 + MAX485):
 *   - MAX485 RO     -> ESP32 GPIO 16 (Serial1 RX)
 *   - MAX485 DI     -> ESP32 GPIO 17 (Serial1 TX)
 *   - MAX485 DE+RE  -> ESP32 GPIO 4  (driver-enable, tie DE and RE together)
 *   - MAX485 VCC    -> 5 V
 *   - MAX485 GND    -> GND
 *   - MAX485 A/B    -> slave A/B (use twisted pair)
 *   - 120 ohm termination resistor across A-B at EACH end of the bus
 *
 * Cloud dashboard setup:
 *   - Create Z0 of type Int — receives the polled register value
 *
 * @category Communication
 * @level Intermediate
 *
 * @hardware
 *   - ESP32 + MAX485 + Modbus RTU slave. Modbus stack is ESP32-only.
 *
 * @platform_notes
 *   The Modbus stack is ESP32-only because it relies on ESP32-specific UART
 *   APIs. On ESP8266 / UNO R4 / STM32 this sketch compiles to an empty loop
 *   with a Serial notice — use a third-party Modbus library (e.g.
 *   ArduinoModbus) on those targets.
 *
 * @wiring
 *   See "Wiring" section above.
 *
 * @usage
 *   1. Replace credentials below.
 *   2. Set SLAVE_ID and REGISTER_ADDR to match your slave device's
 *      datasheet (Modbus slave addresses are 1-247; 0 is broadcast).
 *   3. Flash and open the Serial Monitor at 115200 baud.
 *   4. The sketch polls one holding register every 2 seconds and publishes
 *      the raw uint16 value to Z0.
 *
 * Tips & common mistakes:
 *   - RS-485 needs a 120 ohm termination resistor between A and B at EACH
 *     end of the bus. Skipping termination causes random read errors.
 *   - All devices on the bus MUST agree on baud rate (9600 here) and frame
 *     format (8N1 here). Check your slave's datasheet.
 *   - Holding registers (function 03) are 16-bit unsigned. Some sensors
 *     pack signed values or 32-bit floats across two registers — read the
 *     datasheet carefully.
 *   - DE and RE must be tied together so one GPIO can flip the transceiver
 *     between transmit (DE=high, RE=high) and receive (both low).
 */

#include <ZenoPCBMain.h>

using namespace ZenoPCB;

// ============================================
// Credentials — replace before flashing
// ============================================
#define WIFI_SSID "REPLACE_ME"
#define WIFI_PASS "REPLACE_ME"
#define DEVICE_ID "REPLACE_ME"
#define DEVICE_TOKEN "REPLACE_AT_PROVISIONING"

#if defined(ESP32)

#include <modbus/ModbusRTU.h>

// MAX485 transceiver pins
#define MB_RX_PIN 16    // MAX485 RO -> ESP32 (we receive bytes here)
#define MB_TX_PIN 17    // MAX485 DI <- ESP32 (we send bytes here)
#define MB_DE_PIN 4     // MAX485 DE+RE — high = transmit, low = receive

// Modbus protocol settings — must match the slave device
static const uint8_t  SLAVE_ID      = 1;     // 1..247 (0 = broadcast, do NOT use)
static const uint16_t REGISTER_ADDR = 0;     // change to match your slave

Zeno      zeno;
ModbusRTU mb;
static uint32_t s_lastPollMs = 0;
static const uint32_t POLL_MS = 2000;        // poll cadence (2 s) — adjust for your need
static uint16_t s_lastValue = 0;

void setup()
{
    Serial.begin(115200);
    // 9600 baud, 8 data bits, no parity, 1 stop bit — Modbus RTU classic frame.
    // Pin order is (rx, tx) on ESP32.
    Serial1.begin(9600, SERIAL_8N1, MB_RX_PIN, MB_TX_PIN);
    mb.begin(&Serial1, MB_DE_PIN);           // tell ModbusRTU which UART + DE pin to use
    mb.master();                             // we initiate requests (the slave responds)

    zeno.wifi(WIFI_SSID, WIFI_PASS)
        .device(DEVICE_ID, DEVICE_TOKEN)
        .enableZKeys()
        .setZPublishInterval(1000)
        .begin();
}

void loop()
{
    mb.task();                               // drives the Modbus state machine
    const uint32_t now = millis();
    // Only issue a new request if the previous one finished (mb.slave() == 0)
    // AND the 2 s cadence has elapsed.
    if (!mb.slave() && (now - s_lastPollMs >= POLL_MS))
    {
        s_lastPollMs = now;
        uint16_t buf = 0;
        // readHreg = Modbus function code 03 ("read holding registers").
        // The lambda runs asynchronously when the slave's reply arrives.
        mb.readHreg(SLAVE_ID, REGISTER_ADDR, &buf, 1,
            [](Modbus::ResultCode event, uint16_t /*transactionId*/, void* /*data*/) -> bool
            {
                if (event == Modbus::EX_SUCCESS)
                {
                    // Device -> Cloud: publish the freshly read register value
                    DEVICE_TO_CLOUD(Z0, (int32_t)s_lastValue);
                    Serial.printf("[Modbus] hreg[%u] = %u\n",
                                   REGISTER_ADDR, s_lastValue);
                }
                else
                {
                    // Common errors: 0xE2 = timeout (slave not responding),
                    // 0x02 = illegal data address, 0x03 = illegal data value.
                    Serial.printf("[Modbus] read err = 0x%02X\n", event);
                }
                return true;
            });
        s_lastValue = buf;                   // buf is filled when the lambda fires
    }
    zeno.loop();                             // keep WiFi + MQTT + ZENO_EVERY alive
}

#else   // ----- non-ESP32 boards: Modbus stack is ESP32-only in v0.3.0 -----

void setup()
{
    Serial.begin(115200);
    Serial.println(F("[INFO] Modbus RTU stack is ESP32-only see sketch header."));
}

void loop()
{
    // no-op
}

#endif  // ESP32

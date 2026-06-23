/**
 * @file 02_modbus_rtu_write_coil.ino
 * @brief Cloud bool on Z0 -> Modbus RTU "write coil" command to a connected slave.
 *
 * What you'll learn:
 *   - The Modbus "coil" concept: a single bit that can be written remotely
 *     (often used as a relay or output enable on PLCs / energy meters)
 *   - How to bridge a cloud-controlled bool to a real-world output
 *   - How CLOUD_TO_DEVICE blocks turn dashboard writes into device actions
 *
 * Hardware needed:
 *   - ESP32 dev board (Modbus subsystem is ESP32-only in v0.3.0)
 *   - MAX485 / SP485 RS-485 transceiver module
 *   - Modbus RTU slave with at least one writeable coil (most PLCs do)
 *   - Twisted-pair cable + 120 ohm termination resistor at each bus end
 *
 * Wiring (ESP32 + MAX485):
 *   Same as 01_modbus_rtu_read_register.ino:
 *   - MAX485 RO     -> ESP32 GPIO 16 (Serial1 RX)
 *   - MAX485 DI     -> ESP32 GPIO 17 (Serial1 TX)
 *   - MAX485 DE+RE  -> ESP32 GPIO 4  (driver-enable, tie together)
 *   - MAX485 VCC    -> 5 V, GND -> GND
 *   - MAX485 A/B    -> slave A/B (twisted pair, terminated at each end)
 *
 * Cloud dashboard setup:
 *   - Create Z0 of type Bool — toggle to write the coil (true = ON, false = OFF)
 *
 * @category Communication
 * @level Intermediate
 *
 * @hardware
 *   - ESP32 only. MAX485 on UART2, slave with a writeable coil.
 *
 * @platform_notes
 *   Modbus stack is ESP32-only — same constraint as
 *   01_modbus_rtu_read_register.ino. On other platforms this sketch becomes
 *   a no-op with a Serial notice.
 *
 * @wiring
 *   See "Wiring" section above (same as the read-register sketch).
 *
 * @usage
 *   1. Replace credentials below.
 *   2. Set SLAVE_ID and COIL_ADDR to match your slave's writeable coil.
 *   3. Flash; open Serial Monitor at 115200 baud.
 *   4. From the dashboard, set Z0 = true or false. Each change emits a
 *      Modbus "write single coil" frame to the slave.
 *
 * Tips & common mistakes:
 *   - "Coils" (function 05) are different from "holding registers" (function 03).
 *     Check the slave datasheet — coil addresses do not overlap with register
 *     addresses even when the numbers match.
 *   - Writes are not confirmed by this sketch. Add a follow-up readCoil call
 *     if you need to verify the slave really accepted the change.
 *   - Same RS-485 wiring rules apply: 120 ohm termination at each end,
 *     correct baud rate (9600 here), DE and RE tied together.
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

#define MB_RX_PIN 16    // MAX485 RO  -> ESP32
#define MB_TX_PIN 17    // MAX485 DI  <- ESP32
#define MB_DE_PIN 4     // MAX485 DE+RE driver enable

// Modbus protocol settings — must match the slave
static const uint8_t  SLAVE_ID  = 1;     // 1..247, never 0 (broadcast)
static const uint16_t COIL_ADDR = 0;     // writeable coil address on the slave

Zeno      zeno;
ModbusRTU mb;

// Cloud -> Device: dashboard wrote a new bool to Z0; relay it onto the bus.
CLOUD_TO_DEVICE(Z0)
{
    const bool on = param.toBool();
    // Modbus function 05 ("write single coil"). nullptr = fire-and-forget;
    // pass a result callback here if you want delivery confirmation.
    mb.writeCoil(SLAVE_ID, COIL_ADDR, on, nullptr);
    Serial.printf("[Modbus] writeCoil[%u] = %s\n",
                   COIL_ADDR, on ? "ON" : "OFF");
}

void setup()
{
    Serial.begin(115200);
    // 9600 baud, 8N1 — standard Modbus RTU framing.
    Serial1.begin(9600, SERIAL_8N1, MB_RX_PIN, MB_TX_PIN);
    mb.begin(&Serial1, MB_DE_PIN);
    mb.master();                          // we send requests; the slave responds

    zeno.wifi(WIFI_SSID, WIFI_PASS)
        .device(DEVICE_ID, DEVICE_TOKEN)
        .enableZKeys()
        .begin();
}

void loop()
{
    mb.task();                            // drives the Modbus state machine
    zeno.loop();                          // pumps WiFi + MQTT + ZSignal callbacks
}

#else   // ----- non-ESP32: Modbus stack is ESP32-only in v0.3.0 -----

void setup()
{
    Serial.begin(115200);
    Serial.println(F("[INFO] Modbus RTU stack is ESP32-only see sketch header."));
}

void loop() {}

#endif

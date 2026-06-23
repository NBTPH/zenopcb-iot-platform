/**
 * @file ZenoNetworkProvider.h
 * @brief Abstract interface for pluggable network providers
 *
 * Cho php th vin bn ngoi (Zeno4G, ZenoEthernetW5500, ZenoMultiConnect)
 * cung cp kt ni mng cho ZenoPCB m core library khng ph thuc
 * vo bt k hardware driver no.
 *
 * User to provider, ri truyn vo Zeno class:
 * @code
 * #include <ZenoEthernetW5500.h>
 * ZenoEthernetProvider ethProvider(5, 26);
 * zeno.setNetworkProvider(&ethProvider).begin();
 * @endcode
 *
 * @author ZenoPCB Team
 */

#pragma once

#include <Arduino.h>
#include <Client.h>

namespace ZenoPCB
{

    // Forward declaration full definition in ZenoPCBTypes.h
    struct DeviceConfig;

    /**
     * @brief Abstract base class for all network providers
     *
     * Implement this interface khi to network provider mi.
     * Mi provider cung cp:
     * - begin()        khi to hardware + kt ni mng
     * - loop()         bo tr kt ni (DHCP, GPRS reconnect, ...)
     * - isConnected()  kim tra trng thi
     * - getClient()    tr v Client* cho MQTT s dng
     */
    class ZenoNetworkProvider
    {
    public:
        virtual ~ZenoNetworkProvider() = default;

        /**
         * @brief Khi to network hardware v kt ni
         *
         * c gi t Zeno::begin() sau khi DeviceConfig  load t NVS.
         * Provider c config lin quan (APN, IP tnh, DHCP, ...) t y.
         *
         * @param config DeviceConfig cha thng tin t NVS provisioning
         * @return true nu hardware OK (khng nht thit phi connected ngay)
         */
        virtual bool begin(const DeviceConfig &config) = 0;

        /**
         * @brief Bo tr kt ni  gi t Zeno::loop() mi vng lp
         *
         * Thc hin: DHCP renewal, GPRS reconnect, link check, v.v.
         */
        virtual void loop() = 0;

        /**
         * @brief Kim tra kt ni mng c sn sng khng
         * @return true nu c link + IP hp l
         */
        virtual bool isConnected() const = 0;

        /**
         * @brief Ly Client* cho MQTT s dng
         *
         * Client* phi tn ti sut lifetime ca provider.
         * MQTT s dng pointer ny trc tip.
         *
         * @return Pointer n Arduino Client (EthernetClient, TinyGsmClient, ...)
         */
        virtual Client *getClient() = 0;

        /**
         * @brief Ly Client* ring cho OTA (khng nh hng MQTT)
         *
         * OTA v MQTT phi dng TCP connection C LP.
         * Nu chia s cng 1 Client*: OTA connect() s kill TCP ca MQTT.
         *
         * Default: return getClient() (fallback cho provider cha override).
         * Cc provider c dedicated OTA client (Ethernet, 4G) nn override.
         *
         * @return Pointer n Arduino Client ring cho OTA
         */
        virtual Client *getOTAClient() { return getClient(); }

        /**
         * @brief Ly a ch IP hin ti
         * @return IP di dng String, "0.0.0.0" nu cha kt ni
         */
        virtual String getLocalIP() const = 0;

        /**
         * @brief Tn provider cho logging
         * @return Chui tnh, v d: "Ethernet", "4G", "MultiConnect"
         */
        virtual const char *getName() const = 0;

        /**
         * @brief ng b thi gian qua network provider
         *
         * Cho php provider t sync time bng cch ring (v d: modem NTP
         * qua AT+CNTP cho 4G). Nu thnh cng, provider set ESP32 RTC
         * bng settimeofday() v return true.
         *
         * Default: return false (provider khng h tr, dng configTime fallback).
         *
         * @return true nu sync thnh cng v ESP32 RTC  c set
         * @return false nu provider khng h tr hoc sync tht bi
         */
        virtual bool syncTime() { return false; }

        // ============================================
        // Diagnostics support override in providers
        // ============================================

        /** @brief Signal quality (WiFi: RSSI dBm, 4G: CSQ 0-31, Ethernet: link speed proxy) */
        virtual int16_t getSignalQuality() const { return 0; }

        /** @brief Network operator name (4G only, e.g. "Viettel") */
        virtual String getOperator() const { return ""; }

        /** @brief Network type string ("LTE", "3G", "WiFi", "Ethernet") */
        virtual String getNetworkType() const { return ""; }

        /** @brief Modem IMEI  unique hardware ID for cellular modules */
        virtual String getModemIMEI() const { return ""; }

        /** @brief MAC address or equivalent unique ID (IMEI for 4G) */
        virtual String getMACAddress() const { return ""; }

        // ============================================
        // Pause support pause reconnection during AP mode
        // ============================================

        /** @brief Pause network maintenance (reconnect, etc.) */
        void setPaused(bool paused) { _paused = paused; }

        /** @brief Check if provider is paused */
        bool isPaused() const { return _paused; }

    protected:
        bool _paused = false;
    };

} // namespace ZenoPCB

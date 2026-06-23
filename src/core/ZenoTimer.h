#ifndef ZENOPCB_ZENO_TIMER_H
#define ZENOPCB_ZENO_TIMER_H

/**
 * @file ZenoTimer.h
 * @brief Periodic-callback dispatcher used by the ZENO_EVERY macro.
 *
 * v0.4.0 introduced declarative periodic publish via ZENO_EVERY(ms) blocks
 * that register themselves at static-init time. ZenoTimer is the runtime
 * that:
 *   1. Receives queued registrations from ZenoTimerRegistrar ctors (before
 *      main()).
 *   2. Installs them into a fixed-size active pool when Zeno::begin() calls
 *      commitPending().
 *   3. Fires due callbacks from Zeno::loop() via runDue(millis()).
 *   4. Fires ALL callbacks on GET_ALL request via runAll() so the cloud
 *      always sees a fresh state snapshot.
 *
 * Init order safety: the pending queue is a function-local static (Meyer's
 * singleton), so it's guaranteed initialised on first push regardless of
 * the order static objects are constructed across translation units.
 */

#include <Arduino.h>
#include <functional>
#include "ZKeyTypes.h"

namespace ZenoPCB
{

    // 1 Hz hard floor. Even if user writes ZENO_EVERY(100), the dispatcher
    // throttles to 1000 ms minimum to protect the MQTT broker from spam.
    // Same as Z_KEY_MIN_PUBLISH_INTERVAL.
    constexpr uint32_t ZENO_TIMER_MIN_INTERVAL = 1000;

    // Per-platform active-slot count. Tuned to F103's 20 KB SRAM.
#if defined(STM32F1xx)
    constexpr uint8_t ZENO_TIMER_MAX = 8;
#elif defined(STM32F4xx) || defined(ESP8266) || defined(ARDUINO_UNOR4_WIFI)
    constexpr uint8_t ZENO_TIMER_MAX = 32;
#else
    constexpr uint8_t ZENO_TIMER_MAX = 64; // ESP32 + anything else
#endif

    using ZenoTimerCallback = std::function<void()>;

    class ZenoTimer
    {
    public:
        static ZenoTimer &getInstance();

        // Called from ZenoTimerRegistrar ctor at static-init time.
        // Auto-clamps intervalMs >= ZENO_TIMER_MIN_INTERVAL.
        static void queuePending(uint32_t intervalMs, void (*cb)());

        // Drain pending queue into _slots[]. Called from Zeno::begin().
        // Safe to call multiple times (idempotent if queue empty).
        void commitPending();

        // Fire any callback whose interval has elapsed. Called from Zeno::loop().
        void runDue(unsigned long now);

        // Fire ALL active callbacks immediately. Called from the GET_ALL
        // MQTT handler so the response telemetry includes fresh values from
        // every periodic publisher.
        void runAll();

        uint8_t count() const { return _count; }

    private:
        struct TimerSlot
        {
            uint32_t intervalMs;
            unsigned long lastRunMs;
            ZenoTimerCallback callback;
        };

        TimerSlot _slots[ZENO_TIMER_MAX];
        uint8_t _count = 0;

        ZenoTimer() = default;
        ZenoTimer(const ZenoTimer &) = delete;
        ZenoTimer &operator=(const ZenoTimer &) = delete;
    };

    /**
     * @brief Static-init helper for ZENO_EVERY(ms) macro.
     *
     * Each ZENO_EVERY block at file scope generates a global object whose
     * constructor pushes (interval, callback) into ZenoTimer's pending queue.
     * Zeno::begin() later installs the queue into the active slots.
     */
    class ZenoTimerRegistrar
    {
    public:
        ZenoTimerRegistrar(uint32_t intervalMs, void (*cb)())
        {
            ZenoTimer::queuePending(intervalMs, cb);
        }
    };

    /**
     * @brief Static-init helper for CLOUD_TO_DEVICE(Zx) macro.
     *
     * Mirrors ZenoTimerRegistrar but for ZKey change handlers. Defined here
     * (not in ZKeyBuffer.h) so the user only needs to include ZenoPCBMain.h
     * to get all three macros working.
     */
    class ZKeyHandlerRegistrar
    {
    public:
        ZKeyHandlerRegistrar(ZKey key, void (*cb)(ZKey, const ZValue &));
    };

} // namespace ZenoPCB

#endif // ZENOPCB_ZENO_TIMER_H

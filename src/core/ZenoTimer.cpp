#include "ZenoTimer.h"
#include "ZKeyBuffer.h"
#include "ZenoPCBDebug.h"

#include <vector>

namespace ZenoPCB
{

    // ============================================================================
    // Pending queues (Meyer's singletons — init-order safe across TUs)
    //
    // Each user file with ZENO_EVERY/CLOUD_TO_DEVICE blocks generates global
    // registrar objects. Their ctors run before main() and push into these
    // queues. Zeno::begin() then drains them via commitPending() and
    // ZKeyBuffer::commitPendingHandlers().
    // ============================================================================

    namespace
    {
        struct PendingTimer
        {
            uint32_t intervalMs;
            void (*cb)();
        };

        struct PendingHandler
        {
            ZKey key;
            void (*cb)(ZKey, const ZValue &);
        };

        std::vector<PendingTimer> &pendingTimers()
        {
            static std::vector<PendingTimer> q;
            return q;
        }

        std::vector<PendingHandler> &pendingHandlers()
        {
            static std::vector<PendingHandler> q;
            return q;
        }
    } // namespace

    // ============================================================================
    // ZenoTimer
    // ============================================================================

    ZenoTimer &ZenoTimer::getInstance()
    {
        static ZenoTimer instance;
        return instance;
    }

    void ZenoTimer::queuePending(uint32_t intervalMs, void (*cb)())
    {
        if (cb == nullptr)
            return;
        if (intervalMs < ZENO_TIMER_MIN_INTERVAL)
            intervalMs = ZENO_TIMER_MIN_INTERVAL;
        pendingTimers().push_back({intervalMs, cb});
    }

    void ZenoTimer::commitPending()
    {
        auto &queue = pendingTimers();
        for (const auto &entry : queue)
        {
            if (_count >= ZENO_TIMER_MAX)
            {
                ZENO_LOG_CORE("ZenoTimer: max %u slots exceeded, dropping extra ZENO_EVERY blocks", (unsigned)ZENO_TIMER_MAX);
                break;
            }
            _slots[_count].intervalMs = entry.intervalMs;
            _slots[_count].lastRunMs = 0; // 0 = fire immediately on first runDue()
            _slots[_count].callback = entry.cb;
            _count++;
        }
        queue.clear();
        queue.shrink_to_fit();
    }

    void ZenoTimer::runDue(unsigned long now)
    {
        for (uint8_t i = 0; i < _count; i++)
        {
            TimerSlot &s = _slots[i];
            if (!s.callback)
                continue;
            if (s.lastRunMs == 0 || (now - s.lastRunMs) >= s.intervalMs)
            {
                s.lastRunMs = now;
                s.callback();
            }
        }
    }

    void ZenoTimer::runAll()
    {
        const unsigned long now = millis();
        for (uint8_t i = 0; i < _count; i++)
        {
            TimerSlot &s = _slots[i];
            if (!s.callback)
                continue;
            s.lastRunMs = now;
            s.callback();
        }
    }

    // ============================================================================
    // ZKeyHandlerRegistrar — registers into ZKeyBuffer's pending handler queue
    // ============================================================================

    ZKeyHandlerRegistrar::ZKeyHandlerRegistrar(ZKey key, void (*cb)(ZKey, const ZValue &))
    {
        if (cb == nullptr)
            return;
        pendingHandlers().push_back({key, cb});
    }

    // ============================================================================
    // Bridge used by ZKeyBuffer::commitPendingHandlers() — kept here so the
    // pending queue stays a module-local static.
    // ============================================================================

    void zenoTimer_drainPendingHandlers(void (*installer)(ZKey, void (*)(ZKey, const ZValue &)))
    {
        auto &queue = pendingHandlers();
        for (const auto &entry : queue)
        {
            installer(entry.key, entry.cb);
        }
        queue.clear();
        queue.shrink_to_fit();
    }

} // namespace ZenoPCB

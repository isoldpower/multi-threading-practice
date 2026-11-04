#pragma once

#include <atomic>
#include <chrono>
#include <cstddef>
#include <semaphore>

namespace multithreading::utilities::threads {

    /** @brief Lets consumers sleep until a producer publishes an item, while producers pay for
     * the semaphore only when somebody is actually asleep on it.
     *
     * A producer publishes its item, then checks for waiters; a waiter registers, then checks
     * for items. A seq_cst fence on each side guarantees at least one of them sees the other, so
     * an item is never published unseen while its consumer goes to sleep. */
    class ItemSignal {
    private:
        std::counting_semaphore<> items_available{0};
        std::atomic<size_t> waiters{0};

        /** Keeps the waiter count right even if taking an item throws. */
        class WaiterRegistration {
        private:
            std::atomic<size_t>& waiters;

        public:
            explicit WaiterRegistration(std::atomic<size_t>& waiters)
                : waiters(waiters) {
                waiters.fetch_add(1, std::memory_order_relaxed);
                std::atomic_thread_fence(std::memory_order_seq_cst);
            }

            ~WaiterRegistration() {
                waiters.fetch_sub(1, std::memory_order_relaxed);
            }

            WaiterRegistration(const WaiterRegistration&) = delete;
            WaiterRegistration& operator=(const WaiterRegistration&) = delete;
            WaiterRegistration(WaiterRegistration&&) = delete;
            WaiterRegistration& operator=(WaiterRegistration&&) = delete;
        };

    public:
        /** @brief Called by a producer once its item is visible to consumers. */
        void notify() {
            std::atomic_thread_fence(std::memory_order_seq_cst);
            if (waiters.load(std::memory_order_relaxed) != 0) {
                items_available.release(1);
            }
        }

        /** @brief Calls @p try_take until it yields a value or @p timeout passes, sleeping
         * between attempts. @p try_take returns an optional-like value; its last attempt is
         * returned either way. */
        template <typename TTake>
        auto wait_for(const std::chrono::steady_clock::duration& timeout, const TTake& try_take)
            -> decltype(try_take()) {
            const auto deadline = std::chrono::steady_clock::now() + timeout;
            const WaiterRegistration registration(waiters);

            while (true) {
                // Checked before every sleep: an item published before this thread registered
                // came with no wake-up for it
                if (auto result = try_take(); result.has_value()) {
                    return result;
                }

                const auto now_time = std::chrono::steady_clock::now();
                if (now_time >= deadline || !items_available.try_acquire_for(deadline - now_time)) {
                    return try_take();
                }
                // Woken, though another consumer may have taken the item first; look again
            }
        }
    };
}  // namespace multithreading::utilities::threads

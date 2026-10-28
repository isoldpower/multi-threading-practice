#pragma once

#include <cstddef>
#include <thread>

namespace multithreading::structures_scenarios {

    constexpr size_t SPINS_BEFORE_YIELD = 100;

    /** @brief Retries @p attempt until it returns true, giving up the core every
     * SPINS_BEFORE_YIELD failures. Every scenario waits through this, so no structure is timed
     * with a kinder or harsher wait than another, and runs with more threads than cores still
     * let the thread they wait on make progress. */
    template <typename TAttempt>
    void retry_with_backoff(const TAttempt& attempt) {
        size_t failures = 0;
        while (!attempt()) {
            if (++failures == SPINS_BEFORE_YIELD) {
                std::this_thread::yield();
                failures = 0;
            }
        }
    }
}  // namespace multithreading::structures_scenarios

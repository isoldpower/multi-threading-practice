#pragma once

#include <cstddef>
#include <thread>

namespace multithreading::structures_scenarios {

    constexpr size_t SPINS_BEFORE_YIELD = 100;

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

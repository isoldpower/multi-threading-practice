#pragma once

#include <utilities/threads/RandomGenerator.h>

namespace multithreading::utilities {

    template <typename TResult, typename TCallable, typename TCallable2>
    inline TResult random_event(const double chance, TCallable first, TCallable2 second) {
        double const random_value = threads::generate_random_double();

        // An if rather than ?: : clang-tidy 18's analyzer reports a false leak when both
        // branches yield a prvalue std::unique_ptr.
        if (random_value > chance) {
            return second();
        }
        return first();
    }
}  // namespace multithreading::utilities
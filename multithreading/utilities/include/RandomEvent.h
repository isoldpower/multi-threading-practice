#pragma once

#include "./threads/RandomGenerator.h"


namespace multithreading::utilities {

    template <typename TResult, typename TCallable, typename TCallable2>
    inline TResult random_event(const double chance, TCallable first, TCallable2 second) {
        double const random_value = threads::generate_random_double();

        return random_value > chance ? second() : first();
    };
} // namespace multithreading::utilities
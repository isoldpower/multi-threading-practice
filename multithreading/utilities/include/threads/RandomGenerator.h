#pragma once

#include <functional>
#include <future>
#include <iostream>
#include <random>
#include <thread>
#include <tuple>

namespace multithreading::utilities::threads {

    inline double generate_random_double() {
        static thread_local std::mt19937 generator(std::hash<std::thread::id>{}(std::this_thread::get_id()));
        std::uniform_real_distribution<double> distribution(0.0, 1.0);

        return distribution(generator);
    }

    inline double generate_random_double(const double min, const double max) {
        static thread_local std::mt19937 generator(std::hash<std::thread::id>{}(std::this_thread::get_id()));
        std::uniform_real_distribution<double> distribution(min, max);

        return distribution(generator);
    }
} // namespace multithreading::utilities::threads
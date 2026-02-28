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

    inline size_t generate_random_size_t(const size_t min, const size_t max) {
        static thread_local std::mt19937 generator(std::hash<std::thread::id>{}(std::this_thread::get_id()));
        std::uniform_real_distribution<double> distribution(0, 1);
        double const random_number = distribution(generator);
        double const interpolated_value = static_cast<double>(min) + (static_cast<double>(max - min) * random_number);

        return std::llround(interpolated_value);
    }

    inline double generate_random_double(const double min, const double max) {
        static thread_local std::mt19937 generator(std::hash<std::thread::id>{}(std::this_thread::get_id()));
        std::uniform_real_distribution<double> distribution(min, max);

        return distribution(generator);
    }
} // namespace multithreading::utilities::threads
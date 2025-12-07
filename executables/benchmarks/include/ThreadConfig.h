#pragma once

#include <cstdint>
#include <thread>


namespace executables::benchmarks {

    constexpr size_t THREAD_SIZE = 100;
    constexpr size_t DEQUEUE_TIMEOUT_MS = 1000;
    constexpr size_t QUEUE_SIZE = 10000;
    const size_t THREADS_COUNT = std::thread::hardware_concurrency();
} // namespace executables::benchmarks
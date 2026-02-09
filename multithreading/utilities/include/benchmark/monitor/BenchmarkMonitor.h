#pragma once

#include <atomic>
#include <iostream>
#include <thread>
#include <tuple>

#include "./BenchmarkMeasurement.h"

namespace multithreading::utilities::benchmark {

    constexpr size_t DEFAULT_HEARTBEAT_RATE = 10;

    template <typename ...TMeasures>
    class BenchmarkMonitor {
    private:
        std::tuple<BenchmarkMeasurement<TMeasures>*...> measurements;
        std::chrono::high_resolution_clock::duration heartbeat;

        std::atomic<bool> benchmark_running;
        std::thread heartbeat_thread;
        mutable std::mutex measurements_mutex;

        void monitor_benchmarks() {
            while (benchmark_running.load(std::memory_order_acquire)) {
                {
                    std::scoped_lock lock(measurements_mutex);
                    std::apply([&](auto*... measurements) {
                        (measurements->snapshot(), ...);
                    }, measurements);
                }

                std::this_thread::sleep_for(heartbeat);
            }
        }
    public:
        BenchmarkMonitor(
            const std::tuple<BenchmarkMeasurement<TMeasures>*...>& measurements,
            const size_t heartbeat
        )
            : measurements(measurements)
            , heartbeat(std::chrono::milliseconds(heartbeat))
            , benchmark_running(false)
        {}

        explicit BenchmarkMonitor(
            const std::tuple<BenchmarkMeasurement<TMeasures>*...>& measurements
        )
            : measurements(measurements)
            , heartbeat(std::chrono::milliseconds(DEFAULT_HEARTBEAT_RATE))
            , benchmark_running(false)
        {}

        bool start_monitoring() {
            bool isRunning = benchmark_running.load(std::memory_order_acquire);

            if (isRunning || heartbeat_thread.joinable()) {
                std::cerr << "Could not run BenchmarkMonitor::startMonitoring() as the process "
                             "is already running\n";
                return false;
            } else if (!benchmark_running.compare_exchange_strong(
                isRunning,
                true,
                std::memory_order_release,
                std::memory_order_acquire
            )) {
                std::cerr << "Lost race condition in monitoring process from BenchmarkMonitor\n";
                return false;
            }

            {
                std::scoped_lock lock(measurements_mutex);
                std::apply([&](auto*... measurements) {
                   (measurements->start(), ...);
               }, measurements);
            }

            heartbeat_thread = std::thread(&BenchmarkMonitor::monitor_benchmarks, this);
            return true;
        }

        bool stop_monitoring() {
            bool isRunning = benchmark_running.load(std::memory_order_acquire);
            if (!isRunning || !heartbeat_thread.joinable()) {
                std::cerr << "Could not run BenchmarkMonitor::stopMonitoring() as the process "
                             "is NOT running or was already stopped\n";

                return false;
            } else if (!benchmark_running.compare_exchange_strong(
                isRunning,
                false,
                std::memory_order_release,
                std::memory_order_acquire
            )) {
                std::cerr << "Lost race condition in stopping monitoring process from BenchmarkMonitor\n";

                return false;
            }

            heartbeat_thread.join();
            {
                std::scoped_lock lock(measurements_mutex);
                std::apply([&](auto*... measurements) {
                    (measurements->stop(), ...);
                }, measurements);
            }

            return true;
        }

        std::optional<std::tuple<BenchmarkMeasurementResult<TMeasures>...>> get_results() {
            const bool isRunning = benchmark_running.load(std::memory_order_acquire);
            if (isRunning || heartbeat_thread.joinable()) {
                std::cerr << "Could not run BenchmarkMonitor::get_results() as the benchmark "
                             "is in process\n";

                return std::nullopt;
            }

            std::tuple<BenchmarkMeasurementResult<TMeasures>...> results = [&]() {
                std::scoped_lock lock(measurements_mutex);
                return std::apply([](auto*... measurement) {
                    return std::tuple<BenchmarkMeasurementResult<TMeasures>...>{
                        measurement->get_result().value()...
                    };
                }, measurements);
            }();

            return results;
        }
    };
} // namespace multithreading::utilities::benchmark
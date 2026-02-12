#pragma once

#include <vector>
#include <iostream>

#include "../BenchmarkMeasurer.h"

namespace multithreading::benchmark::views {

    class ConsoleOutput {
    public:
        template <typename ...TArgs>
        static void displayBenchmarkResults(
            std::vector<BenchmarkResult<TArgs...>> results
        ) {
            for (const auto &[threads_count, thread_size, measurements] : results) {
                std::cout << "Threads (" << threads_count << ") Size (" << thread_size << ")\n";
                std::apply([&](const auto&... measurement) {
                    ((std::cout << "\t" << measurement.get_verbose() << ": "
                        << measurement.get_measurement().value() << measurement.get_unit() << std::endl), ...);
                }, measurements);
            }
            std::cout << '\n';
        }
    };
} // namespace multithreading::benchmark::views

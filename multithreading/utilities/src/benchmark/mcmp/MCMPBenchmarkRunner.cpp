#include "../../../include/benchmark/mcmp/MCMPBenchmarkRunner.h"

#include <thread>


namespace multithreading::utilities::benchmark::mcmp {

    MCMPBenchmarkRunner::MCMPBenchmarkRunner(
        const std::shared_ptr<ProducerConsumerBenchmark>& benchmark
    )
        : benchmark(benchmark)
    {}

    void MCMPBenchmarkRunner::run_benchmark_with(
        const BenchmarkMatrixItem& item
    ) {
        const auto half_threads = static_cast<size_t>(floor(item.threads_count / 2));
        std::vector<std::thread> threads {};
        threads.reserve(item.threads_count);

        for (size_t i = 0; i < half_threads; ++i) {
            threads.emplace_back([&]() {
                benchmark->producer_routine(item.thread_size);
            });
        }
        for (size_t j = half_threads; j < item.threads_count; ++j) {
            threads.emplace_back([&]() {
                benchmark->consumer_routine(item.thread_size);
            });
        }

        for (auto& thread : threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }
} // namespace multithreading::utilities::benchmark::mcmp
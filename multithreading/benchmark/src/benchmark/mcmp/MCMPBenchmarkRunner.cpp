#include <benchmark/mcmp/MCMPBenchmarkRunner.h>
#include <utilities/threads/ThreadBarrier.h>

#include <stdexcept>

namespace multithreading::benchmark::mcmp {

    MCMPBenchmarkRunner::MCMPBenchmarkRunner(
        const std::shared_ptr<ProducerConsumerBenchmark>& benchmark
    )
        : benchmark(benchmark) {
    }

    void MCMPBenchmarkRunner::run_benchmark_with(const BenchmarkMatrixItem& item) {
        if (item.threads_count < 2) {
            throw std::invalid_argument("MCMPBenchmarkRunner needs a producer and a consumer");
        }

        // An odd count gets one consumer more than producers, so the produced items are shared
        // out between consumers; otherwise consumers would wait for items nobody produces
        const size_t producers = item.threads_count / 2;
        const size_t consumers = item.threads_count - producers;
        const size_t produced = producers * item.thread_size;

        // Every thread waits at the barrier, so all of them start contending at the same moment
        utilities::threads::ThreadBarrier start_line;
        for (size_t i = 0; i < producers; ++i) {
            start_line.enqueue([this, size = item.thread_size]() {
                benchmark->producer_routine(size);
            });
        }
        for (size_t i = 0; i < consumers; ++i) {
            const size_t share = (produced / consumers) + (i < produced % consumers ? 1 : 0);
            start_line.enqueue([this, share]() {
                benchmark->consumer_routine(share);
            });
        }

        start_line.kickstart();
        start_line.join();
    }
}  // namespace multithreading::benchmark::mcmp

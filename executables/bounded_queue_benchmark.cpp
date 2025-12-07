#include <multithreading/structures/include/bounded_queue/BoundedQueue.h>
#include <multithreading/structures/include/bounded_queue/FGLockBoundedQueue.h>
#include <multithreading/structures/include/bounded_queue/LockFreeBoundedQueue.h>
#include <multithreading/utilities/include/Application.h>
#include <multithreading/utilities/include/benchmark/BenchmarkMatrix.h>
#include <multithreading/utilities/include/benchmark/BenchmarkMeasurer.h>
#include <multithreading/utilities/include/benchmark/MultithreadingTask.h>
#include <multithreading/utilities/include/benchmark/mcmp/MCMPBenchmarkRunner.h>

#include <ostream>

#include "./benchmarks/include/ThreadConfig.h"
#include "./benchmarks/include/mcmp/BoundedQueueMCMPBenchmark.h"

using multithreading::structures::bounded_queue::FGLockBoundedQueue;
using multithreading::structures::bounded_queue::BoundedQueue;
using multithreading::structures::bounded_queue::FGLockBoundedQueue;
using multithreading::structures::bounded_queue::LockFreeBoundedQueue;

using multithreading::utilities::benchmark::BenchmarkTask;
using multithreading::utilities::benchmark::ProducerConsumerBenchmark;
using multithreading::utilities::benchmark::BenchmarkMeasurer;
using multithreading::utilities::benchmark::BenchmarkRunner;
using multithreading::utilities::benchmark::mcmp::MCMPBenchmarkRunner;

using executables::benchmarks::mcmp::BoundedQueueMCMPBenchmark;
using executables::benchmarks::THREADS_COUNT;
using executables::benchmarks::THREAD_SIZE;
using executables::benchmarks::QUEUE_SIZE;


namespace executables {
    template <size_t N>
    static void benchmarkApplication(const std::array<BenchmarkTask<BoundedQueue<int>>, N>& queues) {
        const multithreading::utilities::benchmark::BenchmarkMatrixDefinition matrix {
            .per_thread_sizes = std::vector{ THREAD_SIZE, THREAD_SIZE * 10 },
            .threads_count = std::vector{ THREADS_COUNT, THREADS_COUNT * 2 }
        };
        const BenchmarkMeasurer matrixMeasurer(matrix);

        for (const auto &queue : queues) {
            std::shared_ptr<ProducerConsumerBenchmark> const benchmark =
                std::make_shared<BoundedQueueMCMPBenchmark>(queue.structure);
            std::shared_ptr<BenchmarkRunner> const runner =
                std::make_shared<MCMPBenchmarkRunner>(benchmark);

            std::cout << queue.title << "\n";
            const auto results = matrixMeasurer.measure_benchmark(runner);
            for (const auto &[threads_count, thread_size, execution_time] : results) {
                std::cout << "Threads (" << threads_count << ") Size (" << thread_size << ")\n";
                std::cout << "\tExecution time: " << execution_time.count() << "ns\n";
            }
            std::cout << '\n';
        }
    }
} // namespace executables

auto main() -> int {
    multithreading::utilities::Application benchmarkApplication(
        multithreading::utilities::ApplicationInfo<int>{
            .appName="Unbounded Queue Benchmark",
            .appVersion="1.0.0",
            .beforeTask = std::nullopt,
            .afterTask = std::nullopt
        }
    );
    const std::array queues {
        BenchmarkTask<BoundedQueue<int>>(
            std::make_shared<LockFreeBoundedQueue<int>>(QUEUE_SIZE),
            "Lock-free Lock Queue Benchmark"
        ),
        BenchmarkTask<BoundedQueue<int>>(
            std::make_shared<FGLockBoundedQueue<int>>(QUEUE_SIZE),
            "Fine-Grained Lock Queue Benchmark"
        ),
    };

    try {
        const std::optional<int> executionResult = benchmarkApplication.SafeStart([&]() {
            executables::benchmarkApplication(queues);
            return 1;
        });

        return executionResult.has_value() ? executionResult.value() : -1;
    } catch (std::exception& e) {
        std::cerr << e.what() << '\n';

        return -1;
    }
}

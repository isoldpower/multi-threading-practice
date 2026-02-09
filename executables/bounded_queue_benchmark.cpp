#include <multithreading/structures/include/bounded_queue/BoundedQueue.h>
#include <multithreading/structures/include/bounded_queue/FGLockBoundedQueue.h>
#include <multithreading/structures/include/bounded_queue/LockFreeBoundedQueue.h>
#include <multithreading/utilities/include/Application.h>
#include <multithreading/utilities/include/benchmark/BenchmarkMatrix.h>
#include <multithreading/utilities/include/benchmark/BenchmarkMeasurer.h>
#include <multithreading/utilities/include/benchmark/MultithreadingTask.h>
#include <multithreading/utilities/include/benchmark/mcmp/MCMPBenchmarkRunner.h>
#include <multithreading/utilities/include/benchmark/monitor/SpeedMeasurement.h>
#include <multithreading/utilities/include/benchmark/display/ConsoleOutput.h>

#include <ostream>

#include "./benchmarks/include/ThreadConfig.h"
#include "./benchmarks/include/mcmp/BoundedQueueMCMPBenchmark.h"

using namespace multithreading::structures::bounded_queue;
using namespace multithreading::utilities::benchmark;


namespace executables {

    template <size_t N>
    static void benchmarkApplication(const std::array<BenchmarkTask<BoundedQueue<int>>, N>& queues) {
        const BenchmarkMatrixDefinition matrix {
            .per_thread_sizes = std::vector{ benchmarks::THREAD_SIZE, benchmarks::THREAD_SIZE * 10 },
            .threads_count = std::vector{ benchmarks::THREADS_COUNT, benchmarks::THREADS_COUNT * 2 }
        };
        const auto monitor = std::make_shared<BenchmarkMonitor<DurationType>>(
            RefMeasurementsList<DurationType>(
                new SpeedMeasurement({ .verbose = "Execution Time" })
            )
        );
        const BenchmarkMeasurer matrixMeasurer(matrix, monitor);

        for (const auto &queue : queues) {
            std::shared_ptr<ProducerConsumerBenchmark> const benchmark =
                std::make_shared<benchmarks::mcmp::BoundedQueueMCMPBenchmark>(queue.structure);
            std::shared_ptr<BenchmarkRunner> const runner =
                std::make_shared<mcmp::MCMPBenchmarkRunner>(benchmark);

            std::cout << queue.title << "\n";
            const auto results = matrixMeasurer.measure_benchmark(runner);
            display::displayBenchmarkResults(results);
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
            std::make_shared<LockFreeBoundedQueue<int>>(executables::benchmarks::QUEUE_SIZE),
            "Lock-free Lock Queue Benchmark"
        ),
        BenchmarkTask<BoundedQueue<int>>(
            std::make_shared<FGLockBoundedQueue<int>>(executables::benchmarks::QUEUE_SIZE),
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

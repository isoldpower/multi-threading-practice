#include <multithreading/benchmark/include/BenchmarkMatrix.h>
#include <multithreading/benchmark/include/BenchmarkMeasurer.h>
#include <multithreading/benchmark/include/MultithreadingTask.h>
#include <multithreading/benchmark/include/SpeedMeasurement.h>
#include <multithreading/benchmark/include/mcmp/MCMPBenchmarkRunner.h>
#include <multithreading/benchmark/include/views/ConsoleOutput.h>
#include <multithreading/structures/include/unbounded_queue/FGLockUnboundedQueue.h>
#include <multithreading/structures/include/unbounded_queue/LockFreeUnboundedQueue.h>
#include <multithreading/structures/include/unbounded_queue/UnboundedQueue.h>
#include <multithreading/utilities/include/Application.h>
#include <multithreading/benchmark/include/AverageMemoryMeasurement.h>
#include <multithreading/benchmark/include/PeakMemoryMeasurement.h>

#include <ostream>
#include <memory>

#include "./benchmarks/include/ThreadConfig.h"
#include "./benchmarks/include/mcmp/UnboundedQueueMCMPBenchmark.h"

using namespace multithreading::structures::unbounded_queue;
using namespace multithreading::benchmark;

namespace executables {

    template <size_t N>
    static void benchmarkApplication(const std::array<BenchmarkTask<UnboundedQueue<int>>, N>& queues) {
        const BenchmarkMatrixDefinition matrix {
            .per_thread_sizes = std::vector{ benchmarks::THREAD_SIZE, benchmarks::THREAD_SIZE * 10 },
            .threads_count = std::vector{ benchmarks::THREADS_COUNT, benchmarks::THREADS_COUNT * 2 }
        };
        const auto monitor = std::make_shared<BenchmarkMonitor<DurationType, double, double>>(
            RefMeasurementsList<DurationType, double, double>(
                std::make_unique<SpeedMeasurement>(BenchmarkMeasurementTemplate{ .verbose = "Execution Time" }),
                std::make_unique<PeakMemoryMeasurement>(BenchmarkMeasurementTemplate{ .verbose = "Peak Memory Overhead" }),
                std::make_unique<AverageMemoryMeasurement>(BenchmarkMeasurementTemplate{ .verbose = "Average Memory Usage"  })
            )
        );
        const BenchmarkMeasurer matrixMeasurer(matrix, monitor);

        for (const auto &queue : queues) {
            std::shared_ptr<mcmp::ProducerConsumerBenchmark> const benchmark =
                std::make_shared<benchmarks::mcmp::UnboundedQueueMCMPBenchmark>(queue.structure);
            std::shared_ptr<BenchmarkRunner> const runner =
                std::make_shared<mcmp::MCMPBenchmarkRunner>(benchmark);

            std::cout << queue.title << "\n";
            const auto results = matrixMeasurer.measure_benchmark(runner);
            views::ConsoleOutput::displayBenchmarkResults(results);
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
        BenchmarkTask<UnboundedQueue<int>>(
            std::make_shared<LockFreeUnboundedQueue<int>>(LockFreeQueueConfig{
                .maxUpdateDepth = 10000
            }),
            "Lock-free Queue Benchmark"
        ),
        BenchmarkTask<UnboundedQueue<int>>(
            std::make_shared<FGLockUnboundedQueue<int>>(),
            "Fine-Grained Lock Queue Benchmark"
        )
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

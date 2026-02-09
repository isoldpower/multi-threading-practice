#include <multithreading/structures/include/unbounded_queue/FGLockUnboundedQueue.h>
#include <multithreading/structures/include/unbounded_queue/LockFreeUnboundedQueue.h>
#include <multithreading/structures/include/unbounded_queue/UnboundedQueue.h>
#include <multithreading/utilities/include/Application.h>
#include <multithreading/utilities/include/benchmark/BenchmarkMatrix.h>
#include <multithreading/utilities/include/benchmark/BenchmarkMeasurer.h>
#include <multithreading/utilities/include/benchmark/MultithreadingTask.h>
#include <multithreading/utilities/include/benchmark/mcmp/MCMPBenchmarkRunner.h>
#include <multithreading/utilities/include/benchmark/monitor/SpeedMeasurement.h>
#include <multithreading/utilities/include/benchmark/display/ConsoleOutput.h>
#include <ostream>

#include "./benchmarks/include/ThreadConfig.h"
#include "./benchmarks/include/mcmp/UnboundedQueueMCMPBenchmark.h"

using namespace multithreading::structures::unbounded_queue;
using namespace multithreading::utilities::benchmark;

namespace executables {

    template <size_t N>
    static void benchmarkApplication(const std::array<BenchmarkTask<UnboundedQueue<int>>, N>& queues) {
        const BenchmarkMatrixDefinition matrix {
            .per_thread_sizes = std::vector{ benchmarks::THREAD_SIZE, benchmarks::THREAD_SIZE * 10 },
            .threads_count = std::vector{ benchmarks::THREADS_COUNT, benchmarks::THREADS_COUNT * 2 }
        };
        const auto monitor = std::make_shared<BenchmarkMonitor<DurationType>>(RefMeasurementsList<DurationType>(
            new SpeedMeasurement({ .verbose = "Execution Time" })
        ));
        const BenchmarkMeasurer matrixMeasurer(matrix, monitor);

        for (const auto &queue : queues) {
            std::shared_ptr<ProducerConsumerBenchmark> const benchmark =
                std::make_shared<benchmarks::mcmp::UnboundedQueueMCMPBenchmark>(queue.structure);
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

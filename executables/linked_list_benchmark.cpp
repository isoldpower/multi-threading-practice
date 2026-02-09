#include <multithreading/structures/include/linked_list/FGLockLinkedList.h>
#include <multithreading/structures/include/linked_list/LinkedList.h>
#include <multithreading/structures/include/linked_list/LockFreeLinkedList.h>
#include <multithreading/utilities/include/Application.h>
#include <multithreading/utilities/include/benchmark/BenchmarkMatrix.h>
#include <multithreading/utilities/include/benchmark/BenchmarkMeasurer.h>
#include <multithreading/utilities/include/benchmark/MultithreadingTask.h>
#include <multithreading/utilities/include/benchmark/mcmp/MCMPBenchmarkRunner.h>
#include <multithreading/utilities/include/benchmark/monitor/BenchmarkMonitor.h>
#include <multithreading/utilities/include/benchmark/monitor/SpeedMeasurement.h>
#include <multithreading/utilities/include/benchmark/display/ConsoleOutput.h>
#include <array>
#include <list>

#include "./benchmarks/include/ThreadConfig.h"
#include "./benchmarks/include/mcmp/LinkedListMCMPBenchmark.h"

using namespace multithreading::structures::linked_list;
using namespace multithreading::utilities::benchmark;

namespace executables {

    template <size_t N>
    static void benchmarkApplication(const std::array<BenchmarkTask<LinkedList<int>>, N>& lists) {
        const BenchmarkMatrixDefinition matrix {
            .per_thread_sizes = std::vector{ benchmarks::THREAD_SIZE, benchmarks::THREAD_SIZE * 10 },
            .threads_count = std::vector{ benchmarks::THREADS_COUNT, benchmarks::THREADS_COUNT * 2 }
        };
        const auto monitor = std::make_shared<BenchmarkMonitor<DurationType>>(RefMeasurementsList<DurationType>(
            new SpeedMeasurement({ .verbose = "Execution Time" })
        ));
        const BenchmarkMeasurer matrixMeasurer(matrix, monitor);

        for (const auto &list : lists) {
            std::shared_ptr<ProducerConsumerBenchmark> const benchmark =
                std::make_shared<benchmarks::mcmp::LinkedListMCMPBenchmark>(list.structure);
            std::shared_ptr<BenchmarkRunner> const runner =
                std::make_shared<mcmp::MCMPBenchmarkRunner>(benchmark);

            std::cout << list.title << "\n";
            const auto results = matrixMeasurer.measure_benchmark(runner);
            display::displayBenchmarkResults(results);
        }
    }
} // namespace executables

auto main() -> int {
    multithreading::utilities::Application benchmarkApplication(
        multithreading::utilities::ApplicationInfo<int>{
            .appName="Linked List Benchmark",
            .appVersion="1.0.0",
            .beforeTask = std::nullopt,
            .afterTask = std::nullopt
        }
    );

    const std::array lists {
        BenchmarkTask<LinkedList<int>>(
            std::make_shared<LockFreeLinkedList<int>>(),
            "Lock-free Linked List Benchmark"
        ),
        BenchmarkTask<LinkedList<int>>(
            std::make_shared<FGLockLinkedList<int>>(),
            "Fine-Grained Linked List Benchmark"
        ),
    };

    try {
        const std::optional<int> executionResult = benchmarkApplication.SafeStart([&]() {
            executables::benchmarkApplication(lists);
            return 1;
        });

        return executionResult.has_value() ? executionResult.value() : -1;
    } catch (std::exception& e) {
        std::cerr << e.what() << '\n';

        return -1;
    }
}
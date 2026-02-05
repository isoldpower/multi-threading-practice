#include <multithreading/utilities/include/Application.h>
#include <multithreading/utilities/include/benchmark/BenchmarkMatrix.h>
#include <multithreading/utilities/include/benchmark/BenchmarkMeasurer.h>
#include <multithreading/utilities/include/benchmark/MultithreadingTask.h>
#include <multithreading/utilities/include/benchmark/mcmp/MCMPBenchmarkRunner.h>

#include <multithreading/structures/include/linked_list/LinkedList.h>
#include <multithreading/structures/include/linked_list/LockFreeLinkedList.h>
#include <multithreading/structures/include/linked_list/FGLockLinkedList.h>

#include <array>
#include <ostream>
#include <cstdint>

#include "./benchmarks/include/ThreadConfig.h"
#include "./benchmarks/include/mcmp/LinkedListMCMPBenchmark.h"

using multithreading::structures::linked_list::LinkedList;
using multithreading::structures::linked_list::LockFreeLinkedList;
using multithreading::structures::linked_list::FGLockLinkedList;

using multithreading::utilities::benchmark::BenchmarkTask;
using multithreading::utilities::benchmark::ProducerConsumerBenchmark;
using multithreading::utilities::benchmark::BenchmarkMeasurer;
using multithreading::utilities::benchmark::BenchmarkRunner;
using multithreading::utilities::benchmark::mcmp::MCMPBenchmarkRunner;
using executables::benchmarks::mcmp::LinkedListMCMPBenchmark;

using executables::benchmarks::THREADS_COUNT;
using executables::benchmarks::THREAD_SIZE;

namespace executables {
    template <size_t N>
    static void benchmarkApplication(const std::array<BenchmarkTask<LinkedList<int>>, N>& lists) {
        const multithreading::utilities::benchmark::BenchmarkMatrixDefinition matrix {
            .per_thread_sizes = std::vector{ THREAD_SIZE, THREAD_SIZE * 10 },
            .threads_count = std::vector{ THREADS_COUNT, THREADS_COUNT * 2 }
        };
        const BenchmarkMeasurer matrixMeasurer(matrix);

        for (const auto &list : lists) {
            std::shared_ptr<ProducerConsumerBenchmark> const benchmark =
                std::make_shared<LinkedListMCMPBenchmark>(list.structure);
            std::shared_ptr<BenchmarkRunner> const runner =
                std::make_shared<MCMPBenchmarkRunner>(benchmark);

            std::cout << list.title << "\n";
            const auto results = matrixMeasurer.measure_benchmark(runner);
            for (const auto &[threads_count, thread_size, execution_time] : results) {
                std::cout << "Threads (" << threads_count << ") Size (" << thread_size << ")\n";
                std::cout << "\tExecution time: " << execution_time.count() << "mics\n";
            }
            std::cout << '\n';
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
#include <application/runtime/Application.h>
#include <application/structures_scenarios/ThreadConfig.h>
#include <application/structures_scenarios/mcmp/BoundedQueueMCMPBenchmark.h>
#include <benchmark/AverageMemoryMeasurement.h>
#include <benchmark/BenchmarkMatrix.h>
#include <benchmark/BenchmarkMeasurer.h>
#include <benchmark/MultithreadingTask.h>
#include <benchmark/PeakMemoryMeasurement.h>
#include <benchmark/SpeedMeasurement.h>
#include <benchmark/mcmp/MCMPBenchmarkRunner.h>
#include <benchmark/views/ConsoleOutput.h>
#include <structures/bounded_queue/BoundedQueue.h>
#include <structures/bounded_queue/FGLockBoundedQueue.h>
#include <structures/bounded_queue/LockFreeBoundedQueue.h>

#include <iostream>
#include <memory>
#include <ostream>

// NOLINTBEGIN(google-build-using-namespace)
using namespace multithreading::structures::bounded_queue;
using namespace multithreading::benchmark;
// NOLINTEND(google-build-using-namespace)

namespace multithreading::application {

    template <size_t N>
    static void benchmarkApplication(
        const std::array<BenchmarkTask<BoundedQueue<int>>, N>& queues
    ) {
        const BenchmarkMatrixDefinition matrix{
            .per_thread_sizes =
                std::vector{
                    {structures_scenarios::THREAD_SIZE, structures_scenarios::THREAD_SIZE * 10},
                    std::allocator<size_t>{}
                },
            .threads_count = std::vector{
                {structures_scenarios::THREADS_COUNT, structures_scenarios::THREADS_COUNT * 2},
                std::allocator<size_t>{}
            }
        };
        const auto monitor = std::make_shared<BenchmarkMonitor<DurationType, double, double>>(
            RefMeasurementsList<DurationType, double, double>(
                std::make_unique<SpeedMeasurement>(BenchmarkMeasurementTemplate{
                    .verbose = std::string{"Execution Time", std::allocator<char>{}}
                }),
                std::make_unique<PeakMemoryMeasurement>(BenchmarkMeasurementTemplate{
                    .verbose = std::string{"Peak Memory Overhead", std::allocator<char>{}}
                }),
                std::make_unique<AverageMemoryMeasurement>(BenchmarkMeasurementTemplate{
                    .verbose = std::string{"Average Memory Usage", std::allocator<char>{}}
                })
            )
        );
        const BenchmarkMeasurer matrixMeasurer(matrix, monitor);

        for (const auto& queue : queues) {
            std::shared_ptr<mcmp::ProducerConsumerBenchmark> const benchmark =
                std::make_shared<structures_scenarios::mcmp::BoundedQueueMCMPBenchmark>(
                    queue.structure()
                );
            std::shared_ptr<BenchmarkRunner> const runner =
                std::make_shared<mcmp::MCMPBenchmarkRunner>(benchmark);

            std::cout << queue.title() << "\n";
            const auto results = matrixMeasurer.measure_benchmark(runner);
            views::ConsoleOutput::displayBenchmarkResults(results);
        }
    }
}  // namespace multithreading::application

auto main() -> int {
    try {
        multithreading::runtime::Application benchmarkApplication(
            multithreading::runtime::ApplicationInfo<int>{
                .appName = std::string{"Bounded Queue Benchmark", std::allocator<char>{}},
                .appVersion = std::string{"1.0.0", std::allocator<char>{}},
                .beforeTask = std::nullopt,
                .afterTask = std::nullopt
            }
        );
        const std::array queues{
            BenchmarkTask<BoundedQueue<int>>(
                std::make_shared<LockFreeBoundedQueue<int>>(
                    multithreading::structures_scenarios::QUEUE_SIZE
                ),
                std::string{"Lock-free Lock Queue Benchmark", std::allocator<char>{}}
            ),
            BenchmarkTask<BoundedQueue<int>>(
                std::make_shared<FGLockBoundedQueue<int>>(
                    multithreading::structures_scenarios::QUEUE_SIZE
                ),
                std::string{"Fine-Grained Lock Queue Benchmark", std::allocator<char>{}}
            ),
        };

        const std::optional<int> executionResult = benchmarkApplication.SafeStart([&]() {
            multithreading::application::benchmarkApplication(queues);
            return 1;
        });

        return executionResult.has_value() ? executionResult.value() : -1;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';

        return -1;
    } catch (...) {
        std::cerr << "Unknown exception\n";

        return -1;
    }
}

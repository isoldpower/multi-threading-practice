#include <application/structures_scenarios/ThreadConfig.h>
#include <application/structures_scenarios/mcmp/UnboundedQueueMCMPBenchmark.h>
#include <benchmark/AverageMemoryMeasurement.h>
#include <benchmark/BenchmarkMatrix.h>
#include <benchmark/BenchmarkMeasurer.h>
#include <benchmark/MultithreadingTask.h>
#include <benchmark/PeakMemoryMeasurement.h>
#include <benchmark/SpeedMeasurement.h>
#include <benchmark/mcmp/MCMPBenchmarkRunner.h>
#include <benchmark/views/ConsoleOutput.h>
#include <structures/unbounded_queue/FGLockUnboundedQueue.h>
#include <structures/unbounded_queue/LockFreeUnboundedQueue.h>
#include <structures/unbounded_queue/UnboundedQueue.h>
#include <utilities/Application.h>

#include <iostream>
#include <memory>
#include <ostream>

// NOLINTBEGIN(google-build-using-namespace)
using namespace multithreading::structures::unbounded_queue;
using namespace multithreading::benchmark;
// NOLINTEND(google-build-using-namespace)

namespace multithreading::application {

    template <size_t N>
    static void benchmarkApplication(
        const std::array<BenchmarkTask<UnboundedQueue<int>>, N>& queues
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
                std::make_shared<structures_scenarios::mcmp::UnboundedQueueMCMPBenchmark>(
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
        multithreading::utilities::Application benchmarkApplication(
            multithreading::utilities::ApplicationInfo<int>{
                .appName = std::string{"Unbounded Queue Benchmark", std::allocator<char>{}},
                .appVersion = std::string{"1.0.0", std::allocator<char>{}},
                .beforeTask = std::nullopt,
                .afterTask = std::nullopt
            }
        );
        const std::array queues{
            BenchmarkTask<UnboundedQueue<int>>(
                std::make_shared<LockFreeUnboundedQueue<int>>(
                    LockFreeQueueConfig{.maxUpdateDepth = 10000}
                ),
                std::string{"Lock-free Queue Benchmark", std::allocator<char>{}}
            ),
            BenchmarkTask<UnboundedQueue<int>>(
                std::make_shared<FGLockUnboundedQueue<int>>(),
                std::string{"Fine-Grained Lock Queue Benchmark", std::allocator<char>{}}
            )
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

#include <application/computing_scenarios/merge_sort/SortConfig.h>
#include <application/computing_scenarios/merge_sort/SortInputs.h>
#include <application/computing_scenarios/merge_sort/SortScenarios.h>
#include <application/runtime/Application.h>
#include <benchmark/BenchmarkMeasurer.h>
#include <benchmark/views/ConsoleOutput.h>

#include <cstddef>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <vector>

// NOLINTBEGIN(google-build-using-namespace)
using namespace multithreading::computing_scenarios::merge_sort;
using namespace multithreading::benchmark;
// NOLINTEND(google-build-using-namespace)

namespace multithreading::application {

    static void benchmarkApplication(const std::vector<SortScenario>& scenarios) {
        const auto monitor = make_sort_monitor();

        for (const auto& scenario : scenarios) {
            const BenchmarkMeasurer matrixMeasurer(scenario.matrix, monitor);

            std::cout << scenario.task.title() << "\n";
            const auto results = matrixMeasurer.measure_benchmark(scenario.task.structure());
            views::ConsoleOutput::displayBenchmarkResults(results);
        }
    }
}  // namespace multithreading::application

auto main() -> int {
    try {
        multithreading::runtime::Application benchmarkApplication(
            multithreading::runtime::ApplicationInfo<int>{
                .appName = std::string{"Merge Sort Benchmark", std::allocator<char>{}},
                .appVersion = std::string{"1.0.0", std::allocator<char>{}},
                .beforeTask = std::nullopt,
                .afterTask = std::nullopt,
            }
        );

        const std::optional<int> executionResult = benchmarkApplication.SafeStart([] {
            const std::vector<std::size_t> sizes{
                {SMALL_INPUT_SIZE, MEDIUM_INPUT_SIZE, LARGE_INPUT_SIZE},
                std::allocator<std::size_t>{}
            };
            const auto inputs = std::make_shared<const SortInputs>(sizes, INPUT_SEED);

            const auto sort_scenarios = make_sort_scenarios(sizes, inputs);
            multithreading::application::benchmarkApplication(sort_scenarios);
            return 0;
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

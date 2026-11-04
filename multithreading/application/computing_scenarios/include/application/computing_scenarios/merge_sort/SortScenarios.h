#pragma once

#include <application/computing_scenarios/merge_sort/SortInputs.h>
#include <benchmark/BenchmarkMatrix.h>
#include <benchmark/BenchmarkMeasurement.h>
#include <benchmark/BenchmarkMonitor.h>
#include <benchmark/BenchmarkRunner.h>
#include <benchmark/MultithreadingTask.h>
#include <benchmark/SpeedMeasurement.h>

#include <cstddef>
#include <memory>
#include <vector>

namespace multithreading::computing_scenarios::merge_sort {

    using SortMonitor =
        benchmark::BenchmarkMonitor<benchmark::DurationType, double, double>;

    struct SortScenario {  // NOLINT(altera-struct-pack-align)
        benchmark::BenchmarkTask<benchmark::BenchmarkRunner> task;
        benchmark::BenchmarkMatrixDefinition matrix;
    };

    [[nodiscard]] std::shared_ptr<SortMonitor> make_sort_monitor();

    [[nodiscard]] std::vector<SortScenario> make_sort_scenarios(
        const std::vector<std::size_t>& sizes,
        const std::shared_ptr<const SortInputs>& inputs
    );
}  // namespace multithreading::computing_scenarios::merge_sort

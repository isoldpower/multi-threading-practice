#pragma once

#include <benchmark/BenchmarkMeasurement.h>

#include <chrono>
#include <cstdint>
#include <map>

namespace multithreading::benchmark {

    class AverageMemoryMeasurement final : public BenchmarkMeasurement<double> {
    private:
        std::map<std::chrono::nanoseconds, size_t> snapshots;
        size_t baseline{0};

    public:
        explicit AverageMemoryMeasurement(const BenchmarkMeasurementTemplate& measurement);

        void start() override;

        void snapshot() override;

        void stop() override;

        std::optional<BenchmarkMeasurementResult<double>> get_result() override;
    };
}  // namespace multithreading::benchmark

#pragma once

#include <benchmark/BenchmarkMeasurement.h>

#include <cstdint>
#include <vector>

namespace multithreading::benchmark {

    class PeakMemoryMeasurement final : public BenchmarkMeasurement<double> {
    private:
        double baseline{0};
        std::vector<double> snapshots;

        double recorded_result{0};

    public:
        explicit PeakMemoryMeasurement(const BenchmarkMeasurementTemplate& measurement);

        void start() override;

        void snapshot() override;

        void stop() override;

        std::optional<BenchmarkMeasurementResult<double>> get_result() override;
    };
}  // namespace multithreading::benchmark

#pragma once

#include <cstdint>
#include <map>

#include "./BenchmarkMeasurement.h"


namespace multithreading::benchmark {

    class PeakMemoryMeasurement final : public BenchmarkMeasurement<double> {
    private:
        double baseline;
        std::vector<double> snapshots;

        double recorded_result;
    public:
        ~PeakMemoryMeasurement() override = default;

        explicit PeakMemoryMeasurement(const BenchmarkMeasurementTemplate& measurement);

        void start() override;

        void snapshot() override;

        void stop() override;

        std::optional<BenchmarkMeasurementResult<double>> get_result() override;
    };
} // namespace multithreading::benchmark
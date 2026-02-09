#pragma once

#include <chrono>
#include <optional>

#include "./BenchmarkMeasurement.h"

namespace multithreading::benchmark {

    using DurationType = std::chrono::high_resolution_clock::duration;

    class SpeedMeasurement final : public BenchmarkMeasurement<DurationType>
    {
    private:
        std::chrono::high_resolution_clock::time_point start_time;
        std::chrono::high_resolution_clock::time_point end_time;
    public:
        ~SpeedMeasurement() override = default;
        explicit SpeedMeasurement(const BenchmarkMeasurementTemplate &measurement);

        void start() override;

        void snapshot() override;

        void stop() override;

        std::optional<BenchmarkMeasurementResult<DurationType>> get_result() override;
    };
} // namespace multithreading::benchmark
#pragma once

#include <benchmark/BenchmarkMeasurement.h>

#include <chrono>
#include <cstdint>
#include <optional>

namespace multithreading::benchmark {

    using DurationType = std::int64_t;

    class SpeedMeasurement final : public BenchmarkMeasurement<DurationType> {
    private:
        std::chrono::high_resolution_clock::time_point start_time;
        std::chrono::high_resolution_clock::time_point end_time;

    public:
        explicit SpeedMeasurement(const BenchmarkMeasurementTemplate& measurement);

        void start() override;

        void snapshot() override;

        void stop() override;

        std::optional<BenchmarkMeasurementResult<DurationType>> get_result() override;
    };
}  // namespace multithreading::benchmark
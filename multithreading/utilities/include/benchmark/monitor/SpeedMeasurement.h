#pragma once
#include <chrono>

#include "BenchmarkMeasurement.h"

namespace multithreading::utilities::benchmark {

    using DurationType = std::chrono::high_resolution_clock::duration;

    class SpeedMeasurement final
        : public BenchmarkMeasurement<std::chrono::high_resolution_clock::duration>
    {
    private:
        std::chrono::high_resolution_clock::time_point start_time;
        std::chrono::high_resolution_clock::time_point end_time;
    public:
        ~SpeedMeasurement() override = default;
        explicit SpeedMeasurement(const BenchmarkMeasurementTemplate &measurement)
            : BenchmarkMeasurement(measurement)
        {}

        void start() override {
            this->is_started = true;
            this->start_time = std::chrono::high_resolution_clock::now();
        }

        void snapshot() override {}

        void stop() override {
            this->is_measured = true;
            this->is_started = false;
            this->end_time = std::chrono::high_resolution_clock::now();
        }

        std::optional<
            BenchmarkMeasurementResult<std::chrono::high_resolution_clock::duration>
        > get_result() override {
            if (!is_measured || is_started) {
                return std::nullopt;
            }

            return BenchmarkMeasurementResult(
                information,
                std::chrono::duration_cast<std::chrono::high_resolution_clock::duration>(
                    end_time - start_time
                )
            );
        }
    };

} // namespace multithreading::utilities::benchmark
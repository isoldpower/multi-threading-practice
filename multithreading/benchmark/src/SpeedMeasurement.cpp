#include "../include/SpeedMeasurement.h"

namespace multithreading::benchmark {

    SpeedMeasurement::SpeedMeasurement(const BenchmarkMeasurementTemplate &measurement)
            : BenchmarkMeasurement(measurement)
    {}

    void SpeedMeasurement::start() {
        this->is_started = true;
        this->start_time = std::chrono::high_resolution_clock::now();
    }

    void SpeedMeasurement::snapshot() {
    }

    void SpeedMeasurement::stop() {
        this->is_measured = true;
        this->is_started = false;
        this->end_time = std::chrono::high_resolution_clock::now();
    }

    std::optional<
        BenchmarkMeasurementResult<std::chrono::high_resolution_clock::duration>
    > SpeedMeasurement::get_result() {
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
} // namespace multithreading::benchmark

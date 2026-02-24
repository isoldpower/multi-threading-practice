#include "../include/SpeedMeasurement.h"

#include <string>
#include <cmath>
#include <multithreading/utilities/include/Units.h>

namespace multithreading::benchmark {

    SpeedMeasurement::SpeedMeasurement(const BenchmarkMeasurementTemplate &measurement)
        : BenchmarkMeasurement(measurement)
    {}

    void SpeedMeasurement::start() {
        is_started = true;
        start_time = std::chrono::high_resolution_clock::now();
    }

    void SpeedMeasurement::snapshot() {
    }

    void SpeedMeasurement::stop() {
        if (!is_started) {
            return;
        }

        is_measured = true;
        is_started = false;
        end_time = std::chrono::high_resolution_clock::now();
    }

    std::optional<BenchmarkMeasurementResult<DurationType>> SpeedMeasurement::get_result() {
        if (!is_measured || is_started) {
            return std::nullopt;
        }

        const auto duration = std::chrono::duration_cast<std::chrono::high_resolution_clock::duration>(
            end_time - start_time
        );
	const double duration_ms = static_cast<double>(duration.count()) / static_cast<double>(utilities::NS_PER_MICS);

        return BenchmarkMeasurementResult(
            information,
            std::llround(duration_ms),
            std::string{"mics", std::allocator<char>{}}
        );
    }
} // namespace multithreading::benchmark

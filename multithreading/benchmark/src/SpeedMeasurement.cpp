#include "../include/SpeedMeasurement.h"

#include <multithreading/utilities/include/Units.h>

#include <cmath>
#include <string>

namespace multithreading::benchmark {

    SpeedMeasurement::SpeedMeasurement(const BenchmarkMeasurementTemplate& measurement)
        : BenchmarkMeasurement(measurement) {
    }

    void SpeedMeasurement::start() {
        mark_started();
        start_time = std::chrono::high_resolution_clock::now();
    }

    void SpeedMeasurement::snapshot() {
    }

    void SpeedMeasurement::stop() {
        if (!started()) {
            return;
        }

        mark_measured();
        end_time = std::chrono::high_resolution_clock::now();
    }

    std::optional<BenchmarkMeasurementResult<DurationType>> SpeedMeasurement::get_result() {
        if (!measured() || started()) {
            return std::nullopt;
        }

        const auto duration =
            std::chrono::duration_cast<std::chrono::high_resolution_clock::duration>(
                end_time - start_time
            );
        const double duration_ms =
            static_cast<double>(duration.count()) / static_cast<double>(utilities::NS_PER_MICS);

        return BenchmarkMeasurementResult(
            info(),
            static_cast<DurationType>(std::llround(duration_ms)),
            std::string{"mics", std::allocator<char>{}}
        );
    }
}  // namespace multithreading::benchmark

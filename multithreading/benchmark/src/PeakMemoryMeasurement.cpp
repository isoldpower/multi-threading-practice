#include "../include/PeakMemoryMeasurement.h"

#include <multithreading/utilities/include/performance/Memory.h>

#include <algorithm>

namespace multithreading::benchmark {

    PeakMemoryMeasurement::PeakMemoryMeasurement(const BenchmarkMeasurementTemplate& measurement)
        : BenchmarkMeasurement(measurement) {
    }

    void PeakMemoryMeasurement::start() {
        mark_started();
        baseline = utilities::performance::MemoryMeasurement::currentMemoryUsage();
        snapshots.clear();
    }

    void PeakMemoryMeasurement::snapshot() {
        const double current_usage =
            utilities::performance::MemoryMeasurement::currentMemoryUsage();

        snapshots.emplace_back(current_usage);
    }

    void PeakMemoryMeasurement::stop() {
        if (started()) {
            this->snapshot();
            mark_measured();

            const double highest_snapshot = *std::ranges::max_element(
                this->snapshots.begin(),
                this->snapshots.end(),
                std::ranges::less{},
                std::identity{}
            );
            recorded_result = std::abs(highest_snapshot - baseline);
        }
    }

    std::optional<BenchmarkMeasurementResult<double>> PeakMemoryMeasurement::get_result() {
        if (!measured() || started()) {
            return std::nullopt;
        }

        return BenchmarkMeasurementResult(
            info(),
            recorded_result,
            std::string{"kB", std::allocator<char>{}}
        );
    }
}  // namespace multithreading::benchmark

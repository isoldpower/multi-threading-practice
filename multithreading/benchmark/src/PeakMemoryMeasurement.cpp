#include "../include/PeakMemoryMeasurement.h"

#include <algorithm>
#include <multithreading/utilities/include/performance/Memory.h>


namespace multithreading::benchmark {

    PeakMemoryMeasurement::PeakMemoryMeasurement(const BenchmarkMeasurementTemplate& measurement)
        : BenchmarkMeasurement(measurement)
        , baseline(0)
        , snapshots(std::vector<double>{}, std::allocator<double>{})
        , recorded_result(0)
    {}

    void PeakMemoryMeasurement::start() {
        is_started = true;
        baseline = utilities::performance::MemoryMeasurement::currentMemoryUsage();
        snapshots.clear();
    }

    void PeakMemoryMeasurement::snapshot() {
        const double current_usage = utilities::performance::MemoryMeasurement::currentMemoryUsage();

        snapshots.emplace_back(current_usage);
    }

    void PeakMemoryMeasurement::stop() {
        if (is_started) {
            this->snapshot();
            is_measured = true;
            is_started = false;

            const double highest_snapshot = *std::ranges::max_element(
                this->snapshots.begin(),
                this->snapshots.end(),
                std::ranges::less{},
                std::identity{});
            recorded_result = std::abs(highest_snapshot - baseline);
        }
    }

    std::optional<BenchmarkMeasurementResult<double>> PeakMemoryMeasurement::get_result() {
        if (!is_measured || is_started) {
            return std::nullopt;
        }

        return BenchmarkMeasurementResult(
		information,
		recorded_result,
		std::string{"kB", std::allocator<char>{}}
	);
    }
} // namespace multithreading::benchmark

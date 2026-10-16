#include "../include/AverageMemoryMeasurement.h"

#include <multithreading/utilities/include/performance/Memory.h>

#include <numeric>
#include <utility>

namespace multithreading::benchmark {

    AverageMemoryMeasurement::AverageMemoryMeasurement(
        const BenchmarkMeasurementTemplate& measurement
    )
        : BenchmarkMeasurement(measurement) {
    }

    void AverageMemoryMeasurement::start() {
        mark_started();
        snapshots.clear();
        baseline =
            static_cast<size_t>(utilities::performance::MemoryMeasurement::currentMemoryUsage());
    }

    void AverageMemoryMeasurement::snapshot() {
        if (started()) {
            const auto memory_usage = static_cast<size_t>(
                utilities::performance::MemoryMeasurement::currentMemoryUsage()
            );
            const std::chrono::nanoseconds timestamp =
                std::chrono::high_resolution_clock::now().time_since_epoch();

            snapshots[timestamp] = memory_usage;
        }
    }

    void AverageMemoryMeasurement::stop() {
        if (started()) {
            this->snapshot();
            mark_measured();
        }
    }

    std::optional<BenchmarkMeasurementResult<double>> AverageMemoryMeasurement::get_result() {
        if (!measured() || started() || snapshots.empty()) {
            return std::nullopt;
        }

        const size_t snapshots_summary = std::accumulate(
            snapshots.begin(),
            snapshots.end(),
            size_t{0},
            [&](const size_t accumulated, const std::pair<std::chrono::nanoseconds, size_t>& item) {
                return accumulated + (item.second - baseline);
            }
        );
        const auto snapshots_average =
            static_cast<double>(snapshots_summary) / static_cast<double>(snapshots.size());

        return BenchmarkMeasurementResult(
            info(),
            snapshots_average,
            std::string{"kB", std::allocator<char>{}}
        );
    }
}  // namespace multithreading::benchmark

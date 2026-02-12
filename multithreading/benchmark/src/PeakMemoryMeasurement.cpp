#include "../include/PeakMemoryMeasurement.h"

#include <multithreading/utilities/include/performance/Memory.h>


namespace multithreading::benchmark {

    PeakMemoryMeasurement::PeakMemoryMeasurement(const BenchmarkMeasurementTemplate& measurement)
        : BenchmarkMeasurement(measurement)
        , baseline(0)
        , peak(0)
    {}

    void PeakMemoryMeasurement::start() {
        is_started = true;
        baseline = utilities::performance::MemoryMeasurement::peakMemoryUsage();
    }

    void PeakMemoryMeasurement::snapshot() {
        const double peak_usage = utilities::performance::MemoryMeasurement::peakMemoryUsage();
        const double relative_peak = peak_usage - baseline;

        peak = std::max(peak, relative_peak);
    }

    void PeakMemoryMeasurement::stop() {
        if (is_started) {
            this->snapshot();
            is_measured = true;
            is_started = false;
        }
    }

    std::optional<BenchmarkMeasurementResult<double>> PeakMemoryMeasurement::get_result() {
        if (!is_measured || is_started) {
            return std::nullopt;
        }

        return BenchmarkMeasurementResult(information, peak, "kB");
    }
} // namespace multithreading::benchmark

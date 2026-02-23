#pragma once

#include <utility>
#include <optional>
#include <string>

namespace multithreading::benchmark {

    struct alignas(32) BenchmarkMeasurementTemplate {
        std::string verbose;
    };

    template <typename T>
    struct BenchmarkMeasurementResult {
    private:
        BenchmarkMeasurementTemplate reference_template;
        std::string unit;
        T result;
    public:
        explicit BenchmarkMeasurementResult(
            BenchmarkMeasurementTemplate measurement,
            T result,
            std::string unit
        )
            : reference_template(std::move(measurement))
            , unit(std::move(unit))
            , result(result)
        {}

        [[nodiscard]] std::string get_verbose() const {
            return reference_template.verbose;
        }

        [[nodiscard]] std::optional<T> get_measurement() const {
            return result;
        }

        [[nodiscard]] std::string get_unit() const {
            return unit;
        }
    };

    template <typename T>
    class BenchmarkMeasurement {
    protected:
        bool is_started;
        bool is_measured;
        BenchmarkMeasurementTemplate information;
    public:
        virtual ~BenchmarkMeasurement() = default;
        explicit BenchmarkMeasurement(BenchmarkMeasurementTemplate measurement)
            : is_started(false)
            , is_measured(false)
            , information(std::move(measurement))
        {}

        virtual void start() = 0;
        virtual void snapshot() = 0;
        virtual void stop() = 0;
        virtual std::optional<BenchmarkMeasurementResult<T>> get_result() = 0;
    };

    template <typename ...TArgs>
    using MeasurementsList = std::tuple<BenchmarkMeasurement<TArgs>...>;

    template <typename ...TArgs>
    using RefMeasurementsList = std::tuple<std::unique_ptr<BenchmarkMeasurement<TArgs>>...>;
} // namespace multithreading::benchmark
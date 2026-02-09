#include <utility>

#pragma once


namespace multithreading::utilities::benchmark {

    struct BenchmarkMeasurementTemplate {
    public:
        std::string verbose;
    };

    template <typename T>
    struct BenchmarkMeasurementResult {
    private:
        BenchmarkMeasurementTemplate reference_template;
        T result;
    public:
        explicit BenchmarkMeasurementResult(BenchmarkMeasurementTemplate measurement, T result)
            : reference_template(std::move(measurement))
            , result(result)
        {}

        [[nodiscard]] std::string get_verbose() const {
            return reference_template.verbose;
        }

        [[nodiscard]] std::optional<T> get_measurement() const {
            return result;
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
    using RefMeasurementsList = std::tuple<BenchmarkMeasurement<TArgs>*...>;
} // namespace multithreading::utilities::benchmark
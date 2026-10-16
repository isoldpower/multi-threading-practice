#pragma once

#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <utility>

namespace multithreading::benchmark {

    constexpr size_t MEASUREMENT_TEMPLATE_ALIGNMENT = 32;
    constexpr size_t MEASUREMENT_RESULT_ALIGNMENT = 64;

    struct alignas(MEASUREMENT_TEMPLATE_ALIGNMENT) BenchmarkMeasurementTemplate {
        std::string verbose;
    };

    template <typename T>
    // NOLINTNEXTLINE(altera-struct-pack-align)
    struct alignas(MEASUREMENT_RESULT_ALIGNMENT) BenchmarkMeasurementResult {
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
            , result(result) {
        }

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
    private:
        bool is_started{false};
        bool is_measured{false};
        BenchmarkMeasurementTemplate information;

    protected:
        [[nodiscard]] bool started() const noexcept {
            return is_started;
        }

        [[nodiscard]] bool measured() const noexcept {
            return is_measured;
        }

        [[nodiscard]] const BenchmarkMeasurementTemplate& info() const noexcept {
            return information;
        }

        void mark_started() noexcept {
            is_started = true;
        }

        /** @brief Records that a started measurement has finished and holds a result. */
        void mark_measured() noexcept {
            is_measured = true;
            is_started = false;
        }

    public:
        virtual ~BenchmarkMeasurement() = default;
        explicit BenchmarkMeasurement(BenchmarkMeasurementTemplate measurement)
            : information(std::move(measurement)) {
        }

        BenchmarkMeasurement(const BenchmarkMeasurement&) = delete;
        BenchmarkMeasurement& operator=(const BenchmarkMeasurement&) = delete;
        BenchmarkMeasurement(BenchmarkMeasurement&&) = delete;
        BenchmarkMeasurement& operator=(BenchmarkMeasurement&&) = delete;

        virtual void start() = 0;
        virtual void snapshot() = 0;
        virtual void stop() = 0;
        virtual std::optional<BenchmarkMeasurementResult<T>> get_result() = 0;
    };

    template <typename... TArgs>
    using MeasurementsList = std::tuple<BenchmarkMeasurement<TArgs>...>;

    template <typename... TArgs>
    using RefMeasurementsList = std::tuple<std::unique_ptr<BenchmarkMeasurement<TArgs>>...>;
}  // namespace multithreading::benchmark
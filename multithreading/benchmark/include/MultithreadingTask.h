#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <utility>

namespace multithreading::benchmark {

    constexpr size_t BENCHMARK_TASK_ALIGNMENT = 64;

    template <typename TStruct>
    class alignas(BENCHMARK_TASK_ALIGNMENT) BenchmarkTask {
    private:
        std::string task_title;
        std::shared_ptr<TStruct> task_structure;

    public:
        BenchmarkTask(std::string title, std::shared_ptr<TStruct> structure)
            : task_title(std::move(title))
            , task_structure(std::move(structure)) {
        }

        BenchmarkTask(std::shared_ptr<TStruct> structure, std::string title)
            : task_title(std::move(title))
            , task_structure(std::move(structure)) {
        }

        ~BenchmarkTask() = default;
        BenchmarkTask(const BenchmarkTask&) = delete;
        BenchmarkTask& operator=(const BenchmarkTask&) = delete;
        BenchmarkTask(BenchmarkTask&&) noexcept = default;
        BenchmarkTask& operator=(BenchmarkTask&&) noexcept = default;

        [[nodiscard]] const std::string& title() const noexcept {
            return task_title;
        }

        [[nodiscard]] const std::shared_ptr<TStruct>& structure() const noexcept {
            return task_structure;
        }
    };
}  // namespace multithreading::benchmark
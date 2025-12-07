#pragma once

#include <string>
#include <utility>


namespace multithreading::utilities::benchmark {

    template <typename TStruct>
    struct alignas(64) BenchmarkTask {
        std::string title;
        std::shared_ptr<TStruct> structure;

        BenchmarkTask(std::string title, std::shared_ptr<TStruct> structure)
            : title(std::move(title))
            , structure(structure)
        {}

        BenchmarkTask(std::shared_ptr<TStruct> structure, std::string title)
            : title(std::move(title))
            , structure(structure)
        {}

        BenchmarkTask(const BenchmarkTask&) = delete;
        BenchmarkTask& operator=(const BenchmarkTask&) = delete;

        BenchmarkTask(BenchmarkTask&&) noexcept = default;
        BenchmarkTask& operator=(BenchmarkTask&&) noexcept = default;
    };
} // namespace multithreading::utilities::benchmark
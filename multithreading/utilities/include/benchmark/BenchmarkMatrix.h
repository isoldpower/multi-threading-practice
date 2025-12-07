#pragma once

#include <vector>


namespace multithreading::utilities::benchmark {

    struct alignas(64) BenchmarkMatrixDefinition {
    public:
        std::vector<size_t> per_thread_sizes;
        std::vector<size_t> threads_count;
    };

    struct alignas(16) BenchmarkMatrixItem {
    public:
        size_t threads_count;
        size_t thread_size;
    };
} // namespace multithreading::utilities::benchmark

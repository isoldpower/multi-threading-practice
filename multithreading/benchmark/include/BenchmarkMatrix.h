#pragma once

#include <cstddef>
#include <vector>

namespace multithreading::benchmark {

    constexpr size_t MATRIX_DEFINITION_ALIGNMENT = 64;
    constexpr size_t MATRIX_ITEM_ALIGNMENT = 16;

    struct alignas(MATRIX_DEFINITION_ALIGNMENT) BenchmarkMatrixDefinition {
        std::vector<size_t> per_thread_sizes;
        std::vector<size_t> threads_count;
    };

    struct alignas(MATRIX_ITEM_ALIGNMENT) BenchmarkMatrixItem {
        size_t threads_count;
        size_t thread_size;
    };
}  // namespace multithreading::benchmark

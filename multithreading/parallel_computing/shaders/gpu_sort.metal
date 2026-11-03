#include <metal_stdlib>
using namespace metal;


// Must match multithreading::parallel_computing::merge_sort::GPUSortParameters
struct SortParameters {
    uint first;       // bitonic: compare distance (step), merge: run width
    uint second;      // bitonic: sorted block size (stage), merge: unused
    uint count;       // number of elements in the buffer
    uint descending;  // 0 = ascending, 1 = descending
};

template <typename T>
inline bool comes_before(const T left, const T right, const bool descending) {
    return descending ? (right < left) : (left < right);
}


template <typename T>
kernel void bitonic_step(
    device T* values [[buffer(0)]],
    constant SortParameters& parameters [[buffer(1)]],
    const uint pair_index [[thread_position_in_grid]]
) {
    const uint step = parameters.first;
    const uint stage = parameters.second;

    const uint low = ((pair_index & ~(step - 1)) << 1) | (pair_index & (step - 1));
    const uint high = low | step;
    const bool ascending = ((low & stage) == 0) != (parameters.descending != 0);

    const T first = values[low];
    const T second = values[high];
    const T smaller = min(first, second);
    const T larger = max(first, second);

    values[low] = ascending ? smaller : larger;
    values[high] = ascending ? larger : smaller;
}


constant constexpr uint BITONIC_LOCAL_MAX_GROUP = 1024;

template <typename T>
kernel void bitonic_local(
    device T* values [[buffer(0)]],
    constant SortParameters& parameters [[buffer(1)]],
    const uint local_index [[thread_position_in_threadgroup]],
    const uint group_index [[threadgroup_position_in_grid]],
    const uint group_size [[threads_per_threadgroup]]
) {
    threadgroup T block_values[2 * BITONIC_LOCAL_MAX_GROUP];

    const uint block_size = 2 * group_size;
    const uint block_begin = group_index * block_size;
    const bool descending = parameters.descending != 0;

    block_values[local_index] = values[block_begin + local_index];
    block_values[local_index + group_size] = values[block_begin + local_index + group_size];
    threadgroup_barrier(mem_flags::mem_threadgroup);

    const bool full_sort = parameters.second == 0;
    const uint first_stage = full_sort ? 2 : parameters.second;
    const uint last_stage = full_sort ? block_size : parameters.second;

    for (uint stage = first_stage; stage <= last_stage; stage <<= 1) {
        for (uint step = min(stage, block_size) >> 1; step > 0; step >>= 1) {
            const uint low = ((local_index & ~(step - 1)) << 1) | (local_index & (step - 1));
            const uint high = low | step;
            const bool ascending = (((block_begin + low) & stage) == 0) != descending;

            const T first = block_values[low];
            const T second = block_values[high];
            const T smaller = min(first, second);
            const T larger = max(first, second);
            block_values[low] = ascending ? smaller : larger;
            block_values[high] = ascending ? larger : smaller;

            threadgroup_barrier(mem_flags::mem_threadgroup);
        }
    }

    values[block_begin + local_index] = block_values[local_index];
    values[block_begin + local_index + group_size] = block_values[local_index + group_size];
}

template <typename T>
inline uint rank_in_run(
    device const T* values,
    const uint begin,
    const uint end,
    const T value,
    const bool descending,
    const bool inclusive
) {
    uint low = begin;
    uint high = end;
    while (low < high) {
        const uint middle = low + ((high - low) / 2);
        const T candidate = values[middle];
        const bool candidate_first = inclusive
            ? !comes_before(value, candidate, descending)
            : comes_before(candidate, value, descending);

        if (candidate_first) {
            low = middle + 1;
        } else {
            high = middle;
        }
    }

    return low - begin;
}

template <typename T>
kernel void merge_pass(
    device const T* source [[buffer(0)]],
    device T* destination [[buffer(1)]],
    constant SortParameters& parameters [[buffer(2)]],
    const uint index [[thread_position_in_grid]]
) {
    const uint count = parameters.count;
    if (index >= count) {
        return;
    }

    const uint width = parameters.first;
    const bool descending = parameters.descending != 0;

    const uint left_begin = index - (index % (2 * width));
    const uint left_end = min(left_begin + width, count);
    const uint right_end = min(left_begin + (2 * width), count);
    const T value = source[index];

    uint position = 0;
    if (index < left_end) {
        position = index + rank_in_run(source, left_end, right_end, value, descending, false);
    } else {
        position = left_begin + (index - left_end)
            + rank_in_run(source, left_begin, left_end, value, descending, true);
    }

    destination[position] = value;
}


template [[host_name("bitonic_step_int")]]
kernel void bitonic_step<int>(device int*, constant SortParameters&, uint);
template [[host_name("bitonic_step_uint")]]
kernel void bitonic_step<uint>(device uint*, constant SortParameters&, uint);
template [[host_name("bitonic_step_float")]]
kernel void bitonic_step<float>(device float*, constant SortParameters&, uint);

template [[host_name("bitonic_local_int")]]
kernel void bitonic_local<int>(device int*, constant SortParameters&, uint, uint, uint);
template [[host_name("bitonic_local_uint")]]
kernel void bitonic_local<uint>(device uint*, constant SortParameters&, uint, uint, uint);
template [[host_name("bitonic_local_float")]]
kernel void bitonic_local<float>(device float*, constant SortParameters&, uint, uint, uint);

template [[host_name("merge_pass_int")]]
kernel void merge_pass<int>(device const int*, device int*, constant SortParameters&, uint);
template [[host_name("merge_pass_uint")]]
kernel void merge_pass<uint>(device const uint*, device uint*, constant SortParameters&, uint);
template [[host_name("merge_pass_float")]]
kernel void merge_pass<float>(device const float*, device float*, constant SortParameters&, uint);

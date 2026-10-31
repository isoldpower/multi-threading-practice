#include <metal_stdlib>
using namespace metal;


kernel void add_one(device const float* in_array [[buffer(0)]],
                    device float* out_array [[buffer(1)]],
                    uint index [[thread_position_in_grid]]
) {
    out_array[index] = in_array[index] + 1.0f;
}
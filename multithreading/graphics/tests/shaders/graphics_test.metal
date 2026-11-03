#include <metal_stdlib>
using namespace metal;


// Must match AddParameters in GraphicsTestSupport.h
struct AddParameters {
    uint addend;
};

kernel void add_constant(
    device uint* values [[buffer(0)]],
    constant AddParameters& parameters [[buffer(1)]],
    const uint index [[thread_position_in_grid]]
) {
    values[index] += parameters.addend;
}

kernel void copy_values(
    device const uint* source [[buffer(0)]],
    device uint* destination [[buffer(1)]],
    const uint index [[thread_position_in_grid]]
) {
    destination[index] = source[index];
}

kernel void write_grid_position(
    device uint* values [[buffer(0)]],
    constant uint& width [[buffer(1)]],
    const uint2 position [[thread_position_in_grid]]
) {
    values[(position.y * width) + position.x] = (position.y * 1000) + position.x;
}

kernel void write_group_size(
    device uint* values [[buffer(0)]],
    const uint index [[thread_position_in_grid]],
    const uint group_size [[threads_per_threadgroup]]
) {
    values[index] = group_size;
}

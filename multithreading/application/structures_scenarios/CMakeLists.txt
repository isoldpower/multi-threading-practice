# Compile files
add_library(multithreading_structures_scenarios
        # Structure holders
        src/application/structures_scenarios/BoundedQueueBenchmark.cpp
        src/application/structures_scenarios/UnboundedQueueBenchmark.cpp
        src/application/structures_scenarios/LinkedListBenchmark.cpp
        # Multi-consumer multi-producer scenarios
        src/application/structures_scenarios/mcmp/BoundedQueueMCMPBenchmark.cpp
        src/application/structures_scenarios/mcmp/UnboundedQueueMCMPBenchmark.cpp
        src/application/structures_scenarios/mcmp/LinkedListMCMPBenchmark.cpp
)

# Expose settings
target_include_directories(multithreading_structures_scenarios
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Link dependencies
target_link_libraries(multithreading_structures_scenarios PUBLIC structures benchmark cxx_setup)

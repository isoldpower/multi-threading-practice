# Reusable libraries, owing nothing to any particular lab work.
add_subdirectory(utilities)
add_subdirectory(structures)
add_subdirectory(benchmark)
add_subdirectory(parallel_computing)

# Lab-specific definitions built on top of the libraries above.
add_subdirectory(application)

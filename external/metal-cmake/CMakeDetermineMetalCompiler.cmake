include(${CMAKE_ROOT}/Modules/CMakeDetermineCompiler.cmake)

if(NOT CMAKE_Metal_COMPILER_NAMES)
    set(CMAKE_Metal_COMPILER_NAMES metal)
endif()

if("${CMAKE_GENERATOR}" STREQUAL "Xcode" OR (APPLE AND CMAKE_VERSION VERSION_GREATER_EQUAL "4.0"))
    set(CMAKE_Metal_COMPILER_XCODE_TYPE sourcecode.metal)

    if(CMAKE_Metal_COMPILER)
        _cmake_find_compiler_path(Metal)
    else()
        execute_process(COMMAND xcrun --find metal
                OUTPUT_VARIABLE _xcrun_out OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_VARIABLE _xcrun_err RESULT_VARIABLE _xcrun_result
        )

        if(_xcrun_result EQUAL 0 AND EXISTS "${_xcrun_out}")
            set(CMAKE_Metal_COMPILER "${_xcrun_out}")
        else()
            _cmake_find_compiler_path(Metal)
        endif()
    endif()
else()
    if(CMAKE_Metal_COMPILER)
        _cmake_find_compiler_path(Metal)
    else()
        set(CMAKE_Metal_COMPILER_INIT NOTFOUND)

        if(NOT $ENV{METALC} STREQUAL "")
            get_filename_component(CMAKE_Metal_COMPILER_INIT $ENV{METALC} PROGRAM PROGRAM_ARGS CMAKE_Metal_FLAGS_ENV_INIT)
            if(CMAKE_Metal_FLAGS_ENV_INIT)
                set(CMAKE_Metal_COMPILER_ARG1 "${CMAKE_Metal_FLAGS_ENV_INIT}" CACHE STRING "Arguments to the Metal compiler")
            endif()
            if(NOT EXISTS ${CMAKE_Metal_COMPILER_INIT})
                message(FATAL_ERROR "Could not find compiler set in environment variable METALC\n$ENV{METALC}.\n${CMAKE_Metal_COMPILER_INIT}")
            endif()
        endif()

        if(NOT CMAKE_Metal_COMPILER_INIT)
            set(CMAKE_Metal_COMPILER_LIST metal ${_CMAKE_TOOLCHAIN_PREFIX}metal)
        endif()

        _cmake_find_compiler(Metal)
    endif()

    mark_as_advanced(CMAKE_Metal_COMPILER)
endif()

if(CMAKE_Metal_COMPILER AND NOT CMAKE_Metal_COMPILER_VERSION)
    execute_process(
            COMMAND "${CMAKE_Metal_COMPILER}" --version
            OUTPUT_VARIABLE output ERROR_VARIABLE output
            RESULT_VARIABLE result
            TIMEOUT 10
    )
    message(CONFIGURE_LOG
            "Running the Metal compiler: \"${CMAKE_Metal_COMPILER}\" --version\n"
            "${output}\n"
    )

    if(NOT result EQUAL 0 AND NOT CMAKE_Metal_COMPILER_FORCED)
        string(STRIP "${output}" output)
        string(REPLACE "\n" "\n  " output "${output}")
        message(FATAL_ERROR "The Metal compiler\n  \"${CMAKE_Metal_COMPILER}\"\n"
                "cannot be run:\n  ${output}\n")
    endif()

    if(output MATCHES [[metal version ([0-9]+\.[0-9]+(\.[0-9]+)?)]])
        set(CMAKE_Metal_COMPILER_VERSION "${CMAKE_MATCH_1}")
        if(NOT CMAKE_Metal_COMPILER_ID)
            set(CMAKE_Metal_COMPILER_ID "Apple")
        endif()
    endif()
endif()

if(NOT _CMAKE_TOOLCHAIN_LOCATION)
    get_filename_component(_CMAKE_TOOLCHAIN_LOCATION "${CMAKE_Metal_COMPILER}" PATH)
endif ()

set(_CMAKE_PROCESSING_LANGUAGE "Metal")
include(CMakeFindBinUtils)
unset(_CMAKE_PROCESSING_LANGUAGE)

configure_file(
        ${CMAKE_CURRENT_LIST_DIR}/CMakeMetalCompiler.cmake.in
        ${CMAKE_PLATFORM_INFO_DIR}/CMakeMetalCompiler.cmake
)

set(CMAKE_Metal_COMPILER_ENV_VAR "METALC")
cmake_minimum_required(VERSION 3.12)
project(flux VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_TESTS "build all test suits." OFF)

set(FLUX_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

include(${FLUX_ROOT}/cmake/flags.cmake)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${FLUX_ROOT}/output/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${FLUX_ROOT}/output/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${FLUX_ROOT}/output/lib)

add_subdirectory(vendor)
add_subdirectory(source)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(example)
endif()

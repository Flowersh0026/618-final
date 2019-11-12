cmake_minimum_required(VERSION 3.15)
project(618_final)

set(CMAKE_CXX_STANDARD 17)

# move built executables to bin/
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# import googletest
add_subdirectory(third_party/googletest)
include_directories(${gtest_SOURCE_DIR}/include)

# import google benchmark
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Disable benchmark tests" FORCE)
add_subdirectory(third_party/benchmark)
include_directories(third_party/benchmark/include)

# unittest executable
add_executable(unittest test/sample_test.cc)
target_link_libraries(unittest gtest gtest_main)

# benchmark executable
add_executable(run_benchmark src/main.cc)
target_link_libraries(run_benchmark benchmark benchmark_main)
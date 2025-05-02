cmake_minimum_required(VERSION 3.13)
project(unit_tests C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

include_directories(./src)

add_executable(test_outbreak
    local_unity_test.c
    ./src/unity.c
)

enable_testing()
add_test(NAME adc_to_celsius COMMAND test_outbreak)
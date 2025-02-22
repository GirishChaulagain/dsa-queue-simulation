cmake_minimum_required(VERSION 3.10)
project(dsa-queue-simulator)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

add_subdirectory(generator)
add_subdirectory(simulator)

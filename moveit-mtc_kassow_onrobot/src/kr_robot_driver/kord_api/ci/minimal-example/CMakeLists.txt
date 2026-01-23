cmake_minimum_required(VERSION 3.14)

project(test)

if (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there. You may need to remove CMakeCache.txt.")
endif ()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(TEST_TARGET test)

find_package(kord-api REQUIRED)

# print content of kord_api package
message("kord-api package:")
message("  kord-api_DIR: ${kord-api_DIR}")
message("  kord-api_FOUND: ${kord-api_FOUND}")
message("  kord-api_VERSION: ${kord-api_VERSION}")

add_executable(${TEST_TARGET} main.cpp)

target_link_libraries(${TEST_TARGET} PRIVATE kord-api::kord-api)

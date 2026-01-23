cmake_minimum_required(VERSION 3.24)
project(test_kord)

cmake_policy(SET CMP0135 NEW)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage --coverage")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")

find_package(Eigen3 REQUIRED NO_MODULE)

include(FetchContent)
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.15.2
        CMAKE_ARGS -DINSTALL_GTEST=OFF -DBUILD_SHARED_LIBS=OFF
        FIND_PACKAGE_ARGS NAMES GTest
)
FetchContent_MakeAvailable(googletest)

include_directories(
        utils
        ../include
        ../external/kord-protocol/include
        ../external/kord/asio
        ../external

        ${EIGEN3_INCLUDE_DIR}
)

if (APPLE)
    link_directories(/usr/lib /usr/local/lib $ENV{KR2_INSTALL_FOLDER}/lib ../build)
else ()
    link_directories(/usr/lib /usr/lib/x86_64-linux-gnu /usr/local/lib $ENV{KR2_INSTALL_FOLDER}/lib ../build)
endif ()

enable_testing()

# gtest
add_executable(test-passive tests/passive_test.cpp)
target_link_libraries(test-passive ${KORD_LIB_NAME} GTest::gtest_main --coverage)

add_executable(test-active tests/active_test.cpp)
target_link_libraries(test-active ${KORD_LIB_NAME} GTest::gtest_main "stdc++fs" --coverage)

add_executable(test-examples tests/examples_test.cpp)
target_link_libraries(test-examples ${KORD_LIB_NAME} GTest::gtest_main "stdc++fs" --coverage)

add_executable(test-motion-constraints tests/motion_constraints_test.cpp)
target_link_libraries(test-motion-constraints ${KORD_LIB_NAME} GTest::gtest_main "stdc++fs" Eigen3::Eigen)

include(GoogleTest)
gtest_discover_tests(test-passive)
gtest_discover_tests(test-active)
gtest_discover_tests(test-examples)
gtest_discover_tests(test-motion-constraints)

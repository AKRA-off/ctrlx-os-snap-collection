cmake_minimum_required(VERSION 3.14)

if (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there. You may need to remove CMakeCache.txt.")
endif ()

file(READ "version.txt" KORD_PROTOCOL_FULL_VERSION)
string(STRIP "${KORD_PROTOCOL_FULL_VERSION}" KORD_PROTOCOL_FULL_VERSION)  # Remove whitespace/newlines

# Validate version format
if (NOT KORD_PROTOCOL_FULL_VERSION MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+.*$")
    message(FATAL_ERROR "Invalid version format in version.txt: ${KORD_PROTOCOL_FULL_VERSION}")
endif ()

# Set architecture
if (NOT DEFINED ENV{ARCH})
    execute_process(COMMAND dpkg --print-architecture OUTPUT_VARIABLE ARCH OUTPUT_STRIP_TRAILING_WHITESPACE)
else ()
    set(ARCH $ENV{ARCH})
endif ()
string(TOLOWER "${ARCH}" ARCH) # Force lowercase architecture

string(REGEX MATCH "^[0-9]+\\.[0-9]+\\.[0-9]+" KORD_PROTOCOL_SEMVER ${KORD_PROTOCOL_FULL_VERSION})
if (NOT KORD_PROTOCOL_SEMVER)
    message(FATAL_ERROR "Invalid semver format in version.txt: ${KORD_PROTOCOL_FULL_VERSION}")
endif ()

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)

# Set library name and version
set(KORD_PROTOCOL_LIB_NAME kord-protocol)
set(KORD_PROTOCOL_VERSION ${KORD_PROTOCOL_SEMVER})

# Set coverage output directory
set(COVERAGE_OUTPUT_DIR ${CMAKE_BINARY_DIR}/coverage_report)

# Set gcov executable
set(GCOV_EXECUTABLE "/usr/bin/gcov" CACHE STRING "Path to the gcov executable")

# Set CPack generator
set(CPACK_GENERATOR "DEB")

# Set component names
set(KORD_PROTOCOL_COMPONENT_NAME ${KORD_PROTOCOL_LIB_NAME} CACHE STRING "Component name for the runtime library")
set(KORD_LIB_COMPONENT_NAME ${KORD_PROTOCOL_COMPONENT_NAME}-libs)
set(KORD_LIBS_DEV_COMPONENT_NAME ${KORD_LIB_COMPONENT_NAME}-dev)

# Package name
set(KORD_PROTOCOL_PACKAGE_NAME ${KORD_PROTOCOL_LIB_NAME} CACHE STRING "Package name for the library")

# Set project name and version
project(${KORD_PROTOCOL_LIB_NAME} VERSION ${KORD_PROTOCOL_VERSION})

message(STATUS "Building project: " ${KORD_PROTOCOL_LIB_NAME})

# Options
option(KORD_PROTOCOL_WITH_TESTS "Build with tests" OFF)
option(KORD_PROTOCOL_WITH_COVERAGE "Compile with code coverage flags" OFF)
option(INSTALL_KORD_PROTOCOL "Install the library" ON)
option(KORD_PROTOCOL_PACKAGE_TARGETS "Include the kord-protocol targets in the CPack package" ON)
option(KORD_PROTOCOL_BUILD_NO "Optionally add to the debian package version when >= 0" -1)

# We have to install the library if we want to package it
if (KORD_PROTOCOL_PACKAGE_TARGETS)
    set(INSTALL_KORD_PROTOCOL ON)
endif ()

# Enable coverage flags if CODE_COVERAGE is ON
if (KORD_PROTOCOL_WITH_COVERAGE)
    message(STATUS "Building with code coverage enabled")

    # Set coverage compilation flags for C and C++
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage --coverage -g -O0")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage --coverage -g -O0")

    # Set linker flags for C++ and C
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} --coverage")

    # Define a custom target to run tests
    add_custom_target(run_tests
            COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Running tests to generate coverage data"
    )

    # Define the coverage target that depends on running tests
    add_custom_target(coverage
            DEPENDS run_tests
            COMMAND mkdir -p ${COVERAGE_OUTPUT_DIR} && gcovr -v -r ${CMAKE_SOURCE_DIR}
            --gcov-executable ${GCOV_EXECUTABLE}
            --exclude ".*_deps/.*"
            --object-directory ${CMAKE_BINARY_DIR}/CMakeFiles
            --html-details ${COVERAGE_OUTPUT_DIR}/coverage.html
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating coverage reports with gcovr"
    )

    add_custom_target(coverage_cobertura
            COMMAND mkdir -p ${COVERAGE_OUTPUT_DIR} && gcovr -v -r ${CMAKE_SOURCE_DIR}
            --gcov-executable ${GCOV_EXECUTABLE}
            --exclude ".*_deps/.*"
            --object-directory ${CMAKE_BINARY_DIR}/CMakeFiles
            --cobertura ${COVERAGE_OUTPUT_DIR}/coverage.xml
            --xml-pretty
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating coverage reports with gcovr"
    )
endif ()

# Use GNUInstallDirs to define installation directories
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Get the git SHA1 and version from the repository
execute_process(
        COMMAND git log -1 --format=%H
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_SHA1
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
        COMMAND cat version.txt
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Generate the version header file
configure_file(
        include/kord/protocol/version.h.in
        ${CMAKE_CURRENT_BINARY_DIR}/include/kord/protocol/version.h
)
message(STATUS "CMAKE_CURRENT_BINARY_DIR is set to: " ${CMAKE_CURRENT_BINARY_DIR})

if (APPLE)
    link_directories(/usr/lib /opt/homebrew/lib /usr/local/lib $ENV{KR2_INSTALL_FOLDER}/lib)
endif ()

##########Н
## Build ##
###########

# Add the library
add_library(${KORD_PROTOCOL_LIB_NAME} SHARED
        src/DataFormatDescription.cpp
        src/StatusFrameBuilder.cpp
        src/StatusFrameParser.cpp
        src/ContentFrameBuilder.cpp
        src/ContentFrameParser.cpp
        src/ContentReader.cpp
)

set_target_properties(${KORD_PROTOCOL_LIB_NAME} PROPERTIES
        VERSION ${KORD_PROTOCOL_VERSION}
        SOVERSION 1
)

target_include_directories(${KORD_PROTOCOL_LIB_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        PRIVATE
        src
)

#############
## Install ##
#############

if (INSTALL_KORD_PROTOCOL)
    # Install the library
    install(TARGETS ${KORD_PROTOCOL_LIB_NAME}
            EXPORT ${KORD_PROTOCOL_LIB_NAME}Targets
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            COMPONENT ${KORD_LIB_COMPONENT_NAME})

    # Install the headers
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            COMPONENT ${KORD_LIBS_DEV_COMPONENT_NAME}
            FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp")

    # Install the generated version header
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/kord/protocol/version.h
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/kord/protocol
            COMPONENT ${KORD_LIBS_DEV_COMPONENT_NAME})

    # Export the targets
    install(EXPORT ${KORD_PROTOCOL_LIB_NAME}Targets
            FILE ${KORD_PROTOCOL_LIB_NAME}Targets.cmake
            NAMESPACE ${KORD_PROTOCOL_LIB_NAME}::
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${KORD_PROTOCOL_LIB_NAME}
            COMPONENT ${KORD_LIBS_DEV_COMPONENT_NAME})

    # This makes the project importable from the build directory
    export(EXPORT ${KORD_PROTOCOL_LIB_NAME}Targets
            FILE "${CMAKE_CURRENT_BINARY_DIR}/${KORD_PROTOCOL_LIB_NAME}Targets.cmake"
            NAMESPACE ${KORD_PROTOCOL_LIB_NAME}::)

    # Create and install the package config file
    configure_package_config_file(
            "${CMAKE_CURRENT_SOURCE_DIR}/cmake/${KORD_PROTOCOL_LIB_NAME}Config.cmake.in"
            "${CMAKE_CURRENT_BINARY_DIR}/${KORD_PROTOCOL_LIB_NAME}Config.cmake"
            INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${KORD_PROTOCOL_LIB_NAME}
    )

    # Generate and install the version file
    write_basic_package_version_file(
            "${CMAKE_CURRENT_BINARY_DIR}/${KORD_PROTOCOL_LIB_NAME}ConfigVersion.cmake"
            VERSION ${PROJECT_VERSION}
            COMPATIBILITY SameMajorVersion
    )

    install(FILES
            "${CMAKE_CURRENT_BINARY_DIR}/${KORD_PROTOCOL_LIB_NAME}Config.cmake"
            "${CMAKE_CURRENT_BINARY_DIR}/${KORD_PROTOCOL_LIB_NAME}ConfigVersion.cmake"
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${KORD_PROTOCOL_LIB_NAME}
            COMPONENT ${KORD_LIBS_DEV_COMPONENT_NAME}
    )

    # uninstall target
    if (NOT TARGET uninstall)
        configure_file(
                "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
                "${CMAKE_CURRENT_BINARY_DIR}/cmake/cmake_uninstall.cmake"
                IMMEDIATE @ONLY)

        add_custom_target(uninstall
                COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake/cmake_uninstall.cmake)
    endif ()

endif ()

add_library(${KORD_PROTOCOL_LIB_NAME}::${KORD_PROTOCOL_LIB_NAME} ALIAS ${KORD_PROTOCOL_LIB_NAME})

###########
## Tests ##
###########
if (KORD_PROTOCOL_WITH_TESTS)
    if (POLICY CMP0135)
        cmake_policy(SET CMP0135 NEW)  # Apply NEW behavior project-wide
    endif ()

    include(FetchContent)
    FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG v1.15.2
            CMAKE_ARGS -DINSTALL_GTEST=OFF -DBUILD_SHARED_LIBS=OFF
    )

    FetchContent_Declare(
            crcplusplus
            GIT_REPOSITORY https://github.com/d-bahr/CRCpp.git
            GIT_TAG release-1.2.0.0
    )

    # Disable installation of gtest
    set(INSTALL_GTEST OFF)
    set(KORD_PROTOCOL_TESTS_COMPONENT_NAME ${KORD_PROTOCOL_LIB_NAME}-tests)

    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest crcplusplus)

    # alternatively to use locally installed libgtest-dev
    # add_subdirectory(/usr/src/googletest build/Testing)
    set(kr_tests "")

    function(add_unit_test TEST_NAME TEST_SOURCE)
        add_executable(${TEST_NAME} ${TEST_SOURCE})
        target_include_directories(${TEST_NAME} PUBLIC ${crcplusplus_SOURCE_DIR}/inc)
        target_link_libraries(${TEST_NAME} ${KORD_PROTOCOL_LIB_NAME} gtest_main)

        # Add coverage instrumentation to test executables
        if (KORD_PROTOCOL_WITH_COVERAGE)
            target_compile_options(${TEST_NAME} PRIVATE -g -fprofile-arcs -ftest-coverage --coverage -O0)
            target_link_options(${TEST_NAME} PRIVATE --coverage)
        endif ()

        list(APPEND kr_tests ${TEST_NAME})
        set(kr_tests ${kr_tests} PARENT_SCOPE)

        install(TARGETS ${TEST_NAME}
                RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                COMPONENT ${KORD_PROTOCOL_TESTS_COMPONENT_NAME})
    endfunction(add_unit_test)

    enable_testing()

    # Add tests
    add_unit_test(kord-protocol-test-content-frames tests/test_content_frames.cpp)
    add_unit_test(kord-protocol-test-dfd tests/test_dfd.cpp)
    add_unit_test(kord-protocol-test-status-frame tests/test_status_frame.cpp)
    add_unit_test(kord-protocol-test-frames tests/test_frames.cpp)
    add_unit_test(kord-protocol-test-wr-rd tests/test_wr_rd.cpp)
    add_unit_test(kord-protocol-test-random-access tests/test_random_access.cpp)
    add_unit_test(kord-protocol-test-write-random-access tests/test_write_random_access.cpp)
    add_unit_test(kord-protocol-test-status tests/test_status.cpp)
    add_unit_test(kord-protocol-test-content tests/test_content.cpp)

    include(GoogleTest)

    message("   Adding tests")
    foreach (test ${kr_tests})
        message("      Test: ${test}")
        gtest_discover_tests(${test})
    endforeach ()
endif ()

#############
## Package ##
#############

# CPack configuration
set(CPACK_PACKAGE_NAME ${KORD_PROTOCOL_PACKAGE_NAME})
set(CPACK_PACKAGE_DESCRIPTION "KORD Protocol Library")
set(CPACK_PACKAGE_CONTACT "Kassow Robots <support@kassowrobots.com>")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Kassow Robots <support@kassowrobots.com>")
# set(CPACK_DEBIAN_PACKAGE_DEPENDS "")  # Add dependencies if needed

# Component-based packaging
set(CPACK_DEB_COMPONENT_INSTALL ON)
set(CPACK_COMPONENTS_ALL ${KORD_LIB_COMPONENT_NAME} ${KORD_LIBS_DEV_COMPONENT_NAME} ${KORD_PROTOCOL_TESTS_COMPONENT_NAME})
set(CPACK_DEB_EXCLUDE_COMPONENTS "tests;gmock;gtest")

set(CPACK_SET_DESTDIR true)
set(CMAKE_INSTALL_PREFIX "/usr/local" CACHE PATH "Install prefix")

message(STATUS "Using install prefix: ${CMAKE_INSTALL_PREFIX}")

# Use build number for the package version if provided
if ("${KORD_PROTOCOL_BUILD_NO}" GREATER_EQUAL 0)
    message(STATUS "Build no ${KORD_PROTOCOL_BUILD_NO} used.")
    set(CPACK_PACKAGE_VERSION ${KORD_PROTOCOL_FULL_VERSION}-${KORD_PROTOCOL_BUILD_NO})
else ()
    message(STATUS "No KORD_PROTOCOL_BUILD_NO value used.")
    set(CPACK_PACKAGE_VERSION ${KORD_PROTOCOL_FULL_VERSION})
endif ()
message(STATUS "CPACK_PACKAGE_VERSION is ${CPACK_PACKAGE_VERSION}")

# Runtime library
string(TOUPPER "${KORD_LIB_COMPONENT_NAME}" KORD_LIB_COMPONENT_NAME_UPPER)
set(CPACK_COMPONENT_${KORD_LIB_COMPONENT_NAME_UPPER}_DESCRIPTION "Runtime library for KORD Protocol")
set(CPACK_DEBIAN_${KORD_LIB_COMPONENT_NAME_UPPER}_PACKAGE_NAME ${KORD_PROTOCOL_PACKAGE_NAME}-libs)
set(CPACK_DEBIAN_${KORD_LIB_COMPONENT_NAME_UPPER}_FILE_NAME "${KORD_LIB_COMPONENT_NAME}_${CPACK_PACKAGE_VERSION}_${ARCH}.deb")

# Development files
string(TOUPPER "${KORD_LIBS_DEV_COMPONENT_NAME}" KORD_LIBS_DEV_COMPONENT_NAME_UPPER)
set(CPACK_COMPONENT_${KORD_LIBS_DEV_COMPONENT_NAME_UPPER}_DESCRIPTION "Development files for KORD Protocol")
set(CPACK_DEBIAN_${KORD_LIBS_DEV_COMPONENT_NAME_UPPER}_PACKAGE_NAME ${KORD_PROTOCOL_PACKAGE_NAME}-libs-dev)
set(CPACK_DEBIAN_${KORD_LIBS_DEV_COMPONENT_NAME_UPPER}_FILE_NAME "${KORD_LIBS_DEV_COMPONENT_NAME}_${CPACK_PACKAGE_VERSION}_${ARCH}.deb")

# Tests
if (KORD_PROTOCOL_WITH_TESTS)
    string(TOUPPER "${KORD_PROTOCOL_TESTS_COMPONENT_NAME}" KORD_PROTOCOL_TESTS_COMPONENT_NAME_UPPER)
    set(CPACK_DEBIAN_${KORD_PROTOCOL_TESTS_COMPONENT_NAME_UPPER}_DESCRIPTION "Tests for KORD Protocol")
    set(CPACK_DEBIAN_${KORD_PROTOCOL_TESTS_COMPONENT_NAME_UPPER}_PACKAGE_NAME ${KORD_PROTOCOL_PACKAGE_NAME}-tests)
    set(CPACK_DEBIAN_${KORD_PROTOCOL_TESTS_COMPONENT_NAME_UPPER}_FILE_NAME "${KORD_PROTOCOL_TESTS_COMPONENT_NAME}_${CPACK_PACKAGE_VERSION}_${ARCH}.deb")
    set(CPACK_DEBIAN_${KORD_PROTOCOL_TESTS_COMPONENT_NAME_UPPER}_PACKAGE_DEPENDS "${KORD_PROTOCOL_PACKAGE_NAME}-libs(=${CPACK_PACKAGE_VERSION})")
endif ()

# Include CPack if INSTALL_KORD_PROTOCOL is ON
if (KORD_PROTOCOL_PACKAGE_TARGETS)
    message(STATUS "Including KORD Protocol targets in the CPack package")
    include(CPack)
endif ()

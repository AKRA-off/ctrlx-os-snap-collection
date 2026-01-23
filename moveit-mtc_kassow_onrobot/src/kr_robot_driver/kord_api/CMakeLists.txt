cmake_minimum_required(VERSION 3.14)

# Check for in-source builds
if (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there. You may need to remove CMakeCache.txt.")
endif ()

# Set architecture
if (NOT DEFINED ENV{ARCH})
    execute_process(COMMAND dpkg --print-architecture OUTPUT_VARIABLE ARCH OUTPUT_STRIP_TRAILING_WHITESPACE)
else ()
    set(ARCH $ENV{ARCH})
endif ()
string(TOLOWER "${ARCH}" ARCH) # Force lowercase architecture

# Options
option(KORD_WITH_TESTS "Compile with tests" OFF)
option(KORD_WITH_EXAMPLES "Compile the project including examples" OFF)
option(KORD_WITH_COVERAGE "Enable coverage reporting" OFF)
option(KORD_WITH_DOCS "Build the documentation" OFF)
option(KORD_BUILD_NO "Optionally add to the debian package version when >= 0" -1)

option(INSTALL_KORD_API "Install the kord-api package" ON)
option(INSTALL_KORD_PROTOCOL "Install the kord-protocol package" ON)
option(KORD_API_PACKAGE_TARGETS "Include the kord-api targets in the CPack package" ON)

# Read version from file
file(READ "version.txt" KORD_LIB_FULL_VERSION)
string(STRIP "${KORD_LIB_FULL_VERSION}" KORD_LIB_FULL_VERSION)  # Remove whitespace/newlines

# Validate version format
if (NOT KORD_LIB_FULL_VERSION MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+.*$")
    message(FATAL_ERROR "Invalid version format in version.txt: ${KORD_LIB_FULL_VERSION}")
endif ()

set(KORD_PROTOCOL_VERSION v${KORD_LIB_FULL_VERSION} CACHE STRING "Branch, tag or commit of the kord-protocol to use")
set(KORD_PROTOCOL_NAMESPACE "kassowrobots" CACHE STRING "Namespace of the kord-protocol to use, required in the CI and during development")

string(REGEX MATCH "^[0-9]+\\.[0-9]+\\.[0-9]+" KORD_LIB_SEMVER ${KORD_LIB_FULL_VERSION})
if (NOT KORD_LIB_SEMVER)
    message(FATAL_ERROR "Invalid semver format in version.txt: ${KORD_LIB_FULL_VERSION}")
endif ()

# Set project name and version
set(KORD_LIB_NAME "kord-api")
set(KORD_LIB_VERSION ${KORD_LIB_SEMVER})
project(${KORD_LIB_NAME} VERSION ${KORD_LIB_VERSION})

message(STATUS "Building project: " ${KORD_LIB_NAME})

# Package details
set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "Kassow Open Realtime Data")
set(CPACK_PACKAGE_CONTACT "Kassow Robots <support@kassowrobots.com>")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Kassow Robots <support@kassowrobots.com>")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://gitlab.com/kassowrobots/kord-api/")
set(CPACK_DEB_COMPONENT_INSTALL ON)
set(CPACK_DEB_EXCLUDE_COMPONENTS "tests;gmock;gtest")

set(CPACK_SET_DESTDIR true)
set(CMAKE_INSTALL_PREFIX "/usr/local" CACHE PATH "Install prefix")

message(STATUS "Using install prefix: ${CMAKE_INSTALL_PREFIX}")

# Use build number for the package version if provided
if ("${KORD_BUILD_NO}" GREATER_EQUAL 0)
    message(STATUS "Build no ${KORD_BUILD_NO} used.")
    set(CPACK_PACKAGE_VERSION ${KORD_LIB_FULL_VERSION}-${KORD_BUILD_NO})
else ()
    message(STATUS "No KORD_BUILD_NO value used.")
    set(CPACK_PACKAGE_VERSION ${KORD_LIB_FULL_VERSION})
endif ()
message(STATUS "CPACK_PACKAGE_VERSION is ${CPACK_PACKAGE_VERSION}")

# Package the libraries
set(KORD_PACKAGE_NAME "${KORD_LIB_NAME}" CACHE STRING "CPack package name")
set(KORD_PROTOCOL_PACKAGE_NAME "kord-protocol" CACHE STRING "CPack package name for the kord-protocol")

set(KORD_LIB_COMPONENT "libs")
set(KORD_LIBS_DEV_COMPONENT_NAME "${KORD_LIB_COMPONENT}-dev")
string(TOUPPER ${KORD_LIB_COMPONENT} CPACK_LIB_NAME)
set(CPACK_DEBIAN_${CPACK_LIB_NAME}_PACKAGE_NAME "${KORD_PACKAGE_NAME}-${KORD_LIB_COMPONENT}")
set(CPACK_DEBIAN_${CPACK_LIB_NAME}_FILE_NAME "${KORD_LIB_NAME}-${KORD_LIB_COMPONENT}_${CPACK_PACKAGE_VERSION}_${ARCH}.deb")
set(CPACK_DEBIAN_${CPACK_LIB_NAME}_PACKAGE_ARCHITECTURE "${ARCH}")
set(CPACK_DEBIAN_${CPACK_LIB_NAME}_DESCRIPTION "Libraries for ${CPACK_DEBIAN_PACKAGE_DESCRIPTION}" CACHE STRING "" FORCE)
set(CPACK_DEBIAN_${CPACK_LIB_NAME}_PACKAGE_DEPENDS "${KORD_PROTOCOL_PACKAGE_NAME}-libs(=${CPACK_PACKAGE_VERSION})")
set(CPACK_COMPONENTS_ALL ${KORD_LIB_COMPONENT})

# Set the generator type
set(CPACK_GENERATOR "DEB")

set(CMAKE_VERBOSE_MAKEFILE OFF)
set(CMAKE_CXX_STANDARD 20)

# Set the default gcov executable (can be overridden by the user)
set(GCOV_EXECUTABLE "/usr/bin/gcov" CACHE STRING "Path to the gcov executable")

# Set the source and include directories for filtering coverage (relative to the project root)
set(COVERAGE_FILTER_SRC "${CMAKE_SOURCE_DIR}/src" CACHE STRING "Path to the source directory for coverage filtering")
set(COVERAGE_FILTER_INCLUDE "${CMAKE_SOURCE_DIR}/include" CACHE STRING "Path to the include directory for coverage filtering")

# Output directory for coverage reports
set(COVERAGE_FOLDER coverage CACHE STRING "Target folder for coverage data")
set(COVERAGE_OUTPUT_DIR "${CMAKE_BINARY_DIR}/${COVERAGE_FOLDER}" CACHE STRING "Directory for coverage report output")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

execute_process(
        COMMAND git log -1 --format=%H
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_SHA1
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/include/kord/version.h.in
        ${CMAKE_CURRENT_BINARY_DIR}/include/kord/version.h
)

# Use GNUInstallDirs to define installation directories
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
find_package(Threads REQUIRED)

if (KORD_WITH_EXAMPLES)
    find_package(Eigen3 REQUIRED)
    if (TARGET Eigen3::Eigen)
        message("Eigen includes: ${EIGEN3_INCLUDE_DIR}")
    else ()
        error("Eigen3 not found. Install the Eigen3 library to allow compilation with examples.")
    endif ()
endif ()

# Use local kord-protocol v2.0.2 (compatible with robot firmware)
message(STATUS "Using local kord-protocol from external/kord-protocol")

# Set kord-protocol options before add_subdirectory
set(KORD_PROTOCOL_WITH_TESTS ${KORD_WITH_TESTS})
set(KORD_PROTOCOL_WITH_COVERAGE ${KORD_WITH_COVERAGE})
set(INSTALL_KORD_PROTOCOL ${INSTALL_KORD_PROTOCOL})
set(KORD_PROTOCOL_PACKAGE_TARGETS ${KORD_API_PACKAGE_TARGETS})
set(KORD_PROTOCOL_BUILD_NO ${KORD_BUILD_NO})
set(KORD_PROTOCOL_PACKAGE_NAME ${KORD_PROTOCOL_PACKAGE_NAME})

# Add local kord-protocol subdirectory
add_subdirectory(external/kord-protocol)

if (KORD_WITH_DOCS)
    find_package(Doxygen REQUIRED)
    if (NOT DOXYGEN_FOUND)
        message(FATAL_ERROR "Doxygen is needed to build the documentation.")
    endif ()
    message(STATUS "Doxygen executable: ${DOXYGEN_EXECUTABLE}")

    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/docs DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

    # Configure doxygen
    configure_file(
            ${CMAKE_CURRENT_BINARY_DIR}/docs/Doxyfile.in
            ${CMAKE_CURRENT_BINARY_DIR}/docs/Doxyfile
    )

    # Add doxygen target
    add_custom_target(docs
            COMMAND rm -rf ${CMAKE_CURRENT_BINARY_DIR}/docs/_build ${CMAKE_CURRENT_BINARY_DIR}/docs/_static ${CMAKE_CURRENT_BINARY_DIR}/docs/_doxy_out
            COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/docs/Doxyfile && make html
            COMMAND mv ${CMAKE_CURRENT_BINARY_DIR}/docs/_build/html ${CMAKE_CURRENT_BINARY_DIR}/docs_html
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/docs
    )
endif ()

# Asio
add_library(kord-asio INTERFACE)
target_include_directories(kord-asio INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/external/kord/asio>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/kord/asio>
)
target_link_libraries(kord-asio INTERFACE Threads::Threads)
target_compile_definitions(kord-asio INTERFACE ASIO_STANDALONE)

# Add coverage flags if enabled
if (KORD_WITH_COVERAGE)
    message(STATUS "Building with code coverage enabled")
    message(STATUS "Coverage output directory: ${COVERAGE_OUTPUT_DIR}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage --coverage -O0")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")

    add_custom_target(coverage
            COMMAND mkdir ${COVERAGE_OUTPUT_DIR} && gcovr -v -r ${CMAKE_SOURCE_DIR}
            --gcov-executable ${GCOV_EXECUTABLE}
            --object-directory ${CMAKE_BINARY_DIR}/CMakeFiles
            --filter="${COVERAGE_FILTER_SRC}"
            --filter="${COVERAGE_FILTER_INCLUDE}"
            --html-details ${COVERAGE_OUTPUT_DIR}/coverage.html
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Generating code coverage report with gcovr"
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

# Build tests if enabled
if (KORD_WITH_TESTS)
    add_subdirectory(tests)
endif ()

# Build the library
add_library(${KORD_LIB_NAME}
        SHARED
        src/api/kord.cpp
        src/api/connection_interface.cpp
        src/api/kord_control_interface.cpp
        src/api/kord_receive_interface.cpp
        src/api/kord_io_request.cpp
        src/utils/timex.cpp
        src/utils/utils.cpp
)

set_target_properties(${KORD_LIB_NAME} PROPERTIES
        VERSION ${KORD_LIB_VERSION}
        SOVERSION 1
)

# Define headers for this library. PUBLIC headers are used for
# compiling the library, and will be added to consumers' build
# paths.
target_include_directories(${KORD_LIB_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/external>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# Link the library
target_link_libraries(${KORD_LIB_NAME} PUBLIC kord-protocol::kord-protocol Threads::Threads kord-asio)

if (INSTALL_KORD_API)
    # Install the library
    install(TARGETS ${KORD_LIB_NAME}
            EXPORT ${KORD_LIB_NAME}Targets
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            COMPONENT ${KORD_LIB_COMPONENT}
    )

    # Install asio
    install(TARGETS kord-asio
            EXPORT ${KORD_LIB_NAME}Targets
            INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/kord/asio
    )

    # Export the targets
    install(EXPORT ${KORD_LIB_NAME}Targets
            FILE ${KORD_LIB_NAME}Targets.cmake
            NAMESPACE ${KORD_LIB_NAME}::
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${KORD_LIB_NAME}
            COMPONENT ${KORD_LIBS_DEV_COMPONENT_NAME}
    )

    # This makes the project importable from the build directory
    export(EXPORT ${KORD_LIB_NAME}Targets
            FILE "${CMAKE_CURRENT_BINARY_DIR}/${KORD_LIB_NAME}Targets.cmake"
            NAMESPACE ${KORD_LIB_NAME}::)
endif ()

# Build examples if enabled
if (KORD_WITH_EXAMPLES)
    message(STATUS "Building with examples")

    # Package the examples
    set(KORD_EXAMPLES_COMPONENT_NAME "examples")
    string(TOUPPER ${KORD_EXAMPLES_COMPONENT_NAME} KORD_EXAMPLES_NAME)
    set(CPACK_DEBIAN_${KORD_EXAMPLES_NAME}_PACKAGE_NAME "${KORD_PACKAGE_NAME}-examples")
    set(CPACK_DEBIAN_${KORD_EXAMPLES_NAME}_FILE_NAME "${KORD_LIB_NAME}-${KORD_EXAMPLES_COMPONENT_NAME}_${CPACK_PACKAGE_VERSION}_${ARCH}.deb")
    set(CPACK_DEBIAN_${KORD_EXAMPLES_NAME}_PACKAGE_DEPENDS "${KORD_PACKAGE_NAME}-libs(=${CPACK_PACKAGE_VERSION})")
    set(CPACK_DEBIAN_${KORD_EXAMPLES_NAME}_DESCRIPTION "Examples for ${CPACK_DEBIAN_PACKAGE_DESCRIPTION}" CACHE STRING "" FORCE)
    set(CPACK_COMPONENTS_ALL ${CPACK_COMPONENTS_ALL} ${KORD_EXAMPLES_COMPONENT_NAME})

    add_library(polynomials SHARED
            src/polynomials/from_constraints.cpp
            src/polynomials/polynomials.cpp
            src/polynomials/spline_computation.cpp
            src/polynomials/spline_equations.cpp
    )

    target_include_directories(
            polynomials
            PUBLIC
            ${EIGEN3_INCLUDE_DIR}
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    )

    add_executable(kord-move-joints examples/kord_move_joints.cpp)
    add_executable(kord-move-linear examples/kord_move_linear.cpp)
    add_executable(kord-move-linear-quat examples/kord_move_linear_quat.cpp)
    add_executable(kord-move-velocity examples/kord_move_velocity.cpp)
    add_executable(kord-read-state examples/kord_read_state.cpp)
    add_executable(kord-brake-control examples/kord_brake_control.cpp)
    add_executable(kord-transfer-logs examples/kord_transfer_logs.cpp)
    add_executable(kord-retrieve-errors examples/kord_retrieve_errors.cpp)
    add_executable(kord-move-direct examples/kord_move_direct.cpp)
    add_executable(kord-move-joints-discrete examples/kord_move_joints_discrete.cpp)
    add_executable(kord-move-linear-discrete examples/kord_move_linear_discrete.cpp)
    add_executable(kord-set-oport examples/kord_set_oport.cpp)
    add_executable(kord-self-motion examples/kord_self_motion.cpp)
    add_executable(kord-self-motion-interactive examples/kord_self_motion_interactive.cpp)
    add_executable(kord-read-temperature examples/kord_read_temperature.cpp)
    add_executable(kord-set-frame examples/kord_set_frame.cpp)
    add_executable(kord-set-load examples/kord_set_load.cpp)
    add_executable(kord-get-tcp-and-cog examples/kord_get_tcp_and_cog.cpp)
    add_executable(kord-clean-alarm examples/kord_clean_alarm.cpp)
    add_executable(kord-transfer-json examples/kord_transfer_json.cpp)
    add_executable(kord-transfer-calibration-data examples/kord_transfer_calibration_data.cpp)
    add_executable(kord-transfer-more-files examples/kord_transfer_more_files.cpp)
    add_executable(kord-init examples/kord_init.cpp)
    add_executable(kord-fetch-data examples/kord_fetch_data.cpp)
    add_executable(kord-skip-fetch-data examples/kord_skip_fetch_data.cpp)
    add_executable(kord-init-and-fetch examples/kord_init_and_fetch.cpp)
    add_executable(kord-move-direct-interactive examples/kord_move_direct_interactive.cpp)
    add_executable(kord-set-safe-output examples/kord_set_safe_output.cpp)
    add_executable(kord-move-direct-torque examples/kord_move_direct_torque.cpp)
    add_executable(kord-read-versions examples/kord_read_versions.cpp)
    add_executable(kord-software-update examples/kord_software_update.cpp)
    add_executable(kord-read-safety-zones examples/kord_read_safety_zones.cpp)
    add_executable(kord-read-robot-info examples/kord_read_robot_info.cpp)
    add_executable(kord-perf-test examples/kord_perf_test.cpp)
    add_executable(kord-move-joints-autorecover examples/kord_move_joints_autorecover.cpp)

    target_link_libraries(kord-transfer-more-files ${KORD_LIB_NAME})
    target_link_libraries(kord-set-safe-output ${KORD_LIB_NAME})
    target_link_libraries(kord-move-joints ${KORD_LIB_NAME})
    target_link_libraries(kord-move-linear ${KORD_LIB_NAME})
    target_link_libraries(kord-move-linear-quat ${KORD_LIB_NAME})
    target_link_libraries(kord-move-direct ${KORD_LIB_NAME})
    target_link_libraries(kord-self-motion ${KORD_LIB_NAME})
    target_link_libraries(kord-self-motion-interactive ${KORD_LIB_NAME} ncurses)
    target_link_libraries(kord-move-joints-discrete ${KORD_LIB_NAME})
    target_link_libraries(kord-move-linear-discrete ${KORD_LIB_NAME})
    target_link_libraries(kord-move-velocity ${KORD_LIB_NAME})
    target_link_libraries(kord-read-state ${KORD_LIB_NAME})
    target_link_libraries(kord-brake-control ${KORD_LIB_NAME})
    target_link_libraries(kord-transfer-logs ${KORD_LIB_NAME})
    target_link_libraries(kord-retrieve-errors ${KORD_LIB_NAME})
    target_link_libraries(kord-transfer-json ${KORD_LIB_NAME})
    target_link_libraries(kord-transfer-calibration-data ${KORD_LIB_NAME})
    target_link_libraries(kord-transfer-more-files ${KORD_LIB_NAME})
    target_link_libraries(kord-init ${KORD_LIB_NAME})
    target_link_libraries(kord-fetch-data ${KORD_LIB_NAME})
    target_link_libraries(kord-skip-fetch-data ${KORD_LIB_NAME})
    target_link_libraries(kord-init-and-fetch ${KORD_LIB_NAME})
    target_link_libraries(kord-move-direct-interactive ${KORD_LIB_NAME} polynomials Eigen3::Eigen ncurses ${SDL2_LIBRARIES})
    target_link_libraries(kord-move-direct-torque ${KORD_LIB_NAME})
    target_link_libraries(kord-set-oport ${KORD_LIB_NAME})
    target_link_libraries(kord-read-temperature ${KORD_LIB_NAME})
    target_link_libraries(kord-set-frame ${KORD_LIB_NAME})
    target_link_libraries(kord-set-load ${KORD_LIB_NAME})
    target_link_libraries(kord-get-tcp-and-cog ${KORD_LIB_NAME})
    target_link_libraries(kord-clean-alarm ${KORD_LIB_NAME})
    target_link_libraries(kord-read-versions ${KORD_LIB_NAME})
    target_link_libraries(kord-software-update ${KORD_LIB_NAME})
    target_link_libraries(kord-read-safety-zones ${KORD_LIB_NAME})
    target_link_libraries(kord-read-robot-info ${KORD_LIB_NAME})
    target_link_libraries(kord-perf-test ${KORD_LIB_NAME})
    target_link_libraries(kord-move-joints-autorecover ${KORD_LIB_NAME})

    if (INSTALL_KORD_API)
        # Install the examples
        install(TARGETS
                kord-move-joints
                kord-move-linear
                kord-move-linear-quat
                kord-move-velocity
                kord-read-state
                kord-brake-control
                kord-transfer-logs
                kord-retrieve-errors
                kord-move-direct
                kord-move-joints-discrete
                kord-move-linear-discrete
                kord-set-oport
                kord-self-motion
                kord-self-motion-interactive
                kord-read-temperature
                kord-set-frame
                kord-set-load
                kord-get-tcp-and-cog
                kord-clean-alarm
                kord-transfer-json
                kord-transfer-calibration-data
                kord-transfer-more-files
                kord-init
                kord-fetch-data
                kord-skip-fetch-data
                kord-init-and-fetch
                kord-move-direct-interactive
                kord-set-safe-output
                kord-move-direct-torque
                kord-read-versions
                kord-software-update
                kord-read-safety-zones
                kord-read-robot-info
                kord-perf-test
                kord-move-joints-autorecover
                EXPORT ${KORD_LIB_NAME}ExamplesTargets
                RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                COMPONENT ${KORD_EXAMPLES_COMPONENT_NAME}
        )

        # Export the example targets
        install(EXPORT ${KORD_LIB_NAME}ExamplesTargets
                FILE ${KORD_LIB_NAME}ExamplesTargets.cmake
                NAMESPACE ${KORD_LIB_NAME}::
                DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${KORD_LIB_NAME}
                COMPONENT ${KORD_EXAMPLES_COMPONENT_NAME}
        )

        install(TARGETS polynomials
                EXPORT ${KORD_LIB_NAME}ExamplesTargets
                COMPONENT ${KORD_EXAMPLES_COMPONENT_NAME}
                RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        )
    endif ()
endif ()

# Development package
string(TOUPPER "${KORD_LIBS_DEV_COMPONENT_NAME}" CPACK_LIB_DEV_NAME)
set(CPACK_DEBIAN_${CPACK_LIB_DEV_NAME}_PACKAGE_NAME "${KORD_PACKAGE_NAME}-libs-dev")
set(CPACK_DEBIAN_${CPACK_LIB_DEV_NAME}_FILE_NAME "${KORD_LIB_NAME}-${KORD_LIBS_DEV_COMPONENT_NAME}_${CPACK_PACKAGE_VERSION}_${ARCH}.deb")
set(CPACK_DEBIAN_${CPACK_LIB_DEV_NAME}_DESCRIPTION "Development artifacts for ${CPACK_DEBIAN_PACKAGE_DESCRIPTION}" CACHE STRING "" FORCE)
set(CPACK_DEBIAN_${CPACK_LIB_DEV_NAME}_PACKAGE_DEPENDS "${KORD_PROTOCOL_PACKAGE_NAME}-libs-dev(=${CPACK_PACKAGE_VERSION})")
set(CPACK_COMPONENTS_ALL ${CPACK_COMPONENTS_ALL} ${KORD_LIBS_DEV_COMPONENT_NAME})

if (INSTALL_KORD_API)
    # Install the headers
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            COMPONENT ${KORD_LIBS_DEV_COMPONENT_NAME}
            FILES_MATCHING PATTERN "*.h")
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/external/kord
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            COMPONENT ${KORD_LIBS_DEV_COMPONENT_NAME}
            FILES_MATCHING PATTERN "CRC.h" PATTERN "*.hpp" PATTERN "*.ipp")

    # Install the generated version header
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/kord/version.h
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/kord
            COMPONENT ${KORD_LIBS_DEV_COMPONENT_NAME}
    )

    # Create and install the package config file
    configure_package_config_file(
            "${CMAKE_CURRENT_SOURCE_DIR}/cmake/${KORD_LIB_NAME}Config.cmake.in"
            "${CMAKE_CURRENT_BINARY_DIR}/${KORD_LIB_NAME}Config.cmake"
            INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${KORD_LIB_NAME}
    )

    # Generate and install the version file
    write_basic_package_version_file(
            "${CMAKE_CURRENT_BINARY_DIR}/${KORD_LIB_NAME}ConfigVersion.cmake"
            VERSION ${PROJECT_VERSION}
            COMPATIBILITY SameMajorVersion
    )

    install(FILES
            "${CMAKE_CURRENT_BINARY_DIR}/${KORD_LIB_NAME}Config.cmake"
            "${CMAKE_CURRENT_BINARY_DIR}/${KORD_LIB_NAME}ConfigVersion.cmake"
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${KORD_LIB_NAME}
            COMPONENT ${KORD_LIBS_DEV_COMPONENT_NAME}
    )

    # Uninstall target
    if (NOT TARGET uninstall)
        configure_file(
                "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
                "${CMAKE_CURRENT_BINARY_DIR}/cmake/cmake_uninstall.cmake"
                IMMEDIATE @ONLY)

        add_custom_target(uninstall
                COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake/cmake_uninstall.cmake)
    endif ()
endif ()

# Include CPack for packaging if the package is being installed
if (INSTALL_KORD_API)
    include(CPack)
endif ()

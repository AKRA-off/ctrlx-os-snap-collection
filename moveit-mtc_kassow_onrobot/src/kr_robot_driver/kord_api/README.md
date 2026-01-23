# KORD API

This project is a client-side API for KORD (Kassow Open Realtime Data). It aims to provide an abstraction layer for
real-time access to Kassow Robots' RC (Robot Controller) and its control loop.

Please note that this project is currently in development, and additional information will be provided soon. Stay tuned
for updates.

## Install CBun

To enable the KORD API on external systems, the **KORD** CBun must be installed and activated on the Kassow Robots RC.
This module provides the necessary connectivity and access to the RC software.

1. Download the Master CBun from the [provided link](https://gitlab.com/kassowrobots/kord-api/-/wikis/Master-CBun).
2. Copy the `.cbun` file onto a USB drive.
3. Use the CBun Manager on the Kassow Robots RC to install it.

## Clone repository

```bash
$ git clone https://gitlab.com/kassowrobots/kord-api.git
```

## Compilation

Requirements:

- CMake 3.14 or higher
- C++20 compiler
- libeigen3-dev
- libboost-dev
- libncurses-dev (for examples)

On Ubuntu/Debian, you can install these prerequisites:

```bash
sudo apt update
sudo apt install cmake libncurses-dev libeigen3-dev libboost-dev
```

## Quick Start: Building the Library

- Clone this repository
- Configure and build:

```bash
cd kord-api
cmake -S . -B build
cmake --build build -- -j $(nproc)
 ```

This will produce the kord-api library (`libkord-api.so`) and link it against `kord-protocol` (fetched automatically).

### Configuration Options

The main CMakeLists.txt offers several options:

- `KORD_WITH_EXAMPLES` (OFF by default)
    - Builds example programs (e.g. `kord-move-joints`, `kord-brake-control`, etc.).
- `KORD_WITH_TESTS` (OFF by default)
    - Builds and enables test executables.
- `KORD_WITH_COVERAGE` (OFF by default)
    - Compiles with coverage flags (`-fprofile-arcs -ftest-coverage`, etc.).
- `KORD_WITH_DOCS` (OFF by default)
    - Builds documentation (requires Doxygen).

You can enable these via command line, for example:

```bash
cmake -S . -B build \
      -DKORD_WITH_EXAMPLES=ON \
      -DKORD_WITH_TESTS=ON \
      -DKORD_WITH_COVERAGE=ON

cmake --build build -- -j "$(nproc)"
```

### Other Advanced Options

- `INSTALL_KORD_API` (_ON_ by default)
    - Controls whether the `kord-api` library (and headers) are installed.
- `INSTALL_KORD_PROTOCOL` (_ON_ by default)
    - Controls whether the `kord-protocol` library (and headers) are installed.
- `KORD_PROTOCOL_VERSION` (default: master)
    - Git branch/tag/commit to fetch for `kord-protocol`.
- `KORD_PROTOCOL_NAMESPACE` (default: `kassowrobots`)
    - Namespace (group/owner) in GitLab for `kord-protocol`.
- `KORD_API_PACKAGE_TARGETS` (_ON_ by default)
    - Controls packaging of the `kord-api` targets.
- `CMAKE_INSTALL_PREFIX` (default: `/usr/local`)
    - Installation prefix for the library, headers, etc.
- `GCOV_EXECUTABLE` (default: `/usr/bin/gcov`)
    - Path to the `gcov` executable (should match the compiler’s version).
- `COVERAGE_OUTPUT_DIR` (default: `build/coverage`)
    - Directory where the coverage report (`coverage.html`) is generated.

## Generating Debian Packages

After building successfully, you can generate .deb packages (for the library, dev headers, examples, etc.) using CPack:

1. Enter the build directory:

```bash
cd build
```

2. Run:

```bash
make package
# or
cpack -G DEB
```

This will produce .deb files such as:

- `kord-api-lib_<version>_<arch>.deb`
- `kord-api-lib-dev_<version>_<arch>.deb`
- (If `KORD_WITH_EXAMPLES=ON`) `kord-api-examples_<version>_<arch>.deb`

To install:

```bash
sudo dpkg -i kord-api-lib_<version>_<arch>.deb
sudo dpkg -i kord-api-examples_<version>_<arch>.deb
```

(Substitute actual filenames as needed.)

> Note: By default, the kord-protocol library is fetched and installed side by side in its own package. Make sure to
> install `kord-protocol` first.

## Coverage

If you enabled coverage (`-DKORD_WITH_COVERAGE=ON`), you can generate an HTML report via:

```bash
# from the build directory
make coverage
```

This will place the generated HTML in the folder specified by `COVERAGE_OUTPUT_DIR` (default: `build/coverage`).

> Important: Make sure you run tests first so `.gcda` files get generated.

## Documentation

If `KORD_WITH_DOCS` is ON and you have Doxygen installed, you can run:

```bash
make docs
```

The generated HTML documentation will be placed in `build/docs_html`.

Detailed documentation for the latest KORD API is available on the following public link:

- [Documentation - KORD API](https://kassowrobots.gitlab.io/kord-api-doc)

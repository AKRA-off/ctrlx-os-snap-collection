# Changelog

All notable changes to this project will be documented in this file.

## [v2.0.2] - 2025-06-16

### Changed

- Changed the type of torque deviation data from `double` to `float` to match the type of the data in the status frame.
  This change is backward compatible with the previous version.
- Improved CBun's logging.
  
### Fixed

- Fixed the issue with the torque deviation data not being correctly populated.
- Fixed the issue when `waitSync` was not working as expected when there were buffered RSHB frames. Now, it reads the
latest available frame.
- Fixed memory leak on KORD CBun's side causing the robot to freeze after sending large number of requests.
- Fixed the issue when log transferring was occasionally failing.

## [v2.0.1] - 2025-05-14

### Changed

- Transferred directories and files now have 777 and 666 permissions, respectively, to ensure proper access rights.
- Remove "NA" return value for unknown fields in `protocol::ResponseGetRobotInfo`. Now, it returns an empty string
  instead.

### Fixed

- Addressed the incorrect order of load inertia values.
- Fixed CRC errors for brake engage and disengage commands.

### Security

- Added security checks to prevent the bash command injection.

## [v2.0.0] - 2025-04-03

### Added

- Introduced a new system event to handle and log infeasible move commands, enhancing error reporting and system
  reliability.
- Implemented motion constraints: KORD CBun now proactively verifies whether incoming motion commands fall within the
  permitted limits and rejects any movement that violates these constraints.
- Added logger.
- Added support for reading joint poses represented with quaternion.
- Added response/request communication channel with support for:
    - Reading of robot information - robot model, serial numbers, ToolIO presence.
    - Reading of safety zones.
    - Fetching versions of software, firmware and hardware components.
- Added new test for motion constraints to ensure the robustness and reliability of this feature.
- Added `kord_move_linear_quat` example to demonstrate linear motion with quaternion poses.
- Added `kord_move_joints_autorecover` example to demonstrate how to operate the robot with autorecover functionality (
  e.g., when torque deviation is detected).
- Added new method `ControlInterface::moveL` with support for quaternion poses.
- Added new system event condition enum `kord::protocol::EKordEventConditionID`.
- Added new flag `kord::protocol::ESoftStopEventConditionID::INIT_JOINTS_LOCKED` to indicate that the joints breaks were
  not successfully released during the initialization process.
- Documentation changes:
    - Introduced minor changes to the documentation structure to improve readability.
    - Fixed numerous typos and formatting issues in the documentation.
    - Added version selector to the documentation.
    - Added motion constraints documentation with detailed explanations of joint and workspace constraints.
    - Added safety zones documentation.
    - Added full API reference available at `API > API Reference` section.
- CMake changes:
    - Added `KORD_WITH_DOCS` option to enable building the documentation.
    - Added `INSTALL_KORD_API` option that enables the installation of the KORD API package.
    - Added `KORD_API_PACKAGE_TARGETS` option that allows users to specify if they want to include targets in CPack.
    - Added `KORD_PACKAGE_NAME` options to allow users to specify the package names for the KORD API. If using custom
      name, please make sure to package kord-protocol with the same name and specify `KORD_PROTOCOL_PACKAGE_NAME`
      option.
    - Added new target `docs` to build the documentation.
- Add .clang-format file to ensure consistent code formatting across the project.
- Added CI pipeline for the deployment of the KORD API, including documentation and package generation.

### Changed

- Modified examples to enhance code organization and readability.
- Renamed `test` directory to `tests`.

### Breaking Changes

- Moved all headers to the `include/kord` directory. Users must update their include paths to use the new directory
  structure.
- Renamed examples binaries' names to follow a consistent pattern, e.g., `kord_move_joints` to `kord-move-joints`.
- Refactored CMake configurations to improve packaging and build process.
- Updated .deb package names to `kord-api-{lib,lib-dev,examples}_<version>_<arch>.deb`.
- `kord::protocol::ESoftStopEventConditionID::CBUN_KORD_BAD_CONN_QUALITY` was moved to
  `kord::protocol::EKordEventCondition::CBUN_KORD_BAD_CONN_QUALITY`. Users must update their code to use the new enum
  value.
- Removed kord-protocol submodule in favor of using the FetchContent feature of CMake.

### Fixed

- Addressed issue when clearing alarm stopped working after one hundred commands.
- Lowered timeout period when receiving UDP frames for better responsiveness.
- Moved `asio` and `CRC.h` to `external/kord` to unify the include paths.

### Security

- No security-related fixes or updates in this release.

### Removed

- Removed `ReceiveInterface::getServerResponseFlag` method.
- Removed obsolete service status and progress variables from the status frame.
- Removed `xcode` folder.

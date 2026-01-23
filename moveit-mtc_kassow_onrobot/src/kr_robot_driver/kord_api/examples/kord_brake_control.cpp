#include <atomic>
#include <chrono>
#include <cmath>
#include <csignal>
#include <iomanip>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <kord/api/kord.h>
#include <kord/api/kord_control_interface.h>
#include <kord/api/kord_receive_interface.h>
#include <kord/utils/logger.h>
#include <kord/utils/utils.h>

using namespace kr2;

// Atomic flag for controlling the running state
static std::atomic<bool> g_run{true};

// Struct to hold extra command-line options
struct ExtraOptions {
    std::optional<std::vector<int>> to_lock{};
    std::optional<std::vector<int>> to_unlock{};
    bool engage_mode = true;
};

// Signal handler for graceful shutdown
void signal_handler(int a_signum)
{
    psignal(a_signum, "[KORD-API]");
    g_run = false;
}

int main(int argc, char *argv[])
{
    // Initialize extra options
    ExtraOptions extras;

    // External argument parser lambda
    static utils::LaunchParameters::ExternalArgParser ep = [argc, &argv, &extras](int index) -> void {
        static utils::SOALongOptions ex_options{
            std::array<utils::LongOption,
                       2>{utils::LongOption{{"engage", optional_argument, nullptr, 'e'},
                                            "Lock the specified joints, locks all joints if none provided",
                                            "<1,2,3,4,5,6,7>"},
                          utils::LongOption{{"disengage", optional_argument, nullptr, 'd'},
                                            "Unlock the specified joints, unlocks all joints if none provided",
                                            "<1,2,3,4,5,6,7>"}}};

        if (index <= utils::LaunchParameters::INVALID_INDEX) {
            std::cout << ex_options.helpString() << std::endl;
            return;
        }

        int option_index = 0;
        optind = index;
        int opt = getopt_long(argc, argv, "e:d:", ex_options.getLongOptions(), &option_index);

        switch (opt) {
        case 'e': {
            extras.engage_mode = true;
            std::stringstream ss(optarg);
            std::string item;
            std::vector<int> values;

            while (std::getline(ss, item, ',')) {
                try {
                    values.push_back(std::stoi(item));
                }
                catch (const std::invalid_argument &e) {
                    KORD_LOG_ERROR("Invalid argument for engage option: " << item);
                    exit(EXIT_FAILURE);
                }
            }

            extras.to_lock.emplace(values);
            break;
        }
        case 'd': {
            extras.engage_mode = false;
            std::stringstream ss(optarg);
            std::string item;
            std::vector<int> values;

            while (std::getline(ss, item, ',')) {
                try {
                    values.push_back(std::stoi(item));
                }
                catch (const std::invalid_argument &e) {
                    KORD_LOG_ERROR("Invalid argument for disengage option: " << item);
                    exit(EXIT_FAILURE);
                }
            }

            extras.to_unlock.emplace(values);
            break;
        }
        default: {
            std::cerr << "Unknown option found. optidx: " << optind << ", argc: " << argc << std::endl;
            exit(EXIT_FAILURE);
        }
        }
    };

    // Process launch arguments
    utils::LaunchParameters lp = utils::LaunchParameters::processLaunchArguments(argc, argv, ep);

    if (lp.help_ || !lp.valid_) {
        return EXIT_SUCCESS;
    }

    // Register signal handler
    signal(SIGINT, signal_handler);

    // Initialize real-time parameters if required
    if (lp.useRealtime()) {
        if (!utils::realtime::init_realtime_params(lp.rt_prio_)) {
            KORD_LOG_ERROR("Failed to start with realtime priority");
            utils::LaunchParameters::printUsage(true);
            return EXIT_FAILURE;
        }
    }

    // Logging connection details
    KORD_LOG_INFO("Connecting to: " << lp.remote_controller_ << ":" << lp.port_);
    KORD_LOG_INFO("Session ID: " << lp.session_id_);

    // Create shared pointer for KordCore
    auto kord = std::make_shared<kord::KordCore>(lp.remote_controller_, lp.port_, lp.session_id_, kord::UDP_CLIENT);

    // Initialize control and receiver interfaces
    kord::ControlInterface ctl_iface(kord);
    kord::ReceiverInterface rcv_iface(kord);

    // Attempt to connect to Kord
    if (!kord->connect()) {
        KORD_LOG_ERROR("Connecting to KR failed");
        return EXIT_FAILURE;
    }

    g_run = true;

    // Synchronize with RC
    if (!kord->syncRC()) {
        KORD_LOG_ERROR("Sync RC failed.");
        return EXIT_FAILURE;
    }
    KORD_LOG_INFO("Sync Captured");

    // Fetch initial data
    rcv_iface.fetchData();
    auto start = std::chrono::steady_clock::now();

    // Main loop
    while (g_run) {
        // Wait for the heartbeat
        if (!kord->waitSync(std::chrono::milliseconds(10))) {
            KORD_LOG_ERROR("Sync wait timed out, exiting.");
            break;
        }

        // Prepare to engage or disengage brakes
        int64_t token = 0;
        if (extras.engage_mode) {
            KORD_LOG_INFO("Engaging brakes...");
            if (!extras.to_lock.has_value()) {
                ctl_iface.engageBrakes({1, 2, 3, 4, 5, 6, 7}, token);
            }
            else {
                ctl_iface.engageBrakes(extras.to_lock.value(), token);
            }
        }
        else {
            KORD_LOG_INFO("Disengaging brakes...");
            if (!extras.to_unlock.has_value()) {
                ctl_iface.disengageBrakes({1, 2, 3, 4, 5, 6, 7}, token);
            }
            else {
                ctl_iface.disengageBrakes(extras.to_unlock.value(), token);
            }
        }

        // Wait for acknowledgment
        if (!kord->waitSync(std::chrono::milliseconds(20))) {
            KORD_LOG_ERROR("Sync wait timed out, exiting.");
            break;
        }

        KORD_LOG_INFO("Command sent with token: " << token);

        // Poll for command status
        while (rcv_iface.getCommandStatus(token) == -1 && g_run) {
            if (!kord->waitSync(std::chrono::milliseconds(20))) {
                KORD_LOG_ERROR("Sync wait timed out while waiting for command status, exiting.");
                g_run = false;
                break;
            }

            rcv_iface.fetchData();
        }

        // Retrieve and log the command status
        int8_t status = rcv_iface.getCommandStatus(token);
        if (status != -1) {
            KORD_LOG_INFO("Command status: " << static_cast<int>(status));
        }
        else {
            KORD_LOG_WARN("Command status remains unknown.");
        }

        // Exit after processing the command
        break;
    }

    // Final logging and cleanup
    auto end = std::chrono::steady_clock::now();
    KORD_LOG_INFO("Operation completed.");
    KORD_LOG_INFO("Runtime: " << std::chrono::duration<double>(end - start).count() << " [s]");
    KORD_LOG_INFO("SafetyFlags: " << rcv_iface.getRobotSafetyFlags());
    KORD_LOG_INFO("MotionFlags: " << rcv_iface.getMotionFlags());
    KORD_LOG_INFO("RCState: " << rcv_iface.getRobotSafetyFlags());

    kord->printStats(rcv_iface.getStatisticsStructure());

    return EXIT_SUCCESS;
}

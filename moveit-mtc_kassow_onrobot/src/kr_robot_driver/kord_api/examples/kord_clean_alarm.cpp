#include <chrono>
#include <cmath>
#include <csignal>
#include <iomanip>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <atomic>
#include <thread>
#include <map>

#include <kord/api/kord.h>
#include <kord/api/kord_control_interface.h>
#include <kord/api/kord_receive_interface.h>
#include <kord/utils/logger.h>
#include <kord/utils/utils.h>

using namespace kr2;

// Atomic flag for controlling the running state
static std::atomic<bool> stop{false};

// Mapping of EClearRequest enums to their string representations
const std::map<kord::ControlInterface::EClearRequest, std::string> enum2str{
    {kord::ControlInterface::EClearRequest::CBUN_EVENT, "CBUN_EVENT"},
    {kord::ControlInterface::EClearRequest::CLEAR_HALT, "CLEAR_HALT"},
    {kord::ControlInterface::EClearRequest::UNSUSPEND, "UNSUSPEND"},
    {kord::ControlInterface::EClearRequest::CONTINUE_INIT, "CONTINUE_INIT"},
};

// Signal handler for graceful shutdown
void signal_handler(int a_signum)
{
    psignal(a_signum, "[KORD-API]");
    stop = true;
}

int main(int argc, char *argv[])
{
    // Struct to hold extra command-line options
    struct ExtraOptions {
        bool clear_all = false;
        kord::ControlInterface::EClearRequest command_to_RC = kord::ControlInterface::EClearRequest::CLEAR_HALT;
    } extras;

    // External argument parser lambda
    static utils::LaunchParameters::ExternalArgParser ep = [argc, &argv, &extras](int index) -> void {
        static kr2::utils::SOALongOptions ex_options{
            std::array<kr2::utils::LongOption, 5>{
                kr2::utils::LongOption{
                    {"all", no_argument, nullptr, 'a'},
                    "Clear all errors"
                },
                kr2::utils::LongOption{
                    {"halt", no_argument, nullptr, 'm'},
                    "Clear halt on the controller"
                },
                kr2::utils::LongOption{
                    {"unsuspend", no_argument, nullptr, 'u'},
                    "Unsuspend the robot"
                },
                kr2::utils::LongOption{
                    {"init", no_argument, nullptr, 'n'},
                    "Continue robot initialization if blocked"
                },
                kr2::utils::LongOption{
                    {"cbun", no_argument, nullptr, 'b'},
                    "Acknowledge CBun error and clear it"
                }
            }
        };

        if (index <= utils::LaunchParameters::INVALID_INDEX) {
            std:: cerr << ex_options.helpString() << std::endl;
            return;
        }

        int option_index = 0;
        optind = index;
        int opt = getopt_long(argc, argv, "amunb", ex_options.getLongOptions(), &option_index);

        switch (opt) {
            case 'a':
                extras.clear_all = true;
                break;
            case 'm':
                extras.command_to_RC = kord::ControlInterface::EClearRequest::CLEAR_HALT;
                break;
            case 'u':
                extras.command_to_RC = kord::ControlInterface::EClearRequest::UNSUSPEND;
                break;
            case 'n':
                extras.command_to_RC = kord::ControlInterface::EClearRequest::CONTINUE_INIT;
                break;
            case 'b':
                extras.command_to_RC = kord::ControlInterface::EClearRequest::CBUN_EVENT;
                break;
            default:
                std::cerr << "Unknown option found. optidx: " << optind << ", argc: " << argc << std::endl;
                exit(EXIT_FAILURE);
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

    // Synchronize with RC
    if (!kord->syncRC()) {
        KORD_LOG_ERROR("Sync RC failed.");
        return EXIT_FAILURE;
    }
    KORD_LOG_INFO("Sync Captured");

    // Determine commands to send based on options
    std::vector<kord::ControlInterface::EClearRequest> commands;
    if (extras.clear_all) {
        commands = {
            kord::ControlInterface::EClearRequest::CLEAR_HALT,
            kord::ControlInterface::EClearRequest::CBUN_EVENT,
            kord::ControlInterface::EClearRequest::CONTINUE_INIT,
            kord::ControlInterface::EClearRequest::UNSUSPEND
        };
    }
    else {
        commands = {extras.command_to_RC};
    }

    // Iterate over each command and process
    for (const auto &command : commands) {
        int64_t token = ctl_iface.clearAlarmRequest(command);

        KORD_LOG_INFO(enum2str.at(command) << " command sent with token: " << token);

        // Poll for command status
        while (rcv_iface.getCommandStatus(token) == -1 && !stop) {
            if (!kord->waitSync(std::chrono::milliseconds(10), kord::F_SYNC_FULL_ROTATION)) {
                KORD_LOG_ERROR("Sync wait timed out, exiting.");
                break;
            }

            rcv_iface.fetchData();
        }

        // Retrieve and log the command status
        auto status = rcv_iface.getCommandStatus(token);
        if (status != -1) {
            KORD_LOG_INFO(enum2str.at(command) << " command status: " << static_cast<int>(status));
        } else {
            KORD_LOG_WARN(enum2str.at(command) << " command status remains unknown.");
        }
    }

    // Final logging and cleanup
    KORD_LOG_INFO("Operation completed.");

    return EXIT_SUCCESS;
}

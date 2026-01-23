#include <atomic>
#include <chrono>
#include <cmath>
#include <csignal>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

#include <kord/api/api_request.h>
#include <kord/api/kord.h>
#include <kord/api/kord_control_interface.h>
#include <kord/api/kord_receive_interface.h>
#include <kord/system/RobotControllerFlags.h>
#include <kord/utils/logger.h>
#include <kord/utils/utils.h>

using namespace std::chrono_literals;
using namespace kr2;
using namespace kr2::kord::protocol;

// Atomic flag for controlling the running state
static std::atomic stop_flag{false};

// Signal handler for graceful shutdown
void signal_handler(int a_signum)
{
    psignal(a_signum, "[KORD-API]");
    stop_flag = true;
}

int main(int argc, char *argv[])
{
    // Process launch arguments
    utils::LaunchParameters lp = utils::LaunchParameters::processLaunchArguments(argc, argv);

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

    // Obtain initial q values
    if (!kord->syncRC()) {
        KORD_LOG_ERROR("Sync RC failed.");
        return EXIT_FAILURE;
    }
    KORD_LOG_INFO("Sync Captured");
    rcv_iface.fetchData();

    int counter = 0;

    // Check and handle INIT state
    if (rcv_iface.getMotionFlags() & EMotionFlags::MOTION_FLAG_INIT ||
        rcv_iface.getMotionFlags() & EMotionFlags::MOTION_FLAG_REINIT) {
        KORD_LOG_INFO("In init state, sending the command...");

        if (!kord->waitSync(std::chrono::milliseconds(10))) {
            KORD_LOG_ERROR("Sync wait timed out, exiting.");
        }

        kord::RequestRCAPICommand sys_request;
        sys_request.asUserConsent().addPayload(ERCAPIPayloadCmdConsentId::eInit);
        ctl_iface.transmitRequest(sys_request);

        KORD_LOG_INFO("Init command sent");
        KORD_LOG_INFO("Now we wait until it is applied");

        while (!stop_flag) {
            if (!kord->waitSync(std::chrono::milliseconds(100))) {
                KORD_LOG_ERROR("Sync wait timed out, exiting.");
                break;
            }
            rcv_iface.fetchData();

            if (rcv_iface.getHWFlags() & HW_STAT_INIT_RUID_MISMATCH) {
                KORD_LOG_WARN("Missing calibration, fetch data required");
                break;
            }

            if (!(rcv_iface.getMotionFlags() & EMotionFlags::MOTION_FLAG_INIT) &&
                !(rcv_iface.getMotionFlags() & EMotionFlags::MOTION_FLAG_REINIT)) {
                KORD_LOG_INFO("Not in INIT state anymore");
                break;
            }

            if (counter > 10000) {
                KORD_LOG_ERROR("Init command timed out, exiting.");
                return EXIT_FAILURE;
            }

            counter++;
        }
    }
    else {
        KORD_LOG_INFO("Not in INIT state, ignored and nothing sent");
    }

    KORD_LOG_INFO("Operation terminated gracefully.");
    return EXIT_SUCCESS;
}

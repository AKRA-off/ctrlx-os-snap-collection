#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <map>
#include <memory>
#include <thread>

#include <kord/api/api_request.h>
#include <kord/api/kord.h>
#include <kord/api/kord_control_interface.h>
#include <kord/api/kord_receive_interface.h>
#include <kord/protocol/RequestResponses/ResponseServer.h>
#include <kord/protocol/ServerParameters.h>
#include <kord/protocol/ServerServiceStatus.h>
#include <kord/system/RobotControllerFlags.h>
#include <kord/utils/logger.h>
#include <kord/utils/utils.h>

using namespace std::chrono_literals;
using namespace kr2;
using namespace kr2::kord::protocol;

// Atomic flag for controlling the running state
static std::atomic<bool> stop{false};

// Signal handler for graceful shutdown
void signal_handler(int a_signum)
{
    psignal(a_signum, "[KORD-API]");
    stop = true;
}

// Mapping of EServiceStatus enums to their string representations
const std::map<EServiceStatus, std::string> serviceStatusToStr{
    {EServiceStatus::eProgress, "In Progress"},
    {EServiceStatus::eSuccess, "Success"},
    {EServiceStatus::eFailed, "Failed"},
    {EServiceStatus::eIdle, "Idle"},
};

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

    // Synchronize with RC
    if (!kord->syncRC()) {
        KORD_LOG_ERROR("Sync RC failed.");
        return EXIT_FAILURE;
    }
    KORD_LOG_INFO("Sync Captured");

    // Fetch initial data
    rcv_iface.fetchData();

    // Create and transmit a system communication request
    kord::RequestSystem request;
    request.asServerCommunication(true);
    ctl_iface.transmitRequest(request);

    // Wait for synchronization
    if (!kord->waitSync(std::chrono::milliseconds(100))) {
        KORD_LOG_ERROR("Sync wait timed out, exiting.");
    }
    rcv_iface.fetchData();

    // Wait to ensure the connection is established
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Check for hardware flags indicating a mismatch
    if (rcv_iface.getHWFlags() & HW_STAT_INIT_RUID_MISMATCH) {
        auto parameters = std::make_shared<ServiceFetchKincalParameters>(true);
        auto req = kord::RequestServer().asServiceRequest(EServerServiceCommands::eStart,
                                                          EKORDServerServiceID::eFetchKincalData,
                                                          parameters);

        if (!kord->waitSync(std::chrono::milliseconds(100))) {
            KORD_LOG_ERROR("Sync wait timed out, exiting.");
        }
        rcv_iface.fetchData();

        ctl_iface.transmitRequest(req);
        KORD_LOG_INFO("Kincal fetch start was sent successfully");
        // Sleep to prevent the request from being overridden
        std::this_thread::sleep_for(std::chrono::seconds(3));
    }

    if (!kord->waitSync(std::chrono::milliseconds(100))) {
        KORD_LOG_ERROR("Sync wait timed out, exiting.");
    }
    rcv_iface.fetchData();

    // Create and transmit a status request
    auto status_req = kord::RequestServer().asServiceRequest(EServerServiceCommands::eGetStatus,
                                                             EKORDServerServiceID::eFetchKincalData);
    auto token = ctl_iface.transmitRequest(status_req);

    // Wait until the response is received or the operation is stopped
    const int idle_threshold = 20;
    int idle_counter = 0;

    while (!stop) {
        if (!kord->waitSync(std::chrono::milliseconds(100))) {
            KORD_LOG_ERROR("Sync wait timed out, exiting.");
            break;
        }

        if (idle_counter > idle_threshold) {
            stop = true;
            break;
        }

        rcv_iface.fetchData();

        if (rcv_iface.hasResponse(token)) {
            auto response = rcv_iface.getResponse<kord::protocol::ServerResponse>(token);
            uint status = response.getStatus();
            uint progress = response.getProgress();

            KORD_LOG_INFO("Response - progress: " << progress << ", status: " << status);

            auto it = serviceStatusToStr.find(static_cast<EServiceStatus>(status));
            if (it != serviceStatusToStr.end()) {
                KORD_LOG_INFO(it->second);
            }
            else {
                KORD_LOG_WARN("Unknown service status received.");
            }

            if (status == static_cast<uint8_t>(EServiceStatus::eSuccess) ||
                status == static_cast<uint8_t>(EServiceStatus::eFailed) ||
                status == static_cast<uint8_t>(EServiceStatus::eIdle)) {
                stop = true;
            }

            // Transmit the status request again if needed
            token = ctl_iface.transmitRequest(status_req);
        }

        std::this_thread::sleep_for(1s);
        idle_counter++;
    }

    // Stop the service by transmitting a stop request
    auto stop_req =
        kord::RequestServer().asServiceRequest(EServerServiceCommands::eStop, EKORDServerServiceID::eFetchKincalData);
    ctl_iface.transmitRequest(stop_req);

    if (!kord->waitSync(std::chrono::milliseconds(100))) {
        KORD_LOG_ERROR("Sync wait timed out, exiting.");
        return EXIT_FAILURE;
    }

    // Disable server communication
    request.asServerCommunication(false);
    ctl_iface.transmitRequest(request);

    if (!kord->waitSync(std::chrono::milliseconds(100))) {
        KORD_LOG_ERROR("Sync wait timed out, exiting.");
        return EXIT_FAILURE;
    }

    KORD_LOG_INFO("Operation completed successfully.");
    return EXIT_SUCCESS;
}

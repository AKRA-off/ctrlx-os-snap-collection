#include <atomic>
#include <chrono>
#include <cmath>
#include <csignal>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

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

    // Check if the robot is in INIT or REINIT state
    auto motion_flags = rcv_iface.getMotionFlags();
    bool in_init_state =
        (motion_flags & EMotionFlags::MOTION_FLAG_INIT) || (motion_flags & EMotionFlags::MOTION_FLAG_REINIT);

    if (!in_init_state) {
        KORD_LOG_INFO("Not in INIT state, ignored and nothing sent");
        return EXIT_SUCCESS;
    }

    KORD_LOG_INFO("Sending the init command...");

    // Wait for synchronization before sending the init command
    if (!kord->waitSync(std::chrono::milliseconds(10))) {
        KORD_LOG_ERROR("Sync wait timed out, exiting.");
    }

    // Create and transmit the init command
    kr2::kord::RequestRCAPICommand sys_request;
    sys_request.asUserConsent().addPayload(ERCAPIPayloadCmdConsentId::eInit);
    ctl_iface.transmitRequest(sys_request);

    KORD_LOG_INFO("Init Command sent");
    KORD_LOG_INFO("Now we wait until it is applied");

    int counter = 0;

    // Wait loop until the INIT state is cleared or a signal is received
    while (!stop_flag) {
        if (!kord->waitSync(std::chrono::milliseconds(100))) {
            KORD_LOG_ERROR("Sync wait timed out, exiting.");
            break;
        }
        rcv_iface.fetchData();

        // Check if kincal fetch is required due to hardware flags mismatch
        if (rcv_iface.getHWFlags() & HW_STAT_INIT_RUID_MISMATCH) {
            KORD_LOG_WARN("Missing calibration, fetch data required");

            // Enable server communication
            kord::RequestSystem request;
            request.asServerCommunication(true);
            ctl_iface.transmitRequest(request);

            // Wait for synchronization
            if (!kord->waitSync(std::chrono::milliseconds(100))) {
                KORD_LOG_ERROR("Sync wait timed out, exiting.");
            }
            rcv_iface.fetchData();

            // Sleep to ensure the connection is established
            std::this_thread::sleep_for(std::chrono::seconds(2));

            // Create and transmit the Kincal fetch start command
            auto parameters = std::make_shared<ServiceFetchKincalParameters>(false); // false indicates no ToolIO
            auto req = kord::RequestServer().asServiceRequest(EServerServiceCommands::eStart,
                                                              EKORDServerServiceID::eFetchKincalData,
                                                              parameters);

            if (!kord->waitSync(std::chrono::milliseconds(1000))) {
                KORD_LOG_ERROR("Sync wait timed out, exiting.");
            }
            rcv_iface.fetchData();

            ctl_iface.transmitRequest(req);
            KORD_LOG_INFO("Kincal fetch start was sent successfully");

            // Sleep to prevent the request from being overridden
            std::this_thread::sleep_for(std::chrono::seconds(5));

            if (!kord->waitSync(std::chrono::milliseconds(100))) {
                KORD_LOG_ERROR("Sync wait timed out, exiting.");
            }
            rcv_iface.fetchData();

            // Create and transmit the status request
            auto status_req = kord::RequestServer().asServiceRequest(EServerServiceCommands::eGetStatus,
                                                                     EKORDServerServiceID::eFetchKincalData);
            auto token = ctl_iface.transmitRequest(status_req);

            // Wait until the response is received or the operation is stopped
            const int idle_threshold = 20;
            int idle_counter = 0;
            while (!stop_flag) {
                if (!kord->waitSync(std::chrono::milliseconds(100))) {
                    KORD_LOG_ERROR("Sync wait timed out, exiting.");
                    break;
                }
                if (idle_counter > idle_threshold) {
                    stop_flag = true;
                    break;
                }

                rcv_iface.fetchData();
                if (rcv_iface.hasResponse(token)) {
                    auto response = rcv_iface.getResponse<kord::protocol::ServerResponse>(token);
                    uint progress = response.getProgress();
                    uint status = response.getStatus();

                    KORD_LOG_INFO("Response - progress: " << progress << ", status: " << status);

                    switch (static_cast<EServiceStatus>(status)) {
                    case EServiceStatus::eSuccess:
                        KORD_LOG_INFO("Success");
                        stop_flag = true;
                        break;
                    case EServiceStatus::eFailed:
                        KORD_LOG_INFO("Failed");
                        stop_flag = true;
                        break;
                    case EServiceStatus::eIdle:
                        KORD_LOG_INFO("Service is not running");
                        idle_counter++;
                        stop_flag = true;
                        break;
                    default:
                        KORD_LOG_WARN("Unknown service status received.");
                        break;
                    }

                    // Transmit the status request again if needed
                    token = ctl_iface.transmitRequest(status_req);
                }
                std::this_thread::sleep_for(1s);
            }

            // Stop the service by transmitting a stop request
            auto stop_req = kord::RequestServer().asServiceRequest(EServerServiceCommands::eStop,
                                                                   EKORDServerServiceID::eFetchKincalData);
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
        }

        if (!rcv_iface.getMotionFlags() & EMotionFlags::MOTION_FLAG_INIT &&
            !rcv_iface.getMotionFlags() & EMotionFlags::MOTION_FLAG_REINIT) {
            KORD_LOG_INFO("Not in INIT state anymore");
            break;
        }

        if (counter > 10000) {
            KORD_LOG_ERROR("Init command timed out, exiting.");
            return EXIT_FAILURE;
        }

        counter++;
    }

    KORD_LOG_INFO("Operation terminated gracefully.");
    return EXIT_SUCCESS;
}

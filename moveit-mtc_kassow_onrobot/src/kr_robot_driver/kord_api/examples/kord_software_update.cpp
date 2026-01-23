#include <chrono>
#include <csignal>
#include <iostream>
#include <memory>
#include <thread>

#include <kord/api/api_request.h>
#include <kord/api/kord.h>
#include <kord/api/kord_control_interface.h>
#include <kord/api/kord_receive_interface.h>
#include <kord/protocol/RequestResponses/ResponseSoftwareUpdate.h>
#include <kord/protocol/ServerParameters.h>
#include <kord/protocol/ServerServiceStatus.h>
#include <kord/utils/utils.h>

using namespace kr2;
using namespace kr2::kord::protocol;
using namespace std::chrono_literals;

volatile bool stop = false;

void signal_handler(sig_atomic_t a_signum)
{
    psignal(a_signum, "[KORD-API]");
    stop = true;
}

int main(int argc, char *argv[])
{
    utils::LaunchParameters lp = utils::LaunchParameters::processLaunchArguments(argc, argv);

    if (lp.help_ || !lp.valid_) {
        return EXIT_SUCCESS;
    }

    signal(SIGINT, signal_handler);

    if (lp.useRealtime()) {
        if (!utils::realtime::init_realtime_params(lp.rt_prio_)) {
            KORD_LOG_ERROR("Failed to start with realtime priority");
            utils::LaunchParameters::printUsage(false);
            return EXIT_FAILURE;
        }
    }

    KORD_LOG_INFO("Connecting to: " << lp.remote_controller_ << ":" << lp.port_);
    KORD_LOG_INFO("Session ID: " << lp.session_id_);

    auto kord = std::make_shared<kord::KordCore>(lp.remote_controller_, lp.port_, lp.session_id_, kord::UDP_CLIENT);

    kord::ControlInterface ctl_iface(kord);
    kord::ReceiverInterface rcv_iface(kord);

    int64_t last_token;

    if (!kord->connect()) {
        KORD_LOG_ERROR("Connecting to KR failed");
        return EXIT_FAILURE;
    }

    if (!kord->syncRC()) {
        KORD_LOG_ERROR("Sync RC failed.");
        return EXIT_FAILURE;
    }
    KORD_LOG_INFO("Sync Captured");

    rcv_iface.fetchData();

    kord::RequestSystem request;
    request.asServerCommunication(true);
    last_token = ctl_iface.transmitRequest(request);

    while (!stop) {
        if (!kord->waitSync(100ms)) {
            KORD_LOG_ERROR("Sync wait timed out, exit");
        }
        rcv_iface.fetchData();
        if (rcv_iface.hasResponse(last_token)) {
            auto response = rcv_iface.getResponse<kord::protocol::Response>(last_token);
            KORD_LOG_INFO("Enabled Python interpreter");
            break;
        }
        std::this_thread::sleep_for(1s);
    }

    if (!kord->waitSync(1s)) {
        KORD_LOG_ERROR("Sync wait timed out, exit");
    }
    rcv_iface.fetchData();

    // Need to wait some time so the connection is established
    std::this_thread::sleep_for(2s);

    auto parameters = std::make_shared<ServiceSoftwareUpdateParameters>("update.tar.gz");
    auto req = kord::RequestServer().asServiceRequest(EServerServiceCommands::eStart,
                                                      EKORDServerServiceID::eSoftwareUpdate,
                                                      parameters);

    if (!kord->waitSync(1s)) {
        KORD_LOG_ERROR("Sync wait timed out, exit");
    }
    rcv_iface.fetchData();

    ctl_iface.transmitRequest(req);
    KORD_LOG_INFO("Software update request was sent successfully");

    auto status_req = kord::RequestServer().asServiceRequest(EServerServiceCommands::eGetStatus,
                                                             EKORDServerServiceID::eSoftwareUpdate);
    int64_t token = ctl_iface.transmitRequest(status_req);

    // Wait til finished
    int idle_threshold = 20;
    int idle_counter = 0;
    while (!stop) {
        if (!kord->waitSync(1s)) {
            KORD_LOG_ERROR("Sync wait timed out, exit");
            break;
        }
        if (idle_counter > idle_threshold) {
            stop = true;
            break;
        }
        rcv_iface.fetchData();
        if (rcv_iface.hasResponse(token)) {
            auto response = rcv_iface.getResponse<kord::protocol::ServerResponse>(token);
            KORD_LOG_INFO("Response - progress: " << static_cast<uint>(response.getProgress())
                                                  << ", status: " << static_cast<uint>(response.getStatus()));

            if (response.getStatus() == static_cast<uint8_t>(EServiceStatus::eSuccess)) {
                KORD_LOG_INFO("Success");
                stop = true;
            }
            else if (response.getStatus() == static_cast<uint8_t>(EServiceStatus::eFailed)) {
                KORD_LOG_ERROR("Failed");
                stop = true;
            }
            else if (response.getStatus() == static_cast<uint8_t>(EServiceStatus::eIdle)) {
                KORD_LOG_ERROR("Service is not running");
                idle_counter++;
                stop = true;
            }

            token = ctl_iface.transmitRequest(status_req);
        }
        std::this_thread::sleep_for(1s);
    }

    // Stop service
    auto stop_req =
        kord::RequestServer().asServiceRequest(EServerServiceCommands::eStop, EKORDServerServiceID::eSoftwareUpdate);
    ctl_iface.transmitRequest(stop_req);

    if (!kord->waitSync(100ms)) {
        KORD_LOG_ERROR("Sync wait timed out, exit");
        return EXIT_FAILURE;
    }

    // Disable python interpreter
    request.asServerCommunication(false);
    ctl_iface.transmitRequest(request);

    if (!kord->waitSync(100ms)) {
        KORD_LOG_ERROR("Sync wait timed out, exit");
        return EXIT_FAILURE;
    }

    return 0;
}

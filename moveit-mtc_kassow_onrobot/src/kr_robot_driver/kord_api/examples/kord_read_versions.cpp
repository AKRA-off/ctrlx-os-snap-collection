#include <csignal>
#include <iostream>

#include <kord/api/kord.h>
#include <kord/api/kord_control_interface.h>
#include <kord/api/kord_receive_interface.h>
#include <kord/protocol/RequestResponses/ResponseGetVersion.h>
#include <kord/utils/utils.h>

using namespace kr2;
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
            utils::LaunchParameters::printUsage(true);
            return EXIT_FAILURE;
        }
    }

    KORD_LOG_INFO("Connecting to: " << lp.remote_controller_ << ":" << lp.port_);
    KORD_LOG_INFO("Session ID: " << lp.session_id_);

    std::shared_ptr<kord::KordCore> kord(
        new kord::KordCore(lp.remote_controller_, lp.port_, lp.session_id_, kord::UDP_CLIENT));

    kord::ControlInterface ctl_iface(kord);
    kord::ReceiverInterface rcv_iface(kord);

    if (!kord->connect()) {
        KORD_LOG_ERROR("Connecting to KR failed");
        return EXIT_FAILURE;
    }

    std::vector<double> q(7);

    // Obtain initial q values
    if (!kord->syncRC()) {
        KORD_LOG_ERROR("Sync RC failed.");
        return EXIT_FAILURE;
    }

    auto request = kord::RequestSystem().asGetVersion();
    auto token = ctl_iface.transmitRequest(request);

    while (!stop) {
        if (!kord->waitSync(100ms)) {
            KORD_LOG_ERROR("Sync failed");
            break;
        }
        rcv_iface.fetchData();
        if (rcv_iface.hasResponse(token)) {
            auto response = rcv_iface.getResponse<kord::protocol::GetVersionResponse>(token);

            // print all version info
            KORD_LOG_INFO(response);
            // or print specific version info
            // std::cout << response.getVersionInfo(kord::FetchVersionResponse::VersionType::KORD_CBUN) << std::endl;
            // std::cout << response.getVersionInfo(kord::FetchVersionResponse::VersionType::RESPONSIVE_CONTROLLER) <<
            // std::endl; std::cout << response.getVersionInfo(kord::FetchVersionResponse::VersionType::JB_1_MCU_A) <<
            // std::endl;
            break;
        }
    }

    return 0;
}

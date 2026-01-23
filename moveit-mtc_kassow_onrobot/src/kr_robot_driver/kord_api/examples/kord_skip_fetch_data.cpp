#include <chrono>
#include <cmath>
#include <csignal>
#include <iomanip>
#include <iostream>
#include <sstream>

#include <kord/api/api_request.h>
#include <kord/api/kord.h>
#include <kord/api/kord_control_interface.h>
#include <kord/api/kord_receive_interface.h>
#include <kord/system/RobotControllerFlags.h>
#include <kord/utils/utils.h>

using namespace kr2;
using namespace kr2::kord::protocol;
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

    std::shared_ptr<kord::KordCore> kord(
        new kord::KordCore(lp.remote_controller_, lp.port_, lp.session_id_, kord::UDP_CLIENT));

    kord::ControlInterface ctl_iface(kord);
    kord::ReceiverInterface rcv_iface(kord);

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

    if (rcv_iface.getHWFlags() & HW_STAT_INIT_RUID_MISMATCH) {
        KORD_LOG_INFO("In fetch data state, sending the command...");
        if (!kord->waitSync(std::chrono::milliseconds(100))) {
            KORD_LOG_ERROR("Sync wait timed out, exit");
        }
        kr2::kord::RequestRCAPICommand sys_request;
        sys_request.asUserConsent().addPayload(kr2::kord::protocol::ERCAPIPayloadCmdConsentId::eSkipFetch);
        ctl_iface.transmitRequest(sys_request);
    }

    return 0;
}

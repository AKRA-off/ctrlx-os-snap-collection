//
//  - robot !!!MUST!!! be standstill to transfer log files
//  - this test sends request to retrieve logs from the remote controller
//  - the target and the username must be defined in KORD.ini
//

#include <chrono>
#include <cmath>
#include <csignal>
#include <iomanip>
#include <iostream>

#include <kord/api/api_request.h>
#include <kord/api/kord.h>
#include <kord/api/kord_control_interface.h>
#include <kord/api/kord_receive_interface.h>
#include <kord/utils/utils.h>

using namespace kr2;
static bool g_run = true;

//! Will request the KORD Cbun to initiate and transfer log files to target
int64_t requestCalibrationTransfer(kord::KordCore &, kord::ControlInterface &);

//! Will monitor the status of the request to transfer logs until time out, failure, or success
bool monitorCalibrationTransfer(kord::KordCore &, kord::ReceiverInterface &, utils::LaunchParameters &, int64_t);

void signal_handler(int a_signum)
{
    psignal(a_signum, "[KORD-API]");
    g_run = false;
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

    g_run = true;

    // Obtain initial q values
    if (!kord->syncRC()) {
        KORD_LOG_ERROR("Sync RC failed.");
        return EXIT_FAILURE;
    }
    KORD_LOG_INFO("Sync Captured");
    rcv_iface.fetchData();

    // Initiate the execution of log transfer
    int64_t request_id;
    request_id = requestCalibrationTransfer(*kord, ctl_iface);
    if (request_id <= 0) {
        KORD_LOG_INFO("Failed to sent calibration request");
        return EXIT_FAILURE;
    }

    // Waiting for the execution result
    if (!monitorCalibrationTransfer(*kord, rcv_iface, lp, request_id)) {
        KORD_LOG_ERROR("Failure");
        return EXIT_FAILURE;
    }

    KORD_LOG_INFO("Done");
    return EXIT_SUCCESS;
}

int64_t requestCalibrationTransfer(kord::KordCore &a_kord, kord::ControlInterface &a_ctl_iface)
{
    if (!a_kord.waitSync(std::chrono::milliseconds(10))) {
        KORD_LOG_ERROR("Sync wait timed out, exit");
        return -1;
    }

    // Create a request to the remote controller
    kr2::kord::RequestSystem sys_request;
    sys_request.asCalibrationDataTransfer();
    if (!a_ctl_iface.transmitRequest(sys_request)) {
        return -2;
    }

    KORD_LOG_INFO("TX Request    RID: " << sys_request.request_rid_);
    return sys_request.request_rid_;
}

bool monitorCalibrationTransfer(kord::KordCore &a_kord,
                                kord::ReceiverInterface &a_rcv_iface,
                                utils::LaunchParameters &a_lp,
                                int64_t a_req_id)
{
    kr2::kord::Request response;
    while (g_run) {
        a_kord.waitSync(std::chrono::milliseconds(10));
        a_rcv_iface.fetchData();

        response = a_rcv_iface.getLatestRequest();

        if (a_lp.runtimeElapsed()) {
            KORD_LOG_ERROR("TIMEOUT: Request with RID " << response.request_rid_);
            return false;
        }

        // Check if the correct request is being evaluated
        if (a_req_id != response.request_rid_) {
            continue;
        }

        if (response.request_status_ == kr2::kord::protocol::EControlCommandStatus::eSuccess) {
            KORD_LOG_INFO("SUCCESS: Request with RID " << response.request_rid_ << ", transfer finished.");
            return true;
        }

        if (response.request_status_ == kr2::kord::protocol::EControlCommandStatus::eFailure) {
            KORD_LOG_INFO("FAILURE: Request with RID: " << response.request_rid_);
            KORD_LOG_INFO( "               error code: " << response.error_code_);
            return false;
        }
    }

    KORD_LOG_ERROR("Monitor transfer was interrupted");
    return false;
}

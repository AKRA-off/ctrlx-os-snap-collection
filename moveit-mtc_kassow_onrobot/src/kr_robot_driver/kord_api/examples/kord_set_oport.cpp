#include <chrono>
#include <cmath>
#include <csignal>
#include <iomanip>
#include <iostream>

#include <kord/api/kord.h>
#include <kord/api/kord_control_interface.h>
#include <kord/api/kord_io_request.h>
#include <kord/api/kord_receive_interface.h>
#include <kord/utils/utils.h>

using namespace kr2;
volatile bool g_run = true;

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

    g_run = true;

    // Obtain initial q values
    if (!kord->syncRC()) {
        KORD_LOG_ERROR("Sync RC failed.");
        return EXIT_FAILURE;
    }
    KORD_LOG_INFO("Sync Captured");
    rcv_iface.fetchData();

    if (!kord->waitSync(std::chrono::milliseconds(10))) {
        KORD_LOG_ERROR("Sync wait timed out, exit");
        return EXIT_FAILURE;
    }

    // Create a request to the remote controller
    using DIGITAL_RELAYS = kord::RequestIO::DIGITAL_RELAYS;
    using DIGITAL_IOBOARD = kord::RequestIO::DIGITAL_IOBOARD;
    using DIGITAL_IOTOOLB = kord::RequestIO::DIGITAL_IOTOOLB;

    kord::RequestIO io_request;
    io_request.asSetIODigitalOut()
        .withEnabledPorts( // Enable Relay 1, Digital Output 8, Digital Output 6 and Tool Board 1 [24V]
            static_cast<int>(DIGITAL_RELAYS::RELAY1) | static_cast<int>(DIGITAL_IOBOARD::DO8) |
            static_cast<int>(DIGITAL_IOBOARD::DO6) | static_cast<int>(DIGITAL_IOTOOLB::TB1));

    //   .withDisabledPorts( // Disable Relay 1 and Digital Output 6 and Tool Board 1 [24V]
    //     static_cast<int>(DIGITAL_RELAYS::RELAY1) |
    //     static_cast<int>(DIGITAL_IOBOARD::DO6) |
    //     static_cast<int>(DIGITAL_IOTOOLB::TB1));

    kord->sendCommand(io_request);
    KORD_LOG_INFO("TX Request    RID: " << io_request.request_rid_);

    // Waiting for the execution result
    kr2::kord::Request response;
    while (true) {
        kord->waitSync(std::chrono::milliseconds(10));
        rcv_iface.fetchData();

        response = rcv_iface.getLatestRequest();

        if (lp.runtimeElapsed()) {
            KORD_LOG_ERROR("TIMEOUT: Request with RID " << response.request_rid_);
            return EXIT_FAILURE;
        }

        // Check if the correct request is being evaluated
        if (io_request.request_rid_ != response.request_rid_)
            continue;

        if (response.request_status_ == kr2::kord::protocol::EControlCommandStatus::eSuccess) {
            KORD_LOG_INFO("SUCCESS: Request with RID " << response.request_rid_ << ", transfer finished.");
            break;
        }

        if (response.request_status_ == kr2::kord::protocol::EControlCommandStatus::eFailure) {
            KORD_LOG_INFO("FAILURE: Request with RID: " << response.request_rid_);
            return EXIT_FAILURE;
        }
    }

    KORD_LOG_INFO("Done");
    return EXIT_SUCCESS;
}

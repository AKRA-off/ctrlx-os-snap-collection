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

std::map<uint, std::string> CONFIG2NAME{{0, "Disabled"},
                                        {1, "Disabled"},
                                        {2, "Enabled"},
                                        {3, "PStop"},
                                        {4, "EStop"},
                                        {5, "EStop+PStop"}};

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
    signal(SIGTERM, signal_handler);

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
    kr2::kord::RequestIO io_request;
    io_request.asSetIODigitalOut().withEnabledSafePorts(kr2::kord::RequestIO::DIGITAL_SAFE::SDO1,
                                                        ESafePortConfiguration::eSafePortBothMapped);

    kord->sendCommand(io_request);
    KORD_LOG_INFO("TX Request    RID: " << io_request.request_rid_);

    // We can try setting EStop or PStop and check whether digital output changes
    int64_t last_digital_output = -1;
    uint32_t last_safe_do_config = -1;
    kr2::kord::Request response;
    while (g_run) {
        kord->waitSync(std::chrono::milliseconds(10));
        rcv_iface.fetchData();

        int64_t digital_output = rcv_iface.getDigitalOutput();
        if (last_digital_output != digital_output) {
            KORD_LOG_INFO("DO: " << rcv_iface.getDigitalOutput());
            last_digital_output = digital_output;
        }

        uint32_t safe_do_config = rcv_iface.getSafeDigitalOutputConfig();

        if (last_safe_do_config != safe_do_config) {
            KORD_LOG_INFO("SDO1 config: " << CONFIG2NAME[static_cast<uint32_t>((safe_do_config >> 24) & 0xFF)]);
            KORD_LOG_INFO("SDO2 config: " << CONFIG2NAME[static_cast<uint32_t>((safe_do_config >> 16) & 0xFF)]);
            KORD_LOG_INFO("SDO3 config: " << CONFIG2NAME[static_cast<uint32_t>((safe_do_config >> 8) & 0xFF)]);
            KORD_LOG_INFO("SDO4 config: " << CONFIG2NAME[static_cast<uint32_t>((safe_do_config) & 0xFF)]);
            last_safe_do_config = safe_do_config;
        }

        response = rcv_iface.getLatestRequest();

        if (static_cast<uint32_t>((safe_do_config >> 24) & 0xFF) == 5) {
            KORD_LOG_INFO("SUCCESS: Request with RID " << response.request_rid_);
            break;
        }

        if (lp.runtimeElapsed()) {
            KORD_LOG_ERROR("TIMEOUT: Request with RID " << response.request_rid_);
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}

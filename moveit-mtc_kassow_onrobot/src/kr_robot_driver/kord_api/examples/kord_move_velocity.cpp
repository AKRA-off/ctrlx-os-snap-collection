#include <chrono>
#include <cmath>
#include <csignal>
#include <iomanip>
#include <iostream>

#include <kord/api/kord.h>
#include <kord/api/kord_control_interface.h>
#include <kord/api/kord_receive_interface.h>
#include <kord/utils/utils.h>

using namespace kr2;
static bool g_run = true;

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

    if (!kord->syncRC()) {
        KORD_LOG_ERROR("Sync RC failed.");
        return EXIT_FAILURE;
    }
    KORD_LOG_INFO("Sync Captured");
    rcv_iface.fetchData();
    std::vector<std::variant<double, int>> returned_vec_start_tcp =
        rcv_iface.getFrame(kord::EFrameID::TCP_FRAME, kord::EFrameValue::POSE_VAL_REF_WF);
    std::array<double, 6UL> start_tcp{};
    for (size_t i = 0; i < start_tcp.size(); ++i) {
        start_tcp[i] = std::get<double>(returned_vec_start_tcp[i]);
    }

    std::string start_tcp_str;
    for (double &p : start_tcp)
        start_tcp_str += std::to_string(p) + " ";
    KORD_LOG_INFO("Start TCP: " << start_tcp_str);

    // Send 8 velocity control requests
    for (int i = 0; i < 4; i++) {
        std::array<double, 6UL> vel_target = {-0.05, 0.0, 0.0, 0.0, 0.0, 0.0};
        ctl_iface.moveV(vel_target, 0, 0.2, 1);
        usleep(20000);

        vel_target = {0.05, 0.0, 0.0, 0.0, 0.0, 0.0};
        ctl_iface.moveV(vel_target, 0, 0.2, 1);
        usleep(20000);
    }

    // Send velocity control termination
    std::array<double, 6UL> terminate_target = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    ctl_iface.moveV(terminate_target, 0, 0, 0);

    return 0;
}

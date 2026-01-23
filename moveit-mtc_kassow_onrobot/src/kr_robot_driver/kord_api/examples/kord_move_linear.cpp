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
static volatile bool g_run = true;

void signal_handler(int signum)
{
    psignal(signum, "[KORD-API]");
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

    std::array<double, 6UL> tcp_target{};

    g_run = true;

    // Obtain initial q values
    if (!kord->syncRC()) {
        KORD_LOG_ERROR("Sync RC failed.");
        return EXIT_FAILURE;
    }
    KORD_LOG_INFO("Sync Captured");
    rcv_iface.fetchData();
    std::vector<std::variant<double, int>> returned_vec_start_tcp =
        rcv_iface.getFrame(kord::EFrameID::TCP_FRAME, kord::EFrameValue::POSE_VAL_REF_WF);
    std::array<double, 6UL> start_tcp{};
    for (size_t i = 0; i < returned_vec_start_tcp.size(); ++i) {
        start_tcp[i] = std::get<double>(returned_vec_start_tcp[i]);
    }

    std::string tcp_str = "TCP: ";
    for (double &p : start_tcp)
        tcp_str += std::to_string(p) + " ";
    KORD_LOG_INFO(tcp_str);

    unsigned int i = 0;
    double a = 0.01;
    double t = 0.0;
    double tt_value = 0.008;
    double bt_value = 0.004;

    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

    while (g_run) {
        // calculation
        // update tcp target
        tcp_target[0] = start_tcp[0];
        tcp_target[1] = start_tcp[1];
        tcp_target[2] = (std::cos(t * 2e-4) - 1) * a + start_tcp[2];
        tcp_target[3] = start_tcp[3];
        tcp_target[4] = start_tcp[4];
        tcp_target[5] = start_tcp[5];
        t = i * 7;
        i++;

        if (!kord->waitSync(std::chrono::milliseconds(10))) {
            KORD_LOG_ERROR("Sync wait timed out, exit");
            break;
        }

        ctl_iface.moveL(tcp_target,
                        kr2::kord::TrackingType::TT_TIME,
                        tt_value,
                        kr2::kord::BlendType::BT_TIME,
                        bt_value,
                        kr2::kord::OverlayType::OT_VIAPOINT);

        rcv_iface.fetchData();
        if (rcv_iface.systemAlarmState() || lp.runtimeElapsed()) {
            break;
        }
    }

    KORD_LOG_INFO("Robot stopped");
    KORD_LOG_INFO(rcv_iface.getFormattedInputBits());
    KORD_LOG_INFO(rcv_iface.getFormattedOutputBits());
    KORD_LOG_INFO("SystemAlarmState: ");
    KORD_LOG_INFO(rcv_iface.systemAlarmState());
    switch (rcv_iface.systemAlarmState() & 0b1111) {
    case kr2::kord::protocol::EEventGroup::eUnknown:
        KORD_LOG_INFO("No alarms");
        break;
    case kr2::kord::protocol::EEventGroup::eSafetyEvent:
        KORD_LOG_INFO("Safety Event");
        break;
    case kr2::kord::protocol::EEventGroup::eSoftStopEvent:
        KORD_LOG_INFO("Soft Stop Event");
        break;
    case kr2::kord::protocol::EEventGroup::eKordEvent:
        KORD_LOG_INFO("KORD Event");
        break;
    }

    KORD_LOG_INFO("Safety flags: " << rcv_iface.getRobotSafetyFlags());
    auto end_time = std::chrono::steady_clock::now();
    double runtime_seconds = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start).count() / 1000.0;
    KORD_LOG_INFO("Runtime: " << runtime_seconds << " [s]");
    KORD_LOG_INFO("SafetyFlags: " << rcv_iface.getRobotSafetyFlags());
    KORD_LOG_INFO("MotionFlags: " << rcv_iface.getMotionFlags());
    KORD_LOG_INFO("RCState: " << rcv_iface.getRobotSafetyFlags());

    kord->printStats(rcv_iface.getStatisticsStructure());

    // NA when no stats
    return 0;
}

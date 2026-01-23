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

    std::array<double, 7UL> q{};

    g_run = true;

    // Obtain initial q values
    if (!kord->syncRC()) {
        KORD_LOG_ERROR("Sync RC failed.");
        return EXIT_FAILURE;
    }

    KORD_LOG_INFO("Sync Captured");
    rcv_iface.fetchData();
    if (rcv_iface.systemAlarmState()) {
        // notify alarm
    }

    std::array<double, 7UL> start_q = rcv_iface.getJoint(kord::ReceiverInterface::EJointValue::S_ACTUAL_Q);

    std::string initial_j_conf = "Initial joint configuration: ";
    for (double angl : start_q)
        initial_j_conf += std::to_string(angl / 3.14 * 180) + " ";
    KORD_LOG_INFO(initial_j_conf);

    unsigned int i = 0;
    double a = 0.1;
    double t = 0.0;
    double tt_value = 0.008;
    double bt_value = 0.004;

    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    // kord->setStatisticsWindow(500);

    while (g_run) {
        // calculation
        // update q
        q[0] = (std::cos(t * 2e-4) - 1) * a + start_q[0];
        q[1] = (std::cos(t * 3.3e-4) - 1) * a + start_q[1];
        q[2] = (std::cos(t * 4.5e-4) - 1) * a + start_q[2];
        q[3] = (std::cos(t * 2.4e-4) - 1) * a + start_q[3];
        q[4] = (std::cos(t * 6e-4) - 1) * a + start_q[4];
        q[5] = (std::cos(t * 8e-4) - 1) * a + start_q[5];
        q[6] = (std::cos(t * 1e-3) - 1) * a + start_q[6];
        t = i * 7;
        i++;

        if (!kord->waitSync(std::chrono::milliseconds(10))) {
            KORD_LOG_ERROR("Sync wait timed out, exiting");
            break;
        }

        ctl_iface.moveJ(q,
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
    return 0;
}

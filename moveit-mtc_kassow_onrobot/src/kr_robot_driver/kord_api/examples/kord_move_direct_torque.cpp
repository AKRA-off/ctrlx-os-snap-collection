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

    std::array<double, 7UL> torque{};

    g_run = true;

    // Obtain initial q values
    if (!kord->syncRC()) {
        KORD_LOG_ERROR("Sync RC failed.");
        return EXIT_FAILURE;
    }
    KORD_LOG_INFO("Sync Captured");
    rcv_iface.fetchData();
    std::array<double, 7UL> start_trq = rcv_iface.getJoint(kord::ReceiverInterface::EJointValue::S_SENSED_TRQ);

    double A = 10;
    double t = 0;

    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

    while (g_run) {
        torque[0] = start_trq[0] + (std::sin(t * 1e-3) - 1 / 2) * A;
        torque[1] = start_trq[1];
        torque[2] = start_trq[2];
        torque[3] = start_trq[3];
        torque[4] = start_trq[4];
        torque[5] = start_trq[5];
        torque[6] = start_trq[6];
        t++;

        if (!kord->waitSync(std::chrono::milliseconds(10))) {
            KORD_LOG_ERROR("Sync wait timed out, exit");
            break;
        }

        ctl_iface.directTorqueControl(torque);

        rcv_iface.fetchData();
        if (rcv_iface.systemAlarmState()) {
            break;
        }
    }

    KORD_LOG_INFO("Robot stopped");
    KORD_LOG_INFO(rcv_iface.getFormattedInputBits());
    KORD_LOG_INFO(rcv_iface.getFormattedOutputBits());
    KORD_LOG_INFO("SystemAlarmState: ");
    KORD_LOG_INFO(rcv_iface.systemAlarmState());
    KORD_LOG_INFO("SystemAlarmState's Category: ");
    switch (rcv_iface.systemAlarmState() & 0b1111) {
    case kord::protocol::EEventGroup::eUnknown:
        KORD_LOG_INFO("No alarms");
        break;
    case kord::protocol::EEventGroup::eSafetyEvent:
        KORD_LOG_INFO("Safety Event");
        break;
    case kord::protocol::EEventGroup::eSoftStopEvent:
        KORD_LOG_INFO("Soft Stop Event");
        break;
    case kord::protocol::EEventGroup::eKordEvent:
        KORD_LOG_INFO("Kord Event");
        break;
    }

    KORD_LOG_INFO( "Safety flags: " << rcv_iface.getRobotSafetyFlags());
    auto end_time = std::chrono::steady_clock::now();
    double runtime_seconds =
        std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start).count() / 1000.0;
    KORD_LOG_INFO("Runtime: " << runtime_seconds << " [s]");
    KORD_LOG_INFO("SafetyFlags: " << rcv_iface.getRobotSafetyFlags());
    KORD_LOG_INFO("MotionFlags: " << rcv_iface.getMotionFlags());
    KORD_LOG_INFO("RCState: " << rcv_iface.getRobotSafetyFlags());

    kord->printStats(rcv_iface.getStatisticsStructure());
    return 0;
}

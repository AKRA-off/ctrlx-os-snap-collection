#include <chrono>
#include <cmath>
#include <csignal>
#include <iostream>

#include <kord/api/kord.h>
#include <kord/api/kord_control_interface.h>
#include <kord/api/kord_receive_interface.h>

using namespace kr2;

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
    KORD_LOG_INFO("Sync Captured");
    rcv_iface.fetchData();
    std::array<double, 7UL> temp_q = rcv_iface.getJoint(kord::ReceiverInterface::EJointValue::S_TEMP_BOARD);

    std::string jb_temp = "Joints Board Temperatures:";
    for (double temp : temp_q) {
        jb_temp += std::to_string(temp) + ";";
    }
    KORD_LOG_INFO(jb_temp);

    temp_q = rcv_iface.getJoint(kord::ReceiverInterface::EJointValue::S_TEMP_JOINT_ENCODER);
    std::string je_temp = "Joints Encoder Temperatures:";
    for (double temp : temp_q) {
        je_temp += std::to_string(temp) + ";";
    }
    KORD_LOG_INFO(je_temp);

    temp_q = rcv_iface.getJoint(kord::ReceiverInterface::EJointValue::S_TEMP_ROTOR_ENCODER);
    std::string re_temp = "Rotor Encoder Temperatures:";
    for (double temp : temp_q) {
        re_temp += std::to_string(temp) + ";";
    }
    KORD_LOG_INFO(re_temp);

    if (!kord->waitSync(std::chrono::milliseconds(20))) {
        KORD_LOG_ERROR("Sync wait timed out, exit");
    }

    rcv_iface.fetchData();
    double tempIO_q = rcv_iface.getIOBoardTemperature();

    KORD_LOG_INFO("IOBoard Temperature: " << tempIO_q);

    KORD_LOG_INFO("CPU Temperatures");
    KORD_LOG_INFO("(Zeroes returned if **lm-sensors version 1:3.4.0-4** is not installed to the controller)");

    KORD_LOG_INFO("Pack 0: " << rcv_iface.getCPUState(kord::ReceiverInterface::ECPUStateValue::PACKAGE_ID0_TEMP));
    KORD_LOG_INFO("Core 0: " << rcv_iface.getCPUState(kord::ReceiverInterface::ECPUStateValue::CORE_0_TEMP));
    KORD_LOG_INFO("Core 1: " << rcv_iface.getCPUState(kord::ReceiverInterface::ECPUStateValue::CORE_1_TEMP));
    KORD_LOG_INFO("Core 2: " << rcv_iface.getCPUState(kord::ReceiverInterface::ECPUStateValue::CORE_2_TEMP));
    KORD_LOG_INFO("Pack 3: " << rcv_iface.getCPUState(kord::ReceiverInterface::ECPUStateValue::CORE_3_TEMP));

    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

    std::chrono::time_point<std::chrono::steady_clock, std::chrono::seconds> print_stamp =
        std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::steady_clock::now());

    // Infinite loop
    while (!stop) {
        if (!kord->waitSync(std::chrono::milliseconds(10))) {
            KORD_LOG_ERROR("Sync wait timed out, exit");
            break;
        }

        rcv_iface.fetchData();
        auto now_ts = std::chrono::steady_clock::now();
        if (print_stamp < std::chrono::time_point_cast<std::chrono::seconds>(now_ts) &&
            std::chrono::duration_cast<std::chrono::seconds>(now_ts - start).count() % 2) {
            print_stamp = std::chrono::time_point_cast<std::chrono::seconds>(now_ts);
            KORD_LOG_INFO(
                std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count()
                << ", " << rcv_iface.getRobotSafetyFlags() << ", " << rcv_iface.systemAlarmState());

            KORD_LOG_INFO(rcv_iface.systemAlarmState());
        }

        if (lp.runtimeElapsed()) {
            break;
        }
    }

    kord->printStats(rcv_iface.getStatisticsStructure());
    return 0;
}

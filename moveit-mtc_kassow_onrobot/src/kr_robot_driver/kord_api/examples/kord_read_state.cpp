#include <chrono>
#include <cmath>
#include <csignal>
#include <iostream>
#include <sstream>

#include <kord/api/kord.h>
#include <kord/api/kord_control_interface.h>
#include <kord/api/kord_receive_interface.h>
#include <kord/utils/logger.h>
#include <kord/utils/utils.h>

using namespace kr2;

volatile bool stop = false;

void printVector(const std::vector<std::variant<double, int>> &vec, const std::string &label)
{
    std::ostringstream ss;
    ss << label << " [ ";
    for (const auto &val : vec) {
        ss << std::get<double>(val) << " ";
    }
    ss << "]";
    KORD_LOG_INFO(ss.str());
}

template <typename T, size_t N> 
void printArray(const std::array<T, N> &vec, const std::string &label)
{
    std::ostringstream ss;
    ss << label << " [ ";
    for (auto val : vec) {
        ss << val << " ";
    }
    ss << "]";
    KORD_LOG_INFO(ss.str());
}

void printLoad(const std::vector<std::variant<double, int>>& mass, 
               const std::vector<std::variant<double, int>>& cog,
               const std::vector<std::variant<double, int>>& inertia,
               const std::string& label)
{
    std::ostringstream ss;
    ss << label << ": mass=";
    for (auto el : mass)
        ss << std::get<double>(el) << " ";
    ss << "[kg], cog=";
    for (auto el : cog)
        ss << std::get<double>(el) << " ";
    ss << "[m], inertia=";
    for (auto el : inertia)
        ss << std::get<double>(el) << " ";
    ss << "[kg.m^2]";
    KORD_LOG_INFO(ss.str());
}

void printJointQuaternions(const kord::ReceiverInterface& rcv_iface)
{
    for (int joint = 1; joint <= 7; joint++) {
        auto joint_quat = rcv_iface.getJointPoseWithQuaternion(joint);
        printArray(joint_quat, "J" + std::to_string(joint) + " quaternion:");
    }
}

void printSystemFlags(const kord::ReceiverInterface& rcv_iface)
{
    KORD_LOG_INFO("Safety flags: " << rcv_iface.getRobotSafetyFlags());
    KORD_LOG_INFO(kr2::utils::SystemAlarmStateDecoder::decodeAsString(rcv_iface.systemAlarmState()));
    KORD_LOG_INFO("HW flags: " << rcv_iface.getHWFlags());
    KORD_LOG_INFO("Button flags: " << rcv_iface.getButtonFlags());
    KORD_LOG_INFO("Motion flags: " << rcv_iface.getMotionFlags());
    KORD_LOG_INFO("Safety  mode: " << rcv_iface.getSafetyMode());
    KORD_LOG_INFO("Master speed: " << rcv_iface.getMasterSpeed());
}

void printJointValues(const std::array<double, 7UL>& joints, const std::string& label) 
{
    std::ostringstream ss;
    ss << label << " [ ";
    for (double val : joints) {
        ss << val << " ";
    }
    ss << "]";
    KORD_LOG_INFO(ss.str());
}

void printSystemEvents(kord::ReceiverInterface& rcv_iface)
{
    auto system_events = rcv_iface.getSystemEvents();
    if (system_events.empty()) {
        KORD_LOG_INFO("No system events");
    }
    else {
        KORD_LOG_INFO("System events:");
        for (const auto &event : system_events) {
            KORD_LOG_INFO("\t> Event id: " << event.event_id_ << ", event group: " << event.event_group_);
        }
    }
}

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

    // Obtain initial q values
    if (!kord->syncRC()) {
        KORD_LOG_ERROR("Sync RC failed.");
        return EXIT_FAILURE;
    }

    rcv_iface.fetchData();

    // Get and print joint positions
    auto start_q = rcv_iface.getJoint(kord::ReceiverInterface::EJointValue::S_ACTUAL_Q);
    std::array<double, 7> degrees{};
    std::transform(start_q.begin(), start_q.end(), degrees.begin(), [](double angl) { return (angl * 180.0) / M_PI; });
    auto sensed_trq = rcv_iface.getJoint(kord::ReceiverInterface::EJointValue::S_SENSED_TRQ);
    
    printArray(start_q, "Radians:");
    printArray(degrees, "Degrees:");
    printArray(sensed_trq, "Sensed torques:");

    // Get and print frame data
    auto tcp_to_wf = rcv_iface.getFrame(kord::EFrameID::TCP_FRAME, kord::EFrameValue::POSE_VAL_REF_WF);
    auto tcp_fo_tfc = rcv_iface.getFrame(kord::EFrameID::TCP_FRAME, kord::EFrameValue::POSE_VAL_REF_TFC);
    auto tfc_to_wf = rcv_iface.getFrame(kord::EFrameID::TFC_FRAME, kord::EFrameValue::POSE_VAL_REF_WF);
    auto tfc_fo_tfc = rcv_iface.getFrame(kord::EFrameID::TFC_FRAME, kord::EFrameValue::POSE_VAL_REF_TFC);
    auto tcp_quat = rcv_iface.getTCPWithQuaternion();
    auto tcp = rcv_iface.getTCP();

    printVector(tcp_to_wf, "TCP to WF:");
    printVector(tcp_fo_tfc, "TCP to TFC");
    printVector(tfc_to_wf, "TFC to WF:");
    printVector(tfc_fo_tfc, "TFC to TFC:");
    printArray(tcp, "TCP:");
    printArray(tcp_quat, "TCP quaternion:");

    // Print joint quaternions
    printJointQuaternions(rcv_iface);

    // Get and print load data
    auto mass1 = rcv_iface.getLoad(kord::LOAD1, kord::MASS_VAL);
    auto mass2 = rcv_iface.getLoad(kord::LOAD2, kord::MASS_VAL);
    auto cog1 = rcv_iface.getLoad(kord::LOAD1, kord::COG_VAL);
    auto cog2 = rcv_iface.getLoad(kord::LOAD2, kord::COG_VAL);
    auto inertia1 = rcv_iface.getLoad(kord::LOAD1, kord::INERTIA_VAL);
    auto inertia2 = rcv_iface.getLoad(kord::LOAD2, kord::INERTIA_VAL);

    printLoad(mass1, cog1, inertia1, "LOAD1");
    printLoad(mass2, cog2, inertia2, "LOAD2");

    // Print system flags and events
    printSystemFlags(rcv_iface);
    printSystemEvents(rcv_iface);

    // Print joint acceleration and torque data
    auto acs_joints = rcv_iface.getJoint(kord::ReceiverInterface::EJointValue::S_ACTUAL_QDD);
    printJointValues(acs_joints, "Joints accelerations:");

    auto trq_dev = rcv_iface.getJoint(kord::ReceiverInterface::EJointValue::S_TRQDEV_SMOOTH);
    printJointValues(trq_dev, "Torque deviation:");

    KORD_LOG_INFO("FailEmpty: " << rcv_iface.getStatistics(kord::ReceiverInterface::EStatsValue::FAIL_TO_READ_EMPTY));
    KORD_LOG_INFO("FailError: " << rcv_iface.getStatistics(kord::ReceiverInterface::EStatsValue::FAIL_TO_READ_ERROR));
    return 0;
}

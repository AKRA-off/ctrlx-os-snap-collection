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

// Signal handler to gracefully exit on interrupt
void signal_handler(int signum)
{
    psignal(signum, "[KORD-API]");
    g_run = false;
}

// Helper function to compute a quaternion from axis-angle
std::array<double, 4> quaternion_from_axis_angle(const double axis[3], double angle_rad)
{
    double half_angle = angle_rad / 2.0;
    double sin_half = std::sin(half_angle);
    std::array<double, 4> quat;
    quat[0] = std::cos(half_angle); // W
    quat[1] = axis[0] * sin_half;   // X
    quat[2] = axis[1] * sin_half;   // Y
    quat[3] = axis[2] * sin_half;   // Z

    // Normalize the quaternion
    double norm = std::sqrt(quat[0]*quat[0] + quat[1]*quat[1] + quat[2]*quat[2] + quat[3]*quat[3]);
    for(auto &q : quat) q /= norm;

    return quat;
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

    std::array<double, 7UL> tcp_target{};

    g_run = true;

    // Obtain initial TCP values
    if (!kord->syncRC()) {
        KORD_LOG_ERROR("Sync RC failed.");
        return EXIT_FAILURE;
    }
    KORD_LOG_INFO("Sync Captured");
    rcv_iface.fetchData();

    auto start_tcp = rcv_iface.getTCPWithQuaternion();

    std::string tcp_str = "TCP: ";
    for (double &p : start_tcp)
        tcp_str += std::to_string(p) + " ";
    KORD_LOG_INFO(tcp_str);

    // Rotation parameters
    const double frequency = 250; // Hz
    const double amplitude = 0.1; // radians (45 degrees)
    const double rotation_axis[3] = {0.0, 0.0, 1.0}; // Z-axis

    // Start time
    auto start_time = std::chrono::steady_clock::now();

    while (g_run) {
        // Calculate elapsed time in seconds
        auto current_time = std::chrono::steady_clock::now();
        double t = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time).count() / 1000.0;

        // Compute rotation angle using harmonic (sine) function
        double angle = amplitude * std::sin(2.0 * M_PI * frequency * t);

        // Compute quaternion from axis-angle
        std::array<double, 4> quat = quaternion_from_axis_angle(rotation_axis, angle);

        // Update TCP target
        tcp_target[0] = start_tcp[0]; // X position remains constant
        tcp_target[1] = start_tcp[1]; // Y position remains constant
        tcp_target[2] = start_tcp[2]; // Z position remains constant

        tcp_target[3] = quat[0]; // W component
        tcp_target[4] = quat[1]; // X component
        tcp_target[5] = quat[2]; // Y component
        tcp_target[6] = quat[3]; // Z component

        // Optionally, you can also add harmonic motion to the position
        // For example, oscillate along the X-axis:
        // double position_amplitude = 0.1; // meters
        // tcp_target[0] = start_tcp[0] + position_amplitude * std::sin(2.0 * M_PI * frequency * t);

        // Wait for synchronization
        if (!kord->waitSync(std::chrono::milliseconds(10))) {
            KORD_LOG_ERROR("Sync wait timed out, exit");
            break;
        }

        // Send the updated TCP target to the controller
        ctl_iface.moveL(tcp_target,
                        kr2::kord::TrackingType::TT_TIME,
                        4,  // tt_value
                        kr2::kord::BlendType::BT_TIME,
                        2,  // bt_value
                        kr2::kord::OverlayType::OT_VIAPOINT);

        // Fetch latest data
        rcv_iface.fetchData();

        // Check for alarms or runtime limits
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
    double runtime_seconds = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count() / 1000.0;
    KORD_LOG_INFO("Runtime: " << runtime_seconds << " [s]");
    KORD_LOG_INFO("SafetyFlags: " << rcv_iface.getRobotSafetyFlags());
    KORD_LOG_INFO("MotionFlags: " << rcv_iface.getMotionFlags());
    KORD_LOG_INFO("RCState: " << rcv_iface.getRobotSafetyFlags());

    kord->printStats(rcv_iface.getStatisticsStructure());

    // NA when no stats
    return 0;
}

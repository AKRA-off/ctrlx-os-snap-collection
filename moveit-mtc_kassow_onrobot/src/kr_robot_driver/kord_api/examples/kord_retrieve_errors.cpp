#include <array>
#include <chrono>
#include <cmath>
#include <csignal>
#include <iostream>
#include <thread>
#include <vector>

#include <kord/api/api_request.h>
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

bool hasChanged(const std::array<double, 7> &old_q, const std::array<double, 7> &new_q, double threshold)
{
    for (size_t i = 0; i < old_q.size(); ++i) {
        if (std::abs(old_q[i] - new_q[i]) > threshold) {
            return true;
        }
    }
    return false;
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
    std::array<double, 7> start_q = rcv_iface.getJoint(kord::ReceiverInterface::EJointValue::S_ACTUAL_Q);

    std::string initial_j_conf = "Initial joint configuration: ";
    for (double angl : start_q)
        initial_j_conf += std::to_string(angl / 3.14 * 180) + " ";
    KORD_LOG_INFO(initial_j_conf);

    std::array<double, 7> previous_q = start_q; // Store the previous q values
    auto last_output_time = std::chrono::steady_clock::now();
    const std::chrono::milliseconds debounce_duration(500); // Set debounce duration
    const double change_threshold = 0.05;                   // Set a higher threshold for change detection

    while (g_run) {
        if (!kord->waitSync(std::chrono::milliseconds(10))) {
            KORD_LOG_ERROR("Sync wait timed out, exit");
            break;
        }
        rcv_iface.fetchData();

        // Read and check joint configuration
        std::array<double, 7> current_q = rcv_iface.getJoint(kord::ReceiverInterface::EJointValue::S_ACTUAL_Q);
        if (hasChanged(previous_q, current_q, change_threshold)) {
            auto current_time = std::chrono::steady_clock::now();
            if (current_time - last_output_time >= debounce_duration) {
                std::string updated_joint_conf = "Updated joint configuration: ";
                for (double angl : start_q)
                    updated_joint_conf += std::to_string(angl / 3.14 * 180) + " ";
                KORD_LOG_INFO(updated_joint_conf);

                previous_q = current_q;          // Update previous_q to the current values
                last_output_time = current_time; // Update last output time
            }
        }

        // Check for system events only when they are available
        std::vector<kr2::kord::protocol::SystemEvent> events = rcv_iface.getSystemEvents();
        if (!events.empty()) {

            KORD_LOG_INFO("Read the last system event: ");
            std::string events_str;
            for (std::vector<int>::size_type idx = 0; idx < events.size(); ++idx) {
                if (idx < 1)
                    events_str += events[idx].toString();
                else
                    events_str += events[idx].toStringWithRef(events[idx - 1]);
            }
            KORD_LOG_INFO(events_str);
            rcv_iface.clearSystemEventsBuffer(); // Clear buffer after processing
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return EXIT_SUCCESS;
}

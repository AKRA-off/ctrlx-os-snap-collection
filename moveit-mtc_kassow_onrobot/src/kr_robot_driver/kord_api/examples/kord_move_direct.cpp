#include <atomic>
#include <chrono>
#include <cmath>
#include <csignal>
#include <getopt.h>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <kord/api/kord.h>
#include <kord/api/kord_control_interface.h>
#include <kord/api/kord_receive_interface.h>
#include <kord/utils/logger.h>
#include <kord/utils/utils.h>

using namespace std::chrono_literals;
using namespace kr2;
using namespace kr2::kord::protocol;

// Atomic flag for controlling the running state
static std::atomic<bool> g_run_flag{true};

// Signal handler for graceful shutdown
void signal_handler(int a_signum)
{
    psignal(a_signum, "[KORD-API]");
    g_run_flag = false;
}

// Struct to hold extra command-line options
struct ExtraOptions {
    double amplitude = 1.6; // Default amplitude in radians for every joint
    double period = 4.0;    // Default move period in seconds
};

// Function to parse command-line arguments
ExtraOptions parseCommandLine(int argc, char *argv[])
{
    ExtraOptions options;

    // Define long options
    static struct option long_options[] = {{"amplitude", required_argument, nullptr, 'a'},
                                           {"period", required_argument, nullptr, 'f'},
                                           {"help", no_argument, nullptr, 'h'},
                                           {nullptr, 0, nullptr, 0}};

    int opt;
    int option_index = 0;

    while ((opt = getopt_long(argc, argv, "a:f:h", long_options, &option_index)) != -1) {
        switch (opt) {
        case 'a':
            try {
                options.amplitude = std::stod(optarg);
                KORD_LOG_INFO("Amplitude set to " << options.amplitude << " radians.");
            }
            catch (const std::invalid_argument &e) {
                KORD_LOG_ERROR("Invalid amplitude value: " << optarg);
                exit(EXIT_FAILURE);
            }
            catch (const std::out_of_range &e) {
                KORD_LOG_ERROR("Amplitude value out of range: " << optarg);
                exit(EXIT_FAILURE);
            }
            break;
        case 'f':
            try {
                options.period = std::stod(optarg);
                KORD_LOG_INFO("Period set to " << options.period << " seconds.");
            }
            catch (const std::invalid_argument &e) {
                KORD_LOG_ERROR("Invalid period value: " << optarg);
                exit(EXIT_FAILURE);
            }
            catch (const std::out_of_range &e) {
                KORD_LOG_ERROR("Period value out of range: " << optarg);
                exit(EXIT_FAILURE);
            }
            break;
        case 'h':
            // Display help message
            std::cout << "Usage: " << argv[0] << " [OPTIONS]\n"
                      << "Options:\n"
                      << "  -a, --amplitude <value>   Amplitude in radians for every joint (default: 1.6)\n"
                      << "  -f, --period <value>      Move period in seconds (default: 4.0)\n"
                      << "  -h, --help                Display this help message\n";
            exit(EXIT_SUCCESS);
            break;
        default:
            KORD_LOG_ERROR("Unknown option encountered.");
            exit(EXIT_FAILURE);
        }
    }

    return options;
}

// Function to print joint configuration in degrees
void printJointConfiguration(const std::array<double, 7UL> &joints, const std::string &header)
{
    std::ostringstream oss;
    oss << header << " [ ";
    for (const double angle_rad : joints) {
        double angle_deg = (angle_rad / M_PI) * 180.0;
        oss << std::fixed << std::setprecision(2) << angle_deg << " ";
    }
    oss << "]";
    KORD_LOG_INFO(oss.str());
}

int main(int argc, char *argv[])
{
    // Parse command-line arguments
    ExtraOptions extras = parseCommandLine(argc, argv);

    // Display the execution parameters
    KORD_LOG_INFO("Executing with");
    KORD_LOG_INFO("Amplitude: " << extras.amplitude << " radians");
    KORD_LOG_INFO("Period: " << extras.period << " seconds");

    // Process launch arguments (additional parameters if any)
    utils::LaunchParameters lp = utils::LaunchParameters::processLaunchArguments(argc, argv);

    if (lp.help_ || !lp.valid_) {
        return EXIT_SUCCESS;
    }

    // Register signal handler for graceful shutdown
    signal(SIGINT, signal_handler);

    // Initialize real-time parameters if required
    if (lp.useRealtime()) {
        if (!utils::realtime::init_realtime_params(lp.rt_prio_)) {
            KORD_LOG_ERROR("Failed to start with realtime priority");
            utils::LaunchParameters::printUsage(true);
            return EXIT_FAILURE;
        }
    }

    // Logging connection details
    KORD_LOG_INFO("Connecting to: " << lp.remote_controller_ << ":" << lp.port_);
    KORD_LOG_INFO("[KORD-API] Session ID: " << lp.session_id_);

    // Create shared pointer for KordCore using make_shared for exception safety
    auto kord = std::make_shared<kord::KordCore>(lp.remote_controller_, lp.port_, lp.session_id_, kord::UDP_CLIENT);

    // Initialize control and receiver interfaces
    kord::ControlInterface ctl_iface(kord);
    kord::ReceiverInterface rcv_iface(kord);

    // Attempt to connect to KORD
    if (!kord->connect()) {
        KORD_LOG_ERROR("Connecting to KR failed");
        return EXIT_FAILURE;
    }

    // Initialize joint variables
    std::array<double, 7UL> q{}, qd{}, qdd{}, torque{};

    // Synchronize with KORD controller
    if (!kord->syncRC()) {
        KORD_LOG_ERROR("Sync RC failed.");
        return EXIT_FAILURE;
    }
    KORD_LOG_INFO("Sync Captured");
    rcv_iface.fetchData();

    // Get initial joint positions
    std::array<double, 7UL> start_q;
    try {
        start_q = rcv_iface.getJoint(kord::ReceiverInterface::EJointValue::S_ACTUAL_Q);
    }
    catch (const std::exception &e) {
        KORD_LOG_ERROR("Failed to get initial joint positions: " << e.what());
        return EXIT_FAILURE;
    }

    // Print initial joint configuration in degrees
    printJointConfiguration(start_q, "Initial Joint Configuration");

    unsigned int k = 0; // Main time counter
    double A = extras.amplitude;
    double T = extras.period;
    double w = 2 * M_PI / T; // Angular frequency

    double ts = 0.002; // Sampling time (500 Hz)

    double now_time = 0.0;
    bool mode_switch = false;

    std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now();

    // Control loop
    while (g_run_flag) {
        now_time = k * ts;

        // Check if the period has elapsed to switch mode
        if (now_time > T) {
            mode_switch = !mode_switch;
            k = 0;
            now_time = 0.0;
            if (!g_run_flag) {
                break;
            }
        }

        // Generate joint trajectories
        if (!mode_switch) {
            for (size_t i = 0; i < 7; i++) {
                q[i] = (A / T) * now_time - (A / (T * w)) * std::sin(w * now_time) + start_q[i];
                qd[i] = (A / T) - (A / (T * w)) * std::cos(w * now_time) * w;
                qdd[i] = (A / (T * w)) * std::sin(w * now_time) * w;
                torque[i] = 0.0; // Automatic torque computation
            }
        }
        else {
            for (size_t i = 0; i < 7; i++) {
                q[i] = 2 * A - (A / T) * (now_time + T) + (A / (T * w)) * std::sin(w * (now_time + T)) + start_q[i];
                qd[i] = -(A / T) + (A / (T * w)) * std::cos(w * now_time) * w;
                qdd[i] = -(A / (T * w)) * std::sin(w * now_time) * w;
                torque[i] = 0.0; // Automatic torque computation
            }
        }

        k++;

        // Wait for synchronization
        if (!kord->waitSync(std::chrono::milliseconds(10))) {
            KORD_LOG_ERROR("Sync wait timed out, exiting.");
            break;
        }

        // Send direct joint control commands
        try {
            ctl_iface.directJControl(q, qd, qdd, torque);
        }
        catch (const std::exception &e) {
            KORD_LOG_ERROR("Failed to send joint control command: " << e.what());
            break;
        }

        // Fetch updated data
        rcv_iface.fetchData();

        // Check for system alarms or runtime elapsed
        if (rcv_iface.systemAlarmState() || lp.runtimeElapsed()) {
            KORD_LOG_WARN("System alarm state active or runtime elapsed. Terminating control loop.");
            break;
        }
    }

    // Termination procedures
    KORD_LOG_INFO("Robot stopped");
    KORD_LOG_INFO("Input Bits: " << rcv_iface.getFormattedInputBits());
    KORD_LOG_INFO("Output Bits: " << rcv_iface.getFormattedOutputBits());

    // System Alarm State
    uint32_t alarm_state = rcv_iface.systemAlarmState();
    KORD_LOG_INFO("SystemAlarmState: " << alarm_state);
    KORD_LOG_INFO("SystemAlarmState's Category: ");
    switch (alarm_state & 0b1111) {
    case EEventGroup::eUnknown:
        KORD_LOG_INFO("No alarms");
        break;
    case EEventGroup::eSafetyEvent:
        KORD_LOG_INFO("Safety Event");
        break;
    case EEventGroup::eSoftStopEvent:
        KORD_LOG_INFO("Soft Stop Event");
        break;
    case EEventGroup::eKordEvent:
        KORD_LOG_INFO("Kord Event");
        break;
    default:
        KORD_LOG_WARN("Unknown Event Group");
        break;
    }

    KORD_LOG_INFO("Safety Flags: " << rcv_iface.getRobotSafetyFlags());
    KORD_LOG_INFO("Motion Flags: " << rcv_iface.getMotionFlags());
    KORD_LOG_INFO("RC State: " << rcv_iface.getRobotSafetyFlags());

    // Runtime
    auto end_time = std::chrono::steady_clock::now();
    double runtime_seconds =
        std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count() / 1000.0;
    KORD_LOG_INFO("Runtime: " << runtime_seconds << " [s]");

    // Print statistics
    try {
        kord->printStats(rcv_iface.getStatisticsStructure());
    }
    catch (const std::exception &e) {
        KORD_LOG_ERROR("Failed to print statistics: " << e.what());
    }

    return EXIT_SUCCESS;
}

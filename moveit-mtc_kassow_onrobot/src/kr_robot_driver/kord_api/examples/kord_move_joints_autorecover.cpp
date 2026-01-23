#include <array>
#include <atomic>
#include <bitset>
#include <chrono>
#include <cmath>
#include <csignal>
#include <iostream>

#include <kord/api/kord.h>
#include <kord/api/kord_control_interface.h>
#include <kord/api/kord_receive_interface.h>
#include <kord/system/CommandStatusFlags.h>
#include <kord/system/RobotControllerFlags.h>
#include <kord/utils/utils.h>

constexpr int NOT_SHUTDOWN = 0;

std::atomic<int> gstop{NOT_SHUTDOWN};

void signalHandler(int signum)
{
    std::cout << "Interrupt signal (" << signum << ") received.\n";
    gstop.store(signum, std::memory_order_relaxed);
}

void printReceiverDiagnostics(kr2::kord::ReceiverInterface &rcv_iface)
{
    std::cout << rcv_iface.getFormattedInputBits() << std::endl;
    std::cout << rcv_iface.getFormattedOutputBits() << std::endl;

    std::uint32_t alarmValue = rcv_iface.systemAlarmState();
    std::bitset<32> bits(alarmValue);
    std::cout << "Alarm state: " << bits.to_string() << std::endl;

    // Check the lower bits for alarm grouping
    switch (alarmValue & 0b1111) {
    case kr2::kord::protocol::EEventGroup::eUnknown:
        std::cout << "No alarms\n";
        break;
    case kr2::kord::protocol::EEventGroup::eSafetyEvent:
        std::cout << "Safety Event\n";
        break;
    case kr2::kord::protocol::EEventGroup::eSoftStopEvent:
        std::cout << "Soft Stop Event\n";
        break;
    case kr2::kord::protocol::EEventGroup::eKordEvent:
        std::cout << "Kord Event\n";
        break;
    default:
        // Handle other unexpected bits if necessary
        break;
    }

    std::cout << "Safety Flags: " << rcv_iface.getRobotSafetyFlags() << "\n";
    std::cout << "Button Flags: " << rcv_iface.getButtonFlags() << "\n";
    std::cout << "Motion Flags: " << rcv_iface.getMotionFlags() << "\n";
    std::cout << "---------------------------------------------------------------------------\n";
}

bool initKord(std::shared_ptr<kr2::kord::KordCore> &kord)
{
    if (!kord->connect()) {
        std::cerr << "Connect Kord failed.\n";
        return false;
    }
    std::cout << "Connect OK\n";

    if (!kord->syncRC()) {
        std::cerr << "Sync RC failed.\n";
        return false;
    }
    std::cout << "Sync OK\n";
    return true;
}

bool sendClearRequestAndWait(std::shared_ptr<kr2::kord::KordCore> &kord,
                             kr2::kord::ControlInterface &ctl_iface,
                             kr2::kord::ReceiverInterface &rcv_iface,
                             kr2::kord::ControlInterface::EClearRequest requestType,
                             const char *request_name)
{
    using namespace std::chrono;

    auto t_start = steady_clock::now();
    constexpr auto WAIT_TIMEOUT = milliseconds(10);
    constexpr int8_t NOT_FINISHED = -1;

    // First ensure we sync once before sending
    if (!kord->waitSync(WAIT_TIMEOUT)) {
        std::cerr << "Sync wait timed out before sending " << request_name << ".\n";
        return false;
    }

    int64_t token = ctl_iface.clearAlarmRequest(requestType);
    std::cout << request_name << " request token: " << token << std::endl;

    int8_t answer = NOT_FINISHED;
    // Wait for the command to transition from NOT_FINISHED
    while (gstop.load(std::memory_order_relaxed) == NOT_SHUTDOWN && answer == NOT_FINISHED) {
        if (!kord->waitSync(WAIT_TIMEOUT)) {
            std::cerr << "Sync wait timed out while awaiting " << request_name << " response.\n";
            break;
        }
        rcv_iface.fetchData();
        answer = rcv_iface.getCommandStatus(token);
    }

    auto t_end = steady_clock::now();
    auto diff_ns = duration_cast<nanoseconds>(t_end - t_start).count();

    std::cout << request_name << " token: " << token << " took " << diff_ns << " ns\n";
    printReceiverDiagnostics(rcv_iface);

    return true;
}

bool clearHalt(std::shared_ptr<kr2::kord::KordCore> &kord,
               kr2::kord::ControlInterface &ctl_iface,
               kr2::kord::ReceiverInterface &rcv_iface)
{
    // Check if conditions for HALT+SUSPEND, PSTOP are present
    using namespace kr2::kord::protocol;
    bool is_halt = rcv_iface.getMotionFlags() & MOTION_FLAG_HALT;
    bool is_pstop = rcv_iface.getRobotSafetyFlags() & SAFETY_FLAG_PSTOP;

    if (is_halt && is_pstop) {
        std::cout << "Clearing halt...\n";
        return sendClearRequestAndWait(kord,
                                       ctl_iface,
                                       rcv_iface,
                                       kr2::kord::ControlInterface::EClearRequest::CLEAR_HALT,
                                       "ClearHalt");
    }
    return true;
}

bool unsuspend(std::shared_ptr<kr2::kord::KordCore> &kord,
               kr2::kord::ControlInterface &ctl_iface,
               kr2::kord::ReceiverInterface &rcv_iface)
{
    using namespace kr2::kord::protocol;
    bool is_suspend = rcv_iface.getMotionFlags() & MOTION_FLAG_SUSPENDED;

    if (is_suspend) {
        std::cout << "Unsuspend command...\n";
        return sendClearRequestAndWait(kord,
                                       ctl_iface,
                                       rcv_iface,
                                       kr2::kord::ControlInterface::EClearRequest::UNSUSPEND,
                                       "Unsuspend");
    }
    return true;
}

bool clearCBun(std::shared_ptr<kr2::kord::KordCore> &kord,
               kr2::kord::ControlInterface &ctl_iface,
               kr2::kord::ReceiverInterface &rcv_iface)
{
    std::cout << "Clearing CBun...\n";
    return sendClearRequestAndWait(kord,
                                   ctl_iface,
                                   rcv_iface,
                                   kr2::kord::ControlInterface::EClearRequest::CBUN_EVENT,
                                   "ClearCBun");
}

bool clearErrors(std::shared_ptr<kr2::kord::KordCore> &kord,
                 kr2::kord::ControlInterface &ctl_iface,
                 kr2::kord::ReceiverInterface &rcv_iface)
{
    using namespace kr2::kord::protocol;

    // If nothing to clear, early exit
    if (rcv_iface.getMotionFlags() == 0 && rcv_iface.getRobotSafetyFlags() == 0) {
        std::cout << "No errors.\n";
        return true;
    }

    if (rcv_iface.getRobotSafetyFlags() & SAFETY_FLAG_USER_CONF_REQ) {
        std::cout << "Cannot clear errors, user confirmation required.\n";
        return false;
    }

    std::cout << "Motion flags: " << rcv_iface.getMotionFlags() << "\n";
    std::cout << "Robot safety flags: " << rcv_iface.getRobotSafetyFlags() << "\n";

    bool cbun_needed = rcv_iface.systemAlarmState() & CAT_CBUN_EVENT;

    // Check for KORD event
    auto system_events = rcv_iface.getSystemEvents();
    for (auto &event : system_events) {
        if (event.event_group_ == eKordEvent) {
            cbun_needed = true;
            break;
        }
    }

    if (!clearHalt(kord, ctl_iface, rcv_iface)) {
        std::cerr << "Clear halt failed.\n";
        return false;
    }
    if (!unsuspend(kord, ctl_iface, rcv_iface)) {
        std::cerr << "Unsuspend failed.\n";
        return false;
    }
    if (cbun_needed && !clearCBun(kord, ctl_iface, rcv_iface)) {
        std::cerr << "Clear CBun failed.\n";
        return false;
    }

    return true;
}

int main(int argc, char *argv[])
{
    using namespace kr2;

    // Register signals
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // Launch parameters
    utils::LaunchParameters lp = utils::LaunchParameters::processLaunchArguments(argc, argv);
    if (lp.help_ || !lp.valid_) {
        lp.printUsage(true);
        return EXIT_SUCCESS;
    }

    if (lp.useRealtime()) {
        if (!utils::realtime::init_realtime_params(lp.rt_prio_)) {
            std::cerr << "Failed to start with realtime priority.\n";
            lp.printUsage(false);
            return EXIT_FAILURE;
        }
    }

    std::cout << "Connecting to: " << lp.remote_controller_ << ":" << lp.port_ << "\n";
    std::cout << "[KORD-API] Session ID: " << lp.session_id_ << std::endl;

    auto kord = std::make_shared<kord::KordCore>(lp.remote_controller_, lp.port_, lp.session_id_, kord::UDP_CLIENT);

    kord::ControlInterface ctl_iface(kord);
    kord::ReceiverInterface rcv_iface(kord);

    if (!initKord(kord)) {
        std::cerr << "Could not initialize Kord. Exiting...\n";
        return EXIT_FAILURE;
    }

    // Initial check/clear of errors
    rcv_iface.fetchData();
    if (rcv_iface.systemAlarmState() && !clearErrors(kord, ctl_iface, rcv_iface)) {
        std::cerr << "Got alarm state on startup.\n";
        return EXIT_FAILURE;
    }

    // Get current joint positions
    std::array<double, 7UL> start_q = rcv_iface.getJoint(kord::ReceiverInterface::EJointValue::S_ACTUAL_Q);

    std::cout << "Read initial joint configuration:\n";
    for (double angle : start_q) {
        std::cout << (angle / M_PI) * 180.0 << " ";
    }
    std::cout << "\n";

    // Movement loop
    unsigned int i = 0, j = 0;
    double t = 0.0;

    while (gstop.load(std::memory_order_relaxed) == NOT_SHUTDOWN) {
        if (!kord->waitSync(std::chrono::milliseconds(10))) {
            std::cerr << "Sync wait timed out, exit.\n";
            break;
        }
        rcv_iface.fetchData();

        // If there is an alarm, try to clear
        if (rcv_iface.systemAlarmState()) {
            if (j % 500 == 0) {
                std::cout << "Alarm detected. Not sending movement commands. Attempting to clear.\n";
                printReceiverDiagnostics(rcv_iface);
            }
            ++j;

            if (!clearErrors(kord, ctl_iface, rcv_iface)) {
                // If clearing fails, just retry in next loop iteration
                continue;
            }

            // Reset the movement
            start_q = rcv_iface.getJoint(kord::ReceiverInterface::EJointValue::S_ACTUAL_Q);
            t = 0.0;
            i = 0;
        }

        constexpr double a = 0.05;
        constexpr double tt_time = 0.008;
        constexpr double bt_time = 0.004;

        std::array<double, 7UL> q{};
        q[0] = start_q[0];
        q[1] = start_q[1];
        q[2] = start_q[2];
        q[3] = std::sin(t * 1e-3) * a + start_q[3];
        q[4] = start_q[4];
        q[5] = start_q[5];
        q[6] = start_q[6];

        t = i * 7; // increase time step
        ++i;

        ctl_iface.moveJ(q,
                        kord::TrackingType::TT_TIME, // time-based motion
                        tt_time,
                        kord::BlendType::BT_TIME, // blend by time
                        bt_time,
                        kord::OverlayType::OT_VIAPOINT // via-point
        );

        if (i % 500 == 0) {
            printReceiverDiagnostics(rcv_iface);
        }
    }

    std::cout << "Disconnecting...\n";
    kord->disconnect();
    return 0;
}

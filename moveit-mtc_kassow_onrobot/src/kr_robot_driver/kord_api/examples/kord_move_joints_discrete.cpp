#include <chrono>
#include <cmath>
#include <csignal>
#include <iomanip>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include <kord/api/kord.h>
#include <kord/api/kord_control_interface.h>
#include <kord/api/kord_receive_interface.h>
#include <kord/utils/utils.h>

using namespace kr2;

void signal_handler(int a_signum)
{
    psignal(a_signum, "[KORD-API]");
    exit(1);
}

int main(int argc, char *argv[])
{
    struct ExtraOptions {
        int pose = -1;
        std::optional<std::array<double, 7>> target_joint_configuration = {};
        std::optional<std::array<double, 7>> target_joint_offset = {};
    } extras;

    static utils::LaunchParameters::ExternalArgParser ep = [argc, &argv, &extras](int index) -> void {
        static kr2::utils::SOALongOptions ex_options{std::array<kr2::utils::LongOption, 3>{
            kr2::utils::LongOption{{"pose", optional_argument, nullptr, 'n'},
                                   "number of pre-defined pose robot should move to, available options: [0, 1, 2]",
                                   "<pose_num>"},
            kr2::utils::LongOption{{"target", optional_argument, nullptr, 't'},
                                   "values representing target joints configuration (in degrees)",
                                   "<j1,j2,j3,j4,j5,j6,j7>"},
            kr2::utils::LongOption{{"offset", optional_argument, nullptr, 'o'},
                                   "values representing the offset (in degrees) w.r.t. current joints configuration",
                                   "<j1,j2,j3,j4,j5,j6,j7>"},
        }};

        if (index <= utils::LaunchParameters::INVALID_INDEX) {
            std::cout << ex_options.helpString() << "\n";
            return;
        }

        int option_index = 0;
        optind = index;
        int opt = getopt_long(argc, argv, "n:t:o:", ex_options.getLongOptions(), &option_index);
        switch (opt) {
        case 'n': {
            extras.pose = std::stoi(optarg);
            break;
        }
        case 't': {
            std::stringstream ss(optarg);
            std::string item;
            std::vector<double> values;

            while (std::getline(ss, item, ',')) {
                values.push_back(std::stod(item));
            }

            if (values.size() == 7) {
                extras.target_joint_configuration.emplace();
                std::copy(values.begin(), values.end(), extras.target_joint_configuration->begin());
            }
            else {
                std::cerr << "Error: Expected 7 values for target joint configuration but got " << values.size() << "\n";
                exit(EXIT_FAILURE);
            }
            break;
        }
        case 'o': {
            std::stringstream ss(optarg);
            std::string item;
            std::vector<double> values;

            while (std::getline(ss, item, ',')) {
                values.push_back(std::stod(item));
            }

            if (values.size() == 7) {
                extras.target_joint_offset.emplace();
                std::copy(values.begin(), values.end(), extras.target_joint_offset->begin());
            }
            else {
                std::cerr << "Error: Expected 7 values for target joint configuration but got " << values.size() << "\n";
                exit(EXIT_FAILURE);
            }
            break;
        }
        default:
            std::cout << "Unknown option found" << " optidx: " << optind << ", argc: " << argc << "\n";
            std::cout << opt << std::endl;
            exit(EXIT_FAILURE);
        }
    };

    utils::LaunchParameters lp = utils::LaunchParameters::processLaunchArguments(argc, argv, ep);

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
    std::array<double, 7UL> degs{};

    // Obtain initial q values
    if (!kord->syncRC()) {
        KORD_LOG_ERROR("Sync RC failed.");
        return EXIT_FAILURE;
    }
    KORD_LOG_INFO("Sync Captured");
    rcv_iface.fetchData();
    std::array<double, 7UL> start_q = rcv_iface.getJoint(kord::ReceiverInterface::EJointValue::S_ACTUAL_Q);

    std::string initial_j_conf = "Initial joint configuration: ";
    for (double angl : start_q)
        initial_j_conf += std::to_string(angl / 3.14 * 180) + " ";
    KORD_LOG_INFO(initial_j_conf);

    double tt_value = 5.0;
    double bt_value = 3.0;
    double sync_time = 10.0;

    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

    // Prefer target joint configuration if provided
    if (extras.target_joint_configuration.has_value()) {
        degs = extras.target_joint_configuration.value();
    }
    else if (extras.target_joint_offset.has_value()) {
        degs = {degs[0] + extras.target_joint_offset.value()[0],
                degs[1] + extras.target_joint_offset.value()[1],
                degs[2] + extras.target_joint_offset.value()[2],
                degs[3] + extras.target_joint_offset.value()[3],
                degs[4] + extras.target_joint_offset.value()[4],
                degs[5] + extras.target_joint_offset.value()[5],
                degs[6] + extras.target_joint_offset.value()[6]};
    }
    else {
        switch (extras.pose) {
        case 0:
            degs = {0.0, -15.0, 0.0, 90.0, 0.0, 90.0, 0.0};
            break;
        case 1:
            degs = {0.0, 0.0, 30.0, 120.0, 30.0, 80.0, 10.0};
            break;
        case 2:
            degs = {0.0, 25.0, 50.0, 80.0, 60.0, 60.0, 30.0};
            break;
        default:
            KORD_LOG_INFO("Robot pose is not defined. Go to default.");
            degs = {0.0, -15.0, 0.0, 90.0, 0.0, 90.0, 0.0};
            break;
        }
    }
    if (!kord->waitSync(std::chrono::milliseconds(10))) {
        KORD_LOG_ERROR("Sync wait timed out, exit");
        return EXIT_FAILURE;
    }

    for (size_t i = 0; i < 7; i++)
        q[i] = degs[i] * 3.1415 / 180.0;

    ctl_iface.moveJ(q,
                    kr2::kord::TrackingType::TT_TIME,
                    tt_value,
                    kr2::kord::BlendType::BT_TIME,
                    bt_value,
                    kr2::kord::OverlayType::OT_VIAPOINT,
                    sync_time);

    rcv_iface.fetchData();
    if (rcv_iface.systemAlarmState() || lp.runtimeElapsed()) {
        return EXIT_FAILURE;
    }

    KORD_LOG_INFO("Robot stopped");
    KORD_LOG_INFO(rcv_iface.getFormattedInputBits());
    KORD_LOG_INFO(rcv_iface.getFormattedOutputBits());
    KORD_LOG_INFO("SystemAlarmState: " << rcv_iface.systemAlarmState());
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

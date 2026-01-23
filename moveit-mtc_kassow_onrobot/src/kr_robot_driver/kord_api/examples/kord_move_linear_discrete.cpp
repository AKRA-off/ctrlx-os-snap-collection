#include <chrono>
#include <cmath>
#include <csignal>
#include <iomanip>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

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
    struct ExtraOptions {
        int pose = -1;
        std::optional<std::array<double, 6>> target_coordinates = {};
        std::optional<std::array<double, 6>> translation = {};
    } extras;

    static utils::LaunchParameters::ExternalArgParser ep = [argc, &argv, &extras](int index) -> void {
        static kr2::utils::SOALongOptions ex_options{std::array<kr2::utils::LongOption, 3>{
            kr2::utils::LongOption{{"pose", optional_argument, nullptr, 'n'},
                                   "number of pre-defined pose robot should move to, available options: [0, 1, 2]",
                                   "<pose_num>"},
            kr2::utils::LongOption{{"target", optional_argument, nullptr, 't'},
                                   "values representing target coordinates",
                                   "<x,y,z,r,p,y>"},
            kr2::utils::LongOption{{"offset", optional_argument, nullptr, 'o'},
                                   "values representing the offset w.r.t. current position",
                                   "<x,y,z,r,p,y>"},
        }};

        if (index <= utils::LaunchParameters::INVALID_INDEX) {
            std::cout << ex_options.helpString() << "\n";
            return;
        }

        int option_index = 0;
        optind = index;
        int opt = getopt_long(argc, argv, "n:t:o", ex_options.getLongOptions(), &option_index);

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

            if (values.size() == 6) {
                extras.target_coordinates.emplace();
                std::copy(values.begin(), values.end(), extras.target_coordinates->begin());
            }
            else {
                std::cerr << "Error: Expected 6 values for target joint configuration but got " << values.size() << "\n";
                exit(EXIT_FAILURE);
            }
            break;
        }
        case 'o': {
            std::cout << "optarg: " << optarg << std::endl;
            std::stringstream ss(optarg);
            std::string item;
            std::vector<double> values;

            std::cout << "-------------------" << std::endl;
            while (std::getline(ss, item, ',')) {
                values.push_back(std::stod(item));
            }

            if (values.size() == 6) {
                extras.translation.emplace();
                std::copy(values.begin(), values.end(), extras.translation->begin());
            }
            else {
                std::cerr << "Error: Expected 6 values for target joint configuration but got " << values.size() << "\n";
                exit(EXIT_FAILURE);
            }
            break;
        }
        default:
            std::cout << "Unknown option found" << " optidx: " << optind << ", argc: " << argc << "\n";
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

    std::array<double, 6UL> tcp_target{};

    g_run = true;

    if (!kord->syncRC()) {
        KORD_LOG_ERROR("Sync RC failed.");
        return EXIT_FAILURE;
    }
    KORD_LOG_INFO("Sync Captured");

    rcv_iface.fetchData();
    std::vector<std::variant<double, int>> returned_vec_start_tcp =
        rcv_iface.getFrame(kord::EFrameID::TCP_FRAME, kord::EFrameValue::POSE_VAL_REF_WF);
    std::array<double, 6UL> start_tcp{};
    for (size_t i = 0; i < start_tcp.size(); ++i) {
        start_tcp[i] = std::get<double>(returned_vec_start_tcp[i]);
    }

    std::string tcp_str = "TCP: ";
    for (double &p : start_tcp)
        tcp_str += std::to_string(p) + ";";
    KORD_LOG_INFO(tcp_str);

    double tt_value = 10.0;
    double bt_value = 10.0;
    double sync_time = 10.0;

    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

    if (extras.target_coordinates.has_value()) {
        KORD_LOG_INFO("Using target coordinates");
        tcp_target = extras.target_coordinates.value();
    }
    else if (extras.translation.has_value()) {
        KORD_LOG_INFO("Using translation");
        tcp_target = {
            start_tcp[0] + extras.translation.value()[0],
            start_tcp[1] + extras.translation.value()[1],
            start_tcp[2] + extras.translation.value()[2],
            start_tcp[3] + extras.translation.value()[3],
            start_tcp[4] + extras.translation.value()[4],
            start_tcp[5] + extras.translation.value()[5],
        };
    }
    else {
        KORD_LOG_INFO("Using pose");
        switch (extras.pose) {
        case 0:
            tcp_target =
                {1.1 * start_tcp[0], 0.9 * start_tcp[1], 1.1 * start_tcp[2], start_tcp[3], start_tcp[4], start_tcp[5]};
            break;

        case 1:
            tcp_target =
                {0.9 * start_tcp[0], 1.1 * start_tcp[1], 0.9 * start_tcp[2], start_tcp[3], start_tcp[4], start_tcp[5]};
            break;

        default:
            KORD_LOG_INFO("Robot pose is not defined. Go to default.");
            tcp_target = {start_tcp[0], start_tcp[1], start_tcp[2], start_tcp[3], start_tcp[4], start_tcp[5]};
            break;
        }
    }

    if (!kord->waitSync(std::chrono::milliseconds(10))) {
        KORD_LOG_ERROR("Sync wait timed out, exiting");
        return 1;
    }

    std::string target_tcp_str = "Target TCP: ";
    for (double &p : tcp_target)
        target_tcp_str += std::to_string(p) + ";";
    KORD_LOG_INFO(target_tcp_str);

    ctl_iface.moveL(tcp_target,
                    kr2::kord::TrackingType::TT_TIME,
                    tt_value,
                    kr2::kord::BlendType::BT_TIME,
                    bt_value,
                    kr2::kord::OverlayType::OT_VIAPOINT,
                    sync_time);

    rcv_iface.fetchData();
    if (rcv_iface.systemAlarmState() || lp.runtimeElapsed()) {
        return 1;
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
    double runtime_seconds =
        std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start).count() / 1000.0;
    KORD_LOG_INFO("Runtime: " << runtime_seconds << " [s]");
    KORD_LOG_INFO("SafetyFlags: " << rcv_iface.getRobotSafetyFlags());
    KORD_LOG_INFO("MotionFlags: " << rcv_iface.getMotionFlags());
    KORD_LOG_INFO("RCState: " << rcv_iface.getRobotSafetyFlags());

    kord->printStats(rcv_iface.getStatisticsStructure());
    return 0;
}

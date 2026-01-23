#include <chrono>
#include <csignal>
#include <iostream>
#include <memory>
#include <atomic>
#include <thread>
#include <map>
#include <variant>
#include <vector>
#include <string>

#include <kord/api/kord.h>
#include <kord/api/kord_control_interface.h>
#include <kord/api/kord_receive_interface.h>
#include <kord/utils/logger.h>
#include <kord/utils/utils.h>

using namespace std::chrono_literals;
using namespace kr2;
using namespace kr2::kord::protocol;

// Atomic flag for controlling the running state
static std::atomic<bool> stop_flag{false};

// Signal handler for graceful shutdown
void signal_handler(int a_signum)
{
    psignal(a_signum, "[KORD-API]");
    stop_flag = true;
}

// Function to compare two vectors of variants
bool areVectorsSame(const std::vector<std::variant<double, int>> &vec1,
                   const std::vector<std::variant<double, int>> &vec2)
{
    return vec1 == vec2;
}

// Function to print vectors with logging
void printVector(const std::vector<std::variant<double, int>> &vec, const std::string &label)
{
    std::ostringstream oss;
    oss << label << " [ ";
    for (const auto &val : vec) {
        if (std::holds_alternative<double>(val)) {
            oss << std::get<double>(val) << " ";
        } else if (std::holds_alternative<int>(val)) {
            oss << std::get<int>(val) << " ";
        }
    }
    oss << "]";
    KORD_LOG_INFO(oss.str());
}

int main(int argc, char *argv[])
{
    // Struct to hold extra command-line options
    struct ExtraOptions {
        bool whitelist_enabled = false;
        bool print_tcp = false;
        bool print_tfc = false;
        bool print_load1 = false;
        bool print_load2 = false;
    } extras;

    // External argument parser lambda
    static utils::LaunchParameters::ExternalArgParser ep = [argc, &argv, &extras](int index) -> void {
        static kr2::utils::SOALongOptions ex_options{
            std::array<kr2::utils::LongOption, 4>{
                kr2::utils::LongOption{
                    {"tcp", no_argument, nullptr, 'n'},
                    "Print TCP and TCP pose in TFC",
                    ""
                },
                kr2::utils::LongOption{
                    {"tfc", no_argument, nullptr, 'm'},
                    "Print TFC",
                    ""
                },
                kr2::utils::LongOption{
                    {"load1", no_argument, nullptr, 'k'},
                    "Print LOAD1",
                    ""
                },
                kr2::utils::LongOption{
                    {"load2", no_argument, nullptr, 'l'},
                    "Print LOAD2",
                    ""
                }
            }
        };

        if (index <= utils::LaunchParameters::INVALID_INDEX) {
            std::cout << ex_options.helpString() << std::endl;
            return;
        }

        int option_index = 0;
        optind = index;
        int opt = getopt_long(argc, argv, "nmkl", ex_options.getLongOptions(), &option_index);

        switch (opt) {
            case 'n': {
                extras.whitelist_enabled = true;
                extras.print_tcp = true;
                break;
            }
            case 'm': {
                extras.whitelist_enabled = true;
                extras.print_tfc = true;
                break;
            }
            case 'k': {
                extras.whitelist_enabled = true;
                extras.print_load1 = true;
                break;
            }
            case 'l': {
                extras.whitelist_enabled = true;
                extras.print_load2 = true;
                break;
            }
            default: {
                std::cerr << "Unknown option found. optidx: " << optind << ", argc: " << argc << std::endl;
                exit(EXIT_FAILURE);
            }
        }
    };

    // Process launch arguments
    utils::LaunchParameters lp = utils::LaunchParameters::processLaunchArguments(argc, argv, ep);

    if (lp.help_ || !lp.valid_) {
        return EXIT_SUCCESS;
    }

    // Register signal handler
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
    KORD_LOG_INFO("Session ID: " << lp.session_id_);

    // Create shared pointer for KordCore
    auto kord = std::make_shared<kord::KordCore>(lp.remote_controller_, lp.port_, lp.session_id_, kord::UDP_CLIENT);

    // Initialize control and receiver interfaces
    kord::ControlInterface ctl_iface(kord);
    kord::ReceiverInterface rcv_iface(kord);

    // Attempt to connect to Kord
    if (!kord->connect()) {
        KORD_LOG_ERROR("Connecting to KR failed");
        return EXIT_FAILURE;
    }

    // Obtain initial q values
    if (!kord->syncRC()) {
        KORD_LOG_ERROR("Sync RC failed.");
        return EXIT_FAILURE;
    }
    KORD_LOG_INFO("Sync Captured");
    rcv_iface.fetchData();
    std::array<double, 6UL> fetched_TCP = rcv_iface.getTCP();

    long counter = 0;

    // Refresh frequency variables
    std::vector<std::variant<double, int>> prev_tcp;
    std::vector<std::variant<double, int>> prev_tfc;
    std::vector<std::variant<double, int>> prev_tcp_wrt_tfc;
    std::vector<std::variant<double, int>> prev_load1_cog;
    std::vector<std::variant<double, int>> prev_load1_mass;
    std::vector<std::variant<double, int>> prev_load1_inertia;
    std::vector<std::variant<double, int>> prev_load2_cog;
    std::vector<std::variant<double, int>> prev_load2_mass;
    std::vector<std::variant<double, int>> prev_load2_inertia;

    while (!stop_flag) {
        counter++;

        if (!kord->waitSync(std::chrono::milliseconds(10), kord::F_SYNC_FULL_ROTATION)) {
            KORD_LOG_ERROR("Sync RC failed.");
            return EXIT_FAILURE;
        }

        rcv_iface.fetchData();
        auto tcp_array = rcv_iface.getTCP();
        std::vector<std::variant<double, int>> tcp(tcp_array.begin(), tcp_array.end());
        auto tfc = rcv_iface.getFrame(kord::EFrameID::TFC_FRAME, kord::EFrameValue::POSE_VAL_REF_WF);
        auto tcp_wrt_tfc = rcv_iface.getFrame(kord::EFrameID::TCP_FRAME, kord::EFrameValue::POSE_VAL_REF_TFC);

        auto load1_cog = rcv_iface.getLoad(kord::ELoadID::LOAD1, kord::ELoadValue::COG_VAL);
        auto load1_mass = rcv_iface.getLoad(kord::ELoadID::LOAD1, kord::ELoadValue::MASS_VAL);
        auto load1_inertia = rcv_iface.getLoad(kord::ELoadID::LOAD1, kord::ELoadValue::INERTIA_VAL);

        auto load2_cog = rcv_iface.getLoad(kord::ELoadID::LOAD2, kord::ELoadValue::COG_VAL);
        auto load2_mass = rcv_iface.getLoad(kord::ELoadID::LOAD2, kord::ELoadValue::MASS_VAL);
        auto load2_inertia = rcv_iface.getLoad(kord::ELoadID::LOAD2, kord::ELoadValue::INERTIA_VAL);

        // Check if any vector has changed
        bool anyChange = !areVectorsSame(tcp, prev_tcp) || !areVectorsSame(tfc, prev_tfc) ||
                         !areVectorsSame(tcp_wrt_tfc, prev_tcp_wrt_tfc) || !areVectorsSame(load1_cog, prev_load1_cog) ||
                         !areVectorsSame(load1_mass, prev_load1_mass) ||
                         !areVectorsSame(load1_inertia, prev_load1_inertia) ||
                         !areVectorsSame(load2_cog, prev_load2_cog) || !areVectorsSame(load2_mass, prev_load2_mass) ||
                         !areVectorsSame(load2_inertia, prev_load2_inertia);

        if (anyChange) {
            // Update previous vectors
            prev_tcp = tcp;
            prev_tfc = tfc;
            prev_tcp_wrt_tfc = tcp_wrt_tfc;
            prev_load1_cog = load1_cog;
            prev_load1_mass = load1_mass;
            prev_load1_inertia = load1_inertia;
            prev_load2_cog = load2_cog;
            prev_load2_mass = load2_mass;
            prev_load2_inertia = load2_inertia;

            if (!extras.whitelist_enabled) {
                // Print all vectors
                printVector(tcp, "TCP:");
                printVector(tfc, "TFC Pose in WF:");
                printVector(tcp_wrt_tfc, "TCP Pose in TFC:");
                printVector(load1_cog, "Load1.cog =");
                printVector(load1_mass, "Load1.mass =");
                printVector(load1_inertia, "Load1.inertia =");
                printVector(load2_cog, "Load2.cog =");
                printVector(load2_mass, "Load2.mass =");
                printVector(load2_inertia, "Load2.inertia =");
                KORD_LOG_INFO("\n");
                continue;
            }

            if (extras.print_tcp) {
                printVector(tcp, "TCP:");
                printVector(tcp_wrt_tfc, "TCP Pose in TFC:");
            }
            if (extras.print_tfc) {
                printVector(tfc, "TFC Pose in WF:");
            }
            if (extras.print_load1) {
                printVector(load1_cog, "Load1.cog =");
                printVector(load1_mass, "Load1.mass =");
                printVector(load1_inertia, "Load1.inertia =");
            }
            if (extras.print_load2) {
                printVector(load2_cog, "Load2.cog =");
                printVector(load2_mass, "Load2.mass =");
                printVector(load2_inertia, "Load2.inertia =");
            }

            KORD_LOG_INFO("\n");
        }
    }

    KORD_LOG_INFO("Operation terminated gracefully.");
    return EXIT_SUCCESS;
}

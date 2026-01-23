#include <chrono>
#include <cmath>
#include <csignal>
#include <iomanip>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>

#include <kord/api/kord.h>
#include <kord/api/kord_control_interface.h>
#include <kord/api/kord_receive_interface.h>
#include <kord/utils/utils.h>

using namespace kr2;
volatile bool stop = false;

void signal_handler(sig_atomic_t a_signum)
{
    psignal(a_signum, "[KORD-API]");
    stop = true;
}
int main(int argc, char *argv[])
{
    struct ExtraOptions {
        std::optional<int> new_load;
        std::optional<std::array<double, 3>> new_cog;
        std::optional<std::array<double, 6>> new_inertia;
        std::optional<double> new_mass;
    } extras;
    static utils::LaunchParameters::ExternalArgParser ep = [argc, &argv, &extras](int index) -> void {
        static kr2::utils::SOALongOptions ex_options{std::array<kr2::utils::LongOption, 4>{
            kr2::utils::LongOption{{"load", required_argument, nullptr, 'l'},
                                   "specify which LOAD to set, choices: [0, 1]",
                                   "<int>"},
            kr2::utils::LongOption{{"cog", required_argument, nullptr, 'n'}, "specify center of gravity", "<x,y,z>"},
            kr2::utils::LongOption{{"inertia", required_argument, nullptr, 'k'},
                                   "specify inertia matrix",
                                   "<xx,yy,zz,xy,xz,yz>"},
            kr2::utils::LongOption{{"mass", required_argument, nullptr, 'm'}, "specify the mass", "<int>"},
        }};

        if (index <= utils::LaunchParameters::INVALID_INDEX) {
            std::cout << ex_options.helpString() << "\n";
            return;
        }

        int option_index = 0;
        optind = index;
        int opt = getopt_long(argc, argv, "l:n:k:m:", ex_options.getLongOptions(), &option_index);
        switch (opt) {
        case 'n': {
            std::stringstream ss(optarg);
            std::string item;
            std::vector<double> values;

            while (std::getline(ss, item, ',')) {
                values.push_back(std::stod(item));
            }

            if (values.size() == 3) {
                extras.new_cog.emplace();
                std::copy(values.begin(), values.end(), extras.new_cog->begin());
            }
            else {
                std::cerr << "Error: Expected 3 values for the center of gravity but got " << values.size() << "\n";
                exit(EXIT_FAILURE);
            }

            break;
        }
        case 'k': {
            std::stringstream ss(optarg);
            std::string item;
            std::vector<double> values;

            while (std::getline(ss, item, ',')) {
                values.push_back(std::stod(item));
            }

            if (values.size() == 6) {
                extras.new_inertia.emplace();
                std::copy(values.begin(), values.end(), extras.new_inertia->begin());
            }
            else {
                std::cerr << "Error: Expected 6 values for target joint configuration but got " << values.size() << "\n";
                exit(EXIT_FAILURE);
            }

            break;
        }
        case 'l': {
            extras.new_load.emplace();
            extras.new_load = std::stoi(optarg);
            break;
        }
        case 'm': {
            extras.new_mass.emplace();
            extras.new_mass = std::stoi(optarg);
            break;
        }
        default: {
            std::cout << "Unknown option found" << " optidx: " << optind << ", argc: " << argc << "\n";
            exit(EXIT_FAILURE);
        }
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

    if (!kord->syncRC()) {
        KORD_LOG_ERROR("Sync RC failed.");
        return EXIT_FAILURE;
    }
    KORD_LOG_INFO("Sync Captured");

    int64_t token;
    ctl_iface.setLoad(static_cast<kord::ELoadID>(extras.new_load.value()),
                      extras.new_mass.value(),
                      extras.new_cog.value(),
                      extras.new_inertia.value(),
                      token);

    KORD_LOG_INFO("Command sent with token: " << token);
    while (rcv_iface.getCommandStatus(token) == -1) {
        if (!kord->waitSync(std::chrono::milliseconds(20))) {
            KORD_LOG_ERROR("Sync wait timed out, exit");
            break;
        }

        rcv_iface.fetchData();
    }

    int8_t status = rcv_iface.getCommandStatus(token);
    KORD_LOG_INFO("Command status: " << signed(status));
    return 0;
}

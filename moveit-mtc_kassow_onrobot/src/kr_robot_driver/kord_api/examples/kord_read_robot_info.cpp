#include <csignal>
#include <iostream>

#include <kord/api/kord.h>
#include <kord/api/kord_control_interface.h>
#include <kord/api/kord_receive_interface.h>
#include <kord/protocol/RequestResponses/ResponseGetRobotInfo.h>
#include <kord/utils/utils.h>

using namespace kr2;
using namespace std::chrono_literals;

volatile bool stop = false;

void printVector(const std::vector<std::variant<double, int>> &vec, const std::string &label)
{
    std::string str = label + " [ ";
    for (const auto &val : vec) {
        str += std::to_string(std::get<double>(val)) + " ";
    }
    str += "]";
    KORD_LOG_INFO(str);
}

template <typename T, size_t N> void printArray(const std::array<T, N> &vec, const std::string &label)
{
    std::string str;
    str += label + " [ ";
    for (auto val : vec) {
        str += std::to_string(val) + " ";
    }
    str += "]";
    KORD_LOG_INFO(str);
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

    auto request = kord::RequestSystem().asGetRobotInfo();
    auto token = ctl_iface.transmitRequest(request);

    while (!stop) {
        if (!kord->waitSync(100ms)) {
            KORD_LOG_ERROR("Sync failed");
            break;
        }
        rcv_iface.fetchData();
        if (rcv_iface.hasResponse(token)) {
            auto response = rcv_iface.getResponse<kord::protocol::GetRobotInfoResponse>(token);

            std::string robot_model = response.getRobotModel();
            bool has_tool_io = response.getToolIO();
            std::string controller_sn = response.getControllerSN();
            std::string manipulator_sn = response.getManipulatorSN();
            std::string tablet_sn = response.getTabletSN();

            KORD_LOG_INFO("Robot model: " << robot_model);
            KORD_LOG_INFO("Has tool IO: " << has_tool_io);
            KORD_LOG_INFO("Controller SN: " << controller_sn);
            KORD_LOG_INFO("Manipulator SN: " << manipulator_sn);
            KORD_LOG_INFO("Tablet SN: " << tablet_sn);

            break;
        }
    }

    return 0;
}

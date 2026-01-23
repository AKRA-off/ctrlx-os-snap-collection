#include <csignal>
#include <iostream>

#include <kord/api/kord.h>
#include <kord/api/kord_control_interface.h>
#include <kord/api/kord_receive_interface.h>
#include <kord/protocol/RequestResponses/ResponseGetSafetyZones.h>
#include <kord/utils/utils.h>

using namespace kr2;
using namespace std::chrono_literals;

volatile bool stop = false;

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

    if (!kord->syncRC()) {
        KORD_LOG_ERROR("Sync RC failed.");
        return EXIT_FAILURE;
    }

    auto request = kord::RequestSystem().asGetSafetyZones();
    auto token = ctl_iface.transmitRequest(request);

    while (!stop) {
        if (!kord->waitSync(1s)) {
            KORD_LOG_ERROR("Sync failed");
            break;
        }
        rcv_iface.fetchData();
        if (rcv_iface.hasResponse(token)) {
            auto response = rcv_iface.getResponse<kord::protocol::GetSafetyZonesResponse>(token);

            std::vector<uint32_t> zones_duids = response.getSafetyZonesDUIDs();
            for (auto i : zones_duids) {
                KORD_LOG_INFO("- Safety zone DUID: " << i);
                kord::protocol::KORDSafetyZone zone = response.getSafetyZone(i);
                KORD_LOG_INFO(" > Zone label: " << zone.zone_label_);

                double sens[7];
                std::memcpy(sens, zone.sensitivities_, sizeof(sens));
                KORD_LOG_INFO(" > Sensitivities: (" << sens[0] << ", " << sens[1] << ", " << sens[2] << ", " << sens[3]
                                                    << ", " << sens[4] << ", " << sens[5] << ", " << sens[6] << ")");

                if (zone.n_geometries_ == 0) {
                    continue;
                }

                KORD_LOG_INFO(" > Geometries: ");
                auto geoms = zone.geometries_;
                for (int j = 0; j < zone.n_geometries_; j++) {
                    KORD_LOG_INFO("  - Geometry " << j << ": ");
                    auto geometry = geoms[j];
                    double points[3], normals[3];
                    std::memcpy(points, geometry.points_, sizeof(points));
                    std::memcpy(normals, geometry.normals_, sizeof(normals));
                    KORD_LOG_INFO("points=(" << points[0] << ", " << points[1] << ", " << points[2] << ")"
                                             << ", normals: (" << normals[0] << ", " << normals[1] << ", " << normals[2]
                                             << ")");
                }
            }
            break;
        }
    }

    return 0;
}

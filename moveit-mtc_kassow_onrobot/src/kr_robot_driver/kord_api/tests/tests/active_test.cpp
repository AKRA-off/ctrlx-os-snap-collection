#include <filesystem>
#include <gtest/gtest.h>
#include <kord/api/kord.h>

#include <kord/api/kord_control_interface.h>
#include <kord/api/kord_receive_interface.h>
#include <kord/system/CommandStatusFlags.h>

#include "utils.h"

using namespace kr2;

kr2::utils::LaunchParameters lp;

class KordTest : public testing::Test {
protected:
    KordTest()
    {
        kord_instance =
            std::make_shared<kord::KordCore>(lp.remote_controller_, lp.port_, lp.session_id_, kord::UDP_CLIENT);
    }

    virtual ~KordTest()
    {
        kord_instance = nullptr;

        // cleanup any pending stuff, but no exceptions allowed
    }

    // set up
    virtual void SetUp()
    {
        std::cout << "Trying to connect... \n";
        std::cout << "Check IP and port if it takes too long... \n";
        bool res = kord_instance->connect();
        ASSERT_EQ(res, true);
        if (!res) {
            FAIL() << "Failed to connect \n";
        }
        res &= kord_instance->syncRC();
        ASSERT_EQ(res, true);
        if (!res) {
            FAIL() << "Sync RC failed \n";
        }
    }

    int64_t requestLogTransfer(kord::KordCore &a_kord, kord::ControlInterface &a_ctl_iface)
    {
        if (!a_kord.waitSync(std::chrono::milliseconds(10))) {
            std::cout << "Sync wait timed out, exit \n";
            return -1;
        }

        // Create a request to the remote controller
        kr2::kord::RequestSystem sys_request;
        sys_request.asLogTransfer();
        if (!a_ctl_iface.transmitRequest(sys_request)) {
            return -2;
        }

        std::cout << "TX Request    RID: " << sys_request.request_rid_ << "\n";

        return sys_request.request_rid_;
    }

    bool monitorTransfer(kord::KordCore &a_kord, kord::ReceiverInterface &a_rcv_iface, int64_t a_req_id)
    {
        kr2::kord::Request response;

        auto start = std::chrono::steady_clock::now();

        while (std::chrono::steady_clock::now() - start < std::chrono::seconds(20)) {
            a_kord.waitSync(std::chrono::milliseconds(10));
            a_rcv_iface.fetchData();

            response = a_rcv_iface.getLatestRequest();

            // Check if the correct request is being evaluated
            if (a_req_id != response.request_rid_) {
                continue;
            }

            if (response.request_status_ == kr2::kord::protocol::EControlCommandStatus::eSuccess) {
                std::cout << "SUCCESS: Request with RID " << response.request_rid_ << ", transfer finished.\n";
                return true;
            }

            if (response.request_status_ == kr2::kord::protocol::EControlCommandStatus::eFailure) {
                std::cout << "FAILURE: Request with RID: " << response.request_rid_ << "\n";
                std::cout << "               error code: " << response.error_code_ << "\n";
                return false;
            }
        }

        std::cout << "Monitor transfer was interrupted\n";
        return false;
    }

    int64_t requestJsonTransfer(kord::KordCore &a_kord, kord::ControlInterface &a_ctl_iface)
    {
        if (!a_kord.waitSync(std::chrono::milliseconds(10))) {
            std::cout << "Sync wait timed out, exit \n";
            return -1;
        }

        // Create a request to the remote controller
        kr2::kord::RequestSystem sys_request;
        // sys_request.asDataTranfer().withDahsboardJSon().wasDashboardJsonTransfer();
        sys_request.asDashboardJsonTransfer();
        if (!a_ctl_iface.transmitRequest(sys_request)) {
            return -2;
        }

        std::cout << "TX Request    RID: " << sys_request.request_rid_ << "\n";

        return sys_request.request_rid_;
    }

    int64_t requestCalibrationTransfer(kord::KordCore &a_kord, kord::ControlInterface &a_ctl_iface)
    {
        if (!a_kord.waitSync(std::chrono::milliseconds(10))) {
            std::cout << "Sync wait timed out, exit \n";
            return -1;
        }

        // Create a request to the remote controller
        kr2::kord::RequestSystem sys_request;
        // sys_request.asDataTranfer().withDahsboardJSon().wasDashboardJsonTransfer();
        sys_request.asCalibrationDataTransfer();
        if (!a_ctl_iface.transmitRequest(sys_request)) {
            return -2;
        }

        std::cout << "TX Request    RID: " << sys_request.request_rid_ << "\n";

        return sys_request.request_rid_;
    }

    int64_t requestMoreFilesTransfer(kord::KordCore &a_kord, kord::ControlInterface &a_ctl_iface)
    {
        if (!a_kord.waitSync(std::chrono::milliseconds(10))) {
            std::cout << "Sync wait timed out, exit \n";
            return -1;
        }

        // Create a request to the remote controller
        kr2::kord::RequestTransfer transfer_request;
        // sys_request.asDataTranfer().withDahsboardJSon().wasDashboardJsonTransfer();
        transfer_request
            .asFilesTransfer()
            //.withDashboardJson()
            .withCalibration()
            .withLogs();

        if (!a_ctl_iface.transmitRequest(transfer_request)) {
            return -2;
        }

        std::cout << "TX Request    RID: " << transfer_request.request_rid_ << "\n";

        return transfer_request.request_rid_;
    }

    bool checkIfFolderExistsAndNotEmpty(std::string a_path_relative)
    {
        std::filesystem::path folderPath = std::filesystem::path(getenv("HOME")) / a_path_relative; // default folder

        if (std::filesystem::exists(folderPath) && std::filesystem::is_directory(folderPath)) {
            if (std::filesystem::is_empty(folderPath)) {
                std::cout << "The folder exists but is empty." << std::endl;
                return false;
            }
            else {
                std::cout << "The folder exists and is not empty." << std::endl;
                return true;
            }
        }
        else {
            std::cout << "The folder does not exist." << std::endl;
            return false;
        }
    }

    bool deleteFolder(std::string a_path_relative)
    {
        std::filesystem::path folderPath = std::filesystem::path(getenv("HOME")) / a_path_relative; // default folder
        try {
            std::filesystem::remove_all(folderPath);
            std::cout << "Folder deleted successfully." << std::endl;
        }
        catch (const std::filesystem::filesystem_error &e) {
            std::cout << "Error: " << e.what() << std::endl;
            return false;
        }

        return true;
    }

    std::shared_ptr<kord::KordCore> kord_instance;
};

TEST(KordTestBARE, isConnectionSuccesful)
{
    std::shared_ptr<kord::KordCore> kord(
        new kord::KordCore(lp.remote_controller_, lp.port_, lp.session_id_, kord::UDP_CLIENT));

    std::cout << "Trying to connect... \n";
    std::cout << "Check IP and port if it takes too long... \n";
    bool res = kord->connect();
    ASSERT_EQ(res, true);
    if (!res) {
        FAIL() << "Failed to connect \n";
    }
    res &= kord->syncRC();
    ASSERT_EQ(res, true);
    if (!res) {
        FAIL() << "Sync RC failed \n";
    }
}

TEST_F(KordTest, requestOk)
{
    kord::ControlInterface ctl_iface(kord_instance);
    kord::ReceiverInterface rcv_iface(kord_instance);

    if (!kord_instance->waitSync(std::chrono::milliseconds(10))) {
        FAIL() << "Sync wait timed out, exit \n";
    }

    // Create a request to the remote controller
    using DIGITAL_RELAYS = kord::RequestIO::DIGITAL_RELAYS;
    using DIGITAL_IOBOARD = kord::RequestIO::DIGITAL_IOBOARD;
    using DIGITAL_IOTOOLB = kord::RequestIO::DIGITAL_IOTOOLB;

    kord::RequestIO io_request;
    io_request.asSetIODigitalOut()
        .withEnabledPorts( // Enable Relay 1, Digital Output 8, Digital Output 6 and Tool Board 1 [24V]
            static_cast<int>(DIGITAL_RELAYS::RELAY1) | static_cast<int>(DIGITAL_IOBOARD::DO8) |
            static_cast<int>(DIGITAL_IOBOARD::DO6) | static_cast<int>(DIGITAL_IOTOOLB::TB1));

    kord_instance->sendCommand(io_request);
    std::cout << "TX Request    RID: " << io_request.request_rid_ << "\n";

    // Waiting for the execution result
    kord::Request response;
    while (true) {
        kord_instance->waitSync(std::chrono::milliseconds(10));
        rcv_iface.fetchData();

        response = rcv_iface.getLatestRequest();
        // Check if the correct request is being evaluated
        if (io_request.request_rid_ != response.request_rid_)
            continue;

        if (response.request_status_ == kr2::kord::protocol::EControlCommandStatus::eSuccess) {
            SUCCEED() << "SUCCESS: Request with RID " << response.request_rid_ << ", transfer finished.\n";
        }
        if (response.request_status_ == kr2::kord::protocol::EControlCommandStatus::eFailure) {
            FAIL() << "FAILURE: Request with RID: " << response.request_rid_ << "\n";
        }
        ASSERT_EQ(response.request_status_, kr2::kord::protocol::EControlCommandStatus::eSuccess);
        return;
    }
}

static bool g_run = true;

TEST_F(KordTest, moveJointsDiscreteOk)
{
    kord::ControlInterface ctl_iface(kord_instance);
    kord::ReceiverInterface rcv_iface(kord_instance);

    // master speed (zones)

    double sixty_deg = 60 * 3.1415 / 180.0;
    std::array<double, 7UL> q = {0, sixty_deg, 0, sixty_deg, 0, sixty_deg, 0};
    g_run = true;

    // initial move in joints to good position
    std::cout << "moving to a good initial position...\n";
    if (!kord_instance->waitSync(std::chrono::milliseconds(10))) {
        FAIL() << "Sync wait timed out, exit \n";
    }

    double tt_value = 5.0;
    double bt_value = 3.0;
    double sync_time = 10.0;

    ctl_iface.moveJ(q,
                    kr2::kord::TrackingType::TT_TIME,
                    tt_value,
                    kr2::kord::BlendType::BT_TIME,
                    bt_value,
                    kr2::kord::OverlayType::OT_VIAPOINT,
                    sync_time);
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

    sleep(tt_value);

    std::cout << "in the position...\n";

    if (!kord_instance->waitSync(std::chrono::milliseconds(1000), 2)) {
        FAIL() << "Sync wait timed out, exit \n";
    }

    rcv_iface.fetchData();

    std::array<double, 7UL> start_q = rcv_iface.getJoint(kord::ReceiverInterface::EJointValue::S_ACTUAL_Q);

    std::cout << "[Angle]Read initial joint configuration:\n";
    for (size_t i = 0; i < start_q.size(); i++) {
        double angl = start_q[i];
        std::cout << (angl / 3.14) * 180 << " ";
        q[i] = angl;
    }
    std::cout << "\n";

    tt_value = 5.0;
    bt_value = 3.0;
    sync_time = 10.0;

    if (!kord_instance->waitSync(std::chrono::milliseconds(10))) {
        FAIL() << "Sync wait timed out, exit \n";
    }

    for (size_t i = 0; i < 7; i++) {
        q[i] += 5 * 3.1415 / 180.0; // five degrees for every joint
    }

    ctl_iface.moveJ(q,
                    kr2::kord::TrackingType::TT_TIME,
                    tt_value,
                    kr2::kord::BlendType::BT_TIME,
                    bt_value,
                    kr2::kord::OverlayType::OT_VIAPOINT,
                    sync_time);
    std::cout << "Waiting for the 5 degrees move completion...\n";

    sleep(tt_value);

    if (!kord_instance->waitSync(std::chrono::milliseconds(1000), 2)) {
        FAIL() << "Sync wait timed out, exit \n";
    }

    // repeating 5 times because of packets drops
    for (int i = 0; i < 5; i++) {
        if (!kord_instance->waitSync(std::chrono::milliseconds(10))) {
            FAIL() << "Sync wait timed out, exit \n";
        }
        rcv_iface.fetchData();

        std::array<double, 7UL> now_q = rcv_iface.getJoint(kord::ReceiverInterface::EJointValue::S_ACTUAL_Q);
        double delta = 1e-3;
        std::cout << "[Angle]Read resulted joint configuration:\n";
        for (int el = 0; el < 7; el++) {
            std::cout << (now_q[el] / 3.14) * 180 << " ";
        }

        std::cout << "\n";

        for (int t = 0; t < 7; t++) {
            if (!(now_q[t] >= q[t] - delta && now_q[t] <= q[t] + delta)) {
                std::cout << "Problem, trying again \n";
                break;
            }
            else {
                for (int j = 0; j < 7; j++) {
                    EXPECT_NEAR(now_q[j], q[j], delta);
                }
                return;
            }
        }
    }

    FAIL() << "Not successful move.\n";
}

TEST_F(KordTest, moveLinearDiscreteOk)
{
    kord::ControlInterface ctl_iface(kord_instance);
    kord::ReceiverInterface rcv_iface(kord_instance);

    // TODO: prepare initial q
    std::array<double, 6UL> tcp_target;

    g_run = true;

    // Obtain initial q values
    if (!kord_instance->waitSync(std::chrono::milliseconds(10))) {
        FAIL() << "Sync wait timed out, exit \n";
    }
    std::cout << "Sync Captured \n";
    rcv_iface.fetchData();

    std::array<double, 6ul> start_tcp = rcv_iface.getTCP();

    std::cout << "[TCP]Read initial tcp configuration:\n";
    for (auto p : start_tcp)
        std::cout << p << " ";

    std::cout << std::endl;

    double tt_value = 10.0;
    double bt_value = 10.0;
    double sync_time = 10.0;

    tcp_target[0] = start_tcp[0] + 0.05;
    tcp_target[1] = start_tcp[1] - 0.05; // x, y, z
    tcp_target[2] = start_tcp[2] + 0.05;
    tcp_target[3] = start_tcp[3];
    tcp_target[4] = start_tcp[4]; // r, p , y
    tcp_target[5] = start_tcp[5];

    // kord->spin();
    if (!kord_instance->waitSync(std::chrono::milliseconds(10))) {
        FAIL() << "Sync wait timed out, exit \n";
    }

    ctl_iface.moveL(tcp_target,
                    kr2::kord::TrackingType::TT_TIME,
                    tt_value,
                    kr2::kord::BlendType::BT_TIME,
                    bt_value,
                    kr2::kord::OverlayType::OT_VIAPOINT,
                    sync_time);

    std::cout << "Waiting for move complete...\n";
    sleep(tt_value);

    if (!kord_instance->waitSync(std::chrono::milliseconds(1000), 2)) {
        FAIL() << "Sync wait timed out, exit \n";
    }

    // repeating 5 times because of packets drops
    for (int i = 0; i < 5; i++) {
        if (!kord_instance->waitSync(std::chrono::milliseconds(10))) {
            FAIL() << "Sync wait timed out, exit \n";
        }
        rcv_iface.fetchData();

        std::array<double, 6ul> now_tcp = rcv_iface.getTCP();
        double delta = 1e-3;
        std::cout << "[TCP]Read resulted tcp configuration:\n";
        for (size_t el = 0; el < 6; el++) {
            std::cout << now_tcp[i] << " ";
        }

        std::cout << "\n";

        for (int t = 0; t < 6; t++) {
            if (!(now_tcp[t] >= tcp_target[t] - delta && now_tcp[t] <= tcp_target[t] + delta)) {
                std::cout << "Problem, trying again \n";
                break;
            }
            else {
                for (int j = 0; j < 6; j++) {
                    // EXPECT_TRUE(now_tcp[j] >= tcp_target[j] - delta && now_tcp[j] <= tcp_target[j] + delta);
                    EXPECT_NEAR(now_tcp[j], tcp_target[j], delta);
                }
                return;
            }
        }
    }

    FAIL() << "Not successful move.\n";
}

TEST_F(KordTest, moveLinearOk)
{
    kord::ControlInterface ctl_iface(kord_instance);
    kord::ReceiverInterface rcv_iface(kord_instance);

    // initially in a good position

    // TODO: prepare initial q
    std::array<double, 6UL> tcp_target;
    std::array<double, 6ul> now_tcp;

    g_run = true;

    // Obtain initial q values
    if (!kord_instance->waitSync(std::chrono::milliseconds(10))) {
        FAIL() << "Sync wait timed out, exit \n";
    }
    std::cout << "Sync Captured \n";
    rcv_iface.fetchData();

    std::array<double, 6ul> start_tcp = rcv_iface.getTCP();

    std::cout << "[TCP]Read initial tcp configuration:\n";
    for (auto p : start_tcp)
        std::cout << p << " ";

    std::cout << std::endl;

    unsigned int i = 0;
    double a = 0.1;
    double t = 0.0;
    double tt_value = 0.008;
    double bt_value = 0.004;

    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

    while (g_run) {
        //
        // calculation
        // update tcp target
        //
        tcp_target[0] = start_tcp[0];
        tcp_target[1] = start_tcp[1];
        tcp_target[2] = (std::cos(t * 2e-4) - 1) * a + start_tcp[2];
        tcp_target[3] = start_tcp[3];
        tcp_target[4] = start_tcp[4];
        tcp_target[5] = start_tcp[5];
        t = i * 7;
        i++;

        // kord->spin();
        if (!kord_instance->waitSync(std::chrono::milliseconds(10))) {
            std::cout << "Sync wait timed out, exit \n";
            break;
        }

        ctl_iface.moveL(tcp_target,
                        kr2::kord::TrackingType::TT_TIME,
                        tt_value,
                        kr2::kord::BlendType::BT_TIME,
                        bt_value,
                        kr2::kord::OverlayType::OT_VIAPOINT);

        rcv_iface.fetchData();

        now_tcp = rcv_iface.getTCP();
        double delta = 1e-3;

        for (int j = 0; j < 6; j++) {
            EXPECT_TRUE((now_tcp[j] >= tcp_target[j] - delta) && (now_tcp[j] <= tcp_target[j] + delta));
        }

        if (rcv_iface.systemAlarmState() || t > 31415) // full period
        {
            break;
        }
    }
}

TEST_F(KordTest, moveJointsOk)
{
    kord::ControlInterface ctl_iface(kord_instance);
    kord::ReceiverInterface rcv_iface(kord_instance);

    // Obtain initial q values
    std::array<double, 7UL> q;
    if (!kord_instance->waitSync(std::chrono::milliseconds(10))) {
        std::cout << "Sync wait timed out, exit \n";
    }

    std::cout << "Sync Captured \n";
    rcv_iface.fetchData();
    std::array<double, 7UL> start_q = rcv_iface.getJoint(kord::ReceiverInterface::EJointValue::S_ACTUAL_Q);

    std::cout << "Read initial joint configuration:\n";
    for (double angl : start_q)
        std::cout << (angl / 3.14) * 180 << " ";

    std::cout << "\n";

    unsigned int i = 0;
    double a = 0.01;
    double t = 0.0;
    double tt_value = 0.008;
    double bt_value = 0.004;

    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

    while (g_run) {

        //...insert code here
        //
        // calculation
        // update q
        //
        q[0] = (std::cos(t * 2e-4) - 1) * a + start_q[0];
        q[1] = (std::cos(t * 3.3e-4) - 1) * a + start_q[1];
        q[2] = (std::cos(t * 4.5e-4) - 1) * a + start_q[2];
        q[3] = (std::cos(t * 2.4e-4) - 1) * a + start_q[3];
        q[4] = (std::cos(t * 6e-4) - 1) * a + start_q[4];
        q[5] = (std::cos(t * 8e-4) - 1) * a + start_q[5];
        q[6] = (std::cos(t * 1e-3) - 1) * a + start_q[6];
        t = i * 7;
        i++;

        //        kord->spin();
        if (!kord_instance->waitSync(std::chrono::milliseconds(10))) {
            std::cout << "Sync wait timed out, exit \n";
            break;
        }

        ctl_iface.moveJ(q,
                        kr2::kord::TrackingType::TT_TIME,
                        tt_value,
                        kr2::kord::BlendType::BT_TIME,
                        bt_value,
                        kr2::kord::OverlayType::OT_VIAPOINT);

        rcv_iface.fetchData();

        std::array<double, 7UL> now_joint = rcv_iface.getJoint(kord::ReceiverInterface::EJointValue::S_ACTUAL_Q);
        double delta = 5 * 1e-3;

        for (int j = 0; j < 7; j++) {
            EXPECT_TRUE((now_joint[j] >= q[j] - delta) && (now_joint[j] <= q[j] + delta));
        }

        if (rcv_iface.systemAlarmState() || t > 2 * 31415) {
            break;
        }
    }
}

TEST_F(KordTest, SetFrame)
{
    // STAND BY
    kord::ControlInterface ctl_iface(kord_instance);
    kord::ReceiverInterface rcv_iface(kord_instance);
    std::map<kord::EFrameID, kord::EFrameValue> test_frame_id_ref_map_ = {{kord::EFrameID::ROBOT_BASE_FRAME,
                                                                           kord::EFrameValue::POSE_VAL_REF_WF},
                                                                          {kord::EFrameID::TCP_FRAME,
                                                                           kord::EFrameValue::POSE_VAL_REF_TFC}};

    for (auto test : test_frame_id_ref_map_) {

        if (!kord_instance->waitSync(std::chrono::milliseconds(10))) {
            std::cout << "Sync wait timed out, exit \n";
        }

        rcv_iface.fetchData();

        std::cout << "Getting initial Frame values... \n";
        std::vector<std::variant<double, int>> init_pose = rcv_iface.getFrame(test.first, test.second);

        for (auto p : init_pose)
            std::cout << std::get<double>(p) << " ";
        std::cout << "]" << std::endl;

        std::array<double, 6UL> p = {1, 2, -1, 0.5, 0.3, 0};
        int64_t token;

        std::cout << "Sending Set command... \n";
        ctl_iface.setFrame(test.first, p, test.second, token);
        std::cout << "Command sent \n";
        std::cout << "Command token: " << token << '\n';

        while (rcv_iface.getCommandStatus(token) == -1) {
            std::cout << "not found yet \n";
            if (!kord_instance->waitSync(std::chrono::milliseconds(20))) {
                std::cout << "Sync wait timed out, exit \n";
                break;
            }
            rcv_iface.fetchData();
        }
        int status = rcv_iface.getCommandStatus(token);
        std::cout << '\n' << "Command status: " << status << '\n';

        EXPECT_TRUE(status == kord::protocol::ECommandStatusFlags::COMMAND_STATUS_ACCEPTED);

        if (!kord_instance->waitSync(std::chrono::milliseconds(20), 1)) {
            std::cout << "Sync wait timed out, exit \n";
        }

        rcv_iface.fetchData();

        std::vector<std::variant<double, int>> pose = rcv_iface.getFrame(test.first, test.second);
        std::cout << "Read values: [ ";
        for (int el = 0; el < 6; ++el) {
            std::cout << std::get<double>(pose[el]) << " ";
            EXPECT_NEAR(std::get<double>(pose[el]), p[el], 1e-3);
        }
        std::cout << "]" << std::endl;

        std::cout << "resetting to the old values...\n";

        if (!kord_instance->waitSync(std::chrono::milliseconds(20), 1)) {
            std::cout << "Sync wait timed out, exit \n";
        }

        ctl_iface.setFrame(test.first,
                           {std::get<double>(init_pose[0]),
                            std::get<double>(init_pose[1]),
                            std::get<double>(init_pose[2]),
                            std::get<double>(init_pose[3]),
                            std::get<double>(init_pose[4]),
                            std::get<double>(init_pose[5])},
                           test.second,
                           token);

        while (rcv_iface.getCommandStatus(token) == -1) {
            std::cout << "not found yet \n";
            if (!kord_instance->waitSync(std::chrono::milliseconds(20))) {
                std::cout << "Sync wait timed out, exit \n";
                break;
            }

            rcv_iface.fetchData();
        }
        status = rcv_iface.getCommandStatus(token);
        std::cout << '\n' << "Command status: " << status << '\n';

        EXPECT_TRUE(status == kord::protocol::ECommandStatusFlags::COMMAND_STATUS_ACCEPTED);
    }
}

TEST_F(KordTest, SetLoad)
{
    kord::ControlInterface ctl_iface(kord_instance);
    kord::ReceiverInterface rcv_iface(kord_instance);

    std::array<kord::ELoadID, 2> test_load_id_ = {kord::ELoadID::LOAD1, kord::ELoadID::LOAD2};

    for (auto test : test_load_id_) {

        if (!kord_instance->waitSync(std::chrono::milliseconds(20), 1)) {
            std::cout << "Sync wait timed out, exit \n";
        }
        rcv_iface.fetchData();
        std::vector<std::variant<double, int>> init_inertia = rcv_iface.getLoad(test, kord::ELoadValue::INERTIA_VAL);

        for (auto p : init_inertia)
            std::cout << std::get<double>(p) << " ";
        std::cout << "]" << std::endl;

        std::vector<std::variant<double, int>> init_cog = rcv_iface.getLoad(test, kord::ELoadValue::COG_VAL);

        for (auto p : init_cog)
            std::cout << std::get<double>(p) << " ";
        std::cout << "]" << std::endl;

        std::vector<std::variant<double, int>> init_mass = rcv_iface.getLoad(test, kord::ELoadValue::MASS_VAL);

        for (auto p : init_mass)
            std::cout << std::get<double>(p) << " ";
        std::cout << "]" << std::endl;

        // sending set request
        std::array<double, 3UL> p = {0.1, -0.7, 2};
        std::array<double, 6UL> in = {0.1, 0.1, 0.1, 0.1, 0.1, 0.1};
        double mass = 0.1;
        int64_t token;
        ctl_iface.setLoad(test, mass, p, in, token);

        std::cout << "Command sent \n";
        std::cout << "Command token: " << token << '\n';

        while (rcv_iface.getCommandStatus(token) == -1) {
            std::cout << "not found yet \n";
            if (!kord_instance->waitSync(std::chrono::milliseconds(20))) {
                std::cout << "Sync wait timed out, exit \n";
                break;
            }

            rcv_iface.fetchData();
            // std::cout << "1";
        }
        int status = rcv_iface.getCommandStatus(token);
        std::cout << '\n' << "Command status: " << status << '\n';

        EXPECT_TRUE(status == kord::protocol::ECommandStatusFlags::COMMAND_STATUS_ACCEPTED);

        if (!kord_instance->waitSync(std::chrono::milliseconds(20), 1)) {
            std::cout << "Sync wait timed out, exit \n";
        }

        rcv_iface.fetchData();
        std::vector<std::variant<double, int>> pose = rcv_iface.getLoad(test, kord::ELoadValue::INERTIA_VAL);
        std::cout << "Inertia: [ ";
        for (int el = 0; el < 6; ++el) {
            std::cout << std::get<double>(pose[el]) << " ";
            EXPECT_NEAR(std::get<double>(pose[el]), in[el], 1e-3);
        }
        std::cout << "]" << std::endl;

        pose = rcv_iface.getLoad(test, kord::ELoadValue::COG_VAL);
        std::cout << "CoG: [ ";
        for (int el = 0; el < 3; ++el) {
            std::cout << std::get<double>(pose[el]) << " ";
            EXPECT_NEAR(std::get<double>(pose[el]), p[el], 1e-3);
        }
        std::cout << "]" << std::endl;

        std::vector<std::variant<double, int>> read_mass = rcv_iface.getLoad(test, kord::ELoadValue::MASS_VAL);
        std::cout << "Mass: [ ";
        for (int el = 0; el < 1; ++el) {
            std::cout << std::get<double>(read_mass[el]) << " ";
            EXPECT_NEAR(std::get<double>(read_mass[el]), mass, 1e-3);
        }
        std::cout << "]" << std::endl;

        std::cout << "resetting to the old values...\n";

        if (!kord_instance->waitSync(std::chrono::milliseconds(20), 1)) {
            std::cout << "Sync wait timed out, exit \n";
        }

        ctl_iface.setLoad(test,
                          std::get<double>(init_mass[0]),
                          {std::get<double>(init_cog[0]), std::get<double>(init_cog[1]), std::get<double>(init_cog[2])},
                          {std::get<double>(init_inertia[0]),
                           std::get<double>(init_inertia[1]),
                           std::get<double>(init_inertia[2]),
                           std::get<double>(init_inertia[3]),
                           std::get<double>(init_inertia[4]),
                           std::get<double>(init_inertia[5])},
                          token);

        while (rcv_iface.getCommandStatus(token) == -1) {
            std::cout << "not found yet \n";
            if (!kord_instance->waitSync(std::chrono::milliseconds(20))) {
                std::cout << "Sync wait timed out, exit \n";
                break;
            }

            rcv_iface.fetchData();
        }
        status = rcv_iface.getCommandStatus(token);
        std::cout << '\n' << "Command status: " << status << '\n';

        EXPECT_TRUE(status == kord::protocol::ECommandStatusFlags::COMMAND_STATUS_ACCEPTED);
    }
}

TEST_F(KordTest, transferLogs)
{
    kord::ControlInterface ctl_iface(kord_instance);
    kord::ReceiverInterface rcv_iface(kord_instance);

    const std::string log_path = "kassow_logs/";

    if (checkIfFolderExistsAndNotEmpty(log_path)) {
        FAIL() << "Delete the " << log_path << " folder to perform the test \n";
    }

    // Obtain initial q values
    if (!kord_instance->waitSync(std::chrono::milliseconds(10))) {
        FAIL() << "Sync wait timed out, exit \n";
    }
    std::cout << "Sync Captured \n";
    rcv_iface.fetchData();

    int64_t request_id = 0;
    request_id = requestLogTransfer(*kord_instance, ctl_iface);
    if (request_id <= 0) {
        FAIL() << "Failed to sent log request\n";
    }

    //
    // Waiting for the execution result
    //
    if (!monitorTransfer(*kord_instance, rcv_iface, request_id)) {
        FAIL() << "Failure\n";
    }

    if (checkIfFolderExistsAndNotEmpty(log_path)) {
        deleteFolder(log_path);
    }
    else {
        FAIL() << "The folder is not created with correct data \n";
    }

    std::cout << "Done\n";
}

TEST_F(KordTest, transferCalibration)
{
    kord::ControlInterface ctl_iface(kord_instance);
    kord::ReceiverInterface rcv_iface(kord_instance);

    const std::string calibr_path = "kassow_calibration/";

    if (checkIfFolderExistsAndNotEmpty(calibr_path)) {
        FAIL() << "Delete the " << calibr_path << " folder to perform the test \n";
    }

    // Obtain initial q values
    if (!kord_instance->waitSync(std::chrono::milliseconds(10))) {
        FAIL() << "Sync wait timed out, exit \n";
    }
    std::cout << "Sync Captured \n";
    rcv_iface.fetchData();

    int64_t request_id = 0;
    request_id = requestCalibrationTransfer(*kord_instance, ctl_iface);
    if (request_id <= 0) {
        FAIL() << "Failed to sent log request\n";
    }

    //
    // Waiting for the execution result
    //
    if (!monitorTransfer(*kord_instance, rcv_iface, request_id)) {
        FAIL() << "Failure\n";
    }

    if (checkIfFolderExistsAndNotEmpty(calibr_path)) {
        deleteFolder(calibr_path);
    }
    else {
        FAIL() << "The folder is not created with correct data \n";
    }

    std::cout << "Done\n";
}

TEST_F(KordTest, transferJson)
{
    kord::ControlInterface ctl_iface(kord_instance);
    kord::ReceiverInterface rcv_iface(kord_instance);

    const std::string json_path = "kassow_diagnostics/";

    if (checkIfFolderExistsAndNotEmpty(json_path)) {
        FAIL() << "Delete the " << json_path << " folder to perform the test \n";
    }

    // Obtain initial q values
    if (!kord_instance->waitSync(std::chrono::milliseconds(10))) {
        FAIL() << "Sync wait timed out, exit \n";
    }
    std::cout << "Sync Captured \n";
    rcv_iface.fetchData();

    int64_t request_id = 0;
    request_id = requestJsonTransfer(*kord_instance, ctl_iface);
    if (request_id <= 0) {
        FAIL() << "Failed to sent log request\n";
    }

    //
    // Waiting for the execution result
    //
    std::cout << "Check that the file is present on the robot if it takes too long... \n";
    if (!monitorTransfer(*kord_instance, rcv_iface, request_id)) {
        FAIL() << "Failure\n";
    }

    if (checkIfFolderExistsAndNotEmpty(json_path)) {
        deleteFolder(json_path);
    }
    else {
        FAIL() << "The folder is not created with correct data \n";
    }

    std::cout << "Done\n";
}

TEST_F(KordTest, engageBreaksOk)
{
    kord::ControlInterface ctl_iface(kord_instance);
    kord::ReceiverInterface rcv_iface(kord_instance);

    // Obtain initial q values
    if (!kord_instance->waitSync(std::chrono::milliseconds(10))) {
        FAIL() << "Sync wait timed out, exit \n";
    }
    std::cout << "Sync Captured \n";
    rcv_iface.fetchData();

    if (!kord_instance->waitSync(std::chrono::milliseconds(10))) {
        FAIL() << "Sync wait timed out, exit \n";
    }

    // When heartbeat was captured, transmit the request - brake joints 5,6,7
    std::vector<int> joints{5, 6, 7};
    ASSERT_EQ(ctl_iface.engageBrakes(joints), true);
    ASSERT_EQ(ctl_iface.disengageBrakes(joints), true);
}

volatile bool stop = false;

void signal_handler(sig_atomic_t a_signum)
{
    psignal(a_signum, "[KORD-API]");
    stop = true;
    g_run = true;
    exit(a_signum);
}

int main(int argc, char *argv[])
{
    lp = kr2::utils::LaunchParameters::processLaunchArguments(argc, argv);

    if (lp.help_) {
        lp.printUsage();
        return EXIT_SUCCESS;
    }

    signal(SIGINT, signal_handler);

    if (lp.useRealtime()) {
        if (!kr2::utils::realtime::init_realtime_params(lp.rt_prio_)) {
            std::cerr << "Failed to start with realtime priority\n";
            lp.printUsage();
            return EXIT_FAILURE;
        }
    }

    testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
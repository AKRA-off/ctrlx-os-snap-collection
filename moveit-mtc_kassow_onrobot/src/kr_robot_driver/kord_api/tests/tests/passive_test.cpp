#include <gtest/gtest.h>

#include <kord/api/kord.h>
#include <kord/api/kord_control_interface.h>
#include <kord/api/kord_receive_interface.h>

#include "utils.h"

using namespace kr2;

kr2::utils::LaunchParameters lp;

class KordTest : public testing::Test {
protected:
    KordTest()
    {
        // initialization code here

        kord_instance =
            std::make_shared<kord::KordCore>(lp.remote_controller_, lp.port_, lp.session_id_, kord::UDP_CLIENT);
    }

    virtual ~KordTest()
    {
        kord_instance = nullptr;

        // cleanup any pending stuff, but no exceptions allowed
    }

    std::shared_ptr<kord::KordCore> kord_instance;
};

TEST_F(KordTest, isConnectionSuccesful)
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

TEST_F(KordTest, noErrors)
{
    kord::ControlInterface ctl_iface(kord_instance);
    kord::ReceiverInterface rcv_iface(kord_instance);

    // Obtain initial q values
    if (!kord_instance->syncRC()) {
        FAIL() << "Sync RC failed.\n";
    }

    rcv_iface.fetchData();

    std::vector<kr2::kord::protocol::SystemEvent> events = rcv_iface.getSystemEvents();

    if (events.size() > 0) {
        for (std::vector<int>::size_type idx = 0; idx < events.size(); ++idx)
            if (idx < 1)
                std::cout << events[idx].toString() << std::endl;
            else
                std::cout << events[idx].toStringWithRef(events[idx - 1]) << std::endl;
    }
    ASSERT_EQ(events.size(), 0);
    rcv_iface.clearSystemEventsBuffer();
}

TEST_F(KordTest, temperaturesOk)
{
    kord::ControlInterface ctl_iface(kord_instance);
    kord::ReceiverInterface rcv_iface(kord_instance);

    // Obtain initial q values
    if (!kord_instance->syncRC()) {
        FAIL() << "Sync RC failed.\n";
    }

    rcv_iface.fetchData();

    std::array<double, 7UL> temp_q = rcv_iface.getJoint(kord::ReceiverInterface::EJointValue::S_TEMP_BOARD);
    std::cout << "Joints Temperatures: \n";
    for (double temp : temp_q) {
        std::cout << temp << " ";
        EXPECT_TRUE((temp >= 5) && (temp <= 80)) << "Try to activate robot if all 0";
    }
    std::cout << '\n';

    temp_q = rcv_iface.getJoint(kord::ReceiverInterface::EJointValue::S_TEMP_JOINT_ENCODER);
    std::cout << "Joints Encoder Temperatures: \n";
    for (double temp : temp_q) {
        std::cout << temp << " ";
        EXPECT_TRUE((temp >= 5) && (temp <= 80)) << "Try to activate robot if all 0";
    }
    std::cout << '\n';

    temp_q = rcv_iface.getJoint(kord::ReceiverInterface::EJointValue::S_TEMP_ROTOR_ENCODER);
    std::cout << "Rotor Encoder Temperatures: \n";
    for (double temp : temp_q) {
        std::cout << temp << " ";
        EXPECT_TRUE((temp >= 5) && (temp <= 80)) << "Try to activate robot if all values 0";
    }
    std::cout << '\n';

    double tempIO_q = rcv_iface.getIOBoardTemperature();

    std::cout << "IOBoard Temperature: " << tempIO_q << std::endl;
    EXPECT_TRUE((tempIO_q >= 5) && (tempIO_q <= 80));

    double cpu_temp = rcv_iface.getCPUState(kord::ReceiverInterface::ECPUStateValue::PACKAGE_ID0_TEMP);

    std::cout << "Pack 0: " << cpu_temp << std::endl;
    EXPECT_TRUE((cpu_temp >= 5) && (cpu_temp <= 100));

    cpu_temp = rcv_iface.getCPUState(kord::ReceiverInterface::ECPUStateValue::CORE_0_TEMP);
    std::cout << "Core 0: " << cpu_temp << std::endl;
    EXPECT_TRUE((cpu_temp >= 5) && (cpu_temp <= 100));

    cpu_temp = rcv_iface.getCPUState(kord::ReceiverInterface::ECPUStateValue::CORE_1_TEMP);
    std::cout << "Core 1: " << cpu_temp << std::endl;
    EXPECT_TRUE((cpu_temp >= 5) && (cpu_temp <= 100));

    cpu_temp = rcv_iface.getCPUState(kord::ReceiverInterface::ECPUStateValue::CORE_2_TEMP);
    std::cout << "Core 2: " << cpu_temp << std::endl;
    EXPECT_TRUE((cpu_temp >= 5) && (cpu_temp <= 100));

    cpu_temp = rcv_iface.getCPUState(kord::ReceiverInterface::ECPUStateValue::CORE_3_TEMP);
    std::cout << "Core 3: " << cpu_temp << std::endl;
    EXPECT_TRUE((cpu_temp >= 5) && (cpu_temp <= 100));
}

TEST_F(KordTest, jointsRadiansOk)
{
    kord::ControlInterface ctl_iface(kord_instance);
    kord::ReceiverInterface rcv_iface(kord_instance);

    // Obtain initial q values
    if (!kord_instance->syncRC()) {
        FAIL() << "Sync RC failed.\n";
    }

    rcv_iface.fetchData();

    std::array<double, 7UL> start_q = rcv_iface.getJoint(kord::ReceiverInterface::EJointValue::S_ACTUAL_Q);
    std::cout << "Radians:";
    for (double angl : start_q) {
        std::cout << angl << ";";
        EXPECT_TRUE((angl >= 2 * -3.15) && (angl <= 2 * 3.15));
    }
    std::cout << std::endl;
}

TEST_F(KordTest, robotStatusFlagsOk)
{
    kord::ControlInterface ctl_iface(kord_instance);
    kord::ReceiverInterface rcv_iface(kord_instance);

    if (!kord_instance->syncRC()) {
        FAIL() << "Sync RC failed.\n";
    }

    rcv_iface.fetchData();

    auto res1 = rcv_iface.getRobotSafetyFlags();
    std::cout << "Safety flags: " << res1 << "\n";
    EXPECT_TRUE(res1 >= 0);
    auto res2 = rcv_iface.getMotionFlags();
    std::cout << "Motion flags: " << res2 << "\n";
    EXPECT_TRUE(res2 >= 0);
    auto res3 = rcv_iface.getSafetyMode();
    std::cout << "Safety  mode: " << res3 << "\n";
    EXPECT_TRUE(res3 >= 0);
    auto res4 = rcv_iface.getMasterSpeed();
    std::cout << "Master speed: " << res4 << "\n";
    EXPECT_TRUE(res4 >= 0.0 && res4 <= 1.0);
}

static bool g_run = true;

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

    if (lp.help_ || !lp.valid_) {
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
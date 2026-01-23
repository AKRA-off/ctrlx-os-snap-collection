#include <eigen3/Eigen/Core>
#include <gtest/gtest.h>

#include <kord/api/kord.h>
#include <kord/api/kord_control_interface.h>
#include <kord/api/kord_receive_interface.h>

using namespace kr2;

const std::string hostname = "192.168.100.175";
constexpr unsigned int port = 7582;    // Default
constexpr unsigned int session_id = 1; // Default
constexpr double tt_value = 0.008;     // tracking time
constexpr double bt_value = 0.004;     // blend time
constexpr double JOINT_TARGETING_PRECISION =
    0.005; // precision of a joint angle, everything below is considered "target reached"
constexpr double POSE_TARGETING_PRECISION = 0.01; // precision of a pose, everything below is considered "target reached"

class MoveFixture : public ::testing::Test {
public:
    MoveFixture()
        : m_kord(std::make_shared<kord::KordCore>(hostname, port, session_id, kord::UDP_CLIENT)), m_ctl_iface(m_kord),
          m_rcv_iface(m_kord)
    {
    }

    void SetUp() override
    {
        ASSERT_TRUE(m_kord->connect()) << "Could not connect to the robot";
        ASSERT_TRUE(m_kord->syncRC()) << "Could not sync with the robot";

        // single fetch
        m_rcv_iface.fetchData();

        // check if the robot is in a safe state
        uint32_t alarm_state = m_rcv_iface.systemAlarmState();
        ASSERT_EQ(alarm_state, 0) << "Robot is in an alarm state (" << alarm_state << "). Cannot proceed";

        // skip the confirmation by running cmake with -DSKIP_USER_CONFIRMATION=ON
#ifndef SKIP_USER_CONFIRMATION
        // ask the user to confirm with "y" to proceed
        std::cout << "From this point on, the robot may move. Please confirm with 'y' to proceed: ";
        std::string input;
        std::cin >> input;
        if (input != "y") {
            GTEST_SKIP() << "User did not confirm, exiting";
        }
#endif
    }

    void TearDown() override
    {
        // clear all alarms
        auto commands = {kord::ControlInterface::EClearRequest::CLEAR_HALT,
                         kord::ControlInterface::EClearRequest::CBUN_EVENT,
                         kord::ControlInterface::EClearRequest::CONTINUE_INIT,
                         kord::ControlInterface::EClearRequest::UNSUSPEND};

        for (const auto &command : commands) {
            int64_t token = m_ctl_iface.clearAlarmRequest(command);
            while (m_rcv_iface.getCommandStatus(token) == -1) {
                ASSERT_TRUE(m_kord->waitSync(std::chrono::milliseconds(10)));
                m_rcv_iface.fetchData();
            }
        }

        m_kord->disconnect();
    }

    std::shared_ptr<kord::KordCore> m_kord;
    kord::ControlInterface m_ctl_iface;
    kord::ReceiverInterface m_rcv_iface;
};

TEST_F(MoveFixture, moveJ_whenCommandingCurrentJointAngles_noMovementNoError)
{
    // Arrange
    // get joint angles
    auto joint_angles = m_rcv_iface.getJoint(kord::ReceiverInterface::EJointValue::S_ACTUAL_Q);

    // Act
    // Send commands to robot
    ASSERT_TRUE(m_kord->waitSync(std::chrono::milliseconds(10))) << "Could not sync with the robot";
    m_ctl_iface.moveJ(joint_angles,
                      kord::TrackingType::TT_TIME,
                      tt_value,
                      kord::BlendType::BT_TIME,
                      bt_value,
                      kord::OverlayType::OT_VIAPOINT);

    // Assert
    m_rcv_iface.fetchData();
    ASSERT_FALSE(m_rcv_iface.systemAlarmState()) << "Robot is in an alarm state";

    // check if the robot has moved
    auto new_joint_angles = m_rcv_iface.getJoint(kord::ReceiverInterface::EJointValue::S_ACTUAL_Q);
    for (size_t i = 0; i < joint_angles.size(); ++i) {
        ASSERT_DOUBLE_EQ(joint_angles[i], new_joint_angles[i]) << "Joint " << i << " has moved";
    }

    auto new_joint_velocities = m_rcv_iface.getJoint(kord::ReceiverInterface::EJointValue::S_ACTUAL_QD);
    for (size_t i = 0; i < new_joint_velocities.size(); ++i) {
        ASSERT_DOUBLE_EQ(new_joint_velocities[i], 0) << "Joint " << i << " is moving";
    }
}

TEST_F(MoveFixture, moveJ_whenCommandingTinyJointMovement_moveABitNoError)
{
    // Arrange
    // get joint angles
    const auto original_joint_angles = m_rcv_iface.getJoint(kord::ReceiverInterface::EJointValue::S_ACTUAL_Q);
    auto target_joint_angles = original_joint_angles;
    target_joint_angles[6] += 0.001; // move joint 6 by 1 milliradian

    // Act
    // Move to target position
    for (size_t i = 0; i < 10; ++i) {
        ASSERT_TRUE(m_kord->waitSync(std::chrono::milliseconds(10))) << "Could not sync with the robot";
        m_ctl_iface.moveJ(target_joint_angles,
                          kord::TrackingType::TT_TIME,
                          tt_value,
                          kord::BlendType::BT_TIME,
                          bt_value,
                          kord::OverlayType::OT_VIAPOINT);
        m_rcv_iface.fetchData();
        ASSERT_FALSE(m_rcv_iface.systemAlarmState()) << "Robot is in an alarm state";
    }

    // check where we ended up
    m_rcv_iface.fetchData();
    const auto reached_joint_angles = m_rcv_iface.getJoint(kord::ReceiverInterface::EJointValue::S_ACTUAL_Q);

    // rotate back
    for (size_t i = 0; i < 10; ++i) {
        ASSERT_TRUE(m_kord->waitSync(std::chrono::milliseconds(10))) << "Could not sync with the robot";
        m_ctl_iface.moveJ(original_joint_angles,
                          kord::TrackingType::TT_TIME,
                          tt_value,
                          kord::BlendType::BT_TIME,
                          bt_value,
                          kord::OverlayType::OT_VIAPOINT);
        m_rcv_iface.fetchData();
        ASSERT_FALSE(m_rcv_iface.systemAlarmState()) << "Robot is in an alarm state";
    }

    // Assert
    const auto final_joint_angles = m_rcv_iface.getJoint(kord::ReceiverInterface::EJointValue::S_ACTUAL_Q);
    ASSERT_FALSE(m_rcv_iface.systemAlarmState()) << "Robot is in an alarm state";

    // check if the robot has reached the target
    for (size_t i = 0; i < original_joint_angles.size(); ++i) {
        ASSERT_TRUE(std::abs(reached_joint_angles[i] - target_joint_angles[i]) < JOINT_TARGETING_PRECISION)
            << "Joint " << i << " has not reached the target. Target: " << target_joint_angles[i]
            << ", reached: " << reached_joint_angles[i];
    }

    // check if the robot has moved back to the original position
    for (size_t i = 0; i < original_joint_angles.size(); ++i) {
        ASSERT_TRUE(std::abs(final_joint_angles[i] - original_joint_angles[i]) < JOINT_TARGETING_PRECISION)
            << "Joint " << i << " has not moved back to the original position. Original: " << original_joint_angles[i]
            << ", final: " << final_joint_angles[i];
    }

    ASSERT_EQ(m_rcv_iface.systemAlarmState(), 0) << "Robot is in an alarm state";
}

TEST_F(MoveFixture, moveJ_whenCommandingInfeasibleJointAngleStep_noMovementAndError)
{
    // Arrange
    // get joint angles
    m_rcv_iface.fetchData();
    const auto original_joint_angles = m_rcv_iface.getJoint(kord::ReceiverInterface::EJointValue::S_ACTUAL_Q);
    auto target_joint_angles = original_joint_angles;
    target_joint_angles[6] += M_PI; // this angle cannot be reached in one time step
    // uint32_t expected_alarm_state = 0;

    // Act
    // Send commands to robot
    ASSERT_TRUE(m_kord->waitSync(std::chrono::milliseconds(10))) << "Could not sync with the robot";
    std::cout << "Target joint angles: " << std::endl;
    for (const auto &angle : target_joint_angles) {
        std::cout << angle << std::endl;
    }

    ASSERT_TRUE(m_ctl_iface.moveJ(target_joint_angles,
                                  kord::TrackingType::TT_TIME,
                                  tt_value,
                                  kord::BlendType::BT_TIME,
                                  bt_value,
                                  kord::OverlayType::OT_VIAPOINT))
        << "Sending joint command was unsuccessful";
    // wait a bit
    auto system_events = std::vector<kr2::kord::protocol::SystemEvent>();
    for (size_t i = 0; i < 100; ++i) {
        ASSERT_TRUE(m_kord->waitSync(std::chrono::milliseconds(10))) << "Could not sync with the robot";
        m_rcv_iface.fetchData();

        // append new system events
        // std::cout << i << std::endl;
        const auto new_events = m_rcv_iface.getSystemEvents();
        system_events.insert(system_events.end(), new_events.begin(), new_events.end());
    }

    const auto reached_joint_angles = m_rcv_iface.getJoint(kord::ReceiverInterface::EJointValue::S_ACTUAL_Q);

    // Assert
    // uint32_t actual_alarm_state = m_rcv_iface.systemAlarmState();
    // TODO(Jonas): unclear if the robot should be in alarm state or just produce a system event
    //    ASSERT_EQ(actual_alarm_state, 0) << "Robot should be in an alarm state " << expected_alarm_state << " but is
    // in "
    //                                     << actual_alarm_state;

    // check if we moved. We shouldn't have
    for (size_t i = 0; i < original_joint_angles.size(); ++i) {
        EXPECT_TRUE(std::abs(reached_joint_angles[i] - original_joint_angles[i]) < JOINT_TARGETING_PRECISION)
            << "Joint " << i << " has moved from " << std::endl
            << original_joint_angles[i] << " to " << std::endl
            << reached_joint_angles[i];
    }

    // check if we got the correct system events
    ASSERT_FALSE(system_events.empty()) << "No system events received";
    for (const auto &event : system_events) {
        EXPECT_TRUE(event.event_group_ == kr2::kord::protocol::EEventGroup::eKordEvent)
            << "Event occurred that is not a KORD event";
        EXPECT_TRUE(event.event_id_ == kr2::kord::protocol::EKordEventConditionID::INFEASIBLE_MOVE_COMMAND)
            << "KORD event occurred that is not a model torque limit violation";
    }
}

TEST_F(MoveFixture, moveL_whenCommandingInfeasiblePositionJump_noMovementAndError)
{
    // Arrange
    // get joint angles, into eigen vector
    const auto original_pose = m_rcv_iface.getTCP();

    // target pose is original pose + 10cm in x direction
    auto target_pose = original_pose;
    target_pose[0] += 0.1;
    // uint32_t expected_alarm_state = 0;

    // Act
    // Send commands to robot
    ASSERT_TRUE(m_kord->waitSync(std::chrono::milliseconds(10))) << "Could not sync with the robot";
    ASSERT_TRUE(m_ctl_iface.moveL(target_pose,
                                  kord::TrackingType::TT_TIME,
                                  tt_value,
                                  kord::BlendType::BT_TIME,
                                  bt_value,
                                  kord::OverlayType::OT_VIAPOINT))
        << "Sending pose command was unsuccessful";
    // wait a bit
    auto system_events = std::vector<kr2::kord::protocol::SystemEvent>();
    for (size_t i = 0; i < 100; ++i) {
        ASSERT_TRUE(m_kord->waitSync(std::chrono::milliseconds(10))) << "Could not sync with the robot";
        m_rcv_iface.fetchData();

        // append new system events
        const auto new_events = m_rcv_iface.getSystemEvents();
        system_events.insert(system_events.end(), new_events.begin(), new_events.end());
    }

    std::cout << "Reached pose: ";
    for (int i = 0; i < 6; ++i) {
        std::cout << m_rcv_iface.getTCP()[i] << " ";
    }
    std::cout << std::endl;

    auto reached_tcp = m_rcv_iface.getTCP();
    const auto reached_pose = Eigen::VectorXd::Map(reached_tcp.data(), reached_tcp.size());

    // Assert
    // uint32_t actual_alarm_state = m_rcv_iface.systemAlarmState();
    // TODO(Jonas): unclear if the robot should be in alarm state or just produce a system event
    //    ASSERT_EQ(actual_alarm_state, 0) << "Robot should be in an alarm state " << expected_alarm_state << " but is
    // in
    //    "
    //                                     << actual_alarm_state;

    // check if we moved. We shouldn't have
    const auto original_pose_eigen = Eigen::VectorXd::Map(original_pose.data(), original_pose.size());
    ASSERT_TRUE(((reached_pose - original_pose_eigen).cwiseAbs().array() < POSE_TARGETING_PRECISION).all())
        << "Robot has moved from " << original_pose_eigen.transpose() << " to " << reached_pose.transpose();

    // check if we got the correct system events
    ASSERT_FALSE(system_events.empty()) << "No system events received";
    for (const auto &event : system_events) {
        EXPECT_TRUE(event.event_group_ == kr2::kord::protocol::EEventGroup::eKordEvent)
            << "Event occurred that is not a soft stop event";
        EXPECT_TRUE(event.event_id_ == kr2::kord::protocol::EKordEventConditionID::INFEASIBLE_MOVE_COMMAND)
            << "Soft stop event occurred that is not a model torque limit violation";
    }
}

TEST_F(MoveFixture, moveL_whenCommandingInfeasibleRotationJump_noMovementAndError)
{
    // Arrange
    // get joint angles, into eigen vector
    const auto original_pose = m_rcv_iface.getTCP();

    // target pose is original pose + 10cm in x direction
    auto target_pose = original_pose;
    target_pose[4] += M_PI;
    
    // uint32_t expected_alarm_state = 0;

    // Act
    // Send commands to robot
    ASSERT_TRUE(m_kord->waitSync(std::chrono::milliseconds(10))) << "Could not sync with the robot";
    ASSERT_TRUE(m_ctl_iface.moveL(target_pose,
                                  kord::TrackingType::TT_TIME,
                                  tt_value,
                                  kord::BlendType::BT_TIME,
                                  bt_value,
                                  kord::OverlayType::OT_VIAPOINT))
        << "Sending pose command was unsuccessful";
    // wait a bit
    auto system_events = std::vector<kr2::kord::protocol::SystemEvent>();
    for (size_t i = 0; i < 100; ++i) {
        ASSERT_TRUE(m_kord->waitSync(std::chrono::milliseconds(10))) << "Could not sync with the robot";
        m_rcv_iface.fetchData();

        // append new system events
        const auto new_events = m_rcv_iface.getSystemEvents();
        system_events.insert(system_events.end(), new_events.begin(), new_events.end());
    }

    std::cout << "Reached pose: ";
    for (int i = 0; i < 6; ++i) {
        std::cout << m_rcv_iface.getTCP()[i] << " ";
    }
    std::cout << std::endl;

    auto reached_tcp = m_rcv_iface.getTCP();
    const auto reached_pose = Eigen::VectorXd::Map(reached_tcp.data(), reached_tcp.size());

    // Assert
    // uint32_t actual_alarm_state = m_rcv_iface.systemAlarmState();
    // TODO(Jonas): unclear if the robot should be in alarm state or just produce a system event
    //    ASSERT_EQ(actual_alarm_state, 0) << "Robot should be in an alarm state " << expected_alarm_state << " but is
    // in
    //    "
    //                                     << actual_alarm_state;

    // check if we moved. We shouldn't have
    const auto original_pose_eigen = Eigen::VectorXd::Map(original_pose.data(), original_pose.size());
    ASSERT_TRUE(((reached_pose - original_pose_eigen).cwiseAbs().array() < POSE_TARGETING_PRECISION).all())
        << "Robot has moved from " << original_pose_eigen.transpose() << " to " << reached_pose.transpose();

    // check if we got the correct system events
    ASSERT_FALSE(system_events.empty()) << "No system events received";
    for (const auto &event : system_events) {
        EXPECT_TRUE(event.event_group_ == kr2::kord::protocol::EEventGroup::eKordEvent)
            << "Event occurred that is not a soft stop event";
        EXPECT_TRUE(event.event_id_ == kr2::kord::protocol::EKordEventConditionID::INFEASIBLE_MOVE_COMMAND)
            << "Soft stop event occurred that is not a model torque limit violation";
    }
}

// TEST_F(JDA16_DynamicConstraints, moveJoints_fastMovement_movementAndoErrors)
// {
//     // std::vector<std::array<double, 6>> poses = {{0.854589, 0.088740, 1.290977, 0.836760, 1.095702, 0.948466},
//     //                                             {0.662081, 0.508067, 1.217423, 1.396726, 0.924842, 2.261453},
//     //                                             {0.766688, -0.471182, 1.223300, 0.729896, 1.301150, -0.017832}};
//     double tracking_time = 0.008;
//     double blend_time = 0.004;
//
//     auto start_tcp = m_rcv_iface.getTCP();
//     auto new_tcp = start_tcp;
//
//     auto n_ticks = 10 * 250;
//     for (int i = 0; i < n_ticks; i++) {
//         ASSERT_TRUE(m_kord->waitSync(std::chrono::milliseconds(10))) << "Could not sync with the robot";
//
//         new_tcp[0] = start_tcp[0] + sin((i % 500) * (2.0 * M_PI / 500)) * 0.01;
//         ASSERT_TRUE(m_ctl_iface.moveL(new_tcp,
//                                       kord::TrackingType::TT_TIME,
//                                       tracking_time,
//                                       kord::BlendType::BT_TIME,
//                                       blend_time,
//                                       kord::OverlayType::OT_VIAPOINT))
//             << "Sending pose command was unsuccessful";
//
//         if (!m_rcv_iface.getSystemEvents().empty()) {
//             std::cout << "System event received" << std::endl;
//             FAIL();
//         }
//     }
//
//     // wait a bit
//     auto system_events = std::vector<kr2::kord::protocol::SystemEvent>();
//     for (size_t i = 0; i < 100; ++i) {
//         ASSERT_TRUE(m_kord->waitSync(std::chrono::milliseconds(10))) << "Could not sync with the robot";
//         m_rcv_iface.fetchData();
//
//         // append new system events
//         const auto new_events = m_rcv_iface.getSystemEvents();
//         system_events.insert(system_events.end(), new_events.begin(), new_events.end());
//     }
//
//     // check if we got the correct system events
//     // ASSERT_TRUE(system_events.empty()) << "System events received, but none were expected";
// }

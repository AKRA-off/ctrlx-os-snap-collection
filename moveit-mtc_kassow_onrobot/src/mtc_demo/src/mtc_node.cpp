#include <rclcpp/rclcpp.hpp>
#include <moveit/planning_scene/planning_scene.h>
#include <moveit/planning_scene_interface/planning_scene_interface.h>
#include <moveit/task_constructor/task.h>
#include <moveit/task_constructor/stages.h>
#include <moveit/task_constructor/solvers.h>
#include <moveit/task_constructor/stages/current_state.h>
#include <moveit/task_constructor/stages/move_to.h>
#include <moveit/task_constructor/stages/move_relative.h>
#include <moveit/task_constructor/stages/connect.h>
#include <moveit/task_constructor/stages/modify_planning_scene.h>
#include <moveit_task_constructor_msgs/action/execute_task_solution.hpp>
#include <std_srvs/srv/trigger.hpp>

static const rclcpp::Logger LOGGER = rclcpp::get_logger("mtc_node");
namespace mtc = moveit::task_constructor;

class MtcTaskNode : public rclcpp::Node
{
public:
  MtcTaskNode(const rclcpp::NodeOptions& options);

  void setupPlanningScene();
  void doTask();

private:
  void executeTaskService(
    const std::shared_ptr<std_srvs::srv::Trigger::Request> request,
    std::shared_ptr<std_srvs::srv::Trigger::Response> response);

  mtc::TaskPtr task_;
  rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr execute_service_;
  bool auto_execute_;
};

MtcTaskNode::MtcTaskNode(const rclcpp::NodeOptions& options)
  : Node("mtc_node", options)
{
  this->declare_parameter("auto_execute", false);
  auto_execute_ = this->get_parameter("auto_execute").as_bool();

  execute_service_ = this->create_service<std_srvs::srv::Trigger>(
    "/execute_task",
    std::bind(&MtcTaskNode::executeTaskService, this,
              std::placeholders::_1, std::placeholders::_2));

  RCLCPP_INFO(LOGGER, "MTC node ready. Service '/execute_task' available.");

  if (auto_execute_) {
    RCLCPP_INFO(LOGGER, "Auto-execute enabled, running task...");
    doTask();
  } else {
    RCLCPP_INFO(LOGGER, "Waiting for service call to execute task");
  }
}

void MtcTaskNode::executeTaskService(
  const std::shared_ptr<std_srvs::srv::Trigger::Request> request,
  std::shared_ptr<std_srvs::srv::Trigger::Response> response)
{
  (void)request;

  RCLCPP_INFO(LOGGER, "Task execution requested via service");

  if (doTask()) {
    response->success = true;
    response->message = "Task executed successfully";
  } else {
    response->success = false;
    response->message = "Task execution failed";
  }
}

void MtcTaskNode::setupPlanningScene()
{
  moveit_msgs::msg::CollisionObject object;
  object.id = "object";
  object.header.frame_id = "base";
  object.primitives.resize(1);
  object.primitives[0].type = shape_msgs::msg::SolidPrimitive::CYLINDER;
  object.primitives[0].dimensions = {0.1, 0.02};

  geometry_msgs::msg::Pose pose;
  pose.position.x = 0.3;
  pose.position.y = 0.0;
  pose.position.z = 0.05;
  pose.orientation.w = 1.0;
  object.pose = pose;

  moveit::planning_interface::PlanningSceneInterface psi;
  psi.applyCollisionObject(object);

  RCLCPP_INFO(LOGGER, "Planning scene configured with object at (0.3, 0.0, 0.05)");
}

bool MtcTaskNode::doTask()
{
  setupPlanningScene();

  task_ = std::make_shared<mtc::Task>();
  task_->stages()->setName("Pick and Place Task");
  task_->loadRobotModel(shared_from_this());

  // Configure task properties for Kassow KR810 + RG2
  const std::string arm_group_name = "kr810_arm";
  const std::string hand_group_name = "rg2_gripper";
  const std::string eef_name = "rg2";
  const std::string hand_frame = "gripper_tcp";

  task_->setProperty("group", arm_group_name);
  task_->setProperty("eef", eef_name);
  task_->setProperty("ik_frame", hand_frame);

  // Create solvers
  auto sampling_planner = std::make_shared<mtc::solvers::PipelinePlanner>(shared_from_this());
  auto interpolation_planner = std::make_shared<mtc::solvers::JointInterpolationPlanner>();
  auto cartesian_planner = std::make_shared<mtc::solvers::CartesianPath>();
  cartesian_planner->setMaxVelocityScalingFactor(1.0);
  cartesian_planner->setMaxAccelerationScalingFactor(1.0);
  cartesian_planner->setStepSize(0.01);

  // ===== STAGE 1: Current State =====
  mtc::Stage* current_state_ptr = nullptr;
  {
    auto stage = std::make_unique<mtc::stages::CurrentState>("current");
    current_state_ptr = stage.get();
    task_->add(std::move(stage));
  }

  // ===== STAGE 2: Open Gripper =====
  {
    auto stage = std::make_unique<mtc::stages::MoveTo>("open gripper", interpolation_planner);
    stage->setGroup(hand_group_name);
    stage->setGoal("open");
    task_->add(std::move(stage));
  }

  // ===== PICK CONTAINER (Stages 3-6) =====
  mtc::Stage* attach_object_stage = nullptr;
  {
    auto grasp = std::make_unique<mtc::SerialContainer>("pick object");
    task_->properties().exposeTo(grasp->properties(), {"eef", "group", "ik_frame"});
    grasp->properties().configureInitFrom(mtc::Stage::PARENT, {"eef", "group", "ik_frame"});

    // === STAGE 3: Approach Object ===
    {
      auto stage = std::make_unique<mtc::stages::MoveRelative>("approach object", cartesian_planner);
      stage->properties().set("marker_ns", "approach_object");
      stage->properties().set("link", hand_frame);
      stage->properties().configureInitFrom(mtc::Stage::PARENT, {"group"});
      stage->setMinMaxDistance(0.1, 0.15);

      geometry_msgs::msg::Vector3Stamped vec;
      vec.header.frame_id = hand_frame;
      vec.vector.z = -1.0;
      stage->setDirection(vec);
      grasp->insert(std::move(stage));
    }

    // === STAGE 4: Generate Grasp Pose ===
    {
      auto stage = std::make_unique<mtc::stages::GenerateGraspPose>("generate grasp pose");
      stage->properties().configureInitFrom(mtc::Stage::PARENT);
      stage->properties().set("marker_ns", "grasp_pose");
      stage->setPreGraspPose("open");
      stage->setObject("object");
      stage->setAngleDelta(M_PI / 12);
      stage->setMonitoredStage(current_state_ptr);

      auto wrapper = std::make_unique<mtc::stages::ComputeIK>("grasp pose IK", std::move(stage));
      wrapper->setMaxIKSolutions(8);
      wrapper->setMinSolutionDistance(1.0);
      wrapper->setIKFrame(hand_frame);
      wrapper->properties().configureInitFrom(mtc::Stage::PARENT, {"eef", "group"});
      wrapper->properties().configureInitFrom(mtc::Stage::INTERFACE, {"target_pose"});
      grasp->insert(std::move(wrapper));
    }

    // === STAGE 5: Allow Collision (gripper <-> object) ===
    {
      auto stage = std::make_unique<mtc::stages::ModifyPlanningScene>("allow collision (hand,object)");
      stage->allowCollisions("object",
                            task_->getRobotModel()->getJointModelGroup(hand_group_name)->getLinkModelNamesWithCollisionGeometry(),
                            true);
      attach_object_stage = stage.get();
      grasp->insert(std::move(stage));
    }

    // === STAGE 6: Close Gripper ===
    {
      auto stage = std::make_unique<mtc::stages::MoveTo>("close gripper", interpolation_planner);
      stage->setGroup(hand_group_name);
      stage->setGoal("close");
      grasp->insert(std::move(stage));
    }

    // === STAGE 7: Attach Object ===
    {
      auto stage = std::make_unique<mtc::stages::ModifyPlanningScene>("attach object");
      stage->attachObject("object", hand_frame);
      attach_object_stage = stage.get();
      grasp->insert(std::move(stage));
    }

    // === STAGE 8: Lift Object ===
    {
      auto stage = std::make_unique<mtc::stages::MoveRelative>("lift object", cartesian_planner);
      stage->properties().configureInitFrom(mtc::Stage::PARENT, {"group"});
      stage->setMinMaxDistance(0.05, 0.1);
      stage->setIKFrame(hand_frame);
      stage->properties().set("marker_ns", "lift_object");

      geometry_msgs::msg::Vector3Stamped vec;
      vec.header.frame_id = "base";
      vec.vector.z = 1.0;
      stage->setDirection(vec);
      grasp->insert(std::move(stage));
    }

    task_->add(std::move(grasp));
  }

  // ===== STAGE 9: Move to Place Location =====
  {
    auto stage = std::make_unique<mtc::stages::Connect>(
      "move to place",
      mtc::stages::Connect::GroupPlannerVector{{arm_group_name, sampling_planner}});
    stage->setTimeout(5.0);
    stage->properties().configureInitFrom(mtc::Stage::PARENT);
    task_->add(std::move(stage));
  }

  // ===== PLACE CONTAINER (Stages 10-13) =====
  {
    auto place = std::make_unique<mtc::SerialContainer>("place object");
    task_->properties().exposeTo(place->properties(), {"eef", "group", "ik_frame"});
    place->properties().configureInitFrom(mtc::Stage::PARENT, {"eef", "group", "ik_frame"});

    // === STAGE 10: Lower Object ===
    {
      auto stage = std::make_unique<mtc::stages::MoveRelative>("lower object", cartesian_planner);
      stage->properties().set("marker_ns", "lower_object");
      stage->properties().set("link", hand_frame);
      stage->properties().configureInitFrom(mtc::Stage::PARENT, {"group"});
      stage->setMinMaxDistance(0.05, 0.1);

      geometry_msgs::msg::Vector3Stamped vec;
      vec.header.frame_id = "base";
      vec.vector.z = -1.0;
      stage->setDirection(vec);
      place->insert(std::move(stage));
    }

    // === STAGE 11: Generate Place Pose ===
    {
      auto stage = std::make_unique<mtc::stages::GeneratePlacePose>("generate place pose");
      stage->properties().configureInitFrom(mtc::Stage::PARENT);
      stage->properties().set("marker_ns", "place_pose");
      stage->setObject("object");

      geometry_msgs::msg::PoseStamped target_pose_msg;
      target_pose_msg.header.frame_id = "base";
      target_pose_msg.pose.position.x = 0.0;
      target_pose_msg.pose.position.y = 0.3;
      target_pose_msg.pose.position.z = 0.05;
      target_pose_msg.pose.orientation.w = 1.0;
      stage->setPose(target_pose_msg);
      stage->setMonitoredStage(attach_object_stage);

      auto wrapper = std::make_unique<mtc::stages::ComputeIK>("place pose IK", std::move(stage));
      wrapper->setMaxIKSolutions(8);
      wrapper->setMinSolutionDistance(1.0);
      wrapper->setIKFrame(hand_frame);
      wrapper->properties().configureInitFrom(mtc::Stage::PARENT, {"eef", "group"});
      wrapper->properties().configureInitFrom(mtc::Stage::INTERFACE, {"target_pose"});
      place->insert(std::move(wrapper));
    }

    // === STAGE 12: Open Gripper ===
    {
      auto stage = std::make_unique<mtc::stages::MoveTo>("open gripper", interpolation_planner);
      stage->setGroup(hand_group_name);
      stage->setGoal("open");
      place->insert(std::move(stage));
    }

    // === STAGE 13: Forbid Collision (gripper <-> object) ===
    {
      auto stage = std::make_unique<mtc::stages::ModifyPlanningScene>("forbid collision (hand,object)");
      stage->allowCollisions("object",
                            task_->getRobotModel()->getJointModelGroup(hand_group_name)->getLinkModelNamesWithCollisionGeometry(),
                            false);
      place->insert(std::move(stage));
    }

    // === STAGE 14: Detach Object ===
    {
      auto stage = std::make_unique<mtc::stages::ModifyPlanningScene>("detach object");
      stage->detachObject("object", hand_frame);
      place->insert(std::move(stage));
    }

    // === STAGE 15: Retreat ===
    {
      auto stage = std::make_unique<mtc::stages::MoveRelative>("retreat", cartesian_planner);
      stage->properties().configureInitFrom(mtc::Stage::PARENT, {"group"});
      stage->setMinMaxDistance(0.1, 0.15);
      stage->setIKFrame(hand_frame);
      stage->properties().set("marker_ns", "retreat");

      geometry_msgs::msg::Vector3Stamped vec;
      vec.header.frame_id = hand_frame;
      vec.vector.z = -1.0;
      stage->setDirection(vec);
      place->insert(std::move(stage));
    }

    task_->add(std::move(place));
  }

  // ===== STAGE 16: Move to Home =====
  {
    auto stage = std::make_unique<mtc::stages::MoveTo>("move home", interpolation_planner);
    stage->properties().configureInitFrom(mtc::Stage::PARENT, {"group"});
    stage->setGoal("home");
    task_->add(std::move(stage));
  }

  // Plan the task
  try {
    RCLCPP_INFO(LOGGER, "Planning task...");
    task_->init();

    if (!task_->plan(5)) {
      RCLCPP_ERROR(LOGGER, "Task planning failed");
      return false;
    }

    RCLCPP_INFO(LOGGER, "Planning succeeded! Found %zu solutions", task_->solutions().size());
  }
  catch (const mtc::InitStageException& e) {
    RCLCPP_ERROR(LOGGER, "Task initialization failed: %s", e.what());
    return false;
  }

  // Execute the task
  auto result = task_->execute(*task_->solutions().front());
  if (result.val != moveit_msgs::msg::MoveItErrorCodes::SUCCESS) {
    RCLCPP_ERROR(LOGGER, "Task execution failed");
    return false;
  }

  RCLCPP_INFO(LOGGER, "Task executed successfully!");
  return true;
}

int main(int argc, char** argv)
{
  rclcpp::init(argc, argv);

  rclcpp::NodeOptions options;
  options.automatically_declare_parameters_from_overrides(true);

  auto node = std::make_shared<MtcTaskNode>(options);

  rclcpp::executors::MultiThreadedExecutor executor;
  executor.add_node(node);
  executor.spin();

  rclcpp::shutdown();
  return 0;
}
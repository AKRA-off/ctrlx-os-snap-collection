#!/usr/bin/env python3

"""
Xbox Controller Joystick Teleoperation Node

Reads Xbox Series controller input and publishes robot control commands.

Button/Axis Mapping:
- Left stick X/Y (axes 0/1): XY movement (cartesian velocity)
- Left trigger (axis 2): Z up (positive linear.z)
- Right trigger (axis 5): Z down (negative linear.z)
- Right stick X/Y (axes 3/4): Rotations (angular.x and angular.y)
- LB button (310): Rotate gripper CCW (negative angular.z)
- RB button (311): Rotate gripper CW (positive angular.z)
- A button (304): Close gripper (to D-pad configured width)
- B button (305): Open gripper (to D-pad configured width)
- D-pad Up/Down: Adjust gripper close width
- D-pad Left/Right: Adjust movement speed
- Share button (167): Save home position (in teaching mode)
- X button (307): Save pick position (in teaching mode)
- Y button (308): Save place position (in teaching mode)
- Back button (314): Toggle teaching mode
- Start button (315): Execute MTC task (in manual mode)
"""

import rclpy
from rclpy.node import Node
from rclpy.action import ActionClient
from geometry_msgs.msg import TwistStamped
from control_msgs.action import GripperCommand, FollowJointTrajectory
from trajectory_msgs.msg import JointTrajectory, JointTrajectoryPoint
from builtin_interfaces.msg import Duration
from std_srvs.srv import Trigger
from sensor_msgs.msg import JointState
import time
import yaml
from pathlib import Path

from joystick_control.joystick import Joystick


class JoystickTeleopNode(Node):
    def __init__(self):
        super().__init__('joystick_teleop_node')

        # Parameters
        self.declare_parameter('linear_max_velocity', 0.1)  # m/s (10 cm/s)
        self.declare_parameter('angular_max_velocity', 0.5)  # rad/s (~28 deg/s)
        self.declare_parameter('deadzone', 0.1)
        self.declare_parameter('update_rate', 50.0)  # Hz
        self.declare_parameter('enable_servo', True)
        self.declare_parameter('joystick_device', '')  # Empty string = auto-detect

        # Taught positions file path - configurable for deployment
        # Priority: 1. SNAP_USER_DATA (snap), 2. ROS_DATA_PATH (env var), 3. workspace data dir
        import os
        from ament_index_python.packages import get_package_share_directory

        # Try to find the workspace root from package location
        try:
            pkg_share = get_package_share_directory('joystick_control')
            # pkg_share is typically: <workspace>/install/share/joystick_control
            # Go up 3 levels to get workspace root, then to data directory
            workspace_root = os.path.dirname(os.path.dirname(os.path.dirname(pkg_share)))
            default_data_path = os.path.join(workspace_root, 'data', 'taught_positions.yaml')
        except:
            # Fallback if package not found (shouldn't happen)
            default_data_path = '/tmp/taught_positions.yaml'

        # Check for environment variable overrides (for snap deployment)
        if 'SNAP_USER_DATA' in os.environ:
            # Running in snap - use snap's writable data directory
            default_data_path = os.path.join(os.environ['SNAP_USER_DATA'], 'taught_positions.yaml')
        elif 'ROS_DATA_PATH' in os.environ:
            # Custom data path specified
            default_data_path = os.path.join(os.environ['ROS_DATA_PATH'], 'taught_positions.yaml')

        self.declare_parameter('taught_positions_file', default_data_path)

        self.linear_max_vel = self.get_parameter('linear_max_velocity').value
        self.angular_max_vel = self.get_parameter('angular_max_velocity').value
        self.deadzone = self.get_parameter('deadzone').value
        self.update_rate = self.get_parameter('update_rate').value
        self.enable_servo = self.get_parameter('enable_servo').value
        self.taught_positions_path = self.get_parameter('taught_positions_file').value

        # Publisher for MoveIt Servo
        self.twist_pub = self.create_publisher(
            TwistStamped,
            '/servo_node/delta_twist_cmds',
            10
        )

        # Gripper action client - using GripperCommand for real hardware TCP controller
        self.gripper_client = ActionClient(
            self,
            GripperCommand,
            '/rg2_gripper_controller/gripper_cmd'
        )

        # Joystick state tracking
        self.axis_stats = {}  # Track min/max for normalization
        self.prev_button_a = 0
        self.prev_button_b = 0

        # D-PAD state tracking (for edge detection)
        self.prev_dpad_x = 0  # D-PAD left/right
        self.prev_dpad_y = 0  # D-PAD up/down

        # Speed control (D-PAD left/right)
        self.speed_scaling = 1.0  # Default 100%

        # Gripper width control
        self.gripper_open_width = 0.11  # Always open to max 110mm (RG2 max)
        self.gripper_close_width = 0.0  # Close width adjustable with D-PAD up/down

        # Teaching mode (Back button toggle)
        # Start in manual mode with servo paused
        self.teaching_mode = False
        self.prev_button_back = 0
        self.prev_button_x = 0
        self.prev_button_y = 0
        self.prev_button_share = 0  # Share/Screenshot button for home position
        self.prev_button_start = 0

        # Joint state for position teaching
        self.current_joint_state = None
        self.joint_state_sub = self.create_subscription(
            JointState,
            '/joint_states',
            self.joint_state_callback,
            10
        )

        # MTC execution service client
        self.mtc_trigger_client = self.create_client(Trigger, '/mtc/execute_task')

        # Servo pause/unpause service clients
        self.servo_pause_client = self.create_client(Trigger, '/servo_node/pause_servo')
        self.servo_unpause_client = self.create_client(Trigger, '/servo_node/unpause_servo')

        # Track if servo is paused - start paused in manual mode
        self.servo_paused = True

        # Velocity ramping for smooth acceleration/deceleration
        # Prevents torque spikes from instant velocity changes
        # ULTRA conservative for Joint 1 (base rotation) with very high inertia
        self.prev_linear_vel = [0.0, 0.0, 0.0]  # [x, y, z]
        self.prev_angular_vel = [0.0, 0.0, 0.0]  # [x, y, z]
        self.max_linear_accel = 0.15  # m/s² - ultra conservative ramping
        self.max_angular_accel = 0.15 # rad/s² - ultra conservative for Joint 1 high inertia

        # Exponential smoothing for extra stability (prevents shaking on quick tap-and-release)
        # Alpha = 0.5 means 50% new value, 50% old value (balanced: smooth but responsive)
        self.velocity_smoothing_alpha = 0.5

        # Taught positions file - use parameter value
        self.positions_file = Path(self.taught_positions_path)
        self.get_logger().info(f'Taught positions will be saved to: {self.positions_file}')

        # Initialize joystick
        try:
            joystick_device = self.get_parameter('joystick_device').value
            if joystick_device:
                # Use explicitly specified device
                self.joystick = Joystick(device_path=joystick_device)
                self.get_logger().info(f'Joystick initialized (explicit): {self.joystick.path}')
            else:
                # Auto-detect joystick device
                self.joystick = Joystick()
                self.get_logger().info(f'Joystick initialized (auto-detected): {self.joystick.path}')
        except Exception as e:
            self.get_logger().error(f'Failed to initialize joystick: {e}')
            raise

        # Auto-start servo if enabled, but keep it paused initially
        if self.enable_servo:
            self.start_servo_service()
            # Pause servo immediately - will be unpaused when entering teaching mode
            import time
            time.sleep(0.5)  # Wait for servo to start
            if self.servo_pause_client.wait_for_service(timeout_sec=1.0):
                pause_request = Trigger.Request()
                self.servo_pause_client.call_async(pause_request)

        # Create timer for periodic updates
        timer_period = 1.0 / self.update_rate
        self.timer = self.create_timer(timer_period, self.update_callback)

        self.get_logger().info('Joystick teleop node started')
        self.get_logger().info(f'Linear max velocity: {self.linear_max_vel} m/s')
        self.get_logger().info(f'Angular max velocity: {self.angular_max_vel} rad/s')
        self.get_logger().info(f'Deadzone: {self.deadzone}')
        self.get_logger().info(f'Servo control: {"enabled" if self.enable_servo else "disabled"}')

    def ramp_velocity(self, target_vel: float, prev_vel: float, max_accel: float, dt: float) -> float:
        """
        Smoothly ramp velocity towards target using acceleration limit.
        Prevents instant velocity changes that cause torque spikes.

        Args:
            target_vel: Desired velocity
            prev_vel: Previous velocity
            max_accel: Maximum acceleration (units/s²)
            dt: Time step (seconds)

        Returns:
            Ramped velocity
        """
        vel_diff = target_vel - prev_vel
        max_vel_change = max_accel * dt

        if abs(vel_diff) <= max_vel_change:
            # Can reach target in this timestep
            return target_vel
        else:
            # Ramp towards target at max acceleration
            return prev_vel + (max_vel_change if vel_diff > 0 else -max_vel_change)

    def normalize_axis(self, code: str, raw: int) -> float:
        """
        Normalize an analog axis value using Xbox controller standard ranges.

        - For triggers (axes 2 and 5): map to 0..1
        - For sticks (axes 0,1,3,4): map to -1..1

        Xbox controllers typically report:
        - Sticks: -32768 to 32767 (16-bit signed)
        - Triggers: 0 to 1023 (10-bit unsigned)
        """
        raw_u = int(raw) & 0xFFFFFFFF  # Ensure 32-bit unsigned

        # Convert to signed for stick axes
        if raw_u >= 0x80000000:
            raw_s = raw_u - 0x100000000
        else:
            raw_s = raw_u

        # Triggers: map to 0..1
        if code in ("2", "5"):
            # Triggers are typically 0-1023 range
            # Use adaptive range learning for first few samples
            TRIGGER_DEFAULT_MAX = 1023.0

            stats = self.axis_stats.get(code)
            if stats is None:
                self.axis_stats[code] = {"min": 0, "max": TRIGGER_DEFAULT_MAX}
                stats = self.axis_stats[code]

            # Update max if we see larger values
            if raw_u > stats["max"]:
                stats["max"] = raw_u

            max_v = stats["max"]
            if max_v <= 0:
                return 0.0

            val = raw_u / float(max_v)
            return max(0.0, min(1.0, val))

        # Sticks: map to -1..1
        # Xbox sticks typically use -32768 to 32767
        STICK_DEFAULT_RANGE = 32768.0

        stats = self.axis_stats.get(code)
        if stats is None:
            self.axis_stats[code] = {"center": 0, "range": STICK_DEFAULT_RANGE}
            stats = self.axis_stats[code]

        # Update center estimate (exponential moving average)
        # Only update center if value is small (stick near center)
        if abs(raw_s) < 1000:
            stats["center"] = 0.9 * stats["center"] + 0.1 * raw_s

        # Normalize using center and range
        center = stats["center"]
        half_range = stats["range"]

        val = (raw_s - center) / half_range

        # Invert Y axes (1 and 4) so "up" is positive
        if code in ("1", "4"):
            val = -val

        # Clamp
        val = max(-1.0, min(1.0, val))

        # Apply deadzone
        if -self.deadzone < val < self.deadzone:
            val = 0.0

        return val

    def start_servo_service(self):
        """Automatically start the MoveIt Servo node"""
        self.get_logger().info('Attempting to start servo...')

        # Create service client
        start_client = self.create_client(Trigger, '/servo_node/start_servo')

        # Wait for service with timeout
        retry_count = 0
        max_retries = 30  # Wait up to 3 seconds

        while not start_client.wait_for_service(timeout_sec=0.1):
            retry_count += 1
            if retry_count >= max_retries:
                self.get_logger().warn('Servo start service not available - servo may not be running')
                return

        # Call the service
        request = Trigger.Request()
        future = start_client.call_async(request)

        # Wait for response
        rclpy.spin_until_future_complete(self, future, timeout_sec=1.0)

        if future.done():
            response = future.result()
            if response.success:
                self.get_logger().info('Servo started successfully!')
            else:
                self.get_logger().warn(f'Servo start returned: {response.message}')
        else:
            self.get_logger().warn('Failed to call servo start service')

    def send_gripper_command(self, position: float, max_effort: float = 20.0):
        """Send gripper command via GripperCommand action (for TCP controller)"""
        if not self.gripper_client.wait_for_server(timeout_sec=0.1):
            self.get_logger().warn('Gripper action server not available')
            return

        # Create GripperCommand goal
        goal = GripperCommand.Goal()
        goal.command.position = position  # Position in meters (gripper width)
        goal.command.max_effort = max_effort  # Force in Newtons

        self.get_logger().info(f'Sending gripper command: position={position*1000:.1f}mm, force={max_effort}N')
        self.gripper_client.send_goal_async(goal)

    def joint_state_callback(self, msg):
        """Store current joint state for position teaching"""
        self.current_joint_state = msg

    def save_position(self, position_name):
        """Save current robot position to YAML file"""
        if self.current_joint_state is None:
            self.get_logger().error('No joint state available')
            return False

        # Read existing positions or create new dict
        positions = {}
        if self.positions_file.exists():
            with open(self.positions_file, 'r') as f:
                positions = yaml.safe_load(f) or {}

        # Save joint positions
        # Find the left_finger_joint index
        try:
            # Get all joint positions (we'll save the whole state for simplicity)
            joint_dict = {}
            for i, name in enumerate(self.current_joint_state.name):
                if i < len(self.current_joint_state.position):
                    joint_dict[name] = float(self.current_joint_state.position[i])

            positions[position_name] = {
                'joints': joint_dict,
                'gripper_close_width': self.gripper_close_width,  # Save D-pad configured width
                'timestamp': time.time()
            }

            # Write to file
            with open(self.positions_file, 'w') as f:
                yaml.dump(positions, f, default_flow_style=False)

            self.get_logger().info(f'✓ {position_name} saved to {self.positions_file}')
            return True

        except Exception as e:
            self.get_logger().error(f'Failed to save position: {e}')
            return False

    def execute_mtc_task(self):
        """Trigger MTC task execution via service (servo already paused in manual mode)"""
        if not self.positions_file.exists():
            self.get_logger().error('No taught positions found! Press Back to enter teaching mode.')
            return

        if not self.mtc_trigger_client.wait_for_service(timeout_sec=0.1):
            self.get_logger().warn('MTC execution service not available')
            return

        # Execute MTC task (servo is already paused in manual mode)
        request = Trigger.Request()
        self.get_logger().info('Starting MTC pick-and-place with taught positions...')
        future = self.mtc_trigger_client.call_async(request)
        future.add_done_callback(self._on_mtc_complete)

    def _on_mtc_complete(self, future):
        """Callback when MTC task completes"""
        try:
            response = future.result()
            if response.success:
                self.get_logger().info('✓ MTC task completed successfully!')
            else:
                self.get_logger().error(f'✗ MTC task failed: {response.message}')
        except Exception as e:
            self.get_logger().error(f'MTC execution error: {e}')

    def update_callback(self):
        """Main update loop - read joystick and publish commands"""
        try:
            state = self.joystick.get_state()
        except Exception as e:
            self.get_logger().error(f'Failed to read joystick state: {e}')
            return

        axes = state.get('axes', {})
        buttons = state.get('buttons', {})

        # Normalize axes
        left_stick_x = self.normalize_axis("0", axes.get("0", 0))
        left_stick_y = self.normalize_axis("1", axes.get("1", 0))
        left_trigger = self.normalize_axis("2", axes.get("2", 0))
        right_stick_x = self.normalize_axis("3", axes.get("3", 0))
        right_stick_y = self.normalize_axis("4", axes.get("4", 0))
        right_trigger = self.normalize_axis("5", axes.get("5", 0))

        # Get D-PAD state (axes 16 and 17)
        dpad_x_raw = axes.get("16", 0)  # Left (-1), Center (0), Right (+1)
        dpad_y_raw = axes.get("17", 0)  # Up (-1), Center (0), Down (+1)

        # DEBUG: Print D-PAD values when they change
        if dpad_x_raw != 0 or dpad_y_raw != 0:
            self.get_logger().info(f'DEBUG D-PAD: axis_16={dpad_x_raw}, axis_17={dpad_y_raw}')

        # Normalize D-PAD values to -1, 0, 1
        # D-PAD sends: 1 for right/down, 4294967295 (-1 unsigned) for left/up, 0 for center
        dpad_x = 0
        if dpad_x_raw == 4294967295:  # Left
            dpad_x = -1
        elif dpad_x_raw == 1:  # Right
            dpad_x = 1

        dpad_y = 0
        if dpad_y_raw == 4294967295:  # Up
            dpad_y = -1
        elif dpad_y_raw == 1:  # Down
            dpad_y = 1

        # D-PAD Left/Right: Speed control (on rising edge)
        if dpad_x == -1 and self.prev_dpad_x != -1:
            # D-PAD Left: Decrease speed by 3%
            self.speed_scaling = max(0.1, self.speed_scaling - 0.03)
            self.get_logger().info(f'Speed: {self.speed_scaling:.0%}')
        elif dpad_x == 1 and self.prev_dpad_x != 1:
            # D-PAD Right: Increase speed by 3%
            self.speed_scaling = min(2.0, self.speed_scaling + 0.03)
            self.get_logger().info(f'Speed: {self.speed_scaling:.0%}')

        # D-PAD Up/Down: Gripper close width adjustment (on rising edge)
        if dpad_y == -1 and self.prev_dpad_y != -1:
            # D-PAD Up: Increase gripper close width by 1mm (wider grasp)
            self.gripper_close_width = min(0.11, self.gripper_close_width + 0.001)
            self.get_logger().info(f'Gripper close width: {self.gripper_close_width*1000:.0f}mm')
        elif dpad_y == 1 and self.prev_dpad_y != 1:
            # D-PAD Down: Decrease gripper close width by 1mm (tighter grasp)
            self.gripper_close_width = max(0.0, self.gripper_close_width - 0.001)
            self.get_logger().info(f'Gripper close width: {self.gripper_close_width*1000:.0f}mm')

        # Update D-PAD previous state
        self.prev_dpad_x = dpad_x
        self.prev_dpad_y = dpad_y

        # Get button states
        button_a = int(buttons.get("304", 0))
        button_b = int(buttons.get("305", 0))
        button_x = int(buttons.get("307", 0))
        button_y = int(buttons.get("308", 0))
        button_lb = int(buttons.get("310", 0))  # Left bumper
        button_rb = int(buttons.get("311", 0))  # Right bumper
        button_back = int(buttons.get("314", 0))
        button_start = int(buttons.get("315", 0))
        button_share = int(buttons.get("167", 0))  # Share button

        # DEBUG: Print all button presses
        for code, value in buttons.items():
            if value == 1:  # Button pressed
                self.get_logger().info(f'DEBUG BUTTON: code={code} pressed')

        # Handle Back button: Toggle teaching mode (on rising edge)
        if button_back == 1 and self.prev_button_back == 0:
            self.teaching_mode = not self.teaching_mode
            if self.teaching_mode:
                # Entering teaching mode - unpause servo
                self.get_logger().info('=== TEACHING MODE ENABLED ===')
                self.get_logger().info('  Use joystick to position robot')
                self.get_logger().info('  Press Share to save home position')
                self.get_logger().info('  Press X to save pick position')
                self.get_logger().info('  Press Y to save place position')
                self.get_logger().info('  Press Back again to exit teaching mode')
                # Unpause servo for joystick control
                if self.servo_unpause_client.wait_for_service(timeout_sec=0.5):
                    unpause_request = Trigger.Request()
                    unpause_future = self.servo_unpause_client.call_async(unpause_request)
                    unpause_future.add_done_callback(lambda f: self.get_logger().info('✓ Joystick control active'))
                    self.servo_paused = False
            else:
                # Exiting teaching mode - pause servo
                self.get_logger().info('=== MANUAL MODE ===')
                self.get_logger().info('  Press Start to execute MTC task')
                # Reset velocity ramping state
                self.prev_linear_vel = [0.0, 0.0, 0.0]
                self.prev_angular_vel = [0.0, 0.0, 0.0]
                # Pause servo - joystick disabled in manual mode
                if self.servo_pause_client.wait_for_service(timeout_sec=0.5):
                    pause_request = Trigger.Request()
                    pause_future = self.servo_pause_client.call_async(pause_request)
                    pause_future.add_done_callback(lambda f: self.get_logger().info('✓ Joystick disabled - Start button ready'))
                    self.servo_paused = True

        # Handle Share/X/Y buttons: Save positions (only in teaching mode)
        if self.teaching_mode:
            if button_share == 1 and self.prev_button_share == 0:
                self.save_position('home_position')
            elif button_x == 1 and self.prev_button_x == 0:
                self.save_position('pick_position')
            elif button_y == 1 and self.prev_button_y == 0:
                self.save_position('place_position')

        # Handle Start button: Execute MTC task (not in teaching mode)
        if not self.teaching_mode:
            if button_start == 1 and self.prev_button_start == 0:
                self.execute_mtc_task()

        # Handle gripper buttons (on rising edge) - use dynamic widths
        if button_a == 1 and self.prev_button_a == 0:
            self.send_gripper_command(self.gripper_close_width)  # Close gripper
        elif button_b == 1 and self.prev_button_b == 0:
            self.send_gripper_command(self.gripper_open_width)  # Open gripper (dynamic width)

        # Update button previous states
        self.prev_button_a = button_a
        self.prev_button_b = button_b
        self.prev_button_x = button_x
        self.prev_button_y = button_y
        self.prev_button_share = button_share
        self.prev_button_back = button_back
        self.prev_button_start = button_start

        # Compute and publish twist command (only in teaching mode)
        # IMPORTANT: Publish continuously at fixed rate for KORD timing consistency
        if self.teaching_mode and self.enable_servo:
            # Calculate time step for velocity ramping
            dt = 1.0 / self.update_rate  # Time between updates

            # Calculate target velocities from joystick input
            target_linear_x = left_stick_x * self.linear_max_vel * self.speed_scaling
            target_linear_y = left_stick_y * self.linear_max_vel * self.speed_scaling
            target_linear_z = (left_trigger - right_trigger) * self.linear_max_vel * self.speed_scaling

            target_angular_x = right_stick_x * self.angular_max_vel * self.speed_scaling
            target_angular_y = right_stick_y * self.angular_max_vel * self.speed_scaling

            # Z-axis rotation (gripper wrist rotation) - LB/RB bumpers
            target_angular_z = 0.0
            if button_lb == 1:
                target_angular_z = -self.angular_max_vel * self.speed_scaling  # CCW rotation
            elif button_rb == 1:
                target_angular_z = self.angular_max_vel * self.speed_scaling   # CW rotation

            # Apply velocity ramping to prevent torque spikes from instant changes
            ramped_linear_x = self.ramp_velocity(target_linear_x, self.prev_linear_vel[0],
                                                  self.max_linear_accel, dt)
            ramped_linear_y = self.ramp_velocity(target_linear_y, self.prev_linear_vel[1],
                                                  self.max_linear_accel, dt)
            ramped_linear_z = self.ramp_velocity(target_linear_z, self.prev_linear_vel[2],
                                                  self.max_linear_accel, dt)

            ramped_angular_x = self.ramp_velocity(target_angular_x, self.prev_angular_vel[0],
                                                   self.max_angular_accel, dt)
            ramped_angular_y = self.ramp_velocity(target_angular_y, self.prev_angular_vel[1],
                                                   self.max_angular_accel, dt)
            ramped_angular_z = self.ramp_velocity(target_angular_z, self.prev_angular_vel[2],
                                                   self.max_angular_accel, dt)

            # Apply exponential smoothing to prevent shaking from rapid tap-and-release
            # This adds extra inertia to velocity changes, critical for high-inertia Joint 1
            alpha = self.velocity_smoothing_alpha
            smoothed_linear_x = alpha * ramped_linear_x + (1 - alpha) * self.prev_linear_vel[0]
            smoothed_linear_y = alpha * ramped_linear_y + (1 - alpha) * self.prev_linear_vel[1]
            smoothed_linear_z = alpha * ramped_linear_z + (1 - alpha) * self.prev_linear_vel[2]

            smoothed_angular_x = alpha * ramped_angular_x + (1 - alpha) * self.prev_angular_vel[0]
            smoothed_angular_y = alpha * ramped_angular_y + (1 - alpha) * self.prev_angular_vel[1]
            smoothed_angular_z = alpha * ramped_angular_z + (1 - alpha) * self.prev_angular_vel[2]

            # Update previous velocities for next iteration
            self.prev_linear_vel = [smoothed_linear_x, smoothed_linear_y, smoothed_linear_z]
            self.prev_angular_vel = [smoothed_angular_x, smoothed_angular_y, smoothed_angular_z]

            # Create and publish twist message with smoothed velocities
            twist_msg = TwistStamped()
            twist_msg.header.stamp = self.get_clock().now().to_msg()
            twist_msg.header.frame_id = 'base'  # Base frame for intuitive control

            twist_msg.twist.linear.x = smoothed_linear_x
            twist_msg.twist.linear.y = smoothed_linear_y
            twist_msg.twist.linear.z = smoothed_linear_z
            twist_msg.twist.angular.x = smoothed_angular_x
            twist_msg.twist.angular.y = smoothed_angular_y
            twist_msg.twist.angular.z = smoothed_angular_z

            # Publish twist command continuously for consistent KORD timing
            self.twist_pub.publish(twist_msg)


def main(args=None):
    rclpy.init(args=args)

    try:
        node = JoystickTeleopNode()
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    except Exception as e:
        print(f'Error: {e}')
    finally:
        rclpy.shutdown()


if __name__ == '__main__':
    main()

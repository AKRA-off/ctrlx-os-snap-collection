#!/usr/bin/env python3
"""
OnRobot TCP Gripper Controller Node
"""

import socket
import json
import rclpy
from rclpy.action import ActionServer
from rclpy.node import Node
from control_msgs.action import GripperCommand
from std_srvs.srv import Trigger


class OnRobotTCPController(Node):
    """ROS2 node for controlling OnRobot gripper via TCP socket"""

    def __init__(self):
        super().__init__('onrobot_tcp_controller')

        # Declare parameters
        self.declare_parameter('tcp_host', 'localhost')
        self.declare_parameter('tcp_port', 5000)
        self.declare_parameter('default_force', 20.0)
        self.declare_parameter('socket_timeout', 5.0)
        self.declare_parameter('max_width', 110.0)  # RG2 max width in mm
        self.declare_parameter('min_width', 0.0)     # RG2 min width in mm

        # Get parameters
        self.tcp_host = self.get_parameter('tcp_host').value
        self.tcp_port = self.get_parameter('tcp_port').value
        self.default_force = self.get_parameter('default_force').value
        self.socket_timeout = self.get_parameter('socket_timeout').value
        self.max_width = self.get_parameter('max_width').value
        self.min_width = self.get_parameter('min_width').value

        # Create action server for gripper commands

        self._action_server = ActionServer(
            self,
            GripperCommand,
            'rg2_gripper_controller/gripper_cmd',
            self.execute_gripper_command
        )

        # Create service servers for gripper queries
        self._width_service = self.create_service(
            Trigger,
            'get_gripper_width',
            self.get_width_callback
        )

        self._object_detected_service = self.create_service(
            Trigger,
            'is_object_detected',
            self.is_object_detected_callback
        )

        self.get_logger().info(
            f'OnRobot TCP Controller started - connecting to {self.tcp_host}:{self.tcp_port}'
        )

    def send_tcp_command(self, command_dict):
        """
        Send a JSON command to the TCP server and receive response

        Args:
            command_dict: Dictionary containing the command to send

        Returns:
            Response dictionary from the server, or None if error
        """
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.settimeout(self.socket_timeout)

                # Connect to TCP server
                s.connect((self.tcp_host, self.tcp_port))

                # Send command
                command_json = json.dumps(command_dict) + "\n"
                s.sendall(command_json.encode())

                # Receive response
                response = s.recv(4096).decode().strip()

                # Parse response
                response_dict = json.loads(response)

                if response_dict.get('status') != 'ok':
                    self.get_logger().error(
                        f"Command failed: {response_dict.get('message', 'Unknown error')}"
                    )
                    return None

                return response_dict

        except socket.timeout:
            self.get_logger().error("TCP connection timed out")
            return None
        except ConnectionRefusedError:
            self.get_logger().error(
                f"Connection refused. Is the TCP server running on {self.tcp_host}:{self.tcp_port}?"
            )
            return None
        except json.JSONDecodeError as e:
            self.get_logger().error(f"Failed to parse JSON response: {e}")
            return None
        except Exception as e:
            self.get_logger().error(f"TCP communication error: {e}")
            return None

    def execute_gripper_command(self, goal_handle):
        """
        Execute a gripper command action

        """
        self.get_logger().info('Executing gripper command...')

        # Extract goal parameters
        position = goal_handle.request.command.position  
        max_effort = goal_handle.request.command.max_effort 

        # Convert meters to millimeters
        width_mm = position * 1000.0  # Convert meters to millimeters

        # Clamp width
        width_mm = max(self.min_width, min(width_mm, self.max_width))

        # Use default force if max_effort is 0 or not specified
        force = max_effort if max_effort > 0 else self.default_force

        self.get_logger().info(
            f"Gripper command: width={width_mm:.1f}mm, force={force:.1f}N"
        )

        # Send grip command to TCP server
        command = {
            "command": "grip",
            "width": width_mm,
            "force": force,
            "wait": True
        }

        response = self.send_tcp_command(command)

        if response is None:
            goal_handle.abort()
            result = GripperCommand.Result()
            result.position = 0.0
            result.effort = 0.0
            result.stalled = False
            result.reached_goal = False
            return result

        # Get actual gripper width after command
        width_response = self.send_tcp_command({"command": "get_width"})
        actual_width = 0.0
        if width_response and 'width' in width_response:
            actual_width = width_response['width'] / 1000.0  # Convert mm to meters

        # Check if object was detected
        object_detected_response = self.send_tcp_command({"command": "is_object_detected"})
        stalled = False
        if object_detected_response:
            stalled = object_detected_response.get('detected', False)

        # Mark as succeeded
        goal_handle.succeed()

        # Create result
        result = GripperCommand.Result()
        result.position = actual_width
        result.effort = force
        result.stalled = stalled
        result.reached_goal = not stalled  # If stalled, we didn't reach target position

        self.get_logger().info(
            f"Gripper command completed: position={actual_width:.4f}m, stalled={stalled}"
        )

        return result

    def get_width_callback(self, request, response):
        """Service callback to get current gripper width"""
        width_response = self.send_tcp_command({"command": "get_width"})

        if width_response and 'width' in width_response:
            width_mm = width_response['width']
            response.success = True
            response.message = f"Current width: {width_mm:.1f}mm ({width_mm/1000.0:.4f}m)"
        else:
            response.success = False
            response.message = "Failed to get gripper width"

        return response

    def is_object_detected_callback(self, request, response):
        """Service callback to check if object is detected"""
        detected_response = self.send_tcp_command({"command": "is_object_detected"})

        if detected_response is not None:
            detected = detected_response.get('detected', False)
            response.success = True
            response.message = f"Object detected: {detected}"
        else:
            response.success = False
            response.message = "Failed to check object detection"

        return response


def main(args=None):
    rclpy.init(args=args)

    controller = OnRobotTCPController()

    try:
        rclpy.spin(controller)
    except KeyboardInterrupt:
        pass
    finally:
        controller.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()

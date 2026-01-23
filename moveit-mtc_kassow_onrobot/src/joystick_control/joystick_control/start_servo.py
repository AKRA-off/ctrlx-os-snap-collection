import rclpy
from rclpy.node import Node
from std_srvs.srv import Trigger
import time


class ServoStarter(Node):
    def __init__(self):
        super().__init__('servo_starter')

        # Create service
        self.start_client = self.create_client(Trigger, '/servo_node/start_servo')

        # Wait for servo service
        self.get_logger().info('Waiting for servo start service...')
        retry_count = 0
        max_retries = 50  # Wait up to 5 seconds (50 * 0.1s)

        while not self.start_client.wait_for_service(timeout_sec=0.1):
            retry_count += 1
            if retry_count >= max_retries:
                self.get_logger().error('Servo start service not available after waiting')
                return

        self.get_logger().info('Servo start service available, starting servo...')

        # Call start servo
        future = self.start_client.call_async(Trigger.Request())
        rclpy.spin_until_future_complete(self, future, timeout_sec=2.0)

        if future.done():
            response = future.result()
            if response.success:
                self.get_logger().info('Servo started successfully!')
            else:
                self.get_logger().warn(f'Servo start returned: {response.message}')
        else:
            self.get_logger().error('Failed to call servo start service')


def main(args=None):
    rclpy.init(args=args)

    try:
        node = ServoStarter()
        # Delay
        time.sleep(0.5)
    except Exception as e:
        print(f'Error: {e}')
    finally:
        rclpy.shutdown()


if __name__ == '__main__':
    main()
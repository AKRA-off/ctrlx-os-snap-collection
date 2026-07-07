from setuptools import setup
import os
from glob import glob

package_name = 'joystick_control'

setup(
    name=package_name,
    version='1.0.0',
    packages=[package_name],
    data_files=[
        ('share/ament_index/resource_index/packages',
            ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
        (os.path.join('share', package_name, 'config'), glob('config/*.yaml')),
        (os.path.join('share', package_name, 'launch'), glob('launch/*.py')),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='Akra',
    maintainer_email='your.email@example.com',
    description='Joystick control for Kassow robot with MoveIt Servo',
    license='MIT',
    tests_require=['pytest'],
    entry_points={
        'console_scripts': [
            'joystick_teleop = joystick_control.joystick_teleop_node:main',
            'start_servo = joystick_control.start_servo:main',
        ],
    },
)

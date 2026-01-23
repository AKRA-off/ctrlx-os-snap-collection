# SPDX-FileCopyrightText: Bosch Rexroth AG
#
# SPDX-License-Identifier: MIT
from setuptools import setup

setup(name = 'modbus-rtu-master',
      version='1.0.0',
      description = 'Test Modbus RTU communication',
      author = 'Akra',
      install_requires = ['ctrlx-datalayer', 'ctrlx-fbs'],
      packages = ['helper'],
      scripts = ['main.py'],
      license = 'MIT License'
)

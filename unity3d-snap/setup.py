# SPDX-FileCopyrightText: Bosch Rexroth AG
#
# SPDX-License-Identifier: MIT
from setuptools import setup

setup(name = 'unity-snap',
      version='0.4.3',
      description = 'Unity WebGL app served on port 8080',
      author = 'Akra',
      #install_requires=['websockets'], 
      scripts = ['server.py'],
      license = 'MIT License'
)
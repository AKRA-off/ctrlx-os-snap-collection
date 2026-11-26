# SPDX-FileCopyrightText: Bosch Rexroth AG
#
# SPDX-License-Identifier: MIT
from setuptools import setup

setup(
    name="ctrlx-contour",
    version="1.0.0",
    description="Pick And Place",
    author="Akra",
    install_requires=["flask"],
    packages=["MVImport", "dependencies", "helper"],
    scripts=["main.py"],
    license="MIT License",
)

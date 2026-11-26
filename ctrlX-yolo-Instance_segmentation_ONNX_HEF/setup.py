# SPDX-FileCopyrightText: Bosch Rexroth AG
#
# SPDX-License-Identifier: MIT
from setuptools import setup, Extension
import numpy as np

try:
    from Cython.Build import cythonize
    USE_CYTHON = True
except ImportError:
    USE_CYTHON = False

# Define the Cython extension
extensions = []
if USE_CYTHON:
    extensions = cythonize([
        Extension(
            "post_process.cython_nms",
            ["post_process/cython_nms.pyx"],
            include_dirs=[np.get_include()],
            extra_compile_args=["-O3"],
        )
    ])
else:
    # Fallback to pre-compiled .c file if available
    extensions = [
        Extension(
            "post_process.cython_nms",
            ["post_process/cython_nms.c"],
            include_dirs=[np.get_include()],
            extra_compile_args=["-O3"],
        )
    ]

setup(
    name="ai-detector",
    version="1.0.0",
    description="AI Detector",
    author="Akra",
    packages=["MVImport", "dependencies", "helper", "yoloseg", "common", "common.tracker", "post_process"],
    scripts=["main.py", "instance_segmentation.py"],
    ext_modules=extensions,
    license="MIT License",
    setup_requires=["numpy", "cython"],
)

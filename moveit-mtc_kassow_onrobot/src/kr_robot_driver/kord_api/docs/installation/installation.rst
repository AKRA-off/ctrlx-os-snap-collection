.. _installation-description:

==================
Installation
==================

Running KORD requires two components:

1. **KORD CBun** on the robot controller.
2. **KORD API** on the client.

.. note::
    **CBun is available for customers only.**

.. note::
    Real-time operation is achievable with RT-capable kernels. Using an RT kernel allows you to run streamed movement examples as real-time processes, reducing non-deterministic behavior on the client side.

KORD CBun
=========

The **KORD CBun** must be installed, the network configured correctly, and the CBun activated. It is recommended that the robot cabinet is directly connected to the client without any intermediate network devices. If possible, avoid using USB to Ethernet adapters to ensure optimal performance.

**Installation Steps:**

1. **Check Availability:** Access the robot CBun Manager to verify if the latest KORD CBun version is available on the robot.

2. **Download CBun:** If the latest version is not present, download the `KORD CBun <https://gitlab.com/kassowrobots/kord-api/-/wikis/Master-CBun>`_ from the project's wiki page.

3. **Install CBun:** Use the CBun Manager to install the downloaded CBun onto the robot controller.

KORD API
========

The **KORD API** is essential for client-side operations.
It is possible to compile the KORD API from the source code, or use pre-built binaries (available only for Debian/Ubuntu).

Obtaining the Source
--------------------

.. note::
    Compilation has been tested on Ubuntu versions **Bionic**, **Focal**, and **Jammy**. Ensure your build environment is properly set up before proceeding.

**Steps to Clone the Repository:**

1. **Clone the repository:**

   .. code-block:: shell

       git clone https://gitlab.com/kassowrobots/kord-api.git

2. **Verify Cloning:**
   Ensure that the `kord-api` directory has been created after running the clone command.

Compilation
-----------

**Steps to Compile KORD API:**

1. **Navigate to Directory:**

   .. code-block:: bash

       cd kord-api

2. **Switch to Master Branch:**

   .. code-block:: bash

       git checkout master

3. **Configure the Build:**
    Available options can be found in `README.md` in the project root directory.

   .. code-block:: bash

       cmake -S . -B build -DKORD_WITH_EXAMPLES=ON

4. **Build the Project:**

   .. code-block:: bash

       cmake --build build

   The compiled output, including examples, will be located in the `build` directory.

Packaging
---------

You can create a Debian package for easier installation.

**Steps to Create a Debian Package:**

1. **Enter Build Directory:**

   .. code-block:: bash

       cd build

2. **Run Packaging Tool:**

   .. code-block:: bash

       make package -DKORD_WITH_EXAMPLES=ON

   The Debian packages will be created in the `build` directory.

Installation
------------

**Option 1: Install from Build**

To install the library system-wide:

.. code-block:: bash

    sudo make install

**Option 2: Install from Debian Packages**

If you built Debian packages, install them using `dpkg`:

.. code-block:: bash

    sudo dpkg -i kord-api-lib_$VERSION_amd64.deb
    sudo dpkg -i kord-api-lib-dev_$VERSION_amd64.deb
    sudo dpkg -i kord-api-examples_$VERSION_amd64.deb

Usage
-----

After installation, you can integrate KORD API into your projects using CMake's `find_package`.

**Example CMake Configuration:**

.. code-block:: cmake

    find_package(kord_api REQUIRED)

    add_executable(<your_target> <your_source_files>)

    target_link_libraries(<your_target> PRIVATE kord_api::kord_api)
    target_include_directories(<your_target> PRIVATE ${kord_api_INCLUDE_DIRS})

.. warning::
    KORD API will not function without the CBun installed on the robot controller.


***
FAQ
***

* **Is KORD released yet?**

Yes. There is already stable KORD v1.4 available and we are continuously working on improving and extending its functions.

* **Can I control the robot by directly talking to joints?**

Yes. It is possible, but be aware of that the feature is still experimental. Take a look `Direct Control Movement` example.

* **What is the difference between the** ``syncRC`` **and** ``waitSync`` **functions?**

The difference is that the ``syncRC`` function should only be used to initialize the communication. From that point on, only the ``waitSync`` function should be used. The ``syncRC`` function will **transmit** a request that will effectively initiate the Robot Status Heartbeat (RSHB) message dissipation. From that moment, it is preferable to use the ``waitSync`` function since it will only **wait** for the arriving RSHB.

To summarize, see our examples. You will notice that all of them start with the ``syncRC`` function and then only use ``waitSync``. This is the expected order of usage.

* **Why are there two examples for the movement commands joint and linear?**

It is to demonstrate two modes of operation. The ``kord_move_joints`` example demonstrates the streamed (Real-Time) commands, where it is expected to send a command every 4ms.

The ``kord_move_joints_discrete`` example demonstrates the discrete (timed) command, that can potentially span several seconds depending on parameters. During the movement the RSHB is not being dissipated.

The same applies to the linear movement examples.

* **Is there a Python version of the KORD API?**

No, there are no plans for a Python version of the KORD API at current stage. The API is written in C++ and is intended to be used as a library in C++ projects.
It is possible that as the project matures, Python bindings will be added.

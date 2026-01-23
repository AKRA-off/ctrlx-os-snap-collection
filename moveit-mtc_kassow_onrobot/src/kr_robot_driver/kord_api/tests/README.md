# KORD API Test

# Master CBun
For the KORD TEST/API to work and operate on external system, the master module has to be installed and activated in the RC. The module provides the connectivity and access to RC software. 

## Path to KORD library

The library is taken from the `kord-api/build` folder.

# Building the project

```
mkdir build && cd build && cmake .. && make
```

or it builds automatically as a submodule of `kord-api`.
# Running tests

There are two executable files generated: `passive_test` and `active_test`.

`active_test` performs tests with moves and requests to the robot that change the robot's state.

To successfully pass the `Transfering` tests (e.g. retrieving of Logs), `KORD.ini` with the default parameters must be installed to the robot.
The reference file can be found in the main `kord-test` directory (`IP` and `username` should be changed to the correct, user defined ones).

<em><strong> Be sure that joints configuration (0, 60, 0, 60, 0, 60, 0) is accessible before runnning the active tests</strong></em>.

`passive_test` only reads different robot's state parameters without modifying them.

## Script parameters

``./passive_test -p <PORT> -c <IP>`` where

``<PORT> default 7582``

``<IP>`` should be discovered by connection to the cabinet.

It is always possible to stop the scipt execution with `Cntrl-C` interupt.


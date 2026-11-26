# Modded Node-RED Snap package to be  more industrial

[![platform](https://img.shields.io/badge/platform-Node--RED-red)](https://nodered.org)
[![GitHub version](https://badge.fury.io/gh/node-red%2Fnode-red.svg)](https://badge.fury.io/gh/node-red%2Fnode-red)

The Node-RED graphical wiring tool for Low-code programming of event-driven applications.
Packaged as a Core18 based Ubuntu Snap, intended for multiple architectures.

Listens on port 1891 and runs as as service in strict mode by default.
Fully configurable on port 1882: edit **settings.js** directly from a webpage!

### Modifications

Compared with the upstream snaps, this version adds:

 - Bluetooth support (BlueZ plus a BlueALSA service for Bluetooth audio devices)
 - Audio playback and audio recording (ALSA and PulseAudio client tools, `audio-playback` and `audio-record` plugs)
 - Industrial nodes and the ctrlX integration listed below



The snap by default comes with the following packages installed

 - node-red dashboard
 - CtrlX-Automation-Contrib
 - OPCUA contrib
 - MongoDB3
 - Influxdb 
 





#### Building
   
    sudo snap install multipass
    sudo snap install snapcraft --classic
    snapcraft

## Origin and license

Modified version of [mauringo/node-red-industrial-snap](https://github.com/mauringo/node-red-industrial-snap) by Mauro Riboni (copyright 2020), itself a fork of [dceejay/nodered.snap](https://github.com/dceejay/nodered.snap) (Apache-2.0, copyright Dave Conway-Jones and Luca Capra), packaging [Node-RED](https://github.com/node-red/node-red) (Apache-2.0). Changes in this version: Bluetooth support and audio recording/playback (see Modifications above). Not affiliated with or endorsed by these authors.

This folder is under the original non-commercial "General Conditions of Free Software License" (see `LICENSE` and `NOTICE`); commercial use is not permitted.


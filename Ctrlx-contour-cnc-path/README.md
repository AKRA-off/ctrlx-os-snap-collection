# Ctrlx contour CNC path

## Introduction

The sample demonstrates how to connect Hikrobot camera to CtrlX Core, take a picture and form an array of points based on detected contour. The array is stored in the Datalayer of the Core and process by PLC program to form a dynamic CNC path and execute Motion. 

## Function Description

Program has a webpage to connect camera, take an image or use preloaded image and process it. PLC program attached in PLC Folder.

## Implementation Description 

The module __main.py__ contains Flask webserver, interface to connect camera and take a picture, Datalayer Provider and Client functions
__snap__ including snapcraft.yaml and required hooks
__dependencies__ with Flask files
__configs__ with script for folder creation and integration in the CtrlX side bar
__MVImport__ adding Hikrobot header files
__helper__ dependencies for Datalayer
__PLC__ program for running with PLC app
__setup.py__


## Build

Beware, the source code is designed for CtrlX Core X3 with arm64 architecture, for the devices with amd64 architecture snapcraft.yaml, MVImport and libraries of Hikrobot must be changed.
Download the source code and build the snap executing .sh script depending on your architecture or run command directly:

```shell
snapcraft --destructive-mode
```



___

## License

SPDX-FileCopyrightText: Bosch Rexroth AG
SPDX-License-Identifier: MIT

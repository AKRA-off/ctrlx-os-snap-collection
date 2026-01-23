# Ctrlx YOLOv8 Instance Segmentation with ONNX and HEF models

## Introduction

The sample demonstrates how to connect Hikrobot and usb web cameras to CtrlX Core, take a picture and execute inference based on preloaded model. The results of output boxes, scores and classes are stored in the Datalayer of the Core. 

## Function Description

Program has a webpage to choose between preloaded image, USB and Hik cameras, connect them, take an image and run inference. PLC program attached in PLC Folder.

## Implementation Description 

The module __main.py__ contains Flask webserver, interface to connect cameras and run inference, Datalayer Provider and Client functions
__snap__ including snapcraft.yaml and required hooks
__dependencies__ with Flask files, models and configurations
__configs__ with script for folder creation and integration in the CtrlX side bar
__MVImport__ adding Hikrobot header files
__helper__ dependencies for Datalayer
__yoloseg__ for using ONNX models
__customPackages__, __common__ and __post_process__ for HEF inference
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

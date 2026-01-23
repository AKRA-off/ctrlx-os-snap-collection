# Modbus RTU Master snap

## Introduction

The sample demonstrates how to connect to Modbus RTU slave through USB-to-RS485 adapter connected to CtrlX Core and control it via Datalayer to read register data. 

## Function Description

All communication with the snap is done using CtrlX Datalayer, you can configure baudrate, parity, port, count, slave id, timeout, trigger reading function and display register data.

## Implementation Description 

The module __main.py__ contains Modbus RTU Master, Datalayer Provider and Client functions
__snap__ including snapcraft.yaml
__helper__ dependencies for Datalayer
__setup.py__



## Build

Can be built for arm64 and amd64 devices without limitations
Download the source code and build the snap executing .sh script depending on your architecture or run command directly:

```shell
snapcraft --destructive-mode
```



___

## License

SPDX-FileCopyrightText: Bosch Rexroth AG
SPDX-License-Identifier: MIT

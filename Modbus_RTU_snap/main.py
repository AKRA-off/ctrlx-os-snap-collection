#!/usr/bin/env python3

# SPDX-FileCopyrightText: Bosch Rexroth AG
#
# SPDX-License-Identifier: MIT

import signal
import sys
import threading
from time import sleep
import logging

import pymodbus
from pymodbus.client import ModbusSerialClient as ModbusClient
from pymodbus.framer import FramerType

from helper.ctrlx_datalayer_helper import get_client, get_provider

import ctrlxdatalayer
import flatbuffers

from comm.datalayer import DisplayFormat, Metadata, NodeClass
from ctrlxdatalayer.provider import Provider
from ctrlxdatalayer.provider_node import (
    ProviderNode,
    ProviderNodeCallbacks,
    NodeCallback,
)
from ctrlxdatalayer.variant import Result, Variant, VariantType
from ctrlxdatalayer.metadata_utils import (
    MetadataBuilder,
    AllowedOperation,
    ReferenceType,
)

logger = logging.getLogger(__name__)
logging.basicConfig(level=logging.INFO, format='%(asctime)s %(message)s')

__close_app = False

# Modbus connection settings (will be read from datalayer)
modbus_port = "/dev/ttyUSB0"
modbus_baudrate = 9600
modbus_parity = "N"
modbus_timeout = 0.1
modbus_start_address = 248
modbus_count = 4
modbus_slave_id = 1
modbus_read_enable = False

# Modbus data storage
modbus_registers = []

# Base address for datalayer nodes
address_base = "modbus_rtu/settings/"


class MyProviderNode:
    """MyProviderNode"""

    def __init__(self, provider: Provider, address: str, initialValue: Variant):
        """__init__"""
        self._cbs = ProviderNodeCallbacks(
            self.__on_create,
            self.__on_remove,
            self.__on_browse,
            self.__on_read,
            self.__on_write,
            self.__on_metadata,
        )

        self._providerNode = ProviderNode(self._cbs)

        self._provider = provider
        self._address = address
        self._data = initialValue
        self._metadata = MyProviderNode.__get_metadata(address)

    @staticmethod
    def __get_metadata(address: str) -> Variant:
        builder = MetadataBuilder(
            allowed=AllowedOperation.READ | AllowedOperation.WRITE
        )
        builder = builder.set_display_name(address)
        builder = builder.set_node_class(NodeClass.NodeClass.Variable)
        if address.rfind("string") != -1:
            builder.add_reference(ReferenceType.read(), "types/datalayer/string")
            builder.add_reference(ReferenceType.write(), "types/datalayer/string")
        return builder.build()

    def register_node(self):
        """register_node"""
        return self._provider.register_node(self._address, self._providerNode)

    def unregister_node(self):
        """unregister_node"""
        self._provider.unregister_node(self._address)
        self._metadata.close()
        self._data.close()

    def set_value(self, value: Variant):
        """set_value"""
        self._data = value

    def __on_create(
        self,
        userdata: ctrlxdatalayer.clib.userData_c_void_p,
        address: str,
        data: Variant,
        cb: NodeCallback,
    ):
        """__on_create"""
        print("__on_create()", "address:", address, "userdata:", userdata, flush=True)
        cb(Result.OK, data)

    def __on_remove(
        self,
        userdata: ctrlxdatalayer.clib.userData_c_void_p,
        address: str,
        cb: NodeCallback,
    ):
        """__on_remove"""
        print("__on_remove()", "address:", address, "userdata:", userdata, flush=True)
        cb(Result.UNSUPPORTED, None)

    def __on_browse(
        self,
        userdata: ctrlxdatalayer.clib.userData_c_void_p,
        address: str,
        cb: NodeCallback,
    ):
        """__on_browse"""
        print("__on_browse()", "address:", address, "userdata:", userdata, flush=True)
        with Variant() as new_data:
            new_data.set_array_string([])
            cb(Result.OK, new_data)

    def __on_read(
        self,
        userdata: ctrlxdatalayer.clib.userData_c_void_p,
        address: str,
        data: Variant,
        cb: NodeCallback,
    ):
        """__on_read"""
        new_data = self._data
        cb(Result.OK, new_data)

    def __on_write(
        self,
        userdata: ctrlxdatalayer.clib.userData_c_void_p,
        address: str,
        data: Variant,
        cb: NodeCallback,
    ):
        """__on_write"""
        if self._data.get_type() != data.get_type():
            cb(Result.TYPE_MISMATCH, None)
            return

        result, self._data = data.clone()
        cb(Result.OK, self._data)

    def __on_metadata(
        self,
        userdata: ctrlxdatalayer.clib.userData_c_void_p,
        address: str,
        cb: NodeCallback,
    ):
        """__on_metadata"""
        cb(Result.OK, self._metadata)


def handler(signum, frame):
    """handler"""
    global __close_app
    __close_app = True


def provide_string(provider: ctrlxdatalayer.provider, name: str, initial_value: str):
    """provide_string"""
    print("Creating string provider node " + address_base + name, flush=True)
    variantString = Variant()
    variantString.set_string(initial_value)
    provider_node_str = MyProviderNode(provider, address_base + name, variantString)
    result = provider_node_str.register_node()
    if result != ctrlxdatalayer.variant.Result.OK:
        print(
            "ERROR Registering node " + address_base + name + " failed with:",
            result,
            flush=True,
        )
    return provider_node_str


def provide_int(provider: ctrlxdatalayer.provider, name: str, initial_value: int):
    """provide_int"""
    print("Creating int provider node " + address_base + name, flush=True)
    variantInt = Variant()
    variantInt.set_int32(initial_value)
    provider_node_int = MyProviderNode(provider, address_base + name, variantInt)
    result = provider_node_int.register_node()
    if result != ctrlxdatalayer.variant.Result.OK:
        print(
            "ERROR Registering node " + address_base + name + " failed with:",
            result,
            flush=True,
        )
    return provider_node_int


def provide_float(provider: ctrlxdatalayer.provider, name: str, initial_value: float):
    """provide_float"""
    print("Creating float provider node " + address_base + name, flush=True)
    variantFloat = Variant()
    variantFloat.set_float32(initial_value)
    provider_node_flt = MyProviderNode(provider, address_base + name, variantFloat)
    result = provider_node_flt.register_node()
    if result != ctrlxdatalayer.variant.Result.OK:
        print(
            "ERROR Registering node " + address_base + name + " failed with:",
            result,
            flush=True,
        )
    return provider_node_flt


def provide_bool(provider: ctrlxdatalayer.provider, name: str, initial_value: bool):
    """provide_bool"""
    print("Creating bool provider node " + address_base + name, flush=True)
    variantBool = Variant()
    variantBool.set_bool8(initial_value)
    provider_node_bl = MyProviderNode(provider, address_base + name, variantBool)
    result = provider_node_bl.register_node()
    if result != ctrlxdatalayer.variant.Result.OK:
        print(
            "ERROR Registering node " + address_base + name + " failed with:",
            result,
            flush=True,
        )
    return provider_node_bl


def provide_int_array(provider: ctrlxdatalayer.provider, name: str):
    """provide_int_array"""
    print("Creating int array provider node " + address_base + name, flush=True)
    variantIntArray = Variant()
    variantIntArray.set_array_int32([])
    provider_node_arr = MyProviderNode(provider, address_base + name, variantIntArray)
    result = provider_node_arr.register_node()
    if result != ctrlxdatalayer.variant.Result.OK:
        print(
            "ERROR Registering node " + address_base + name + " failed with:",
            result,
            flush=True,
        )
    return provider_node_arr


def dl_provider():
    """Datalayer Provider Thread"""
    with ctrlxdatalayer.system.System("") as datalayer_system:
        datalayer_system.start(False)

        provider, connection_string = get_provider(
            datalayer_system, ip="192.168.1.1", user="boschrexroth", password="boschrexroth"
        )
        if provider is None:
            print("ERROR Connecting", connection_string, "failed.", flush=True)
            sys.exit(1)

        with provider:
            result = provider.start()
            if result != Result.OK:
                print(
                    "ERROR Starting ctrlX Data Layer Provider failed with:",
                    result,
                    flush=True,
                )
                return

            # Create provider nodes for Modbus settings
            provide_node_port = provide_string(provider, "port", modbus_port)
            provide_node_baudrate = provide_int(provider, "baudrate", modbus_baudrate)
            provide_node_parity = provide_string(provider, "parity", modbus_parity)
            provide_node_timeout = provide_float(provider, "timeout", modbus_timeout)
            provide_node_start_address = provide_int(provider, "start_address", modbus_start_address)
            provide_node_count = provide_int(provider, "count", modbus_count)
            provide_node_slave_id = provide_int(provider, "slave_id", modbus_slave_id)
            provide_node_read_enable = provide_bool(provider, "read_enable", modbus_read_enable)

            # Output node for holding registers
            provide_node_registers = provide_int_array(provider, "registers")

            print("INFO Running provider loop...", flush=True)
            while provider.is_connected() and not __close_app:
                sleep(1.0)

            print("ERROR ctrlX Data Layer Provider is disconnected", flush=True)

            # Cleanup
            provide_node_port.unregister_node()
            del provide_node_port
            provide_node_baudrate.unregister_node()
            del provide_node_baudrate
            provide_node_parity.unregister_node()
            del provide_node_parity
            provide_node_timeout.unregister_node()
            del provide_node_timeout
            provide_node_start_address.unregister_node()
            del provide_node_start_address
            provide_node_count.unregister_node()
            del provide_node_count
            provide_node_slave_id.unregister_node()
            del provide_node_slave_id
            provide_node_read_enable.unregister_node()
            del provide_node_read_enable
            provide_node_registers.unregister_node()
            del provide_node_registers

            print("Stopping ctrlX Data Layer provider:", end=" ", flush=True)
            result = provider.stop()
            print(result, flush=True)

        stop_ok = datalayer_system.stop(False)
        print("System Stop", stop_ok, flush=True)


def dl_client():
    """Datalayer Client Thread"""
    global modbus_port, modbus_baudrate, modbus_parity, modbus_timeout
    global modbus_start_address, modbus_count, modbus_slave_id
    global modbus_read_enable, modbus_registers

    with ctrlxdatalayer.system.System("") as datalayer_system:
        datalayer_system.start(False)

        datalayer_client, datalayer_client_connection_string = get_client(
            datalayer_system, ip="192.168.1.1", user="boschrexroth", password="boschrexroth"
        )
        if datalayer_client is None:
            print(
                f"ERROR Connecting {datalayer_client_connection_string} failed.",
                flush=True
            )
            sys.exit(1)

        with datalayer_client:
            if datalayer_client.is_connected() is False:
                print(f"ERROR ctrlX Data Layer is NOT connected: {datalayer_client_connection_string}",
                    flush=True
                )
                sys.exit(1)

            while datalayer_client.is_connected() and not __close_app:
                # Read Modbus settings from datalayer
                addr = address_base + "port"
                result, string_var = datalayer_client.read_sync(addr)
                if result == Result.OK:
                    modbus_port = string_var.get_string()

                addr = address_base + "baudrate"
                result, int_var = datalayer_client.read_sync(addr)
                if result == Result.OK:
                    modbus_baudrate = int_var.get_int32()

                addr = address_base + "parity"
                result, string_var = datalayer_client.read_sync(addr)
                if result == Result.OK:
                    modbus_parity = string_var.get_string()

                addr = address_base + "timeout"
                result, float_var = datalayer_client.read_sync(addr)
                if result == Result.OK:
                    modbus_timeout = float_var.get_float32()

                addr = address_base + "start_address"
                result, int_var = datalayer_client.read_sync(addr)
                if result == Result.OK:
                    modbus_start_address = int_var.get_int32()

                addr = address_base + "count"
                result, int_var = datalayer_client.read_sync(addr)
                if result == Result.OK:
                    modbus_count = int_var.get_int32()

                addr = address_base + "slave_id"
                result, int_var = datalayer_client.read_sync(addr)
                if result == Result.OK:
                    modbus_slave_id = int_var.get_int32()

                addr = address_base + "read_enable"
                result, bool_var = datalayer_client.read_sync(addr)
                if result == Result.OK:
                    modbus_read_enable = bool_var.get_bool8()

                # Write registers data back to datalayer
                with Variant() as data:
                    data.set_array_int32(modbus_registers)
                    addr = address_base + "registers"
                    result = datalayer_client.write_sync(addr, data)

                sleep(0.5)

            print("ERROR ctrlX Data Layer Client is NOT connected")

        stop_ok = datalayer_system.stop(False)
        print("System Stop", stop_ok, flush=True)


def modbus_master():
    """Modbus Reading"""
    global modbus_port, modbus_baudrate, modbus_parity, modbus_timeout
    global modbus_start_address, modbus_count, modbus_slave_id
    global modbus_read_enable, modbus_registers

    client = None
    last_config = None

    while not __close_app:
        # New settings
        current_config = (modbus_port, modbus_baudrate, modbus_parity, modbus_timeout)

        if modbus_read_enable:
            # Reconnect if configuration changed
            if current_config != last_config:
                if client is not None:
                    try:
                        client.close()
                    except:
                        pass

                logger.info(f"Connecting to Modbus RTU: port={modbus_port}, baudrate={modbus_baudrate}, parity={modbus_parity}, timeout={modbus_timeout}")

                try:
                    client = ModbusClient(
                        port=modbus_port,
                        baudrate=modbus_baudrate,
                        parity=modbus_parity,
                        timeout=modbus_timeout
                    )
                    connection = client.connect()
                    if connection:
                        logger.info("Modbus RTU connected successfully")
                        last_config = current_config
                    else:
                        logger.error("Failed to connect to Modbus RTU")
                        client = None
                        sleep(2.0)
                        continue
                except Exception as e:
                    logger.error(f"Error connecting to Modbus RTU: {e}")
                    client = None
                    sleep(2.0)
                    continue

            # Read holding registers
            if client is not None:
                try:
                    read_vals = client.read_holding_registers(
                        modbus_start_address,
                        count=modbus_count,
                        device_id=modbus_slave_id
                    )

                    if read_vals is not None and not read_vals.isError():
                        modbus_registers = list(read_vals.registers)
                        logger.info(f"Read registers: {modbus_registers}")
                    else:
                        logger.error(f"Error reading registers: {read_vals}")
                        modbus_registers = []

                except Exception as e:
                    logger.error(f"Exception reading Modbus registers: {e}")
                    modbus_registers = []

            sleep(1.0)  # Delay
        else:
            # Disconnect when read is disabled
            if client is not None:
                try:
                    client.close()
                except:
                    pass
                client = None
                last_config = None
                logger.info("Modbus reading disabled, disconnected")

            sleep(0.5)


if __name__ == "__main__":
    signal.signal(signal.SIGINT, handler)
    signal.signal(signal.SIGTERM, handler)
    signal.signal(signal.SIGABRT, handler)

    try:
        logger.info('Starting Modbus RTU Snap with Datalayer Integration')

        # Start provider thread
        logger.info('Starting Data Layer Provider thread')
        t_provider = threading.Thread(target=dl_provider)
        t_provider.start()

        sleep(3.0)  # Wait for provider to initialize

        # Start client thread
        logger.info('Starting Data Layer Client thread')
        t_client = threading.Thread(target=dl_client)
        t_client.start()

        sleep(1.0)  # Wait for client to initialize

        # Start Modbus master thread
        logger.info('Starting Modbus Master thread')
        t_modbus = threading.Thread(target=modbus_master)
        t_modbus.start()

        # Wait for threads
        t_provider.join()
        t_client.join()
        t_modbus.join()

    except Exception as e:
        logger.error("Error: " + str(e))
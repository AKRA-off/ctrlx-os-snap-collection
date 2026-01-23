#!/usr/bin/env python3
# -- coding: utf-8 --

# SPDX-FileCopyrightText: Bosch Rexroth AG
#
# SPDX-License-Identifier: MIT

import signal
import time
import sys
import threading
import termios
import os
from datetime import datetime
from typing import List

from flask import Flask, request, render_template, redirect, send_from_directory, Response, stream_with_context, url_for
import shutil
#import config

import onnxruntime
import numpy as np
import cv2
import argparse
import ctrlxdatalayer
import flatbuffers

from waitress import serve
from gevent import pywsgi

import yaml
import glob

from ctypes import *

from MVImport.MvCameraControl_class import *
from helper.ctrlx_datalayer_helper import get_client
from helper.ctrlx_datalayer_helper import get_provider

import logging
from time import time
from time import sleep, perf_counter
from math import atan2, cos, sin, sqrt, pi, tan, radians, degrees, hypot, dist, pi, ceil

#from pyModbusTCP.server import ModbusServer, DataBank
from pyModbusTCP.server import ModbusServer

from comm.datalayer import DisplayFormat, Metadata, NodeClass
from ctrlxdatalayer.provider import Provider
from ctrlxdatalayer.provider_node import (
    ProviderNode,
    ProviderNodeCallbacks,
    NodeCallback,
)
from ctrlxdatalayer.variant import Result, Variant
from ctrlxdatalayer.metadata_utils import (
    MetadataBuilder,
    AllowedOperation,
    ReferenceType,
)

from datetime import datetime
from ctrlxdatalayer.variant import Result, Variant, VariantType
import random
from ctrlxdatalayer.provider import Provider

from instance_segmentation import run_inference_pipeline
from yoloseg.utils import xywh2xyxy, nms, draw_detections, sigmoid, class_init
from yoloseg.YOLOSeg import YOLOSeg


logger = logging.getLogger(__name__)

__close_app = False


global g_numArray
g_numArray = None

address_base = "AI_Detector/data/"

MEDIA_FOLDER = '/var/snap/rexroth-solutions/common/solutions/activeConfiguration/AI_Detector/'
general_path = '/var/snap/rexroth-solutions/common/solutions/activeConfiguration/AI_Detector/'

variable = [99]
bool_flask = [0]
model_name = [".onnx"]
model_name_temp = [".onnx"]
use_hailo = [False]

output_boxes0 = [0.0, 0.0, 0.0, 0.0, 0.0]
output_score0 = [0.0, 0.0, 0.0, 0.0, 0.0]
output_classes0 = [1,1,1,1,1]

initialize_var = True
hmi_conf_thres = [0.3]
hmi_iou_thres = [0.5]
hmi_password = "boschrexroth"
hmi_password_temp = ""
disc_hik = [0]
conn_hik = [0]
time_toweb = [0.0]
time_start = 0
save_config = 0
hmi_cam_settings = [0]
step = 0
log=0
hik_connected=0
hik_conn_arr = [0]
web_conn_arr = [0]
hik_counter = 0
web_counter = 0


app = Flask(__name__)
app.secret_key = b'_1#y2l"F4Q8z\n\xec]/'

logging.basicConfig(level=logging.DEBUG, format='%(asctime)s %(message)s')

@app.route('/uploads/<path:filename>')
def download_file(filename):
    return send_from_directory(MEDIA_FOLDER, filename, as_attachment=True)
@app.route('/', methods=['GET', 'POST'])
def login():
    return render_template("login.html")


@app.route("/logout")
def logout():
    global log
    log = 0
    return render_template("login.html")


@app.route("/login_post", methods=["POST", "GET"])
def login_post():
    flash_data = "error"
    global hmi_password
    if request.method == "POST":
        user = request.form["email"]
        pass1 = request.form["password"]
        if user:
            if (user == "boschrexroth" and pass1==hmi_password) or (user == "admin" and pass1=="br3xr0th@(_&^#$"):
                global log
                log = 1
                return redirect(url_for("index"))
            else:
                return render_template("login.html", result=flash_data)
        else:
            return render_template("login.html", result=flash_data)
@app.route('/index', methods=['GET', 'POST'])
def index():
    global time_toweb
    global log
    global bool_flask
    global hmi_password
    
    if log == 0:
        return render_template("login.html")
    else:    
        if request.method == "POST":
            try:
                if request.form.get("bool_flask"):
                    bool_flask[0] = int(request.form.get("bool_flask"))
                    return redirect(url_for("index"))
                elif request.form.get("time_toweb"):
                    time_toweb[0] = float(request.form.get("time_toweb"))
                    return redirect(url_for("settings"))
            except:
                print("Wrong data type")
            return redirect(url_for("index"))
                
        ##

        return render_template('index.html', bool_flask=bool_flask[0], time_toweb=time_toweb[0])
#
@app.route("/settings", methods=['GET', 'POST'])
def settings():
    global variable
    global bool_flask
    global model_name
    global model_name_temp
    global hmi_conf_thres
    global hmi_iou_thres
    global log
    global hmi_password
    global disc_hik
    global conn_hik
    global save_config
    global hmi_cam_settings
    global MEDIA_FOLDER
    global hik_connected
    global web_conn_arr
    global initialize_var
    global use_hailo


    abs_folder_path = general_path

    # Create lists to store directories, subdirectories, and files
    directories = []
    subdirectories = []
    files = []
    if log == 0:
        return render_template("login.html")
    else:
        # Loop through each item in the folder
        for item in os.listdir(abs_folder_path):
            # Get the absolute path of the item
            abs_item_path = os.path.join(abs_folder_path, item)
    
            # Check if the item is a directory or a file
            if os.path.isdir(abs_item_path):
                # Add the directory name to the directories list
                directories.append(item)
            else:
                # Add the file name to the files list
                files.append(item)
    
        # Loop through each directory in the folder
        for directory in directories:
            # Get the absolute path of the subdirectory
            abs_subdirectory_path = os.path.join(abs_folder_path, directory)
    
            # Loop through each item in the subdirectory
            for item in os.listdir(abs_subdirectory_path):
                # Get the absolute path of the item
                abs_item_path = os.path.join(abs_subdirectory_path, item)
    
                # Check if the item is a directory or a file
                if os.path.isdir(abs_item_path):
                    # Add the subdirectory path to the subdirectories list
                    subdirectories.append(os.path.join(directory, item))
                else:
                    # Add the file path to the files list
                    files.append(os.path.join(directory, item))
        
        if request.method == "POST" and not initialize_var:
            try:
                if request.form.get("variable"):
                    variable[0] = int(request.form.get("variable"))
                    #return redirect('/')  url_for("settings")
                    return redirect(url_for("settings"))
                elif request.form.get("model_name"):
                    tempstr = str(request.form.get("model_name"))
                    if os.path.exists(MEDIA_FOLDER + tempstr):
                        model_name[0] = tempstr
                    else:
                        print("No Model File found in Directory, keeping default")
                        model_name[0] = "yolov8n-seg.onnx"                  
                    return redirect(url_for("settings"))
                elif request.form.get("bool_flask"):
                    bool_flask[0]= int(request.form.get("bool_flask"))
                    return redirect(url_for("settings"))
                elif request.form.get("hmi_conf_thres"):
                    hmi_conf_thres[0] = float(request.form.get("hmi_conf_thres"))
                    return redirect(url_for("settings"))
                elif request.form.get("hmi_iou_thres"):
                    hmi_iou_thres[0] = float(request.form.get("hmi_iou_thres"))
                    return redirect(url_for("settings")) 
                elif request.form.get("hmi_password"):       
                    hmi_password = str(request.form.get("hmi_password"))
                    return redirect(url_for("settings"))
                elif request.form.get("disc_hik"):
                    disc_hik[0] = int(request.form.get("disc_hik"))
                    return redirect(url_for("settings"))    
                elif request.form.get("conn_hik"):
                    conn_hik[0] = int(request.form.get("conn_hik"))
                    return redirect(url_for("settings"))  
                elif request.form.get("save_config"):
                    save_config = int(request.form.get("save_config"))
                    return redirect(url_for("settings"))
                elif request.form.get("hmi_cam_settings"):
                    hmi_cam_settings[0] = int(request.form.get("hmi_cam_settings"))
                    return redirect(url_for("settings"))
                elif request.form.get("hik_connected"):
                    hik_connected = int(request.form.get("hik_connected"))
                    return redirect(url_for("settings"))
                elif request.form.get("web_conn_arr"):
                    web_conn_arr[0] = int(request.form.get("web_conn_arr"))
                    return redirect(url_for("settings"))
                elif request.form.get("use_hailo") is not None:
                    use_hailo[0] = int(request.form.get("use_hailo")) == 1
                    return redirect(url_for("settings"))


            except:
                print("Wrong data type")
            return redirect(url_for("settings"))
                
        ##

        return render_template('settings.html', hik_connected=hik_connected, web_conn_arr=web_conn_arr[0], bool_flask=bool_flask[0], variable=variable[0], model_name=model_name[0], directories=directories, subdirectories=subdirectories, files=files, hmi_conf_thres=hmi_conf_thres[0], hmi_iou_thres=hmi_iou_thres[0], hmi_password=hmi_password, disc_hik=disc_hik[0], conn_hik=conn_hik[0], save_config=save_config, hmi_cam_settings=hmi_cam_settings[0], use_hailo=use_hailo[0])

#Create new directory
@app.route('/create_dir', methods=['POST'])
def create_dir():
    directory = request.form['dir_path']
    dir_path = general_path + directory 
    try:
        os.makedirs(dir_path)
        return redirect(url_for("settings"))
    except:
        return 'Unable to create directory'
    
#Delete directory    
@app.route('/delete_dir', methods=['POST'])
def delete_dir():
    directory = request.form['dir_path']
    dir_path = general_path + directory 
    try:
        shutil.rmtree(dir_path)
        return redirect(url_for("settings"))
    except:
        return 'Unable to delete directory'

#Upload file
@app.route('/upload_file', methods=['POST'])
def upload_file():
    file = request.files['file']
    directory = request.form['dir_path']
    dir_path = general_path + directory 
    #dir_path = general_path
    try:
        file.save(os.path.join(dir_path, file.filename))
        return redirect(url_for("settings"))
    except:
        return 'Unable to upload file'
    
#Delete file    
@app.route('/delete_file', methods=['POST'])
def delete_file():
    directory = request.form['file_path']
    file_path = general_path + directory 
    try:
        if file_path != (MEDIA_FOLDER + 'yolov8n.onnx'):
            print("Removing ", file_path)
            os.remove(file_path)
        else:
            print("Cannot Remove Default Model yolov8n.onnx")
        return redirect(url_for("settings"))
    except:
        return 'Unable to delete file'


def startApp():
    #app.run(debug=False, host='0.0.0.0', port=8021) #8021
    #http_server = WSGIServer(("127.0.0.1", 8021), app)
    #http_server.serve_forever()
    
    serve(app,host='0.0.0.0',port=8021)
    #http_server = pywsgi.WSGIServer(('0.0.0.0', 8021), app, keyfile='server.key', certfile='server.crt')
    #http_server.serve_forever()



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
        """__on_read
        print(
            "__on_read()",
            "address:",
            address,
            "data:",
            self._data,
            "userdata:",
            userdata,
            flush=True,
        )
        """
        new_data = self._data
        cb(Result.OK, new_data)
        
    def __on_write(
        self,
        userdata: ctrlxdatalayer.clib.userData_c_void_p,
        address: str,
        data: Variant,
        cb: NodeCallback,
    ):
        """__on_write
        print(
            "__on_write()",
            "address:",
            address,
            "data:",
            data,
            "userdata:",
            userdata,
            flush=True,
        )
        """
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
        #print("__on_metadata()", "address:", address, flush=True)
        cb(Result.OK, self._metadata)  # Take metadata from metadata.mddb


def handler(signum, frame):
    """handler"""
    global __close_app
    __close_app = True
    # print('Here you go signum: ', signum, __close_app, flush=True)


def provide_string(provider: ctrlxdatalayer.provider, name: str):
    """provide_string"""
    # Create and register simple string provider node
    print("Creating string  provider node " + address_base + name, flush=True)
    variantString = Variant()
    variantString.set_string("myString")
    provider_node_str = MyProviderNode(provider, address_base + name, variantString)
    result = provider_node_str.register_node()
    if result != ctrlxdatalayer.variant.Result.OK:
        print(
            "ERROR Registering node " + address_base + name + " failed with:",
            result,
            flush=True,
        )

    return provider_node_str

def provide_float(provider: ctrlxdatalayer.provider, name: str):
    """provide_string"""
    # Create and register simple string provider node
    print("Creating float  provider node " + address_base + name, flush=True)

    global output_boxes0
    variantFloat = Variant()
    variantFloat.set_array_float32(output_boxes0)
    provider_node_flt = MyProviderNode(provider, address_base + name, variantFloat)
    result = provider_node_flt.register_node()
    if result != ctrlxdatalayer.variant.Result.OK:
        print(
            "ERROR Registering node " + address_base + name + " failed with:",
            result,
            flush=True,
        )

    return provider_node_flt


def provide_float2(provider: ctrlxdatalayer.provider, name: str):
    """provide_string"""
    # Create and register simple string provider node
    print("Creating float  provider node " + address_base + name, flush=True)

    global output_score0
    variantFloat = Variant()
    variantFloat.set_array_float32(output_score0)
    provider_node_flt = MyProviderNode(provider, address_base + name, variantFloat)
    result = provider_node_flt.register_node()
    if result != ctrlxdatalayer.variant.Result.OK:
        print(
            "ERROR Registering node " + address_base + name + " failed with:",
            result,
            flush=True,
        )

    return provider_node_flt


def provide_bool(provider: ctrlxdatalayer.provider, name: str):
    """provide_string"""
    # Create and register simple string provider node
    print("Creating float  provider node " + address_base + name, flush=True)  
    variantBool = Variant()
    variantBool.set_bool8(False)
    provider_node_bl = MyProviderNode(provider, address_base + name, variantBool)
    result = provider_node_bl.register_node()
    if result != ctrlxdatalayer.variant.Result.OK:
        print(
            "ERROR Registering node " + address_base + name + " failed with:",
            result,
            flush=True,
        )

    return provider_node_bl

def provide_int_arr(provider: ctrlxdatalayer.provider, name: str):
    """provide_string"""
    # Create and register simple string provider node
    print("Creating float  provider node " + address_base + name, flush=True)
    global output_classes0    
    variantFloat = Variant()
    variantFloat.set_array_int16(output_classes0)
    provider_node_flt = MyProviderNode(provider, address_base + name, variantFloat)
    result = provider_node_flt.register_node()
    if result != ctrlxdatalayer.variant.Result.OK:
        print(
            "ERROR Registering node " + address_base + name + " failed with:",
            result,
            flush=True,
        )

    return provider_node_flt


def provide_int(provider: ctrlxdatalayer.provider, name: str):
    """provide_string"""
    # Create and register simple string provider node
    print("Creating float  provider node " + address_base + name, flush=True)
    mode = 0
    
    variantFloat = Variant()
    variantFloat.set_int16(mode)
    provider_node_flt = MyProviderNode(provider, address_base + name, variantFloat)
    result = provider_node_flt.register_node()
    if result != ctrlxdatalayer.variant.Result.OK:
        print(
            "ERROR Registering node " + address_base + name + " failed with:",
            result,
            flush=True,
        )

    return provider_node_flt


def create_var(var_name, var_value):
    globals()[var_name] = var_value




def dl_provider():
       
    ### Provider add
    with ctrlxdatalayer.system.System("") as datalayer_system:
        datalayer_system.start(False)

        # ip="10.0.2.2", ssl_port=8443: ctrlX COREvirtual with port forwarding and default port mapping
        provider, connection_string = get_provider(
            datalayer_system, ip="10.0.2.2", ssl_port=8443
        )
        if provider is None:
            print("ERROR Connecting", connection_string, "failed.", flush=True)
            sys.exit(1)

        with (
            provider
        ):  # provider.close() is called automatically when leaving with... block
            result = provider.start()
            if result != Result.OK:
                print(
                    "ERROR Starting ctrlX Data Layer Provider failed with:",
                    result,
                    flush=True,
                )
                return
                

            # Create nodes
            provide_node_float = provide_float(provider, "output_boxes")
            provide_node_float2 = provide_float2(provider, "output_score")
            provide_node_bool = provide_bool(provider, "start_signal")
            provide_node_int_arr = provide_int_arr(provider, "output_classes")
            
                       

            print("INFO Running endless loop...", flush=True)
            while provider.is_connected() and not __close_app:
                sleep(1.0)  # Seconds

            print("ERROR ctrlX Data Layer Provider is disconnected", flush=True)



            provide_node_float.unregister_node()
            del provide_node_float
            provide_node_float2.unregister_node()
            del provide_node_float2
            provide_node_bool.unregister_node()
            del provide_node_bool
            provide_node_int_arr.unregister_node()
            del provide_node_int_arr

                        

            print("Stopping ctrlX Data Layer provider:", end=" ", flush=True)
            result = provider.stop()
            print(result, flush=True)
             
            
            
        # Attention: Doesn't return if any provider or client instance is still running
        stop_ok = datalayer_system.stop(False)
        print("System Stop", stop_ok, flush=True)
    ###Provider
   
    
def dl_client():
     global output_boxes0
     global output_score0
     global output_classes0
     with ctrlxdatalayer.system.System("") as datalayer_system:
        
        datalayer_system.start(False)

        datalayer_client, datalayer_client_connection_string = get_client(
            datalayer_system, ip="10.0.2.2", ssl_port=8443
        )
        if datalayer_client is None:
            print(
                f"ERROR Connecting {datalayer_client_connection_string} failed.",
                flush=True
            )
            sys.exit(1)

        with (
            datalayer_client
        ):  # datalayer_client is closed automatically when leaving with block
            if datalayer_client.is_connected() is False:
                print(f"ERROR ctrlX Data Layer is NOT connected: {datalayer_client_connection_string}",
                    flush=True
                )
                sys.exit(1)

            while datalayer_client.is_connected() and not __close_app:
                dt_str = datetime.now().strftime("%H:%M:%S.%f")

    

                
               
                with Variant() as data:


                    data.set_array_float32(output_boxes0)
                    addr = "AI_Detector/data/output_boxes"
                    result = datalayer_client.write_sync(addr, data)

                    data.set_array_float32(output_score0)
                    addr = "AI_Detector/data/output_score"
                    result = datalayer_client.write_sync(addr, data)

                    data.set_array_int16(output_classes0)
                    addr = "AI_Detector/data/output_classes"
                    result = datalayer_client.write_sync(addr, data)
                    
            

                sleep(1.0)
                

            print("ERROR ctrlX Data Layer is NOT connected")

        stop_ok = datalayer_system.stop(
            False
        )  # Attention: Doesn't return if any provider or client instance is still running
        print("System Stop", stop_ok, flush=True)
    


# Mono图像转为python数组
def Mono_numpy(data, nWidth, nHeight):
    data_ = np.frombuffer(data, count=int(nWidth * nHeight), dtype=np.uint8, offset=0)
    data_mono_arr = data_.reshape(nWidth, nHeight)
    numArray = np.zeros([nWidth, nHeight, 1], "uint8")
    numArray[:, :, 0] = data_mono_arr
    return numArray


# 彩色图像转为python数组
def Color_numpy(data, nWidth, nHeight):
    data_ = np.frombuffer(data, count=int(nWidth * nHeight * 3), dtype=np.uint8, offset=0)
    data_r = data_[0:nWidth * nHeight * 3:3]
    data_g = data_[1:nWidth * nHeight * 3:3]
    data_b = data_[2:nWidth * nHeight * 3:3]

    data_r_arr = data_r.reshape(nWidth, nHeight)
    data_g_arr = data_g.reshape(nWidth, nHeight)
    data_b_arr = data_b.reshape(nWidth, nHeight)
    numArray = np.zeros([nWidth, nHeight, 3], "uint8")

    numArray[:, :, 0] = data_r_arr
    numArray[:, :, 1] = data_g_arr
    numArray[:, :, 2] = data_b_arr
    return numArray

def press_any_key_exit():
    fd = sys.stdin.fileno()
    old_ttyinfo = termios.tcgetattr(fd)
    new_ttyinfo = old_ttyinfo[:]
    new_ttyinfo[3] &= ~termios.ICANON
    new_ttyinfo[3] &= ~termios.ECHO
    #sys.stdout.write(msg)
    #sys.stdout.flush()
    termios.tcsetattr(fd, termios.TCSANOW, new_ttyinfo)
    try:
        os.read(fd, 7)
    except:
        pass
    finally:
        termios.tcsetattr(fd, termios.TCSANOW, old_ttyinfo)


def handler(signum, frame):
    """handler"""
    global __close_app
    __close_app = True
    # print('Here you go signum: ', signum, __close_app, flush=True)


def sig():
    signal.signal(signal.SIGINT, handler)
    signal.signal(signal.SIGTERM, handler)
    signal.signal(signal.SIGABRT, handler)


def camera_processing(j):
    
    global time_start
    global step
    global hik_connected
    global cam
    global hik_conn_arr
    global web_conn_arr
    global hik_counter
    global web_counter

    if  conn_hik[j] == 1 and variable[j] == 0:
                    
        SDKVersion = MvCamera.MV_CC_GetSDKVersion()
        print ("SDKVersion[0x%x]" % SDKVersion)
    
        deviceList = MV_CC_DEVICE_INFO_LIST()
        tlayerType = MV_GIGE_DEVICE | MV_USB_DEVICE
    
        # ch:枚举设备 | en:Enum device
        ret = MvCamera.MV_CC_EnumDevices(tlayerType, deviceList)
        if ret != 0:
            print ("enum devices fail! ret[0x%x]" % ret)
            sys.exit()
    
        if deviceList.nDeviceNum == 0:
            print ("find no device!")
            sys.exit()
    
        print ("Find %d devices!" % deviceList.nDeviceNum)
    
        for i in range(0, deviceList.nDeviceNum):
            mvcc_dev_info = cast(deviceList.pDeviceInfo[i], POINTER(MV_CC_DEVICE_INFO)).contents
            if mvcc_dev_info.nTLayerType == MV_GIGE_DEVICE:
                print ("\ngige device: [%d]" % i)
                strModeName = ""
                for per in mvcc_dev_info.SpecialInfo.stGigEInfo.chModelName:
                    strModeName = strModeName + chr(per)
                print ("device model name: %s" % strModeName)
    
                nip1 = ((mvcc_dev_info.SpecialInfo.stGigEInfo.nCurrentIp & 0xff000000) >> 24)
                nip2 = ((mvcc_dev_info.SpecialInfo.stGigEInfo.nCurrentIp & 0x00ff0000) >> 16)
                nip3 = ((mvcc_dev_info.SpecialInfo.stGigEInfo.nCurrentIp & 0x0000ff00) >> 8)
                nip4 = (mvcc_dev_info.SpecialInfo.stGigEInfo.nCurrentIp & 0x000000ff)
                print ("current ip: %d.%d.%d.%d\n" % (nip1, nip2, nip3, nip4))
            elif mvcc_dev_info.nTLayerType == MV_USB_DEVICE:
                print ("\nu3v device: [%d]" % i)
                strModeName = ""
                for per in mvcc_dev_info.SpecialInfo.stUsb3VInfo.chModelName:
                    if per == 0:
                        break
                    strModeName = strModeName + chr(per)
                print ("device model name: %s" % strModeName)
    
                strSerialNumber = ""
                for per in mvcc_dev_info.SpecialInfo.stUsb3VInfo.chSerialNumber:
                    if per == 0:
                        break
                    strSerialNumber = strSerialNumber + chr(per)
                print ("user serial number: %s" % strSerialNumber)

        nConnectionNum = 0
        hik_conn_arr[j] = 1

        if int(nConnectionNum) >= deviceList.nDeviceNum:
            print ("intput error!")
            sys.exit()
    
        # ch:创建相机实例 | en:Creat Camera Object
        cam = MvCamera()
        
    
        # ch:选择设备并创建句柄| en:Select device and create handle
        stDeviceList = cast(deviceList.pDeviceInfo[int(nConnectionNum)], POINTER(MV_CC_DEVICE_INFO)).contents
    
        ret = cam.MV_CC_CreateHandle(stDeviceList)
        if ret != 0:
            print ("create handle fail! ret[0x%x]" % ret)
            sys.exit()
    
        # ch:打开设备 | en:Open device
        ret = cam.MV_CC_OpenDevice(MV_ACCESS_Exclusive, 0)
        if ret != 0:
            print ("open device fail! ret[0x%x]" % ret)
            sys.exit()
    
        # ch:探测网络最佳包大小(只对GigE相机有效) | en:Detection network optimal package size(It only works for the GigE camera)
        if stDeviceList.nTLayerType == MV_GIGE_DEVICE:
            nPacketSize = cam.MV_CC_GetOptimalPacketSize()
            if int(nPacketSize) > 0:
                ret = cam.MV_CC_SetIntValue("GevSCPSPacketSize",nPacketSize)
                if ret != 0:
                    print ("Warning: Set Packet Size fail! ret[0x%x]" % ret)
            else:
                print ("Warning: Get Packet Size fail! ret[0x%x]" % nPacketSize)
    
        # ch:设置触发模式为off | en:Set trigger mode as off
        ret = cam.MV_CC_SetEnumValue("TriggerMode", MV_TRIGGER_MODE_OFF)
        if ret != 0:
            print ("set trigger mode fail! ret[0x%x]" % ret)
            sys.exit()
    
        # ch:获取数据包大小 | en:Get payload size
        stParam =  MVCC_INTVALUE()
        memset(byref(stParam), 0, sizeof(MVCC_INTVALUE))
    
        ret = cam.MV_CC_GetIntValue("PayloadSize", stParam)
        if ret != 0:
            print ("get payload size fail! ret[0x%x]" % ret)
            sys.exit()
        nPayloadSize = stParam.nCurValue

        conn_hik[j] = 0
        hik_connected = 1
        #READY TO GRAB

    if bool_flask[j] == 1:
        ###CAMERA###

        print("Starting inference Number: ", j)
        print("Starting inference Number: ", j)
        print("Starting inference Number: ", j)
        print("Starting inference Number: ", j)
        print("Starting inference Number: ", j)



        step = j
    
        class_init(j)
        

        
        if variable[j] == 0 and conn_hik[j] == 0: #hik

        
            time_start = time() 
            
            # ch:开始取流 | en:Start grab image
            ret = cam.MV_CC_StartGrabbing()
            if ret != 0:
                print ("start grabbing fail! ret[0x%x]" % ret)
                sys.exit()
        
            stOutFrame = MV_FRAME_OUT()
            memset(byref(stOutFrame), 0, sizeof(stOutFrame))
        
            ret = cam.MV_CC_GetImageBuffer(stOutFrame, 1000)
            if None != stOutFrame.pBufAddr and 0 == ret:
                print("get one frame: Width[%d], Height[%d], nFrameNum[%d],nFrameLen[%d]" % (
                stOutFrame.stFrameInfo.nWidth, stOutFrame.stFrameInfo.nHeight, stOutFrame.stFrameInfo.nFrameNum,stOutFrame.stFrameInfo.nFrameLen))
                
                buf_cache = (c_ubyte * stOutFrame.stFrameInfo.nFrameLen)()
                        # 图像数据拷贝
                memmove(byref(buf_cache), stOutFrame.pBufAddr, stOutFrame.stFrameInfo.nFrameLen)
                if PixelType_Gvsp_Mono8 == stOutFrame.stFrameInfo.enPixelType:
                    g_numArray = Mono_numpy(buf_cache,stOutFrame.stFrameInfo.nWidth, stOutFrame.stFrameInfo.nHeight)
                    g_numArray = g_numArray.reshape(stOutFrame.stFrameInfo.nHeight, stOutFrame.stFrameInfo.nWidth, 1)
                elif PixelType_Gvsp_RGB8_Packed == stOutFrame.stFrameInfo.enPixelType:
                    g_numArray = Color_numpy(buf_cache,stOutFrame.stFrameInfo.nWidth, stOutFrame.stFrameInfo.nHeight)
                    g_numArray = g_numArray.reshape(stOutFrame.stFrameInfo.nHeight, stOutFrame.stFrameInfo.nWidth, 3)
                else:
                    print("Not Support")
                    sys.exit()
        
                cv2.imwrite(MEDIA_FOLDER + "pre" + str(j) + ".jpg", g_numArray)
                    
                nRet = cam.MV_CC_FreeImageBuffer(stOutFrame)
        
            else:
                print("no data[0x%x]" % ret)
                
            #print ("press a key to stop grabbing.")
            #press_any_key_exit()
        
            # ch:停止取流 | en:Stop grab image
            ret = cam.MV_CC_StopGrabbing()
            if ret != 0:
                print ("stop grabbing fail! ret[0x%x]" % ret)
                del data_buf
                sys.exit()
        
        
            
            yolo_inf(j)
    
        elif variable[j] == 1:    #webcam

            time_start = time()

            # Improved webcam capture with better error handling
            cap = cv2.VideoCapture(web_conn_arr[j], cv2.CAP_V4L2)

            # Set resolution (can be made configurable later)
            cap.set(cv2.CAP_PROP_FRAME_WIDTH, 1280)
            cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 720)

            # Try MJPEG format for better performance
            fourcc = cv2.VideoWriter_fourcc(*'MJPG')
            cap.set(cv2.CAP_PROP_FOURCC, fourcc)

            ret, frame = cap.read()
            if ret and frame is not None:
                cv2.imwrite(MEDIA_FOLDER + "pre" + str(j) + ".jpg", frame)
            else:
                print(f"Failed to capture frame from webcam {web_conn_arr[j]}")

            cap.release()
            yolo_inf(j)

        elif variable[j] == 2:
            
            time_start = time()
            
            yolo_inf(j)

            
    if disc_hik[j] == 1 and conn_hik[j] == 0 and variable[j] == 0:

        # ch:关闭设备 | Close device
        ret = cam.MV_CC_CloseDevice()
        if ret != 0:
            print ("close deivce fail! ret[0x%x]" % ret)
            del data_buf
            sys.exit()

        # ch:销毁句柄 | Destroy handle
        ret = cam.MV_CC_DestroyHandle()
        if ret != 0:
            print ("destroy handle fail! ret[0x%x]" % ret)
            del data_buf
            sys.exit()

        disk_hik[j] = 0
        hik_connected = 0

def hailo_inf(camid):
    """
    Run HAILO accelerator inference using instance_segmentation module
    """
    global bool_flask
    global MEDIA_FOLDER
    global model_name
    global time_start
    global time_toweb
    global output_boxes0
    global output_score0
    global output_classes0

    # Initialize detection outputs as empty arrays
    # Note: HAILO pipeline doesn't currently return detection results
    # To implement this, instance_segmentation.py would need to be modified
    output_boxes0 = np.array([])
    output_score0 = np.array([])
    output_classes0 = np.array([])

    model_path = MEDIA_FOLDER + model_name[camid]
    input_image_path = MEDIA_FOLDER + "pre" + str(camid) + ".jpg"
    output_dir = MEDIA_FOLDER

    try:
        data_yaml_path = general_path + 'data' + str(camid) + '.yaml'
        labels_list = []

        try:
            with open(data_yaml_path, encoding="utf8") as f:
                yaml_data = yaml.safe_load(f)

            for k in yaml_data['names'].values():
                labels_list.append(k)

            print(f"Loaded {len(labels_list)} labels from {data_yaml_path}", flush=True)
        except Exception as e:
            print(f"Warning: Could not load labels from {data_yaml_path}: {e}", flush=True)
            labels_list = ["object"]

        temp_labels_file = MEDIA_FOLDER + f"temp_labels_{camid}.txt"
        with open(temp_labels_file, 'w') as f:
            for label in labels_list:
                f.write(f"{label}\n")


        arch = "v8"  # Default to YOLOv8
        if "yolov5" in model_name[camid].lower() or "v5" in model_name[camid].lower():
            arch = "v5"
        elif "fastsam" in model_name[camid].lower() or "fast" in model_name[camid].lower():
            arch = "fast"


        print(f"Running HAILO inference with model: {model_path}", flush=True)
        detection_results = run_inference_pipeline(
            net=model_path,
            input_path=input_image_path,
            arch=arch,
            batch_size=1,
            labels_file=temp_labels_file,
            output_dir=output_dir,
            save_stream_output=True,
            resolution="sd",
            enable_tracking=False,
            show_fps=False
        )

        # Extract detection results from HAILO inference
        if detection_results:
            output_boxes0 = detection_results.get('boxes', np.array([]))
            output_score0 = detection_results.get('scores', np.array([]))
            output_classes0 = detection_results.get('classes', np.array([]))
            print(f"HAILO detected {len(output_score0)} objects", flush=True)
        else:
            print("Warning: No detection results returned from HAILO", flush=True)

        try:
            os.remove(temp_labels_file)
        except:
            pass


        output_image_path = MEDIA_FOLDER + "inf" + str(camid) + ".jpg"


        output_files = []
        for ext in ['output_*.jpg', 'output_*.png']:
            output_files.extend(glob.glob(os.path.join(output_dir, ext)))

        if output_files:
            # Get the most recent output file
            latest_output = max(output_files, key=os.path.getctime)
            shutil.copy(latest_output, output_image_path)
            print(f"HAILO output saved to: {output_image_path}", flush=True)
        else:
            print("Warning: No HAILO output image found", flush=True)
            # Copy the input image as fallback
            shutil.copy(input_image_path, output_image_path)

        time_end = time()
        time_toweb[camid] = round(time_end - time_start, 3)
        bool_flask[camid] = 0

    except Exception as e:
        print(f"HAILO inference error: {e}", flush=True)
        import traceback
        traceback.print_exc()
        bool_flask[camid] = 0


def yolo_inf(camid):

    global bool_flask
    global MEDIA_FOLDER
    global model_name
    global hmi_conf_thres
    global hmi_iou_thres
    global time_start
    global time_toweb
    global use_hailo
    global output_boxes0
    global output_score0
    global output_classes0

    # Check if using HEF (Hailo) or ONNX
    if use_hailo[camid]:
        hailo_inf(camid)
        return

    # ONNX inference with YOLOSeg (Instance Segmentation)
    yoloseg = YOLOSeg(MEDIA_FOLDER + model_name[camid], conf_thres=hmi_conf_thres[camid], iou_thres=hmi_iou_thres[camid])
    img = cv2.imread(MEDIA_FOLDER + "pre"+ str(camid) + ".jpg")

    # Segment Objects
    boxes, scores, class_ids, mask_maps = yoloseg(img)

    # Update global output variables for datalayer
    output_boxes0 = np.ravel(boxes) if len(boxes) > 0 else np.array([])
    output_score0 = np.ravel(scores) if len(scores) > 0 else np.array([])
    output_classes0 = np.ravel(class_ids) if len(class_ids) > 0 else np.array([])

    print("Detected:", len(boxes), "objects")

    # Draw segmentation masks
    combined_img = yoloseg.draw_masks(img)

    cv2.imwrite(MEDIA_FOLDER +"inf" + str(camid) + ".jpg", combined_img)

    time_end = time()
    time_toweb[camid] = round(time_end - time_start, 3)

    bool_flask[camid] = 0

    

def main():

    global variable
    global bool_flask
    global MEDIA_FOLDER
    global model_name
    global model_name_temp
    global initialize_var
    global hmi_password
    global hmi_password_temp
    global disc_hik
    global conn_hik
    global time_toweb
    global time_start
    global hmi_conf_thres
    global hmi_iou_thres
    global save_config
    global hmi_cam_settings
    global use_hailo



    if initialize_var:

        with open(MEDIA_FOLDER + "config.yaml") as f:
            cfg = yaml.load(f, Loader=yaml.FullLoader)
            print(cfg)

            hmi_password = cfg["boschrexroth"]
            hmi_password_temp = hmi_password

            model_name[0] = cfg["onnx"]
            variable[0] = cfg["cam_select"]
            hmi_conf_thres[0] = cfg["conf_thres"]
            hmi_iou_thres[0] = cfg["iou_thres"]
            use_hailo[0] = cfg.get("use_hailo", False)
            model_name_temp[0] = model_name[0]
        initialize_var = False
        
    
    while not __close_app:

        if model_name_temp[0] != model_name[0]:
            model_name_temp[0] = model_name[0]
        
        if save_config == 1:

            cfg["boschrexroth"] = hmi_password

            cfg["onnx"] = model_name[0]
            cfg["cam_select"] = variable[0]
            cfg["conf_thres"] = hmi_conf_thres[0]
            cfg["iou_thres"] = hmi_iou_thres[0]
            cfg["use_hailo"] = use_hailo[0] 
            
            with open(MEDIA_FOLDER + "config.yaml", "w") as f:
                cfg = yaml.dump(
                    cfg, stream=f, default_flow_style=False, sort_keys=False
                )
            ###   

            with open(MEDIA_FOLDER + "config.yaml") as f:
                cfg = yaml.load(f, Loader=yaml.FullLoader)
                print(cfg)

                hmi_password = cfg["boschrexroth"]
                hmi_password_temp = hmi_password

                model_name[0] = cfg["onnx"]
                variable[0] = cfg["cam_select"]
                hmi_conf_thres[0] = cfg["conf_thres"]
                hmi_iou_thres[0] = cfg["iou_thres"]
                use_hailo[0] = cfg.get("use_hailo", False)
                model_name_temp[0] = model_name[0]
            save_config = 0      

        camera_processing(0)
        
                
if __name__ == "__main__":        
    sig()

    try:
        logger.info(f'First thread')
        t1 = threading.Thread(target=main)
        t1.start()
        
        logger.info(f'Second thread')
        t2 = threading.Thread(target=dl_provider)
        t2.start()
        
        sleep(3.0)
        
        logger.info(f'Third thread')
        t3 = threading.Thread(target=dl_client)
        t3.start()
        
        logger.info(f'Fourth thread')
        t4 = threading.Thread(target=startApp)
        t4.start()
        
        # Wait for all threads to complete
        t1.join()
        t2.join()
        t3.join()
        t4.join()
        
    except KeyboardInterrupt:
        logger.info("Interrupted by user, shutting down gracefully...")
    except Exception as e:
        logger.error("Error: " + str(e))  
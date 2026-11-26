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

from flask import Flask, request, render_template, redirect, send_from_directory, Response, stream_with_context
import shutil
#import config

import numpy as np
import cv2
import argparse
import ctrlxdatalayer
import flatbuffers

from ctypes import *

from MVImport.MvCameraControl_class import *
from helper.ctrlx_datalayer_helper import get_client
from helper.ctrlx_datalayer_helper import get_provider

import threading
import logging
from time import time
from time import sleep
from math import atan2, cos, sin, sqrt, pi, tan, radians, degrees, hypot, dist, pi

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



logger = logging.getLogger(__name__)


# os.system("printenv")

# print(cv2.__version__)

__close_app = False


global g_numArray
g_numArray = None

MEDIA_FOLDER = '/var/snap/rexroth-solutions/common/solutions/activeConfiguration/contour/'

#general_path = '/var/snap/rexroth-solutions/common/webdav/'  #To access from the webdav directory 
#Make sure that in the case the user wants to change the active configuration when using webdav it is required to write appdata/app_name

app = Flask(__name__)
app.secret_key = b'_1#y2l"F4Q8z\n\xec]/'

variable = 99
lst = "EMPTY"
time_duration = 0.0
email_ext = "EMPTY"
time_toweb = 0.0
s = "TEMP"
ip_modbus = " "
img_scale= 0
jpeg_quality = 25
temp_ip = " "
reg_string = "\S+@\S+"
bool_flask = 0
point_resol = 25
erode_cont = 30

address_base = "contour/datalayer/provider/"

angles = [0,0,0,0,0]
angles_ct = [0,0,0,0,0]
positions_x = [0,0,0,0,0]
positions_y = [0,0,0,0,0]
areaC = [5000, 7000, 4, 7, 180] 
areaT = [4000, 5500, 10, 14.5, 90] 
areaR = [7700, 10000, 4, 8, 145]
areaL = [3500, 5500, 14.6, 22, 219]
areaX = [5500, 8000, 0, 3, 0]
areaALL = [3000, 24000]  
c_count_arr = []

mode = 0
threshold_val = 0
threshold_val_intern = 0
threshold_val_feedback = 0
#
video = cv2.VideoCapture(0)

def video_stream():
    while(True):
        ret, frame = video.read()
        if not ret:
            break
        else:
            ret, buffer = cv2.imencode('.jpeg',frame)
            frame = buffer.tobytes()
            yield (b'--frame\r\n' b'Content-type: image/jpeg\r\n\r\n' + frame + b'\r\n')
#

@app.route('/uploads/<path:filename>')
def download_file(filename):
    return send_from_directory(MEDIA_FOLDER, filename, as_attachment=True)
@app.route('/', methods=['GET', 'POST'])
@app.route('/index', methods=['GET', 'POST'])
def index():
    global variable
    global email_ext
    global time_toweb
    global time_duration
    global ip_modbus
    global img_scale
    global jpeg_quality
    global reg_string
    global bool_flask
    global point_resol
    global erode_cont
    if request.method == "POST":
        try:
            if request.form.get("variable"):
                variable = int(request.form.get("variable"))
                return redirect('/')   
            elif request.form.get("ip_modbus"):       
                ip_modbus = str(request.form.get("ip_modbus"))
                return redirect('/')
            elif request.form.get("img_scale"):
                img_scale = int(request.form.get("img_scale"))
                return redirect('/')
            elif request.form.get("jpeg_quality"):
                jpeg_quality = int(request.form.get("jpeg_quality"))
                return redirect('/')
            elif request.form.get("point_resol"):
                point_resol = int(request.form.get("point_resol"))
                return redirect('/')
            elif request.form.get("erode_cont"):
                erode_cont = int(request.form.get("erode_cont"))
                return redirect('/')
            elif request.form.get("reg_string"):
                reg_string = str(request.form.get("reg_string"))
                return redirect('/')
            elif request.form.get("bool_flask"):
                bool_flask = int(request.form.get("bool_flask"))
                return redirect('/')
        except:
            print("Wrong data type")
        return redirect('/')
        
    return render_template('index.html', variable=variable, email_ext=email_ext, time_toweb=time_toweb, ip_modbus=ip_modbus, img_scale=img_scale, jpeg_quality=jpeg_quality, reg_string=reg_string, bool_flask=bool_flask, point_resol=point_resol, erode_cont=erode_cont)
#
@app.route('/video_feed')

def video_feed():
    return Response(video_stream(), mimetype= 'multipart/x-mixed-replace; boundary=frame')
#
###Provider
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
        #print("__on_remove()", "address:", address, "userdata:", userdata, flush=True)
        cb(Result.UNSUPPORTED, None)

    def __on_browse(
        self,
        userdata: ctrlxdatalayer.clib.userData_c_void_p,
        address: str,
        cb: NodeCallback,
    ):
        """__on_browse"""
        #print("__on_browse()", "address:", address, "userdata:", userdata, flush=True)
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

##Provider

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
    variantFloat = Variant()
    global angles
    variantFloat.set_array_float32(angles)
    provider_node_flt = MyProviderNode(provider, address_base + name, variantFloat)
    result = provider_node_flt.register_node()
    if result != ctrlxdatalayer.variant.Result.OK:
        print(
            "ERROR Registering node " + address_base + name + " failed with:",
            result,
            flush=True,
        )

    return provider_node_flt

def provide_float_ct(provider: ctrlxdatalayer.provider, name: str):
    """provide_string"""
    # Create and register simple string provider node
    print("Creating float  provider node " + address_base + name, flush=True)
    variantFloat = Variant()
    global angles_ct
    variantFloat.set_array_float32(angles)
    provider_node_flt = MyProviderNode(provider, address_base + name, variantFloat)
    result = provider_node_flt.register_node()
    if result != ctrlxdatalayer.variant.Result.OK:
        print(
            "ERROR Registering node " + address_base + name + " failed with:",
            result,
            flush=True,
        )

    return provider_node_flt

def provide_float_pos_x(provider: ctrlxdatalayer.provider, name: str):
    """provide_string"""
    # Create and register simple string provider node
    print("Creating float  provider node " + address_base + name, flush=True)
    variantFloat = Variant()
    global positions_x
    variantFloat.set_array_float32(positions_x)
    provider_node_flt = MyProviderNode(provider, address_base + name, variantFloat)
    result = provider_node_flt.register_node()
    if result != ctrlxdatalayer.variant.Result.OK:
        print(
            "ERROR Registering node " + address_base + name + " failed with:",
            result,
            flush=True,
        )

    return provider_node_flt

def provide_float_pos_y(provider: ctrlxdatalayer.provider, name: str):
    """provide_string"""
    # Create and register simple string provider node
    print("Creating float  provider node " + address_base + name, flush=True)
    variantFloat = Variant()
    global positions_y
    variantFloat.set_array_float32(positions_y)
    provider_node_flt = MyProviderNode(provider, address_base + name, variantFloat)
    result = provider_node_flt.register_node()
    if result != ctrlxdatalayer.variant.Result.OK:
        print(
            "ERROR Registering node " + address_base + name + " failed with:",
            result,
            flush=True,
        )

    return provider_node_flt

def provide_float_c(provider: ctrlxdatalayer.provider, name: str):
    """provide_string"""
    # Create and register simple string provider node
    print("Creating float  provider node " + address_base + name, flush=True)
    variantFloat = Variant()
    global areaC
    variantFloat.set_array_float32(areaC)
    provider_node_flt = MyProviderNode(provider, address_base + name, variantFloat)
    result = provider_node_flt.register_node()
    if result != ctrlxdatalayer.variant.Result.OK:
        print(
            "ERROR Registering node " + address_base + name + " failed with:",
            result,
            flush=True,
        )

    return provider_node_flt

def provide_float_t(provider: ctrlxdatalayer.provider, name: str):
    """provide_string"""
    # Create and register simple string provider node
    print("Creating float  provider node " + address_base + name, flush=True)
    variantFloat = Variant()
    global areaT
    variantFloat.set_array_float32(areaT)
    provider_node_flt = MyProviderNode(provider, address_base + name, variantFloat)
    result = provider_node_flt.register_node()
    if result != ctrlxdatalayer.variant.Result.OK:
        print(
            "ERROR Registering node " + address_base + name + " failed with:",
            result,
            flush=True,
        )

    return provider_node_flt

def provide_float_r(provider: ctrlxdatalayer.provider, name: str):
    """provide_string"""
    # Create and register simple string provider node
    print("Creating float  provider node " + address_base + name, flush=True)
    variantFloat = Variant()
    global areaR
    variantFloat.set_array_float32(areaR)
    provider_node_flt = MyProviderNode(provider, address_base + name, variantFloat)
    result = provider_node_flt.register_node()
    if result != ctrlxdatalayer.variant.Result.OK:
        print(
            "ERROR Registering node " + address_base + name + " failed with:",
            result,
            flush=True,
        )

    return provider_node_flt

def provide_float_l(provider: ctrlxdatalayer.provider, name: str):
    """provide_string"""
    # Create and register simple string provider node
    print("Creating float  provider node " + address_base + name, flush=True)
    variantFloat = Variant()
    global areaL
    variantFloat.set_array_float32(areaL)
    provider_node_flt = MyProviderNode(provider, address_base + name, variantFloat)
    result = provider_node_flt.register_node()
    if result != ctrlxdatalayer.variant.Result.OK:
        print(
            "ERROR Registering node " + address_base + name + " failed with:",
            result,
            flush=True,
        )

    return provider_node_flt

def provide_float_x(provider: ctrlxdatalayer.provider, name: str):
    """provide_string"""
    # Create and register simple string provider node
    print("Creating float  provider node " + address_base + name, flush=True)
    variantFloat = Variant()
    global areaX
    variantFloat.set_array_float32(areaX)
    provider_node_flt = MyProviderNode(provider, address_base + name, variantFloat)
    result = provider_node_flt.register_node()
    if result != ctrlxdatalayer.variant.Result.OK:
        print(
            "ERROR Registering node " + address_base + name + " failed with:",
            result,
            flush=True,
        )

    return provider_node_flt


def provide_float_ar(provider: ctrlxdatalayer.provider, name: str):
    """provide_string"""
    # Create and register simple string provider node
    print("Creating float  provider node " + address_base + name, flush=True)
    variantFloat = Variant()
    global areaALL
    variantFloat.set_array_float32(areaALL)
    provider_node_flt = MyProviderNode(provider, address_base + name, variantFloat)
    result = provider_node_flt.register_node()
    if result != ctrlxdatalayer.variant.Result.OK:
        print(
            "ERROR Registering node " + address_base + name + " failed with:",
            result,
            flush=True,
        )

    return provider_node_flt


def provide_int_arr(provider: ctrlxdatalayer.provider, name: str):
    """provide_string"""
    # Create and register simple string provider node
    print("Creating float  provider node " + address_base + name, flush=True)
    global threshold_val 
    
    variantFloat = Variant()
    variantFloat.set_int16(threshold_val)
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
    global mode 
    
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



def provide_to_DL():

    global angles
    global angles_ct
    global positions_x
    global positions_y

    
    ### Provider add
    with ctrlxdatalayer.system.System("") as datalayer_system:
        datalayer_system.start(False)

        # ip="10.0.2.2", ssl_port=8443: ctrlX COREvirtual with port forwarding and default port mapping
        provider, connection_string = get_provider(
            datalayer_system, ip="192.168.1.1", user="boschrexroth", password="boschrexroth"
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
            provide_node_float_ar_x = provide_float_ar(provider, "coordx")   
            provide_node_float_ar_y = provide_float_ar(provider, "coordy")
              

            print("INFO Running endless loop...", flush=True)
            while provider.is_connected() and not __close_app:
                sleep(1.0)  # Seconds

            print("ERROR ctrlX Data Layer Provider is disconnected", flush=True)

            provide_node_float_ar_x.unregister_node()
            del provide_node_float_ar_x
            provide_node_float_ar_y.unregister_node()
            del provide_node_float_ar_y



            print("Unregistering", type_sampleSchema_inertialValue, end=" ", flush=True)
            result = provider.unregister_type(type_sampleSchema_inertialValue)
            print(result, flush=True)

            print("Stopping ctrlX Data Layer provider:", end=" ", flush=True)
            result = provider.stop()
            print(result, flush=True)

        # Attention: Doesn't return if any provider or client instance is still running
        stop_ok = datalayer_system.stop(False)
        print("System Stop", stop_ok, flush=True)

def dl_client():
    global areaC
    global areaT
    global areaR
    global areaL
    global areaX
    global angles
    global angles_ct
    global positions_x
    global positions_y
    global areaALL
    global threshold_val
    global mode
    global bool_flask
    global threshold_val_feedback

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

                    
                    if bool_flask == 1:

                        data.set_bool8(True)
                        addr = "plc/app/Application/sym/GVL/en_move"
                        result = datalayer_client.write_sync(addr, data)
                        print(
                            f"INFO {dt_str} Sync write '{addr}': result {result}",
                            flush=True
                        )

                        bool_flask = 0

                    data.set_array_float32(positions_x)
                    addr = "contour/datalayer/provider/coordx"
                    result = datalayer_client.write_sync(addr, data)
                    print(
                        f"INFO {dt_str} Sync write '{addr}': result {result}",
                        flush=True
                    )           

                    data.set_array_float32(positions_y)
                    addr = "contour/datalayer/provider/coordy"
                    result = datalayer_client.write_sync(addr, data)
                    print(
                        f"INFO {dt_str} Sync write '{addr}': result {result}",
                        flush=True
                    )                                 


                sleep(1.0)
                

            print("ERROR ctrlX Data Layer is NOT connected")

        stop_ok = datalayer_system.stop(
            False
        )  # Attention: Doesn't return if any provider or client instance is still running
        print("System Stop", stop_ok, flush=True)
    


def handler(signum, frame):
    """handler"""
    global __close_app
    __close_app = True
    # print('Here you go signum: ', signum, __close_app, flush=True)




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

def startApp():
    app.run(debug=False, host='0.0.0.0', port=8121)

def sig():
    signal.signal(signal.SIGINT, handler)
    signal.signal(signal.SIGTERM, handler)
    signal.signal(signal.SIGABRT, handler)

def mbServ():

    global ip_modbus    
    global variable
    global email_ext
    global lst
    global temp_ip
    
   

    
    while not __close_app:

        server = ModbusServer(ip_modbus, 12345, no_block=True)


        if ip_modbus!=temp_ip:

            temp_ip=ip_modbus
            print("Starting Server...")
            server.start()
            print("Server is online")
            state = [0]    
        
        if variable == 2:               
            
            res = bytes(lst[0], 'utf-8')
            server.data_bank.set_holding_registers(0, res)
            print(res)
        
            
        sleep(1.0)



def read_text_from_image(image):
    """Reads text from an image file and outputs found text to text file"""
    # Convert the image to grayscale
    global s
    gray_image = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)

    # Perform OTSU Threshold
    ret, thresh = cv2.threshold(gray_image, 0, 255, cv2.THRESH_OTSU | cv2.THRESH_BINARY_INV)

    rect_kernel = cv2.getStructuringElement(cv2.MORPH_RECT, (18, 18))

    dilation = cv2.dilate(thresh, rect_kernel, iterations = 1)

    contours, hierachy = cv2.findContours(dilation, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_NONE)

    image_copy = image.copy()

    for contour in contours:
        x, y, w, h = cv2.boundingRect(contour)

        cropped = image_copy[y : y + h, x : x + w]
   
        s = pytesseract.image_to_string(cropped, lang='eng', config='--psm 11, --oem 3')

    
def angle_between(a, b):
    angle = degrees(atan2(a[1] - b[1], b[0] - a[0]))
    #if angle < 0:
    #    angle += 360
    return angle

def angle_between2(a, b):
    angle = degrees(atan2(a[1] - b[1], b[0] - a[0]))
    if angle < 0:
        angle += 360
    return angle


def rotateImage(image, angel):#parameter angel in degrees
    height = image.shape[0]
    width = image.shape[1]
    height_big = height * 2
    width_big = width * 2
    image_big = cv2.resize(image, (width_big, height_big))
    image_center = (width_big/2, height_big/2)#rotation center
    rot_mat = cv2.getRotationMatrix2D(image_center,angel, 0.5)
    result = cv2.warpAffine(image_big, rot_mat, (width_big, height_big), flags=cv2.INTER_LINEAR)
    return result


def getPointInDir(x0, y0, angX, angY, distance):
    x1 = x0 + distance*cos(angX*pi/180)
    y1 = y0 + distance*sin(angY*pi/180)

    result = [x1, y1]
    return result

def drawAxis(img, p_, q_, color, scale):
  p = list(p_)
  q = list(q_)

#def edges(cont):
 

 
  ## [visualization1]
  angle = atan2(p[1] - q[1], p[0] - q[0]) # angle in radians
  hypotenuse = sqrt((p[1] - q[1]) * (p[1] - q[1]) + (p[0] - q[0]) * (p[0] - q[0]))
 
  # Here we lengthen the arrow by a factor of scale
  q[0] = p[0] - scale * hypotenuse * cos(angle)
  q[1] = p[1] - scale * hypotenuse * sin(angle)
  cv2.line(img, (int(p[0]), int(p[1])), (int(q[0]), int(q[1])), color, 3, cv2.LINE_AA)
 
  # create the arrow hooks
  p[0] = q[0] + 9 * cos(angle + pi / 4)
  p[1] = q[1] + 9 * sin(angle + pi / 4)
  cv2.line(img, (int(p[0]), int(p[1])), (int(q[0]), int(q[1])), color, 3, cv2.LINE_AA)
 
  p[0] = q[0] + 9 * cos(angle - pi / 4)
  p[1] = q[1] + 9 * sin(angle - pi / 4)
  cv2.line(img, (int(p[0]), int(p[1])), (int(q[0]), int(q[1])), color, 3, cv2.LINE_AA)
  ## [visualization1]
 
def getOrientation(pts, img):
  ## [pca]
  # Construct a buffer used by the pca analysis
  sz = len(pts)
  data_pts = np.empty((sz, 2), dtype=np.float64)
  for i in range(data_pts.shape[0]):
    data_pts[i,0] = pts[i,0,0]
    data_pts[i,1] = pts[i,0,1]
 
  # Perform PCA analysis
  mean = np.empty((0))
  mean, eigenvectors, eigenvalues = cv2.PCACompute2(data_pts, mean)
 
  # Store the center of the object
  cntr = (int(mean[0,0]), int(mean[0,1]))
  ## [pca]
 
  ## [visualization]
  # Draw the principal components
  cv2.circle(img, cntr, 3, (255, 0, 255), 2)
  p1 = (cntr[0] + 0.02 * eigenvectors[0,0] * eigenvalues[0,0], cntr[1] + 0.02 * eigenvectors[0,1] * eigenvalues[0,0])
  p2 = (cntr[0] - 0.02 * eigenvectors[1,0] * eigenvalues[1,0], cntr[1] - 0.02 * eigenvectors[1,1] * eigenvalues[1,0])
  drawAxis(img, cntr, p1, (255, 255, 0), 1)
  drawAxis(img, cntr, p2, (0, 0, 255), 5)
 
  angle = atan2(eigenvectors[0,1], eigenvectors[0,0]) # orientation in radians
  ## [visualization]
  print(angle)
 
  # Label with the rotation angle
  #label = "  Rotation Angle: " + str(-int(np.rad2deg(angle)) - 90) + " degrees"
  #textbox = cv2.rectangle(img, (cntr[0], cntr[1]-25), (cntr[0] + 250, cntr[1] + 10), (255,255,255), -1)
  #cv2.putText(img, label, (cntr[0], cntr[1]), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0,0,0), 1, cv2.LINE_AA)
 
  return angle, cntr


def main():
    """main"""    

    global variable
    global time_duration
    global lst
    global s
    global email_ext
    global time_toweb
    global jpeg_quality
    global reg_string
    global areaC
    global areaT
    global areaR
    global areaL
    global areaX
    global angles
    global angles_ct
    global positions_x
    global positions_y
    global areaALL
    global mode
    global threshold_val
    global bool_flask   
    global threshold_val_intern
    global threshold_val_feedback
    global img_scale
    global point_resol
    global erode_cont
    global c_count_arr

    while not  __close_app:


        if variable==0:

            SDKVersion = MvCamera.MV_CC_GetSDKVersion()
            print("SDKVersion[0x%x]" % SDKVersion)
            deviceList = MV_CC_DEVICE_INFO_LIST()
            tlayerType = MV_GIGE_DEVICE | MV_USB_DEVICE

            # ch:枚举设备 | en:Enum device
            ret = MvCamera.MV_CC_EnumDevices(tlayerType, deviceList)
            if ret != 0:
                print("enum devices fail! ret[0x%x]" % ret)
                sys.exit()

            if deviceList.nDeviceNum == 0:
                print("find no device!")
                sys.exit()

            print("Find %d devices!" % deviceList.nDeviceNum)

            for i in range(0, deviceList.nDeviceNum):
                mvcc_dev_info = cast(deviceList.pDeviceInfo[i], POINTER(MV_CC_DEVICE_INFO)).contents
                if mvcc_dev_info.nTLayerType == MV_GIGE_DEVICE:
                    print("\ngige device: [%d]" % i)
                    strModeName = ""
                    for per in mvcc_dev_info.SpecialInfo.stGigEInfo.chModelName:
                        strModeName = strModeName + chr(per)
                    print("device model name: %s" % strModeName)

                    nip1 = ((mvcc_dev_info.SpecialInfo.stGigEInfo.nCurrentIp & 0xff000000) >> 24)
                    nip2 = ((mvcc_dev_info.SpecialInfo.stGigEInfo.nCurrentIp & 0x00ff0000) >> 16)
                    nip3 = ((mvcc_dev_info.SpecialInfo.stGigEInfo.nCurrentIp & 0x0000ff00) >> 8)
                    nip4 = (mvcc_dev_info.SpecialInfo.stGigEInfo.nCurrentIp & 0x000000ff)
                    print("current ip: %d.%d.%d.%d\n" % (nip1, nip2, nip3, nip4))
                elif mvcc_dev_info.nTLayerType == MV_USB_DEVICE:
                    print("\nu3v device: [%d]" % i)
                    strModeName = ""
                    for per in mvcc_dev_info.SpecialInfo.stUsb3VInfo.chModelName:
                        if per == 0:
                            break
                        strModeName = strModeName + chr(per)
                    print("device model name: %s" % strModeName)

                    strSerialNumber = ""
                    for per in mvcc_dev_info.SpecialInfo.stUsb3VInfo.chSerialNumber:
                        if per == 0:
                            break
                        strSerialNumber = strSerialNumber + chr(per)
                    print("user serial number: %s" % strSerialNumber)

            nConnectionNum = 0

            if int(nConnectionNum) >= deviceList.nDeviceNum:
                print("intput error!")
                sys.exit()

            # ch:创建相机实例 | en:Creat Camera Object
            cam = MvCamera()

            # ch:选择设备并创建句柄| en:Select device and create handle
            stDeviceList = cast(deviceList.pDeviceInfo[int(nConnectionNum)], POINTER(MV_CC_DEVICE_INFO)).contents


            ret = cam.MV_CC_CreateHandle(stDeviceList)
            if ret != 0:
                print("create handle fail! ret[0x%x]" % ret)
                sys.exit()

            # ch:打开设备 | en:Open device
            ret = cam.MV_CC_OpenDevice(MV_ACCESS_Exclusive, 0)
            if ret != 0:
                print("open device fail! ret[0x%x]" % ret)
                sys.exit()

            # ch:探测网络最佳包大小(只对GigE相机有效) | en:Detection network optimal package size(It only works for the GigE camera)
            if stDeviceList.nTLayerType == MV_GIGE_DEVICE:
                nPacketSize = cam.MV_CC_GetOptimalPacketSize()
                if int(nPacketSize) > 0:
                    ret = cam.MV_CC_SetIntValue("GevSCPSPacketSize", nPacketSize)
                    if ret != 0:
                        print("Warning: Set Packet Size fail! ret[0x%x]" % ret)
                else:
                    print("Warning: Get Packet Size fail! ret[0x%x]" % nPacketSize)

            # ch:设置触发模式为off | en:Set trigger mode as off
            ret = cam.MV_CC_SetEnumValue("TriggerMode", MV_TRIGGER_MODE_OFF)
            if ret != 0:
                print("set trigger mode fail! ret[0x%x]" % ret)
                sys.exit()

            # ch:获取数据包大小 | en:Get payload size
            stParam = MVCC_INTVALUE()
            memset(byref(stParam), 0, sizeof(MVCC_INTVALUE))

            ret = cam.MV_CC_GetIntValue("PayloadSize", stParam)
            if ret != 0:
                print("get payload size fail! ret[0x%x]" % ret)
                sys.exit()
            nPayloadSize = stParam.nCurValue

            
          
            variable = 99    



        if mode == 1 and threshold_val == 2: #set auto mode from PLC
            threshold_val_intern = 1
        else:
            threshold_val_intern = 0

        if threshold_val_intern == 1:
            variable = 1
            sleep(0.5)
            threshold_val_feedback = 1
        else:
            threshold_val_feedback = 0
        

        if variable == 1:

            # ch:开始取流 | en:Start grab image
            ret = cam.MV_CC_StartGrabbing()
            if ret != 0:
                print("start grabbing fail! ret[0x%x]" % ret)
                sys.exit()
        
            stOutFrame = MV_FRAME_OUT()
            memset(byref(stOutFrame), 0, sizeof(stOutFrame))

            ret = cam.MV_CC_GetImageBuffer(stOutFrame, 1000)
            time_start=time()

            if None != stOutFrame.pBufAddr and 0 == ret:
                print("get one frame: Width[%d], Height[%d], nFrameNum[%d],nFrameLen[%d]" % (
                    stOutFrame.stFrameInfo.nWidth, stOutFrame.stFrameInfo.nHeight, stOutFrame.stFrameInfo.nFrameNum, stOutFrame.stFrameInfo.nFrameLen))

                buf_cache = (c_ubyte * stOutFrame.stFrameInfo.nFrameLen)()
            # 图像数据拷贝
                memmove(byref(buf_cache), stOutFrame.pBufAddr, stOutFrame.stFrameInfo.nFrameLen)
                if PixelType_Gvsp_Mono8 == stOutFrame.stFrameInfo.enPixelType:
                    g_numArray = Mono_numpy(buf_cache, stOutFrame.stFrameInfo.nWidth, stOutFrame.stFrameInfo.nHeight)
                    g_numArray = g_numArray.reshape(stOutFrame.stFrameInfo.nHeight, stOutFrame.stFrameInfo.nWidth, 1)
                elif PixelType_Gvsp_RGB8_Packed == stOutFrame.stFrameInfo.enPixelType:
                    g_numArray = Color_numpy(buf_cache, stOutFrame.stFrameInfo.nWidth, stOutFrame.stFrameInfo.nHeight)
                    g_numArray = g_numArray.reshape(stOutFrame.stFrameInfo.nHeight, stOutFrame.stFrameInfo.nWidth, 3)
                else:
                    print("Not Support")
                    sys.exit()            
            
        
                cv2.imwrite('/var/snap/rexroth-solutions/common/solutions/activeConfiguration/contour/Image_Mat.bmp', g_numArray, [cv2.IMWRITE_JPEG_QUALITY, jpeg_quality])
                img2 = cv2.imread('/var/snap/rexroth-solutions/common/solutions/activeConfiguration/contour/Image_Mat.bmp')
             

             
                points_list = [[]]
                cc = [[]]

                img3 = cv2.bitwise_not(img2) #invert
                img4 = cv2.cvtColor(img3,cv2.COLOR_BGR2GRAY)
                gray = np.float32(img4)

                _, gray2 = cv2.threshold(img4, 220, 255, 0)
                # Creating kernel 
                kernel = np.ones((erode_cont, erode_cont), np.uint8) 

                # Using cv2.erode() method  
                gray2 = cv2.erode(gray2, kernel, cv2.BORDER_REFLECT)  
                cnts, hierarchy = cv2.findContours(gray2, cv2.RETR_TREE, cv2.CHAIN_APPROX_NONE)

                all_points = [[]]
                xx = []
                yy = []

                counter = 0
                c_count = 0

                for i, c in enumerate(cnts):
                    area = cv2.contourArea(c)
                    c_count += 1
                    #c_count_arr[c_count] = area
                    #c_count_arr.append(area)
                    print("Contours: ", c_count)
                    for point in c:
                        x, y = point[0]
                        if counter%point_resol==0:
                            all_points.append([int(x), int(y)])
                            xx.append(int(x))
                            yy.append(int(-y))
                            cv2.circle(img3,(int(x),int(y)),3,(255, 0, 255),-1)
                        counter += 1
                    xx.append(9999999)
                    yy.append(9999999)
                
                c_count = 0
                counter = 0

                cv2.imwrite("/var/snap/rexroth-solutions/common/solutions/activeConfiguration/contour/Image_Mat.jpg", img3)

                print(all_points)

                send_arr = []

                for cc in all_points:
                    for pp in cc:
                        send_arr.append(int(pp))

                print("X: ", xx)
                print("Y: ", yy)
                print("All: ", all_points)
                print("Send arr: ", send_arr)
                #areaALL = float(send_arr)

                xx2 = [float(i) for i in xx]
                yy2 = [float(i) for i in yy]

                positions_x = xx2
                positions_y = yy2

                test = [1,2,3,4]

                #print(sys.getsizeof(send_arr))
                #print(sys.getsizeof(test))


                time_end=time()
                time_full=time_end-time_start
    
                time_duration=round(time_full, 3)

                print("Time to execute: ", time_duration)


                nRet = cam.MV_CC_FreeImageBuffer(stOutFrame)

                # ch:停止取流 | en:Stop grab image
                ret = cam.MV_CC_StopGrabbing()
                if ret != 0:
                    print ("stop grabbing fail! ret[0x%x]" % ret)
                    del data_buf
                    sys.exit()

        
                    
                variable = 99
                email_ext = lst
                time_toweb = time_duration
            
            else:
                print("no data[0x%x]" % ret)

            if mode == 1:
                sleep(1.0)
                bool_flask = 1
            
            variable = 99
        if variable == 2:

            """
            server = ModbusServer("192.168.1.2", 5023, no_block=True)
            print("Starting Server...")
            server.start()
            print("Server is online")
            state = [0]

            print(type(test))

            res = bytes(send_arr, 'utf-8')
            res = send_arr
            server.data_bank.set_holding_registers(0, send_arr)
            print(send_arr)
            """
            img2 = cv2.imread('/var/snap/rexroth-solutions/common/solutions/activeConfiguration/contour/Image_Mat.bmp')
             
            points_list = [[]]
            cc = [[]]

            img3 = cv2.bitwise_not(img2) #invert
            img4 = cv2.cvtColor(img3,cv2.COLOR_BGR2GRAY)
            gray = np.float32(img4)

            _, gray2 = cv2.threshold(img4, 220, 255, 0)
            # Creating kernel 
            kernel = np.ones((erode_cont, erode_cont), np.uint8) 

            # Using cv2.erode() method  
            gray2 = cv2.erode(gray2, kernel, cv2.BORDER_REFLECT)  
            cnts, hierarchy = cv2.findContours(gray2, cv2.RETR_TREE, cv2.CHAIN_APPROX_NONE)

            all_points = [[]]
            xx = []
            yy = []

            c_count = 0
            counter = 0

            for i, c in enumerate(cnts):
                area = cv2.contourArea(c)
                c_count += 1
                #c_count_arr[c_count] = area
                #c_count_arr.append(area)
                print("Contours: ", c_count)
                for point in c:
                    x, y = point[0]
                    if counter%point_resol==0:
                        all_points.append([int(x), int(y)])
                        xx.append(int(x))
                        yy.append(int(-y))
                        cv2.circle(img3,(int(x),int(y)),3,(255, 0, 255),-1)
                    counter += 1
                xx.append(9999999)
                yy.append(9999999)

            c_count = 0
            counter = 0

            cv2.imwrite("/var/snap/rexroth-solutions/common/solutions/activeConfiguration/contour/Image_Mat.jpg", img3)

            print(all_points)

            send_arr = []

            for cc in all_points:
                for pp in cc:
                    send_arr.append(int(pp))

            print("X: ", xx)
            print("Y: ", yy)
            print("All: ", all_points)
            print("Send arr: ", send_arr)
            #areaALL = float(send_arr)

            xx2 = [float(i) for i in xx]
            yy2 = [float(i) for i in yy]

            positions_x = xx2
            positions_y = yy2

            test = [1,2,3,4]

            #print(sys.getsizeof(send_arr))
            #print(sys.getsizeof(test))



            variable = 99

        if variable == 3:
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
                
            variable = 99
        
        



if __name__ == "__main__":
    sig()
    try:
        logger.info(f'First thread')
        t1 = threading.Thread(target=startApp).start()
        logger.info(f'Second thread')
        t2 = threading.Thread(target=main).start()
        logger.info(f'Third thread')
        t3 = threading.Thread(target=provide_to_DL).start()
        sleep(5.0)
        logger.info(f'Fourth thread')
        t4 = threading.Thread(target=dl_client).start()


        

        
    except Exception as e:
        logger.error("Error: " + str(e))  

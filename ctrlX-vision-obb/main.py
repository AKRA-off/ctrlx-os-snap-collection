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

import subprocess

import csv

#from pypylon import pylon, genicam #Basler
#from utils import xywh2xyxy, draw_detections, multiclass_nms, class_init
from yoloseg.utils import xywh2xyxy, nms, draw_detections, sigmoid, class_init


from waitress import serve
from gevent import pywsgi

import yaml

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


import ctrlxdatalayer
import flatbuffers

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

from ctrlxdatalayer.variant import Result, Variant, VariantType
import random
from ctrlxdatalayer.provider import Provider


from datetime import datetime


from yoloseg.YOLOSeg import YOLOSeg
import glob

from instance_segmentation import run_inference_pipeline
from pathlib import Path

logger = logging.getLogger(__name__)

__close_app = False


global g_numArray
g_numArray = None

address_base = "AI_Detector/data/"

MEDIA_FOLDER = '/var/snap/rexroth-solutions/common/solutions/activeConfiguration/AI_Detector/'
general_path = '/var/snap/rexroth-solutions/common/solutions/activeConfiguration/AI_Detector/'

#MEDIA_FOLDER = '/snap/ai-detector/current/'
#general_path = '/snap/ai-detector/current/'


variable = [99, 99, 99, 99, 99] #
bool_flask = [0, 0, 0, 0, 0]
model_name = [".onnx", ".onnx", ".onnx", ".onnx", ".onnx"]
model_name_temp = [".onnx", ".onnx", ".onnx", ".onnx", ".onnx"]

output_boxes0 = [0.0, 0.0, 0.0, 0.0, 0.0]
output_score0 = [0.0, 0.0, 0.0, 0.0, 0.0]
output_classes0 = [1,1,1,1,1]
output_qty = 0
output_angles = []  # OBB mode: per-detection angle in degrees (0-360 with heading model)

# Latest inference results for the Data Layer provider. Replaced as a
# whole dict (atomic under the GIL); "seq" tells the provider thread
# that new data is ready to publish.
dl_results = {"seq": 0, "classes": [], "names": [], "scores": [],
              "x": [], "y": [], "angles": [], "calibrated": False}

# Data Layer branch per processing mode: AI_Detector/data/<branch>/...
MODE_BRANCH = {"segmentation": "Seg", "obb": "OBB", "detection": "Det"}


def get_calibration(camid):
    """Loaded Calibration for this camera or None (cached by file)."""
    import calibration as calib_mod
    path = MEDIA_FOLDER + f"calib{camid}.yaml"
    if not os.path.isfile(path):
        _calib_cache["path"] = None
        _calib_cache["obj"] = None
        return None
    if _calib_cache["path"] != path or _calib_cache["obj"] is None:
        try:
            _calib_cache["obj"] = calib_mod.Calibration.load(path)
            _calib_cache["path"] = path
            print(f"Calibration loaded: calib{camid}.yaml", flush=True)
        except Exception as e:
            print(f"Calibration load failed: {e}", flush=True)
            return None
    return _calib_cache["obj"]


def publish_dl_results(class_ids, scores, xs, ys, angles, camid=0):
    """Store one inference's results for the Data Layer provider thread.
    scores are published as percent. With calibration enabled, positions
    become world mm and angles world degrees. Empty lists clear."""
    global dl_results

    xs = [float(v) for v in xs]
    ys = [float(v) for v in ys]
    angles = [float(a) for a in angles]

    calib = get_calibration(camid) if use_calibration[camid] else None
    if calib is not None and xs:
        angles = [calib.angle_to_world(x, y, a)
                  for x, y, a in zip(xs, ys, angles)] if angles else []
        xs, ys = calib.px_to_world(xs, ys)

    names = get_class_names(camid)
    dl_results = {
        "seq": dl_results["seq"] + 1,
        "classes": [int(c) for c in class_ids],
        "names": [names[int(c)] if names and 0 <= int(c) < len(names)
                  else str(int(c)) for c in class_ids],
        "scores": [round(float(s) * 100.0, 1) for s in scores],
        "x": [round(v, 2) for v in xs],
        "y": [round(v, 2) for v in ys],
        "angles": [round(a, 1) for a in angles],
        "calibrated": calib is not None,
    }

# OBB 360: heading classifier ONNX per camera; "" or "none" disables (angles stay 180-ambiguous)
heading_model_name = ["heading.onnx", "heading.onnx", "heading.onnx", "heading.onnx", "heading.onnx"]

# Class-names yaml per camera (selectable in settings - no more renaming
# uploads to data0.yaml)
class_yaml_name = ["data0.yaml", "data1.yaml", "data2.yaml", "data3.yaml", "data4.yaml"]

# Preloaded-image mode: which file from the media folder to run on
# (selectable on the home page; copied to pre<camid>.jpg before inference)
preload_image_name = ["", "", "", "", ""]

# Optional 3D viewer iframe on the home page. Only the PATH is stored;
# the page prepends https://<host the browser is connected to>/ so it
# works on any device IP without reconfiguration.
iframe_show = False
iframe_path = "3dviewer/Demo?id=3dv1"

# Chessboard calibration (px -> world mm); see calibration.py
calib_pattern_w = 9        # inner corners per row
calib_pattern_h = 6        # inner corners per column
calib_square_mm = 25.0     # chessboard square size
use_calibration = [False, False, False, False, False]  # publish mm instead of px

# CLAHE lighting normalization (yoloobb/preprocess.py). Toggle in settings;
# only enable with a model TRAINED on CLAHE data, else input/model mismatch.
use_clahe = [False]

# Training-data logger: when on, every inference saves the raw capture to
# <logger_path>/Logger/Shot and the drawn result to <logger_path>/Logger/
# Process (timestamped). logger_path empty = the media/solutions folder;
# set it to a removable-media mount (e.g. /media/...) to log to a USB stick.
logger_enabled = [False]
logger_path = [""]
_calib_cache = {"path": None, "obj": None}
_calib_pending = None      # detection awaiting the 4-corner world table

initialize_var = True
hmi_conf_thres = [0.3, 0.3, 0.3, 0.3, 0.3]
hmi_iou_thres = [0.5, 0.5, 0.5, 0.5, 0.5]
hmi_password = "boschrexroth"
hmi_password_temp = ""
disc_hik = [0, 0, 0, 0, 0]
conn_hik = [0, 0, 0, 0, 0]
time_toweb = [0.0, 0,0, 0,0, 0,0, 0,0]
time_start = 0
save_config = 0
camera_qty = 1
hmi_cam_settings = [0, 0, 0, 0, 0]
step = 99
log=0
hik_connected=0
hik_conn_arr = [0,0,0,0,0]
web_conn_arr = [0,0,0,0,0]
hik_counter = 0
web_counter = 0
webcam_width = [1280, 1280, 1280, 1280, 1280]
webcam_height = [720, 720, 720, 720, 720]

# HAILO accelerator settings
use_hailo = [False, False, False, False, False]  # Enable/disable HAILO mode per camera

# Processing mode settings
processing_mode = ["segmentation", "segmentation", "segmentation", "segmentation", "segmentation"]  # Options: "segmentation", "obb", "detection"


app = Flask(__name__)
app.secret_key = b'_1#y2l"F4Q8z\n\xec]/'

APP_URL_PREFIX = os.environ.get("APP_URL_PREFIX", "/aidetector").rstrip("/")


class PrefixMiddleware:
    """Allow the app to run both directly and behind a path-prefix proxy."""

    def __init__(self, wrapped_app, prefix):
        self.wrapped_app = wrapped_app
        self.prefix = prefix

    def __call__(self, environ, start_response):
        path = environ.get("PATH_INFO", "")
        if self.prefix and (
            path == self.prefix or path.startswith(self.prefix + "/")
        ):
            environ["SCRIPT_NAME"] = self.prefix
            environ["PATH_INFO"] = path[len(self.prefix):] or "/"
        return self.wrapped_app(environ, start_response)


if APP_URL_PREFIX:
    app.wsgi_app = PrefixMiddleware(app.wsgi_app, APP_URL_PREFIX)

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
    global camera_qty
    global output_qty

    if log == 0:
        return render_template("login.html")
    else:
        if request.method == "POST":
            try:
                if request.form.get("bool_flask"):
                    bool_flask[0] = int(request.form.get("bool_flask"))
                    return redirect(url_for("index"))
                elif "preload_image" in request.form:
                    sel = str(request.form.get("preload_image"))
                    if sel == "" or os.path.isfile(MEDIA_FOLDER + sel):
                        preload_image_name[0] = sel  # "" = last capture
                    return redirect(url_for("index"))
                elif request.form.get("time_toweb"):
                    time_toweb[0] = float(request.form.get("time_toweb"))
                    return redirect(url_for("settings"))

            except:
                print("Wrong data type")
            return redirect(url_for("index"))

        # Preloaded-image mode: offer the image files in the media folder
        preload_images = []
        if variable[0] == 3:
            skip = ("inf", "calibprev", "temp_")
            for f in sorted(os.listdir(MEDIA_FOLDER)):
                if f.lower().endswith((".jpg", ".jpeg", ".png", ".bmp")) \
                        and not f.startswith(skip):
                    preload_images.append(f)

        return render_template('index.html',
                             bool_flask=bool_flask[0],
                             time_toweb=time_toweb[0],
                             output_qty=output_qty,
                             iframe_show=iframe_show,
                             iframe_path=iframe_path,
                             variable=variable[0],
                             preload_images=preload_images,
                             preload_selected=preload_image_name[0])

@app.route('/api/detection_data', methods=['GET'])
def get_detection_data():
    """API endpoint to get current detection data"""
    global output_qty
    global output_angles
    global bool_flask
    global time_toweb

    res = dl_results
    names = get_class_names(0)
    detections = []
    for i in range(len(res["classes"])):
        cid = res["classes"][i]
        detections.append({
            'cls': names[cid] if names and 0 <= cid < len(names) else str(cid),
            'score': res["scores"][i],
            'x': res["x"][i],
            'y': res["y"][i],
            'angle': res["angles"][i] if i < len(res["angles"]) else None,
        })

    return {
        'is_processing': bool_flask[0] == 1,
        'time_toweb': time_toweb[0],
        'output_qty': output_qty,
        'angles': output_angles,
        'detections': detections,
        'calibrated': bool(res.get("calibrated", False)),
        'timestamp': time()
    }

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
    global camera_qty
    global hmi_cam_settings
    global MEDIA_FOLDER
    global hik_connected
    global calib_pattern_w
    global calib_pattern_h
    global calib_square_mm
    global _calib_pending
    global iframe_show
    global iframe_path
    global web_conn_arr
    global initialize_var
    global webcam_width
    global webcam_height
    global use_hailo
    global processing_mode


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
                elif request.form.get("heading_model_name"):
                    tempstr = str(request.form.get("heading_model_name")).strip()
                    if tempstr.lower() in ("none", "off", "disable", "-"):
                        heading_model_name[0] = ""  # 360 resolution disabled
                    elif os.path.exists(MEDIA_FOLDER + tempstr):
                        heading_model_name[0] = tempstr
                    else:
                        print("No heading model file found in directory, keeping previous")
                    return redirect(url_for("settings"))
                elif request.form.get("class_yaml_name"):
                    tempstr = str(request.form.get("class_yaml_name")).strip()
                    if os.path.exists(MEDIA_FOLDER + tempstr):
                        class_yaml_name[0] = tempstr
                    else:
                        print("No class yaml found in directory, keeping previous")
                    return redirect(url_for("settings"))
                elif request.form.get("calib_detect"):
                    import calibration as calib_mod
                    calib_pattern_w = int(request.form.get("calib_pw", calib_pattern_w))
                    calib_pattern_h = int(request.form.get("calib_ph", calib_pattern_h))
                    calib_square_mm = float(request.form.get("calib_sq", calib_square_mm))
                    # Take a fresh shot with the selected camera, then detect
                    bool_flask[0] = 1
                    for _ in range(100):  # up to 20 s
                        if bool_flask[0] == 0:
                            break
                        sleep(0.2)
                    try:
                        img = cv2.imread(MEDIA_FOLDER + "pre0.jpg")
                        if img is None:
                            raise ValueError("No captured image found")
                        det = calib_mod.detect_board(
                            img, calib_pattern_w, calib_pattern_h)
                        cv2.imwrite(MEDIA_FOLDER + "calibprev0.jpg",
                                    calib_mod.draw_preview(
                                        img, det, calib_pattern_w,
                                        calib_pattern_h))
                        _calib_pending = {
                            "detection": det,
                            "table": calib_mod.default_world_table(
                                det, calib_pattern_w, calib_pattern_h,
                                calib_square_mm),
                        }
                        print("Calibration: board detected", flush=True)
                    except Exception as e:
                        _calib_pending = {"error": str(e)}
                        print(f"Calibration detect failed: {e}", flush=True)
                    return redirect(url_for("settings"))
                elif request.form.get("calib_save"):
                    import calibration as calib_mod
                    if _calib_pending and "detection" in _calib_pending:
                        try:
                            table = {}
                            for lbl in ("TL", "TR", "BL", "BR"):
                                table[lbl] = (
                                    float(request.form.get(f"calib_{lbl}_x")),
                                    float(request.form.get(f"calib_{lbl}_y")))
                            H, rms_b, rms_a = calib_mod.build_calibration(
                                _calib_pending["detection"], calib_pattern_w,
                                calib_pattern_h, calib_square_mm, table)
                            c = calib_mod.Calibration(H, meta={
                                "pattern": [calib_pattern_w, calib_pattern_h],
                                "square_mm": calib_square_mm,
                                "world_table": {k: list(v) for k, v in table.items()},
                                "rms_board_mm": round(rms_b, 3),
                                "rms_anchor_mm": round(rms_a, 3)})
                            c.save(MEDIA_FOLDER + "calib0.yaml")
                            # Keep the board photo of THIS calibration
                            # (calibprev0 gets overwritten by later detects)
                            try:
                                shutil.copy(MEDIA_FOLDER + "calibprev0.jpg",
                                            MEDIA_FOLDER + "calibsaved0.jpg")
                            except OSError:
                                pass
                            _calib_cache["path"] = None  # force reload
                            _calib_pending = None
                            print(f"Calibration saved (board rms {rms_b:.2f} mm, "
                                  f"anchor rms {rms_a:.2f} mm)", flush=True)
                        except Exception as e:
                            _calib_pending["error"] = str(e)
                            print(f"Calibration save failed: {e}", flush=True)
                    return redirect(url_for("settings"))
                elif request.form.get("use_clahe") is not None and \
                        request.form.get("use_clahe") != "":
                    use_clahe[0] = int(request.form.get("use_clahe")) == 1
                    return redirect(url_for("settings"))
                elif request.form.get("logger_enabled") is not None and \
                        request.form.get("logger_enabled") != "":
                    logger_enabled[0] = int(request.form.get("logger_enabled")) == 1
                    return redirect(url_for("settings"))
                elif request.form.get("logger_path") is not None:
                    logger_path[0] = str(request.form.get("logger_path")).strip()
                    return redirect(url_for("settings"))
                elif request.form.get("use_calibration") is not None and \
                        request.form.get("use_calibration") != "":
                    use_calibration[0] = int(request.form.get("use_calibration")) == 1
                    return redirect(url_for("settings"))
                elif request.form.get("calib_adjust"):
                    import calibration as calib_mod
                    c = get_calibration(0)
                    if c is not None:
                        try:
                            table = {}
                            for lbl in ("TL", "TR", "BL", "BR"):
                                table[lbl] = (
                                    float(request.form.get(f"calib_{lbl}_x")),
                                    float(request.form.get(f"calib_{lbl}_y")))
                            new_c, rms = calib_mod.adjust_world_table(c, table)
                            new_c.save(MEDIA_FOLDER + "calib0.yaml")
                            _calib_cache["path"] = None  # force reload
                            print(f"Calibration re-anchored "
                                  f"(delta rms {rms:.2f} mm)", flush=True)
                        except Exception as e:
                            print(f"Calibration adjust failed: {e}",
                                  flush=True)
                    return redirect(url_for("settings"))
                elif request.form.get("iframe_set"):
                    tempstr = str(request.form.get("iframe_path", "")).strip()
                    if tempstr:
                        iframe_path = tempstr.lstrip("/")
                    return redirect(url_for("settings"))
                elif request.form.get("iframe_show") is not None and \
                        request.form.get("iframe_show") != "":
                    iframe_show = int(request.form.get("iframe_show")) == 1
                    return redirect(url_for("settings"))
                elif request.form.get("calib_clear"):
                    for fname in ("calib0.yaml", "calibsaved0.jpg"):
                        try:
                            os.remove(MEDIA_FOLDER + fname)
                        except OSError:
                            pass
                    _calib_cache["path"] = None
                    _calib_cache["obj"] = None
                    use_calibration[0] = False
                    _calib_pending = None
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
                elif request.form.get("camera_qty"):
                    camera_qty = int(request.form.get("camera_qty"))
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
                elif request.form.get("webcam_width"):
                    webcam_width[0] = int(request.form.get("webcam_width"))
                    return redirect(url_for("settings"))
                elif request.form.get("webcam_height"):
                    webcam_height[0] = int(request.form.get("webcam_height"))
                    return redirect(url_for("settings"))
                elif request.form.get("use_hailo") is not None:
                    use_hailo[0] = int(request.form.get("use_hailo")) == 1
                    return redirect(url_for("settings"))
                elif request.form.get("processing_mode"):
                    mode = str(request.form.get("processing_mode"))
                    if mode in ["segmentation", "obb", "detection"]:
                        processing_mode[0] = mode
                    return redirect(url_for("settings"))



            except:
                print("Wrong data type")
            return redirect(url_for("settings"))

        ##

        calib_exists = os.path.isfile(MEDIA_FOLDER + "calib0.yaml")
        calib_meta = {}
        if calib_exists:
            c = get_calibration(0)
            if c is not None:
                calib_meta = c.meta
        calib_photo = os.path.isfile(MEDIA_FOLDER + "calibsaved0.jpg")
        return render_template('settings.html', hik_connected=hik_connected, web_conn_arr=web_conn_arr[0], bool_flask=bool_flask[0], variable=variable[0], model_name=model_name[0], heading_model_name=(heading_model_name[0] or "none"), directories=directories, subdirectories=subdirectories, files=files, hmi_conf_thres=hmi_conf_thres[0], hmi_iou_thres=hmi_iou_thres[0], hmi_password=hmi_password, disc_hik=disc_hik[0], conn_hik=conn_hik[0], save_config=save_config, camera_qty=camera_qty, webcam_width=webcam_width[0], webcam_height=webcam_height[0], use_hailo=use_hailo[0], processing_mode=processing_mode[0], calib_pattern_w=calib_pattern_w, calib_pattern_h=calib_pattern_h, calib_square_mm=calib_square_mm, calib_pending=_calib_pending, calib_exists=calib_exists, calib_meta=calib_meta, use_calibration=use_calibration[0], iframe_show=iframe_show, iframe_path=iframe_path, class_yaml_name=class_yaml_name[0], calib_photo=calib_photo, use_clahe=use_clahe[0], logger_enabled=logger_enabled[0], logger_path=logger_path[0])

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
    try:
        # basename strips any path components from the browser-supplied
        # filename (path traversal hardening)
        safe_name = os.path.basename(file.filename.replace("\\", "/"))
        if not safe_name:
            return 'Invalid file name'
        file.save(os.path.join(dir_path, safe_name))
        return redirect(url_for("settings"))
    except Exception:
        return 'Unable to upload file'
    
#Delete file    
@app.route('/delete_file', methods=['POST'])
def delete_file():
    directory = request.form['file_path']
    if ".." in directory.replace("\\", "/").split("/"):
        return 'Invalid path'
    file_path = general_path + directory
    try:
        if file_path != (MEDIA_FOLDER + 'yolov8n-seg.onnx'):
            print("Removing ", file_path)
            os.remove(file_path)
        else:
            print("Cannot Remove Default Model yolov8n-seg.onnx")
        return redirect(url_for("settings"))
    except:
        return 'Unable to delete file'


def startApp():

    #serve(app,host='0.0.0.0',port=8021)
###cr
    # In ctrlX deployments the reverse proxy terminates HTTPS and forwards
    # plain HTTP to this service. Direct-port HTTPS remains opt-in for tests.
    port = int(os.environ.get("APP_PORT", "8021"))
    direct_https = os.environ.get("APP_ENABLE_DIRECT_HTTPS", "0") == "1"
    cert_path = os.environ.get("SSL_CERT_FILE", os.path.join(MEDIA_FOLDER, "server.crt"))
    key_path = os.environ.get("SSL_KEY_FILE", os.path.join(MEDIA_FOLDER, "server.key"))

    try:
        if direct_https and os.path.exists(cert_path) and os.path.exists(key_path):
            logger.info(f"Starting HTTPS server on port {port} (cert: {cert_path})")
            http_server = pywsgi.WSGIServer(('0.0.0.0', port), app, keyfile=key_path, certfile=cert_path)
            http_server.serve_forever()
        else:
            logger.info(f"Starting HTTP server on port {port}")
            serve(app, host='0.0.0.0', port=port)
    except Exception as e:
        logger.error(f"Failed to start server: {e}")


def generate_self_signed_cert(common_name: str = "localhost", days: int = 825):
    """
    Generate a self-signed cert/key pair in MEDIA_FOLDER using openssl.
    Returns (cert_path, key_path).
    """
    cert_path = os.path.join(MEDIA_FOLDER, "server.crt")
    key_path = os.path.join(MEDIA_FOLDER, "server.key")

    if os.path.exists(cert_path) and os.path.exists(key_path):
        logger.info("Existing certificate and key found; skipping generation.")
        return cert_path, key_path

    os.makedirs(MEDIA_FOLDER, exist_ok=True)

    subj = f"/CN={common_name}"
    san = f"subjectAltName=DNS:{common_name},IP:127.0.0.1"

    try:
        # One-shot key + cert generation (OpenSSL 1.1.1+/3.0+ supports -addext)
        subprocess.run([
            "openssl", "req", "-x509", "-nodes", "-newkey", "rsa:2048",
            "-keyout", key_path,
            "-out", cert_path,
            "-days", str(days),
            "-subj", subj,
            "-addext", san
        ], check=True)
        # Restrict key permissions
        try:
            os.chmod(key_path, 0o600)
        except Exception:
            pass
        logger.info(f"Generated self-signed certificate at {cert_path}")
        return cert_path, key_path
    except FileNotFoundError:
        raise RuntimeError("OpenSSL not available. Install openssl or provide SSL_CERT_FILE/SSL_KEY_FILE.")
    except subprocess.CalledProcessError as e:
        raise RuntimeError(f"OpenSSL failed: {e}")


@app.route('/generate_cert', methods=['POST'])
def generate_cert_route():
    global log
    if log == 0:
        return render_template("login.html")
    cn = request.form.get('cn', 'localhost')
    try:
        generate_self_signed_cert(common_name=cn)
        return redirect(url_for("settings"))
    except Exception as e:
        return f"Failed to generate certificate: {e}", 500
###/cr



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

    # Convenience accessors so the provider loop can read/write its own
    # nodes directly (no Data Layer client connection needed)
    def get_bool(self):
        try:
            return bool(self._data.get_bool8())
        except Exception:
            return False

    def get_int(self):
        try:
            return int(self._data.get_int16())
        except Exception:
            return 0

    def set_bool(self, value):
        v = Variant()
        v.set_bool8(bool(value))
        self.set_value(v)

    def set_int(self, value):
        v = Variant()
        v.set_int16(int(value))
        self.set_value(v)

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
                

            

            # Static nodes
            provide_node_int = provide_int(provider, "output_qty")
            provide_node_start = provide_bool(provider, "capture")
            provide_node_error = provide_bool(provider, "capture_error")
            provide_node_calib = provide_bool(provider, "calibrated")
            # Camera control from the PLC side
            provide_node_camsel = provide_int(provider, "cam_select")
            provide_node_conn = provide_bool(provider, "connect_camera")
            provide_node_disc = provide_bool(provider, "disconnect_camera")
            provide_node_camstat = provide_bool(provider, "camera_connected")

            def make_mode_nodes(branch):
                """Register result nodes under AI_Detector/data/<branch>/:
                classes, scores (percent), pos_x, pos_y (pixels),
                angles (degrees, OBB mode)."""
                base = address_base + branch + "/"
                nodes = {}
                v = Variant()
                v.set_array_int32([])
                nodes["classes"] = MyProviderNode(provider, base + "classes", v)
                v = Variant()
                v.set_array_string([])
                nodes["class_names"] = MyProviderNode(
                    provider, base + "class_names", v)
                for nm in ("scores", "pos_x", "pos_y", "angles"):
                    v = Variant()
                    v.set_array_float64([])
                    nodes[nm] = MyProviderNode(provider, base + nm, v)
                for nm, node in nodes.items():
                    res = node.register_node()
                    if res != Result.OK:
                        print(f"ERROR Registering {base}{nm} failed: {res}",
                              flush=True)
                print(f"Data Layer: providing {base}"
                      f"{{classes, scores, pos_x, pos_y, angles}}", flush=True)
                return nodes

            current_branch = None
            mode_nodes = {}
            last_seq = -1

            # PLC handshake state (all handled provider-side by reading
            # our own node variants - no Data Layer client needed)
            dl_capture_pending = False
            pending_since = 0.0
            CAPTURE_TIMEOUT_S = 60.0
            CAM_TIMEOUT_S = 30.0
            camsel_prev_node = None
            last_cam_status = None
            conn_pending_since = 0.0
            disc_pending_since = 0.0

            print("INFO Running endless loop...", flush=True)
            while provider.is_connected() and not __close_app:
                # Node branch follows the selected processing mode
                branch = MODE_BRANCH.get(processing_mode[0], "Det")
                if branch != current_branch:
                    for node in mode_nodes.values():
                        node.unregister_node()
                    mode_nodes = make_mode_nodes(branch)
                    current_branch = branch
                    last_seq = -1

                # Publish new inference results (dl_results is swapped
                # atomically by the inference thread)
                res = dl_results
                if res["seq"] != last_seq:
                    last_seq = res["seq"]
                    v = Variant()
                    v.set_array_int32(res["classes"])
                    mode_nodes["classes"].set_value(v)
                    v = Variant()
                    v.set_array_string(res.get("names", []))
                    mode_nodes["class_names"].set_value(v)
                    for nm, key in (("scores", "scores"), ("pos_x", "x"),
                                    ("pos_y", "y"), ("angles", "angles")):
                        v = Variant()
                        v.set_array_float64(res[key])
                        mode_nodes[nm].set_value(v)
                    provide_node_int.set_int(len(res["classes"]))
                    provide_node_calib.set_bool(res.get("calibrated", False))

                # ---- camera selection sync (PLC <-> web/app) ----
                node_val = provide_node_camsel.get_int()
                if camsel_prev_node is None:
                    provide_node_camsel.set_int(variable[0])
                    camsel_prev_node = variable[0]
                elif node_val != camsel_prev_node:
                    if node_val in (0, 1, 3):  # PLC wrote a new selection
                        variable[0] = node_val
                        print(f"Data Layer: camera selection -> {node_val}",
                              flush=True)
                    camsel_prev_node = node_val
                elif variable[0] != node_val:  # changed from the web UI
                    provide_node_camsel.set_int(variable[0])
                    camsel_prev_node = variable[0]

                # ---- Hik connect / disconnect (PLC-triggered) ----
                if provide_node_conn.get_bool():
                    if conn_pending_since == 0.0 and variable[0] == 0 \
                            and hik_connected == 0 and conn_hik[0] == 0:
                        print("Data Layer: connect camera", flush=True)
                        conn_hik[0] = 1
                        conn_pending_since = time()
                    elif variable[0] != 0:
                        provide_node_conn.set_bool(False)
                if conn_pending_since > 0.0:
                    if hik_connected == 1 or \
                            (time() - conn_pending_since) > CAM_TIMEOUT_S:
                        provide_node_conn.set_bool(False)
                        conn_pending_since = 0.0

                if provide_node_disc.get_bool():
                    if disc_pending_since == 0.0 and hik_connected == 1 \
                            and disc_hik[0] == 0:
                        print("Data Layer: disconnect camera", flush=True)
                        disc_hik[0] = 1
                        disc_pending_since = time()
                    elif hik_connected == 0:
                        provide_node_disc.set_bool(False)
                if disc_pending_since > 0.0:
                    if hik_connected == 0 or \
                            (time() - disc_pending_since) > CAM_TIMEOUT_S:
                        provide_node_disc.set_bool(False)
                        disc_pending_since = 0.0

                # ---- connection status mirror ----
                if last_cam_status != (hik_connected == 1):
                    last_cam_status = (hik_connected == 1)
                    provide_node_camstat.set_bool(last_cam_status)

                # ---- capture trigger handshake ----
                if provide_node_start.get_bool() and not dl_capture_pending \
                        and bool_flask[0] == 0:
                    print("Data Layer: capture triggered", flush=True)
                    dl_capture_pending = True
                    pending_since = time()
                    provide_node_error.set_bool(False)
                    bool_flask[0] = 1  # same trigger as the web UI

                if dl_capture_pending:
                    finished = bool_flask[0] == 0
                    timed_out = (time() - pending_since) > CAPTURE_TIMEOUT_S
                    if finished or timed_out:
                        provide_node_start.set_bool(False)
                        if timed_out:
                            print("Data Layer: capture TIMEOUT", flush=True)
                            provide_node_error.set_bool(True)
                            bool_flask[0] = 0  # release a stuck trigger
                        dl_capture_pending = False

                sleep(0.1)  # Seconds

            print("ERROR ctrlX Data Layer Provider is disconnected", flush=True)

            for node in mode_nodes.values():
                node.unregister_node()
            provide_node_int.unregister_node()
            del provide_node_int
            provide_node_start.unregister_node()
            del provide_node_start
            provide_node_error.unregister_node()
            del provide_node_error
            provide_node_calib.unregister_node()
            del provide_node_calib
            provide_node_camsel.unregister_node()
            del provide_node_camsel
            provide_node_conn.unregister_node()
            del provide_node_conn
            provide_node_disc.unregister_node()
            del provide_node_disc
            provide_node_camstat.unregister_node()
            del provide_node_camstat

                     
            
            #print("Unregistering", type_sampleSchema_inertialValue, end=" ", flush=True)
            #result = provider.unregister_type(type_sampleSchema_inertialValue)
            #print(result, flush=True)

            print("Stopping ctrlX Data Layer provider:", end=" ", flush=True)
            result = provider.stop()
            print(result, flush=True)
             
            
            
        # Attention: Doesn't return if any provider or client instance is still running
        stop_ok = datalayer_system.stop(False)
        print("System Stop", stop_ok, flush=True)
    ###Provider
   
    

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

_mvs_initialized = False


def mvs_initialize_once():
    """MVS SDK >= 4.x requires MV_CC_Initialize before any camera call."""
    global _mvs_initialized
    if not _mvs_initialized:
        ret = MvCamera.MV_CC_Initialize()
        if ret != 0:
            print("MV_CC_Initialize fail! ret[0x%x]" % ret, flush=True)
        else:
            _mvs_initialized = True


def save_image_to_file(stOutFrame, cam_instance, file_path):
    """Save a grabbed frame to BMP via the SDK - handles every pixel
    format internally (replaces the old Mono8/RGB8-only numpy path)."""
    stSaveParam = MV_SAVE_IMAGE_TO_FILE_PARAM_EX()
    stSaveParam.enPixelType = stOutFrame.stFrameInfo.enPixelType
    stSaveParam.nWidth = stOutFrame.stFrameInfo.nWidth
    stSaveParam.nHeight = stOutFrame.stFrameInfo.nHeight
    stSaveParam.nDataLen = stOutFrame.stFrameInfo.nFrameLen
    stSaveParam.pData = stOutFrame.pBufAddr
    stSaveParam.enImageType = MV_Image_Bmp
    stSaveParam.pcImagePath = create_string_buffer(file_path.encode('ascii'))
    stSaveParam.iMethodValue = 1
    stSaveParam.nQuality = 80
    return cam_instance.MV_CC_SaveImageToFileEx(stSaveParam)


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


def find_working_webcam_device():
    """
    Searches /dev/video* and returns the first working device path.
    """
    for node in glob.glob("/dev/video*"):
        cap = cv2.VideoCapture(node, cv2.CAP_V4L2)
        if not cap.isOpened():
            continue
        ret, _ = cap.read()
        cap.release()
        if ret:
            print(f"Found working webcam device: {node}", flush=True)
            return node
    raise RuntimeError("No working video devices found")


def init_webcam(device_path, width=1280, height=720, use_mjpeg=True):
    """
    Initialize webcam with specified settings.
    """
    cap = cv2.VideoCapture(device_path, cv2.CAP_V4L2)
    if not cap.isOpened():
        raise RuntimeError(f"Camera {device_path} cannot be opened")

    # Set resolution
    cap.set(cv2.CAP_PROP_FRAME_WIDTH, width)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, height)

    # Set MJPEG mode
    if use_mjpeg:
        fourcc = cv2.VideoWriter_fourcc(*'MJPG')
        cap.set(cv2.CAP_PROP_FOURCC, fourcc)

    print(f"Webcam initialized: {device_path} at {width}x{height}", flush=True)
    return cap




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
    global webcam_width
    global webcam_height

    if  conn_hik[j] == 1 and variable[j] == 0:

        mvs_initialize_once()  # required by MVS SDK 4.x
        SDKVersion = MvCamera.MV_CC_GetSDKVersion()
        print ("SDKVersion[0x%x]" % SDKVersion)
    
        deviceList = MV_CC_DEVICE_INFO_LIST()
        tlayerType = MV_GIGE_DEVICE | MV_USB_DEVICE
    
        # ch:枚举设备 | en:Enum device
        ret = MvCamera.MV_CC_EnumDevices(tlayerType, deviceList)
        if ret != 0:
            print ("enum devices fail! ret[0x%x]" % ret)
            conn_hik[j] = 0
            return

        if deviceList.nDeviceNum == 0:
            print ("find no device!")
            conn_hik[j] = 0
            return
    
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

        counter = 0
        for kk in range(5):
            if hik_conn_arr[kk] == 1:
                counter+=1

        nConnectionNum = counter
        hik_conn_arr[j] = 1

        if int(nConnectionNum) >= deviceList.nDeviceNum:
            print ("intput error!")
            conn_hik[j] = 0
            hik_conn_arr[j] = 0
            return
    
        # ch:创建相机实例 | en:Creat Camera Object
        cam = MvCamera()
        
    
        # ch:选择设备并创建句柄| en:Select device and create handle
        stDeviceList = cast(deviceList.pDeviceInfo[int(nConnectionNum)], POINTER(MV_CC_DEVICE_INFO)).contents
    
        ret = cam.MV_CC_CreateHandle(stDeviceList)
        if ret != 0:
            print ("create handle fail! ret[0x%x]" % ret)
            conn_hik[j] = 0
            hik_conn_arr[j] = 0
            return
    
        # ch:打开设备 | en:Open device
        ret = cam.MV_CC_OpenDevice(MV_ACCESS_Exclusive, 0)
        if ret != 0:
            print ("open device fail! ret[0x%x]" % ret)
            cam.MV_CC_DestroyHandle()
            conn_hik[j] = 0
            hik_conn_arr[j] = 0
            return
    
        # ch:探测网络最佳包大小(只对GigE相机有效) | en:Detection network optimal package size(It only works for the GigE camera)
        if stDeviceList.nTLayerType == MV_GIGE_DEVICE:
            nPacketSize = cam.MV_CC_GetOptimalPacketSize()
            if int(nPacketSize) > 0:
                ret = cam.MV_CC_SetIntValue("GevSCPSPacketSize",nPacketSize)
                if ret != 0:
                    print ("Warning: Set Packet Size fail! ret[0x%x]" % ret)
            else:
                print ("Warning: Get Packet Size fail! ret[0x%x]" % nPacketSize)

            # GigE packet resend: lost packets get re-requested instead of
            # rendering as black stripes (pure SDK/GVSP feature - no extra
            # snap interfaces needed)
            ret = cam.MV_GIGE_SetResend(1)
            if ret != 0:
                print ("Warning: enable packet resend fail! ret[0x%x]" % ret)

        # A few extra SDK-internal image buffers smooth out bursts
        cam.MV_CC_SetImageNodeNum(3)
    
        # ch:设置触发模式为off | en:Set trigger mode as off
        ret = cam.MV_CC_SetEnumValue("TriggerMode", MV_TRIGGER_MODE_OFF)
        if ret != 0:
            print ("set trigger mode fail! ret[0x%x]" % ret)
            cam.MV_CC_CloseDevice()
            cam.MV_CC_DestroyHandle()
            conn_hik[j] = 0
            hik_conn_arr[j] = 0
            return
    
        # ch:获取数据包大小 | en:Get payload size
        stParam =  MVCC_INTVALUE()
        memset(byref(stParam), 0, sizeof(MVCC_INTVALUE))
    
        ret = cam.MV_CC_GetIntValue("PayloadSize", stParam)
        if ret != 0:
            print ("get payload size fail! ret[0x%x]" % ret)
            cam.MV_CC_CloseDevice()
            cam.MV_CC_DestroyHandle()
            conn_hik[j] = 0
            hik_conn_arr[j] = 0
            return
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

        try:
            class_init(j, MEDIA_FOLDER + class_yaml_name[j])
        except Exception as e:
            print(f"class_init failed ({class_yaml_name[j]}): {e}", flush=True)
        

        
        if variable[j] == 0 and conn_hik[j] == 0: #hik

        
            time_start = time() 
            
            # ch:开始取流 | en:Start grab image
            ret = cam.MV_CC_StartGrabbing()
            if ret != 0:
                print ("start grabbing fail! ret[0x%x]" % ret)
                bool_flask[j] = 0  # release the trigger (web + PLC)
                return
        
            stOutFrame = MV_FRAME_OUT()
            memset(byref(stOutFrame), 0, sizeof(stOutFrame))

            # Discard warm-up frames: the first frames after StartGrabbing
            # are the usual source of partial (black-striped) images
            for _ in range(2):
                ret = cam.MV_CC_GetImageBuffer(stOutFrame, 2000)
                if ret == 0 and stOutFrame.pBufAddr is not None:
                    cam.MV_CC_FreeImageBuffer(stOutFrame)

            ret = cam.MV_CC_GetImageBuffer(stOutFrame, 5000)
            if None != stOutFrame.pBufAddr and 0 == ret:
                print("get one frame: Width[%d], Height[%d], nFrameNum[%d],nFrameLen[%d]" % (
                stOutFrame.stFrameInfo.nWidth, stOutFrame.stFrameInfo.nHeight, stOutFrame.stFrameInfo.nFrameNum,stOutFrame.stFrameInfo.nFrameLen))

                # SDK-side save handles every pixel format (Bayer, YUV, ...)
                bmp_path = MEDIA_FOLDER + "pre" + str(j) + ".bmp"
                nRet = save_image_to_file(stOutFrame, cam, bmp_path)
                if nRet != 0:
                    print("save image fail! ret[0x%x]" % nRet)
                else:
                    frame = cv2.imread(bmp_path)
                    if frame is not None:
                        cv2.imwrite(MEDIA_FOLDER + "pre" + str(j) + ".jpg", frame)
                    else:
                        print("saved BMP could not be read back", flush=True)

                nRet = cam.MV_CC_FreeImageBuffer(stOutFrame)

            else:
                print("no data[0x%x]" % ret)
                
            #print ("press a key to stop grabbing.")
            #press_any_key_exit()
        
            # ch:停止取流 | en:Stop grab image
            ret = cam.MV_CC_StopGrabbing()
            if ret != 0:
                print ("stop grabbing fail! ret[0x%x]" % ret)
        
        
            
            yolo_inf(j)
    
        elif variable[j] == 1:    #webcam

            time_start = time()

            # Find and initialize working webcam device
            try:
                webcam_device = find_working_webcam_device()
                cap = init_webcam(webcam_device, width=webcam_width[j], height=webcam_height[j], use_mjpeg=True)

                # Capture frame
                ret, frame = cap.read()

                if ret and frame is not None:
                    # Save captured image
                    cv2.imwrite(MEDIA_FOLDER + "pre" + str(j) + ".jpg", frame)
                    print(f"Webcam image saved: pre{j}.jpg", flush=True)
                else:
                    print(f"Failed to capture frame from webcam", flush=True)

                cap.release()
                yolo_inf(j)

            except RuntimeError as e:
                print(f"Webcam error: {e}", flush=True)
                bool_flask[j] = 0  # release the trigger (web + PLC)

        elif variable[j] == 3:

            time_start = time()

            # Preloaded-image mode: run on the image chosen on the home
            # page (copied to pre<j>.jpg so the rest of the pipeline is
            # identical to the camera modes)
            sel = preload_image_name[j]
            if sel and sel != "pre" + str(j) + ".jpg":
                src = cv2.imread(MEDIA_FOLDER + sel)
                if src is not None:
                    cv2.imwrite(MEDIA_FOLDER + "pre" + str(j) + ".jpg", src)
                else:
                    print(f"Preloaded image not readable: {sel}", flush=True)

            yolo_inf(j)

            
    if disc_hik[j] == 1 and conn_hik[j] == 0 and hik_connected == 1:

        # ch:关闭设备 | Close device
        ret = cam.MV_CC_CloseDevice()
        if ret != 0:
            print ("close device fail! ret[0x%x]" % ret)

        # ch:销毁句柄 | Destroy handle
        ret = cam.MV_CC_DestroyHandle()
        if ret != 0:
            print ("destroy handle fail! ret[0x%x]" % ret)

        hik_conn_arr[j] = 0
        disc_hik[j] = 0
        hik_connected = 0
        print("Hik camera disconnected", flush=True)


def hailo_inf(camid):
    """
    Run HAILO accelerator inference using instance_segmentation module

    Note: This function uses the run_inference_pipeline from instance_segmentation.py
    The model file should be a .hef file (HAILO model format)
    """
    global bool_flask
    global MEDIA_FOLDER
    global model_name
    global time_start
    global time_toweb
    global output_qty

    output_qty = 0

    # Model name should be .hef file for HAILO
    model_path = MEDIA_FOLDER + model_name[camid]
    input_image_path = MEDIA_FOLDER + "pre" + str(camid) + ".jpg"
    #output_dir = MEDIA_FOLDER + "hailo_output"
    output_dir = MEDIA_FOLDER

    try:
        # Import the HAILO inference pipeline
        

        # Create output directory if it doesn't exist
        #os.makedirs(output_dir, exist_ok=True)

        # Load labels from the class yaml selected in settings
        labels_list = get_class_names(camid) or ["object"]
        print(f"Loaded {len(labels_list)} labels from "
              f"{class_yaml_name[camid]}", flush=True)

        # Create a temporary labels file for HAILO pipeline
        temp_labels_file = MEDIA_FOLDER + f"temp_labels_{camid}.txt"
        with open(temp_labels_file, 'w') as f:
            for label in labels_list:
                f.write(f"{label}\n")

        # Determine architecture from model name or use default
        # You may want to add an architecture field to config.yaml
        arch = "v8"  # Default to YOLOv8
        if "yolov5" in model_name[camid].lower() or "v5" in model_name[camid].lower():
            arch = "v5"
        elif "fastsam" in model_name[camid].lower() or "fast" in model_name[camid].lower():
            arch = "fast"

        # Run HAILO inference pipeline
        print(f"Running HAILO inference with model: {model_path}", flush=True)
        run_inference_pipeline(
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

        # Clean up temp labels file
        try:
            os.remove(temp_labels_file)
        except:
            pass

        # Find the output image and copy it to the expected location
        output_image_path = MEDIA_FOLDER + "inf" + str(camid) + ".jpg"

        # Look for the output file in the output directory
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

        # TODO: Extract detection count from HAILO output
        # The instance_segmentation.py module needs to be modified to return
        # the detection count instead of just saving the visualization.

        # Until then: publish EMPTY results so the Data Layer never
        # carries stale positions from a previous run in this mode
        publish_dl_results([], [], [], [], [])

        time_end = time()
        time_toweb[camid] = round(time_end - time_start, 3)
        bool_flask[camid] = 0

    except Exception as e:
        print(f"HAILO inference error: {e}", flush=True)
        import traceback
        traceback.print_exc()
        bool_flask[camid] = 0


def yolo_segmentation_inf(camid):
    """Instance Segmentation inference using YOLOSeg (onnxruntime)"""
    global bool_flask
    global MEDIA_FOLDER
    global model_name
    global hmi_conf_thres
    global hmi_iou_thres
    global time_start
    global time_toweb
    global output_qty

    output_qty = 0

    yoloseg = YOLOSeg(MEDIA_FOLDER + model_name[camid], conf_thres=hmi_conf_thres[camid], iou_thres=hmi_iou_thres[camid])
    img = read_frame(camid)

    # Detect Objects
    boxes, scores, class_ids, masks = yoloseg(img)

    # Count detected instances
    output_qty = len(boxes)

    # Publish to Data Layer (boxes are xyxy -> centers; no angles in seg)
    publish_dl_results(class_ids, scores,
                       [(b[0] + b[2]) / 2 for b in boxes],
                       [(b[1] + b[3]) / 2 for b in boxes], [])

    # Draw detections
    combined_img = yoloseg.draw_masks(img)

    cv2.imwrite(MEDIA_FOLDER +"inf" + str(camid) + ".jpg", combined_img)

    time_end = time()
    time_toweb[camid] = round(time_end - time_start, 3)

    bool_flask[camid] = 0


# Cached OBB model instances so the sessions are not rebuilt every frame.
# Keyed by model path; conf/iou thresholds are refreshed on every call.
_obb_detector = {"key": None, "model": None}
_hailo_obb_detector = {"key": None, "model": None}
_obb_heading = {"path": None, "model": None}


def get_obb_detector(camid):
    """ONNX (onnxruntime CPU) OBB detector, cached."""
    from yoloobb.YOLOOBB import YOLOOBB
    key = model_name[camid]
    if _obb_detector["key"] != key:
        _obb_detector["model"] = YOLOOBB(
            MEDIA_FOLDER + model_name[camid],
            conf_thres=hmi_conf_thres[camid],
            iou_thres=hmi_iou_thres[camid],
            angle_mode="180")  # stock Ultralytics OBB export
        _obb_detector["key"] = key
        print(f"OBB detector loaded: {model_name[camid]}", flush=True)
    detector = _obb_detector["model"]
    detector.conf_threshold = hmi_conf_thres[camid]
    detector.iou_threshold = hmi_iou_thres[camid]
    return detector


def get_hailo_obb_detector(camid):
    """Hailo-accelerated OBB detector, cached. Lazy import so the app
    still runs on machines without hailo_platform installed."""
    from hailo_obb import HailoOBB
    key = model_name[camid]
    if _hailo_obb_detector["key"] != key:
        if _hailo_obb_detector["model"] is not None:
            try:
                _hailo_obb_detector["model"].close()
            except Exception:
                pass
        _hailo_obb_detector["model"] = HailoOBB(
            MEDIA_FOLDER + model_name[camid],
            conf_thres=hmi_conf_thres[camid],
            iou_thres=hmi_iou_thres[camid])
        _hailo_obb_detector["key"] = key
        print(f"HAILO OBB detector loaded: {model_name[camid]}", flush=True)
    detector = _hailo_obb_detector["model"]
    detector.conf_threshold = hmi_conf_thres[camid]
    detector.iou_threshold = hmi_iou_thres[camid]
    return detector


def get_heading_classifier():
    """Returns the heading classifier if one is configured (settings page,
    'Heading Model') and its file exists, else None. With it, reported
    angles are the true 0-360 heading; without it, angles stay
    180-degree ambiguous (standard OBB)."""
    from yoloobb.heading import HeadingClassifier
    name = heading_model_name[0]
    if not name or not os.path.isfile(MEDIA_FOLDER + name):
        _obb_heading["path"] = None
        _obb_heading["model"] = None
        return None
    path = MEDIA_FOLDER + name
    if not _obb_heading["path"] == path or _obb_heading["model"] is None:
        _obb_heading["model"] = HeadingClassifier(path)
        _obb_heading["path"] = path
        print(f"Heading classifier loaded: {name}", flush=True)
    return _obb_heading["model"]


_class_names_cache = {"key": None, "names": None}


def read_frame(camid):
    """Read pre<camid>.jpg and, when CLAHE is enabled, apply the SAME
    lighting normalization the training data was preprocessed with
    (yoloobb/preprocess.py). Used by all inference modes so the detector
    and heading classifier see the identical normalized frame."""
    img = cv2.imread(MEDIA_FOLDER + "pre" + str(camid) + ".jpg")
    if use_clahe[0] and img is not None:
        from yoloobb.preprocess import preprocess_frame
        img = preprocess_frame(img)
    return img


def get_class_names(camid):
    """Class names for overlays from the yaml selected in settings
    (default data<camid>.yaml); None if unavailable. Cached by file
    mtime - this is called by the 500 ms web poll."""
    path = MEDIA_FOLDER + class_yaml_name[camid]
    try:
        key = (path, os.path.getmtime(path))
        if _class_names_cache["key"] != key:
            with open(path, encoding="utf8") as f:
                names = yaml.safe_load(f)['names']
            # support both yaml forms: {0: A, 1: B} and [A, B]
            _class_names_cache["names"] = (
                list(names.values()) if isinstance(names, dict)
                else list(names))
            _class_names_cache["key"] = key
        return _class_names_cache["names"]
    except Exception:
        return None


def _obb_run(camid, detector_factory, backend):
    """Shared OBB inference body for both backends (onnxruntime / Hailo).

    Standard OBB detection plus, when heading.onnx is present in the
    media folder, 360-degree heading resolution per detection."""
    global bool_flask
    global MEDIA_FOLDER
    global time_start
    global time_toweb
    global output_qty
    global output_angles

    output_qty = 0
    output_angles = []

    try:
        detector = detector_factory(camid)

        img = read_frame(camid)

        boxes, scores, class_ids, angles = detector(img)
        output_qty = len(boxes)

        # Resolve 180-ambiguous OBB angles to true 0-360 headings
        # (CPU stage, identical for both backends)
        class_names = get_class_names(camid)
        heading_model = get_heading_classifier()
        if heading_model is not None and output_qty > 0:
            headings, flipped = heading_model.resolve(img, boxes, angles)
            output_angles = [round(float(np.degrees(a)), 1) for a in headings]
            combined_img = detector.draw_obb(img, draw_angle_arrow=True,
                                             angles=headings,
                                             class_names=class_names)
        else:
            output_angles = [round(float(np.degrees(a)) % 360, 1) for a in angles]
            combined_img = detector.draw_obb(img, draw_angle_arrow=True,
                                             class_names=class_names)

        cv2.imwrite(MEDIA_FOLDER + "inf" + str(camid) + ".jpg", combined_img)

        # Publish to Data Layer (box centers px + heading degrees)
        publish_dl_results(class_ids, scores,
                           [b[0] for b in boxes], [b[1] for b in boxes],
                           output_angles)

        print(f"OBB Detection complete ({backend}): {output_qty} objects, "
              f"angles: {output_angles}", flush=True)

    except Exception as e:
        publish_dl_results([], [], [], [], [])  # no stale data for the PLC
        print(f"OBB inference error ({backend}): {e}", flush=True)
        import traceback
        traceback.print_exc()

        # Fallback: copy input image with error message
        img = cv2.imread(MEDIA_FOLDER + "pre" + str(camid) + ".jpg")
        if img is not None:
            cv2.putText(img, f"OBB Detection Error: {str(e)[:50]}", (50, 50),
                       cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 0, 255), 2)
            cv2.imwrite(MEDIA_FOLDER + "inf" + str(camid) + ".jpg", img)

    time_end = time()
    time_toweb[camid] = round(time_end - time_start, 3)
    bool_flask[camid] = 0


def yolo_obb_inf(camid):
    """Oriented Bounding Box detection inference (onnxruntime)."""
    _obb_run(camid, get_obb_detector, "onnxruntime")


def hailo_obb_inf(camid):
    """Oriented Bounding Box detection inference (Hailo accelerator).
    Expects model_name[camid] to be a HEF compiled with the 9 raw OBB
    end nodes (see 360-OBB-Research-Findings.md section 1.3)."""
    _obb_run(camid, get_hailo_obb_detector, "hailo")


# Cached detection model instances (same pattern as the OBB caches)
_det_detector = {"key": None, "model": None}
_hailo_det_detector = {"key": None, "model": None}


def get_det_detector(camid):
    """ONNX (onnxruntime CPU) object detector, cached."""
    from yolodet.YOLODet import YOLODet
    key = model_name[camid]
    if _det_detector["key"] != key:
        _det_detector["model"] = YOLODet(
            MEDIA_FOLDER + model_name[camid],
            conf_thres=hmi_conf_thres[camid],
            iou_thres=hmi_iou_thres[camid])
        _det_detector["key"] = key
        print(f"Detection model loaded: {model_name[camid]}", flush=True)
    detector = _det_detector["model"]
    detector.conf_threshold = hmi_conf_thres[camid]
    detector.iou_threshold = hmi_iou_thres[camid]
    return detector


def get_hailo_det_detector(camid):
    """Hailo-accelerated object detector, cached. Lazy import."""
    from hailo_detection import HailoDetection
    key = model_name[camid]
    if _hailo_det_detector["key"] != key:
        if _hailo_det_detector["model"] is not None:
            try:
                _hailo_det_detector["model"].close()
            except Exception:
                pass
        _hailo_det_detector["model"] = HailoDetection(
            MEDIA_FOLDER + model_name[camid],
            conf_thres=hmi_conf_thres[camid],
            iou_thres=hmi_iou_thres[camid])
        _hailo_det_detector["key"] = key
        print(f"HAILO detection model loaded: {model_name[camid]}", flush=True)
    detector = _hailo_det_detector["model"]
    detector.conf_threshold = hmi_conf_thres[camid]
    detector.iou_threshold = hmi_iou_thres[camid]
    return detector


def _det_run(camid, detector_factory, backend):
    """Shared object-detection inference body for both backends."""
    global bool_flask
    global MEDIA_FOLDER
    global time_start
    global time_toweb
    global output_qty
    global output_angles

    output_qty = 0
    output_angles = []  # not applicable in detection mode

    try:
        detector = detector_factory(camid)

        img = read_frame(camid)

        boxes, scores, class_ids = detector(img)
        output_qty = len(boxes)

        combined_img = detector.draw_detections(
            img, class_names=get_class_names(camid))
        cv2.imwrite(MEDIA_FOLDER + "inf" + str(camid) + ".jpg", combined_img)

        # Publish to Data Layer (box centers px; no angles in detection)
        publish_dl_results(class_ids, scores,
                           [b[0] for b in boxes], [b[1] for b in boxes], [])

        print(f"Detection complete ({backend}): {output_qty} objects",
              flush=True)

    except Exception as e:
        publish_dl_results([], [], [], [], [])  # no stale data for the PLC
        print(f"Detection inference error ({backend}): {e}", flush=True)
        import traceback
        traceback.print_exc()

        img = cv2.imread(MEDIA_FOLDER + "pre" + str(camid) + ".jpg")
        if img is not None:
            cv2.putText(img, f"Detection Error: {str(e)[:50]}", (50, 50),
                       cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 0, 255), 2)
            cv2.imwrite(MEDIA_FOLDER + "inf" + str(camid) + ".jpg", img)

    time_end = time()
    time_toweb[camid] = round(time_end - time_start, 3)
    bool_flask[camid] = 0


def yolo_detection_inf(camid):
    """Object Detection inference (onnxruntime)."""
    _det_run(camid, get_det_detector, "onnxruntime")


def hailo_detection_inf(camid):
    """Object Detection inference (Hailo accelerator). Supports both
    model-zoo HEFs (on-chip NMS) and raw 6-end-node compiles."""
    _det_run(camid, get_hailo_det_detector, "hailo")


def log_shot(camid):
    """Training-data logger: copy the raw capture (pre<camid>.jpg) and the
    drawn result (inf<camid>.jpg) into <base>/Logger/Shot and Process with
    a timestamp. Best-effort - never breaks inference. base defaults to
    the media folder; set logger_path to a removable-media mount for USB."""
    if not logger_enabled[0]:
        return
    try:
        base = logger_path[0].strip() or MEDIA_FOLDER
        shot_dir = os.path.join(base, "Logger", "Shot")
        proc_dir = os.path.join(base, "Logger", "Process")
        os.makedirs(shot_dir, exist_ok=True)
        os.makedirs(proc_dir, exist_ok=True)
        ts = datetime.now().strftime("%Y%m%d_%H%M%S_%f")[:-3] + "_cam" + str(camid)
        pre = MEDIA_FOLDER + "pre" + str(camid) + ".jpg"
        inf = MEDIA_FOLDER + "inf" + str(camid) + ".jpg"
        if os.path.isfile(pre):
            shutil.copy2(pre, os.path.join(shot_dir, ts + ".jpg"))
        if os.path.isfile(inf):
            shutil.copy2(inf, os.path.join(proc_dir, ts + ".jpg"))
    except Exception as e:
        print(f"Logger error: {e}", flush=True)


def yolo_inf(camid):
    """Main inference dispatcher - routes to appropriate mode"""
    global processing_mode
    global use_hailo

    mode = processing_mode[camid]

    if use_hailo[camid]:
        if mode == "segmentation":
            hailo_inf(camid)
        elif mode == "obb":
            hailo_obb_inf(camid)  # HEF with 9 raw OBB end nodes
        elif mode == "detection":
            hailo_detection_inf(camid)  # model-zoo HEF or raw 6 end nodes
        else:
            print(f"Unknown processing mode for HAILO: {mode}")
    else:
        # onnxruntime inference based on mode
        if mode == "segmentation":
            yolo_segmentation_inf(camid)
        elif mode == "obb":
            yolo_obb_inf(camid)
        elif mode == "detection":
            yolo_detection_inf(camid)
        else:
            print(f"Unknown processing mode: {mode}, defaulting to segmentation")
            yolo_segmentation_inf(camid)

    log_shot(camid)  # training-data logger (no-op unless enabled)

    

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
    global camera_qty
    global hmi_cam_settings
    global webcam_width
    global webcam_height
    global use_hailo
    global processing_mode
    global calib_pattern_w
    global calib_pattern_h
    global calib_square_mm
    global iframe_show
    global iframe_path



    if initialize_var:

        with open(MEDIA_FOLDER + "config.yaml") as f:
            cfg = yaml.load(f, Loader=yaml.FullLoader)
            print(cfg)

            hmi_password = cfg["boschrexroth"]
            camera_qty = cfg["cameras"]
            
            hmi_password_temp = hmi_password

            psstr = "First"

            model_name[0] = cfg["onnx"][psstr]
            variable[0] = cfg["cam_select"][psstr]
            hmi_conf_thres[0] = cfg["conf_thres"][psstr]
            hmi_iou_thres[0] = cfg["iou_thres"][psstr]
            webcam_width[0] = cfg.get("webcam_width", {}).get(psstr, 1280)
            webcam_height[0] = cfg.get("webcam_height", {}).get(psstr, 720)
            use_hailo[0] = cfg.get("use_hailo", {}).get(psstr, False)
            processing_mode[0] = cfg.get("processing_mode", {}).get(psstr, "segmentation")
            heading_model_name[0] = cfg.get("heading_onnx", {}).get(psstr, "heading.onnx")
            class_yaml_name[0] = cfg.get("class_yaml", {}).get(psstr, "data0.yaml")
            preload_image_name[0] = cfg.get("preload_image", {}).get(psstr, "")
            use_calibration[0] = cfg.get("use_calibration", {}).get(psstr, False)
            use_clahe[0] = cfg.get("use_clahe", False)
            logger_enabled[0] = cfg.get("logger_enabled", False)
            logger_path[0] = cfg.get("logger_path", "")
            calib_pattern_w = cfg.get("calib_pattern_w", 9)
            calib_pattern_h = cfg.get("calib_pattern_h", 6)
            calib_square_mm = cfg.get("calib_square_mm", 25.0)
            iframe_show = cfg.get("iframe_show", False)
            iframe_path = cfg.get("iframe_path", "3dviewer/Demo?id=3dv1")
            model_name_temp[0] = model_name[0]


        initialize_var = False
        
    
    while not __close_app:     
        

        if model_name_temp[0] != model_name[0]: #Reset classes
            #class_names = []          ##EDIT
            model_name_temp[0] = model_name[0]

        
        if save_config == 1:

            
            cfg["boschrexroth"] = hmi_password
            
            cfg["cameras"] = camera_qty

            psstr = "First"
            
            cfg["onnx"][psstr] = model_name[0]
            cfg["cam_select"][psstr] = variable[0]
            cfg["conf_thres"][psstr] = hmi_conf_thres[0]
            cfg["iou_thres"][psstr] = hmi_iou_thres[0]
            cfg["webcam_width"][psstr] = webcam_width[0]
            cfg["webcam_height"][psstr] = webcam_height[0]
            cfg["use_hailo"][psstr] = use_hailo[0]

            # Ensure processing_mode dict exists
            if "processing_mode" not in cfg:
                cfg["processing_mode"] = {}
            cfg["processing_mode"][psstr] = processing_mode[0]

            if "heading_onnx" not in cfg:
                cfg["heading_onnx"] = {}
            cfg["heading_onnx"][psstr] = heading_model_name[0]

            if "class_yaml" not in cfg:
                cfg["class_yaml"] = {}
            cfg["class_yaml"][psstr] = class_yaml_name[0]
            if "preload_image" not in cfg:
                cfg["preload_image"] = {}
            cfg["preload_image"][psstr] = preload_image_name[0]

            if "use_calibration" not in cfg:
                cfg["use_calibration"] = {}
            cfg["use_calibration"][psstr] = use_calibration[0]
            cfg["use_clahe"] = use_clahe[0]
            cfg["logger_enabled"] = logger_enabled[0]
            cfg["logger_path"] = logger_path[0]
            cfg["calib_pattern_w"] = calib_pattern_w
            cfg["calib_pattern_h"] = calib_pattern_h
            cfg["calib_square_mm"] = calib_square_mm
            cfg["iframe_show"] = iframe_show
            cfg["iframe_path"] = iframe_path


            with open(MEDIA_FOLDER + "config.yaml", "w") as f:
                cfg = yaml.dump(
                    cfg, stream=f, default_flow_style=False, sort_keys=False
                )
            ###   

            with open(MEDIA_FOLDER + "config.yaml") as f:
                cfg = yaml.load(f, Loader=yaml.FullLoader)
                print(cfg)
    
                hmi_password = cfg["boschrexroth"]
                camera_qty = cfg["cameras"]
                hmi_password_temp = hmi_password
       
                psstr = "First"    
                    
                model_name[0] = cfg["onnx"][psstr]
                variable[0] = cfg["cam_select"][psstr]
                hmi_conf_thres[0] = cfg["conf_thres"][psstr]
                hmi_iou_thres[0] = cfg["iou_thres"][psstr]
                webcam_width[0] = cfg.get("webcam_width", {}).get(psstr, 1280)
                webcam_height[0] = cfg.get("webcam_height", {}).get(psstr, 720)
                use_hailo[0] = cfg.get("use_hailo", {}).get(psstr, False)
                processing_mode[0] = cfg.get("processing_mode", {}).get(psstr, "segmentation")
                heading_model_name[0] = cfg.get("heading_onnx", {}).get(psstr, "heading.onnx")
                class_yaml_name[0] = cfg.get("class_yaml", {}).get(psstr, "data0.yaml")
                preload_image_name[0] = cfg.get("preload_image", {}).get(psstr, "")
                use_calibration[0] = cfg.get("use_calibration", {}).get(psstr, False)
                use_clahe[0] = cfg.get("use_clahe", False)
                logger_enabled[0] = cfg.get("logger_enabled", False)
                logger_path[0] = cfg.get("logger_path", "")
                model_name_temp[0] = model_name[0]


            save_config = 0      

        #for kk in range(5):
        camera_processing(0)
        sleep(0.2)
    
        #camera_processing(0)
        #camera_processing(1)
        #camera_processing(2)
        #camera_processing(3)
        #camera_processing(4)
        
                
if __name__ == "__main__":        
    #sig()

    try:
        logger.info(f'First thread')
        t1 = threading.Thread(target=main)
        t1.start()
        
        sleep(3.0)
        
        logger.info(f'Second thread')
        t2 = threading.Thread(target=dl_provider)
        t2.start()
        
        sleep(3.0)
        
        # dl_client removed: the provider thread reads/writes its own
        # node variants directly (no client connection needed)
        
        logger.info(f'Fourth thread')
        t4 = threading.Thread(target=startApp)
        t4.start()
        
        # Wait for all threads to complete
        t1.join()
        t2.join()
        t4.join()
        
    except KeyboardInterrupt:
        logger.info("Interrupted by user, shutting down gracefully...")
    except Exception as e:
        logger.error("Error: " + str(e))  

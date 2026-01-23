# SPDX-FileCopyrightText: Bosch Rexroth AG
# SPDX-License-Identifier: MIT

import os
import struct
import json
from datetime import datetime

EVENT_FORMAT = "llHHI"
EVENT_SIZE = struct.calcsize(EVENT_FORMAT)

EV_SYN = 0x00
EV_KEY = 0x01
EV_REL = 0x02
EV_ABS = 0x03


class Joystick:
    def __init__(self, device_path=None):
        """
        device_path: e.g. /dev/input/event3
        If None, we scan /dev/input/event0..event15 and try to find
        something that looks like a joystick / gamepad.
        """
        if device_path is None:
            device_path = self._find_device()

        if device_path is None:
            raise RuntimeError("No suitable /dev/input/eventX joystick device found")

        self.path = device_path
        self.fd = os.open(self.path, os.O_RDONLY | os.O_NONBLOCK)

        # debug: print kernel device name
        dev = os.path.basename(self.path)  # "eventX"
        name_path = f"/sys/class/input/{dev}/device/name"
        try:
            with open(name_path, "r", encoding="utf-8") as f:
                devname = f.read().strip()
            print(f"Joystick device: {self.path} ({devname})", flush=True)
        except Exception as e:
            print(f"Joystick device: {self.path} (could not read name: {e})",
                  flush=True)

        self.axes = {}
        self.buttons = {}
        self.device_name = dev
        self.debug_events = 0  # for initial logging

    def _find_device(self):
        """
        Scan /dev/input/event0..event15 and choose the one whose
        name contains typical joystick/gamepad keywords.
        """
        candidates = []
        for i in range(16):
            p = f"/dev/input/event{i}"
            if not os.path.exists(p):
                continue

            dev = os.path.basename(p)
            name_path = f"/sys/class/input/{dev}/device/name"
            devname = ""
            try:
                with open(name_path, "r", encoding="utf-8") as f:
                    devname = f.read().strip()
            except Exception:
                pass

            print(f"Found input device {p} name='{devname}'", flush=True)
            candidates.append((p, devname))

        if not candidates:
            print("No /dev/input/event* devices found", flush=True)
            return None

        # Prioritize specific gamepad/joystick keywords over generic ones
        # Check high-priority keywords first (specific gamepads)
        high_priority_keywords = [
            "xbox",
            "playstation",
            "dualshock",
            "dualsense",
            "gamepad",
            "joystick",
        ]

        for p, name in candidates:
            lname = name.lower()
            if any(k in lname for k in high_priority_keywords):
                print(f"Selected joystick candidate {p} ('{name}')", flush=True)
                return p

        # Fallback to generic controller keywords (excludes mice)
        low_priority_keywords = [
            "wireless controller",
            "controller",
        ]

        for p, name in candidates:
            lname = name.lower()
            # Exclude mice
            if "mouse" in lname or "touchpad" in lname or "keyboard" in lname:
                continue
            if any(k in lname for k in low_priority_keywords):
                print(f"Selected joystick candidate {p} ('{name}')", flush=True)
                return p

        # fallback: if nothing matches, return first device
        print("No joystick-like device found, falling back to", candidates[0][0],
              flush=True)
        return candidates[0][0]

    def _read_events(self):
        events = []
        while True:
            try:
                data = os.read(self.fd, EVENT_SIZE)
            except BlockingIOError:
                break
            if len(data) < EVENT_SIZE:
                break

            (tv_sec, tv_usec, etype, code, value) = struct.unpack(EVENT_FORMAT, data)
            events.append((etype, code, value))

            # debug: show first 50 key/axis events
            if self.debug_events < 50 and etype in (EV_KEY, EV_ABS):
                print(
                    f"input event: type=0x{etype:02x}, code={code}, value={value}",
                    flush=True,
                )
                self.debug_events += 1

        return events

    def get_state_json(self):
        for etype, code, value in self._read_events():
            if etype == EV_ABS:
                self.axes[str(code)] = value
            elif etype == EV_KEY:
                self.buttons[str(code)] = value

        payload = {
            "timestamp": datetime.utcnow().isoformat() + "Z",
            "device": self.device_name,
            "axes": self.axes,
            "buttons": self.buttons,
        }
        return json.dumps(payload)

    def get_state(self):
        """Get joystick state as a dictionary"""
        for etype, code, value in self._read_events():
            if etype == EV_ABS:
                self.axes[str(code)] = value
            elif etype == EV_KEY:
                self.buttons[str(code)] = value

        return {
            "axes": self.axes.copy(),
            "buttons": self.buttons.copy(),
        }

import os
import sys
from math import sqrt

try:
    from PyQt5.QtGui import *
    from PyQt5.QtCore import *
    from PyQt5.QtWidgets import *
except ImportError:
    from PyQt4.QtGui import *
    from PyQt4.QtCore import *

# Modern icon override: PNGs in <app dir>/icons_modern/ take precedence
# over the compiled Qt resources (regenerate with generate_modern_icons.py).
# When frozen by PyInstaller the icons are bundled at sys._MEIPASS.
if getattr(sys, 'frozen', False):
    _MODERN_ICON_DIR = os.path.join(sys._MEIPASS, 'icons_modern')
else:
    _MODERN_ICON_DIR = os.path.join(
        os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'icons_modern')


def newIcon(icon):
    modern = os.path.join(_MODERN_ICON_DIR, icon + '.png')
    if os.path.isfile(modern):
        return QIcon(modern)
    return QIcon(':/' + icon)


def newButton(text, icon=None, slot=None):
    b = QPushButton(text)
    if icon is not None:
        b.setIcon(newIcon(icon))
    if slot is not None:
        b.clicked.connect(slot)
    return b


def newAction(parent, text, slot=None, shortcut=None, icon=None,
              tip=None, checkable=False, enabled=True):
    """Create a new action and assign callbacks, shortcuts, etc."""
    a = QAction(text, parent)
    if icon is not None:
        a.setIcon(newIcon(icon))
    if shortcut is not None:
        if isinstance(shortcut, (list, tuple)):
            a.setShortcuts(shortcut)
        else:
            a.setShortcut(shortcut)
    if tip is not None:
        a.setToolTip(tip)
        a.setStatusTip(tip)
    if slot is not None:
        a.triggered.connect(slot)
    if checkable:
        a.setCheckable(True)
    a.setEnabled(enabled)
    return a


def addActions(widget, actions):
    for action in actions:
        if action is None:
            widget.addSeparator()
        elif isinstance(action, QMenu):
            widget.addMenu(action)
        else:
            widget.addAction(action)


def labelValidator():
    return QRegExpValidator(QRegExp(r'^[^ \t].+'), None)


class struct(object):

    def __init__(self, **kwargs):
        self.__dict__.update(kwargs)


def distance(p):
    return sqrt(p.x() * p.x() + p.y() * p.y())


def fmtShortcut(text):
    mod, key = text.split('+', 1)
    return '<b>%s</b>+<b>%s</b>' % (mod, key)

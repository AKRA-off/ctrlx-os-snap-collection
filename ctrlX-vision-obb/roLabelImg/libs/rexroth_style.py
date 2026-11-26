# Bosch Rexroth theme for roLabelImg (Qt stylesheet)
# Mirrors the ctrlX AI Detector web UI palette:
# dependencies/static/bootstrap/main-style.css

PRIMARY_BLUE = "#005691"
SECONDARY_BLUE = "#0082C8"
DARK_BLUE = "#003d63"
LIGHT_BLUE = "#E6F2F7"
DARK_GRAY = "#333333"
MEDIUM_GRAY = "#6c757d"
LIGHT_GRAY = "#F5F5F5"
WHITE = "#FFFFFF"
ACCENT_ORANGE = "#FF6900"
SUCCESS_GREEN = "#00A650"

STYLESHEET = f"""
/* ------- base ------- */
QMainWindow, QDialog {{
    background-color: {LIGHT_GRAY};
    color: {DARK_GRAY};
    font-family: 'Segoe UI', 'Roboto', 'Ubuntu', sans-serif;
}}
QWidget {{
    color: {DARK_GRAY};
    font-size: 9pt;
}}

/* ------- menu bar: blue header like .main-header ------- */
QMenuBar {{
    background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                stop:0 {PRIMARY_BLUE}, stop:1 {SECONDARY_BLUE});
    color: {WHITE};
    font-weight: 600;
    padding: 2px;
}}
QMenuBar::item {{
    background: transparent;
    color: {WHITE};
    padding: 6px 12px;
    border-radius: 4px;
}}
QMenuBar::item:selected {{
    background: {DARK_BLUE};
}}

QMenu {{
    background-color: {WHITE};
    color: {DARK_GRAY};
    border: 1px solid #e0e0e0;
    border-radius: 5px;
    padding: 4px;
}}
QMenu::item {{
    padding: 6px 28px 6px 20px;
    border-radius: 4px;
}}
QMenu::item:selected {{
    background-color: {PRIMARY_BLUE};
    color: {WHITE};
}}
QMenu::item:disabled {{
    color: {MEDIUM_GRAY};
}}
QMenu::separator {{
    height: 1px;
    background: #e0e0e0;
    margin: 4px 8px;
}}

/* ------- toolbars ------- */
QToolBar {{
    background-color: {WHITE};
    border: none;
    border-right: 2px solid #e0e0e0;
    spacing: 2px;
    padding: 4px;
}}
QToolButton {{
    background: transparent;
    color: {DARK_GRAY};
    border: 1px solid transparent;
    border-radius: 5px;
    padding: 4px;
    margin: 1px;
}}
QToolButton:hover {{
    background-color: {LIGHT_BLUE};
    border: 1px solid {SECONDARY_BLUE};
}}
QToolButton:pressed {{
    background-color: {LIGHT_BLUE};
    border: 1px solid {PRIMARY_BLUE};
}}
QToolButton:checked {{
    background-color: {LIGHT_BLUE};
    border: 2px solid {PRIMARY_BLUE};
    color: {PRIMARY_BLUE};
    font-weight: 600;
}}
QToolButton:disabled {{
    color: #b0b0b0;
}}

/* ------- dock widgets (Box Labels / File List) ------- */
QDockWidget {{
    color: {WHITE};
    font-weight: 600;
    titlebar-close-icon: none;
}}
QDockWidget::title {{
    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 {PRIMARY_BLUE}, stop:1 {SECONDARY_BLUE});
    color: {WHITE};
    padding: 6px 10px;
    text-align: left;
}}

/* ------- lists ------- */
QListWidget {{
    background-color: {WHITE};
    border: 1px solid #e0e0e0;
    border-radius: 5px;
    padding: 2px;
}}
QListWidget::item {{
    padding: 4px 6px;
    border-radius: 4px;
}}
QListWidget::item:hover {{
    background-color: {LIGHT_BLUE};
}}
QListWidget::item:selected {{
    background-color: {PRIMARY_BLUE};
    color: {WHITE};
}}

/* ------- buttons ------- */
QPushButton {{
    background-color: {PRIMARY_BLUE};
    color: {WHITE};
    border: none;
    border-radius: 5px;
    padding: 6px 16px;
    font-weight: 600;
}}
QPushButton:hover {{
    background-color: {SECONDARY_BLUE};
}}
QPushButton:pressed {{
    background-color: {DARK_BLUE};
}}
QPushButton:disabled {{
    background-color: #c0c8cf;
}}

/* ------- inputs ------- */
QLineEdit, QComboBox, QSpinBox {{
    background-color: {WHITE};
    border: 1px solid #d0d0d0;
    border-radius: 5px;
    padding: 4px 8px;
    selection-background-color: {SECONDARY_BLUE};
}}
QLineEdit:focus, QComboBox:focus, QSpinBox:focus {{
    border: 1px solid {SECONDARY_BLUE};
}}
QCheckBox {{
    spacing: 6px;
}}
QCheckBox::indicator:checked {{
    background-color: {PRIMARY_BLUE};
    border: 1px solid {PRIMARY_BLUE};
    border-radius: 3px;
}}
QCheckBox::indicator:unchecked {{
    background-color: {WHITE};
    border: 1px solid #d0d0d0;
    border-radius: 3px;
}}

/* ------- status bar: heading readout lives here ------- */
QStatusBar {{
    background-color: {DARK_BLUE};
    color: {WHITE};
    font-weight: 600;
}}
QStatusBar QLabel {{
    color: {WHITE};
}}

/* ------- scrollbars ------- */
QScrollBar:vertical {{
    background: {LIGHT_GRAY};
    width: 10px;
    margin: 0;
}}
QScrollBar::handle:vertical {{
    background: #c0c8cf;
    border-radius: 5px;
    min-height: 30px;
}}
QScrollBar::handle:vertical:hover {{
    background: {SECONDARY_BLUE};
}}
QScrollBar:horizontal {{
    background: {LIGHT_GRAY};
    height: 10px;
    margin: 0;
}}
QScrollBar::handle:horizontal {{
    background: #c0c8cf;
    border-radius: 5px;
    min-width: 30px;
}}
QScrollBar::handle:horizontal:hover {{
    background: {SECONDARY_BLUE};
}}
QScrollBar::add-line, QScrollBar::sub-line {{
    height: 0; width: 0;
}}
QScrollBar::add-page, QScrollBar::sub-page {{
    background: none;
}}

/* ------- scroll area / canvas frame ------- */
QScrollArea {{
    border: 1px solid #e0e0e0;
    border-radius: 5px;
    background-color: {LIGHT_GRAY};
}}
"""

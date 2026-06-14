"""
env_info_panel.py — 环境信息面板
E同学：Python应用层负责人
显示字段：temp（温度）、humi（湿度）、lux（光照）
使用 QLCDNumber 风格数字显示
"""

from PyQt5.QtWidgets import (
    QGroupBox, QHBoxLayout, QVBoxLayout, QLabel, QLCDNumber, QFrame
)
from PyQt5.QtCore import Qt
from PyQt5.QtGui import QFont

import config


class EnvGauge(QFrame):
    """单个环境指标仪表：图标 + LCD数字 + 单位"""

    def __init__(self, icon, title, unit, color="#00E676", parent=None):
        super().__init__(parent)
        layout = QVBoxLayout(self)
        layout.setAlignment(Qt.AlignCenter)
        layout.setSpacing(4)

        # 图标和标题
        header = QLabel(f"{icon} {title}")
        header.setAlignment(Qt.AlignCenter)
        header.setFont(QFont("Microsoft YaHei", 11, QFont.Bold))
        header.setStyleSheet("color: #B0B0D0; background: transparent;")
        layout.addWidget(header)

        # LCD 数字显示
        self._lcd = QLCDNumber(6)
        self._lcd.setSegmentStyle(QLCDNumber.Flat)
        self._lcd.setMinimumHeight(56)
        self._lcd.setStyleSheet(
            f"QLCDNumber {{ color: {color}; background-color: #1A1A2A; "
            f"border: 1px solid #3A3A5C; border-radius: 6px; }}"
        )
        self._lcd.display(0)
        layout.addWidget(self._lcd)

        # 单位
        unit_label = QLabel(unit)
        unit_label.setAlignment(Qt.AlignCenter)
        unit_label.setFont(QFont("Microsoft YaHei", 10))
        unit_label.setStyleSheet(f"color: {color}; background: transparent;")
        layout.addWidget(unit_label)

    def set_value(self, value):
        """设置显示值"""
        self._lcd.display(value)


class EnvInfoPanel(QGroupBox):
    """
    环境信息面板

    显示三个环境指标：
        - 🌡️ 温度（℃）
        - 💧 湿度（%RH）
        - ☀️ 光照（lux）
    """

    def __init__(self, parent=None):
        super().__init__("🌍 环境信息", parent)
        self._init_ui()

    def _init_ui(self):
        layout = QHBoxLayout(self)
        layout.setSpacing(20)

        # 三个仪表盘
        self._temp_gauge = EnvGauge("🌡️", "温度", "℃", "#FF7043")
        self._humi_gauge = EnvGauge("💧", "湿度", "%RH", "#42A5F5")
        self._lux_gauge = EnvGauge("☀️", "光照", "lux", "#FFCA28")

        layout.addWidget(self._temp_gauge)
        layout.addWidget(self._humi_gauge)
        layout.addWidget(self._lux_gauge)

    def update_data(self, data: dict):
        """
        更新环境信息面板

        参数：
            data — 传感器数据字典，包含 temp/humi/lux 字段
        """
        if config.FIELD_TEMP in data:
            self._temp_gauge.set_value(float(data[config.FIELD_TEMP]))

        if config.FIELD_HUMI in data:
            self._humi_gauge.set_value(float(data[config.FIELD_HUMI]))

        if config.FIELD_LUX in data:
            self._lux_gauge.set_value(int(data[config.FIELD_LUX]))

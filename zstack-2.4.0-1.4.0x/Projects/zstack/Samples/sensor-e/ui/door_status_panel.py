"""
door_status_panel.py — 门禁状态看板
E同学：Python应用层负责人
显示字段：pir（访客状态）、door（门状态）、tch（门铃状态）、alert（安全模式）
"""

from PyQt5.QtWidgets import (
    QGroupBox, QVBoxLayout, QHBoxLayout, QLabel, QFrame
)
from PyQt5.QtCore import Qt
from PyQt5.QtGui import QFont

import config


class StatusIndicator(QFrame):
    """单个状态指示器：圆形图标 + 标签 + 状态文字"""

    def __init__(self, title, parent=None):
        super().__init__(parent)
        layout = QVBoxLayout(self)
        layout.setAlignment(Qt.AlignCenter)
        layout.setSpacing(6)

        # 状态圆点
        self._dot = QLabel("●")
        self._dot.setAlignment(Qt.AlignCenter)
        self._dot.setFont(QFont("Arial", 28))
        self._dot.setStyleSheet("color: #555555; background: transparent;")
        layout.addWidget(self._dot)

        # 标题
        title_label = QLabel(title)
        title_label.setAlignment(Qt.AlignCenter)
        title_label.setFont(QFont("Microsoft YaHei", 11, QFont.Bold))
        title_label.setStyleSheet("color: #B0B0D0; background: transparent;")
        layout.addWidget(title_label)

        # 状态文字
        self._status = QLabel("--")
        self._status.setAlignment(Qt.AlignCenter)
        self._status.setFont(QFont("Microsoft YaHei", 10))
        self._status.setStyleSheet("color: #808090; background: transparent;")
        self._status.setObjectName("status_label")
        layout.addWidget(self._status)

    def set_active(self, active, text=""):
        """设置激活状态（绿色/灰色）"""
        if active:
            self._dot.setStyleSheet("color: #2ECC71; background: transparent;")
            self._status.setStyleSheet("color: #2ECC71; background: transparent;")
        else:
            self._dot.setStyleSheet("color: #555555; background: transparent;")
            self._status.setStyleSheet("color: #808090; background: transparent;")
        if text:
            self._status.setText(text)

    def set_color(self, color_hex, text=""):
        """设置自定义颜色"""
        self._dot.setStyleSheet(f"color: {color_hex}; background: transparent;")
        self._status.setStyleSheet(f"color: {color_hex}; background: transparent;")
        if text:
            self._status.setText(text)


class DoorStatusPanel(QGroupBox):
    """
    门禁状态看板面板

    显示四个状态指示器：
        - 访客状态（PIR）
        - 门状态（DOOR）
        - 门铃状态（TCH）
        - 安全模式（ALERT）
    """

    def __init__(self, parent=None):
        super().__init__("🚪 门禁状态看板", parent)
        self._init_ui()

    def _init_ui(self):
        layout = QHBoxLayout(self)
        layout.setSpacing(16)

        # 四个状态指示器
        self._pir_indicator = StatusIndicator("👤 访客状态")
        self._door_indicator = StatusIndicator("🚪 门状态")
        self._tch_indicator = StatusIndicator("🔔 门铃状态")
        self._alert_indicator = StatusIndicator("🛡️ 安全模式")

        layout.addWidget(self._pir_indicator)
        layout.addWidget(self._door_indicator)
        layout.addWidget(self._tch_indicator)
        layout.addWidget(self._alert_indicator)

        # 初始状态
        self._pir_indicator.set_active(False, "无人")
        self._door_indicator.set_active(False, "已关")
        self._tch_indicator.set_active(False, "静默")
        self._alert_indicator.set_color("#2ECC71", "安全")

    def update_data(self, data: dict):
        """
        更新门禁状态看板

        参数：
            data — 传感器数据字典，包含 pir/door/tch/alert 字段
        """
        # PIR 访客状态
        if config.FIELD_PIR in data:
            pir = int(data[config.FIELD_PIR])
            if pir:
                self._pir_indicator.set_color("#E74C3C", "有人靠近")
            else:
                self._pir_indicator.set_active(False, "无人")

        # DOOR 门状态
        if config.FIELD_DOOR in data:
            door = int(data[config.FIELD_DOOR])
            if door:
                self._door_indicator.set_color("#E67E22", "已开")
            else:
                self._door_indicator.set_color("#2ECC71", "已关")

        # TCH 门铃状态
        if config.FIELD_TCH in data:
            tch = int(data[config.FIELD_TCH])
            if tch:
                self._tch_indicator.set_color("#3498DB", "🔔 响铃中")
            else:
                self._tch_indicator.set_active(False, "静默")

        # ALERT 告警等级
        if config.FIELD_ALERT in data:
            alert = int(data[config.FIELD_ALERT])
            if alert == config.ALERT_SAFE:
                self._alert_indicator.set_color("#2ECC71", "✅ 安全")
            elif alert == config.ALERT_ATTENTION:
                self._alert_indicator.set_color("#3498DB", "⚠️ 注意")
            elif alert == config.ALERT_ALARM:
                self._alert_indicator.set_color("#E74C3C", "🚨 报警")

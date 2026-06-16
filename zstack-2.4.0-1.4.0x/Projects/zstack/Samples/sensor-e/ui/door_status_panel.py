"""
door_status_panel.py — 门禁状态看板
E同学：Python应用层负责人
v2.0: 补充 sensor-c 完整字段：flame, gas, grating, stay, night
显示字段：pir, door, tch, alert, flame, gas, grating, stay, night
"""

from PyQt5.QtWidgets import (
    QGroupBox, QVBoxLayout, QHBoxLayout, QGridLayout, QLabel, QFrame, QSizePolicy
)
from PyQt5.QtCore import Qt, QTimer, QPropertyAnimation, QEasingCurve
from PyQt5.QtGui import QFont, QColor

import config


class StatusIndicator(QFrame):
    """单个状态指示器：圆形图标 + 标签 + 状态文字"""

    def __init__(self, title, parent=None):
        super().__init__(parent)
        self.setObjectName("status_card")
        self.setMinimumHeight(104)
        self.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Preferred)
        layout = QVBoxLayout(self)
        layout.setAlignment(Qt.AlignCenter)
        layout.setContentsMargins(10, 10, 10, 10)
        layout.setSpacing(4)

        # 状态圆点
        self._dot = QLabel("●")
        self._dot.setAlignment(Qt.AlignCenter)
        self._dot.setFont(QFont("Arial", 18))
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

        # 告警闪烁状态
        self._blink_active = False
        self._blink_timer = QTimer(self)
        self._blink_timer.timeout.connect(self._toggle_blink)
        self._blink_on = False

    def set_active(self, active, text=""):
        """设置激活状态（绿色/灰色）"""
        self._stop_blink()
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

    def set_alert_blink(self, color_hex, text=""):
        """设置告警闪烁状态"""
        if text:
            self._status.setText(text)
        self._blink_color = color_hex
        if not self._blink_active:
            self._blink_active = True
            self._blink_timer.start(500)

    def _toggle_blink(self):
        """闪烁切换"""
        self._blink_on = not self._blink_on
        if self._blink_on:
            self._dot.setStyleSheet(f"color: {self._blink_color}; background: transparent;")
            self._status.setStyleSheet(f"color: {self._blink_color}; background: transparent;")
        else:
            self._dot.setStyleSheet("color: #555555; background: transparent;")
            self._status.setStyleSheet("color: #555555; background: transparent;")

    def _stop_blink(self):
        """停止闪烁"""
        if self._blink_active:
            self._blink_active = False
            self._blink_timer.stop()
            self._blink_on = False


class EnvValueCard(QFrame):
    """环境数值卡片：图标 + 标题 + 数值 + 单位"""

    def __init__(self, title, unit="", parent=None):
        super().__init__(parent)
        self.setObjectName("env_card")
        self.setMinimumHeight(90)
        self.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Preferred)

        layout = QVBoxLayout(self)
        layout.setAlignment(Qt.AlignCenter)
        layout.setContentsMargins(10, 8, 10, 8)
        layout.setSpacing(2)

        # 标题
        self._title = QLabel(title)
        self._title.setAlignment(Qt.AlignCenter)
        self._title.setFont(QFont("Microsoft YaHei", 10))
        self._title.setStyleSheet("color: #95A3B8; background: transparent;")
        layout.addWidget(self._title)

        # 数值行
        val_row = QHBoxLayout()
        val_row.setAlignment(Qt.AlignCenter)

        self._value = QLabel("--")
        self._value.setAlignment(Qt.AlignCenter)
        self._value.setFont(QFont("Microsoft YaHei", 20, QFont.Bold))
        self._value.setStyleSheet("color: #E8ECF3; background: transparent;")
        val_row.addWidget(self._value)

        if unit:
            self._unit = QLabel(unit)
            self._unit.setAlignment(Qt.AlignBottom)
            self._unit.setFont(QFont("Microsoft YaHei", 10))
            self._unit.setStyleSheet("color: #95A3B8; background: transparent;")
            val_row.addWidget(self._unit)

        layout.addLayout(val_row)

    def set_value(self, value, color="#E8ECF3"):
        """更新数值"""
        self._value.setText(str(value))
        self._value.setStyleSheet(f"color: {color}; background: transparent;")


class DoorStatusPanel(QGroupBox):
    """
    门禁状态看板面板

    v2.0 显示所有传感器状态指示器：
        上排(原有): 访客状态(PIR), 门状态(DOOR), 门铃状态(TCH), 安全模式(ALERT)
        下排(新增): 火焰(FLAME), 可燃气体(GAS), 红外光栅(GRATING), 停留时间(STAY), 夜间模式(NIGHT)
    """

    def __init__(self, parent=None):
        super().__init__("🚪 门禁状态看板", parent)
        self._init_ui()

    def _init_ui(self):
        layout = QVBoxLayout(self)
        layout.setSpacing(10)

        # ---- 上排：原有4个状态指示器 ----
        top_grid = QGridLayout()
        top_grid.setSpacing(10)

        self._pir_indicator = StatusIndicator("👤 访客状态")
        self._door_indicator = StatusIndicator("🚪 门状态")
        self._tch_indicator = StatusIndicator("🔔 门铃状态")
        self._alert_indicator = StatusIndicator("🛡️ 安全模式")

        top_grid.addWidget(self._pir_indicator, 0, 0)
        top_grid.addWidget(self._door_indicator, 0, 1)
        top_grid.addWidget(self._tch_indicator, 0, 2)
        top_grid.addWidget(self._alert_indicator, 0, 3)
        for col in range(4):
            top_grid.setColumnStretch(col, 1)

        layout.addLayout(top_grid)

        # ---- 下排：新增 sensor-c 扩展字段 ----
        bottom_grid = QGridLayout()
        bottom_grid.setSpacing(10)

        self._flame_indicator = StatusIndicator("🔥 火焰检测")
        self._gas_indicator = StatusIndicator("☁️ 可燃气体")
        self._grating_indicator = StatusIndicator("📡 红外光栅")
        self._stay_card = EnvValueCard("⏱️ 停留时间", "秒")
        self._night_indicator = StatusIndicator("🌙 夜间模式")

        bottom_grid.addWidget(self._flame_indicator, 0, 0)
        bottom_grid.addWidget(self._gas_indicator, 0, 1)
        bottom_grid.addWidget(self._grating_indicator, 0, 2)
        bottom_grid.addWidget(self._stay_card, 0, 3)
        bottom_grid.addWidget(self._night_indicator, 0, 4)
        for col in range(5):
            bottom_grid.setColumnStretch(col, 1)

        layout.addLayout(bottom_grid)

        # 初始状态
        self._pir_indicator.set_active(False, "无人")
        self._door_indicator.set_active(False, "已关")
        self._tch_indicator.set_active(False, "静默")
        self._alert_indicator.set_color("#2ECC71", "安全")
        self._flame_indicator.set_active(False, "正常")
        self._gas_indicator.set_active(False, "正常")
        self._grating_indicator.set_active(False, "正常")
        self._stay_card.set_value(0)
        self._night_indicator.set_active(False, "白天")

    def update_data(self, data: dict):
        """
        更新门禁状态看板

        参数：
            data — 传感器数据字典
        """
        # ---- PIR 访客状态 ----
        if config.FIELD_PIR in data:
            pir = int(data[config.FIELD_PIR])
            if pir:
                self._pir_indicator.set_color("#E74C3C", "有人靠近")
            else:
                self._pir_indicator.set_active(False, "无人")

        # ---- DOOR 门状态 ----
        if config.FIELD_DOOR in data:
            door = int(data[config.FIELD_DOOR])
            if door:
                self._door_indicator.set_color("#E67E22", "已开")
            else:
                self._door_indicator.set_color("#2ECC71", "已关")

        # ---- TCH 门铃状态 ----
        if config.FIELD_TCH in data:
            tch = int(data[config.FIELD_TCH])
            if tch:
                self._tch_indicator.set_color("#3498DB", "🔔 响铃中")
            else:
                self._tch_indicator.set_active(False, "静默")

        # ---- ALERT 告警等级 ----
        if config.FIELD_ALERT in data:
            alert = int(data[config.FIELD_ALERT])
            if alert == config.ALERT_SAFE:
                self._alert_indicator.set_color("#2ECC71", "✅ 安全")
            elif alert == config.ALERT_ATTENTION:
                self._alert_indicator.set_color("#3498DB", "⚠️ 注意")
            elif alert == config.ALERT_ALARM:
                self._alert_indicator.set_alert_blink("#E74C3C", "🚨 报警")

        # ---- FLAME 火焰检测 ----
        if config.FIELD_FLAME in data:
            flame = int(data[config.FIELD_FLAME])
            if flame:
                self._flame_indicator.set_alert_blink("#E74C3C", "🔥 检测到")
            else:
                self._flame_indicator.set_active(False, "正常")

        # ---- GAS 可燃气体 ----
        if config.FIELD_GAS in data:
            gas = int(data[config.FIELD_GAS])
            if gas:
                self._gas_indicator.set_alert_blink("#E74C3C", "⚠️ 超标")
            else:
                self._gas_indicator.set_active(False, "正常")

        # ---- GRATING 红外光栅 ----
        if config.FIELD_GRATING in data:
            grating = int(data[config.FIELD_GRATING])
            if grating:
                self._grating_indicator.set_color("#E67E22", "遮断")
            else:
                self._grating_indicator.set_active(False, "正常")

        # ---- STAY 停留时间 ----
        if config.FIELD_STAY in data:
            stay = int(data[config.FIELD_STAY])
            color = "#E74C3C" if stay > 10 else "#E8ECF3"
            self._stay_card.set_value(stay, color)

        # ---- NIGHT 夜间模式 ----
        if config.FIELD_NIGHT in data:
            night = int(data[config.FIELD_NIGHT])
            if night:
                self._night_indicator.set_color("#8E44AD", "🌙 夜间")
            else:
                self._night_indicator.set_active(False, "白天")

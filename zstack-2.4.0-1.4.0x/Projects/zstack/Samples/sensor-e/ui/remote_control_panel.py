"""
remote_control_panel.py — 远程控制区
E同学：Python应用层负责人
实现：开门按钮、门铃触发、告警控制
下发命令格式遵循通信接口说明
"""

from PyQt5.QtWidgets import (
    QGroupBox, QVBoxLayout, QHBoxLayout, QPushButton, QLabel, QFrame
)
from PyQt5.QtCore import Qt, pyqtSignal
from PyQt5.QtGui import QFont


class RemoteControlPanel(QGroupBox):
    """
    远程控制区面板

    功能按钮：
        - 🔓 远程开门   → {unlock=1, rgb=[0,255,0]}
        - 🔔 触发门铃   → {buzz=500, rgb=[0,0,255]}
        - 🚨 触发告警   → {alert=2}  (向sensor-b)
        - ✅ 解除告警   → {reset=1}

    信号：
        command_requested(dict) — 当用户点击按钮时发出，携带命令字典
    """

    command_requested = pyqtSignal(dict)

    def __init__(self, parent=None):
        super().__init__("🎮 远程控制", parent)
        self._init_ui()

    def _init_ui(self):
        layout = QVBoxLayout(self)
        layout.setSpacing(12)

        # ---- 门锁控制行 ----
        lock_row = QHBoxLayout()
        lock_row.setSpacing(12)

        btn_unlock = QPushButton("🔓 远程开门")
        btn_unlock.setObjectName("btn_unlock")
        btn_unlock.setToolTip("下发 {unlock=1}，继电器打开+绿灯+3秒自动关门")
        btn_unlock.clicked.connect(self._on_unlock)
        lock_row.addWidget(btn_unlock)

        btn_lock = QPushButton("🔒 手动关门")
        btn_lock.setToolTip("下发 {unlock=0}")
        btn_lock.clicked.connect(self._on_lock)
        lock_row.addWidget(btn_lock)

        layout.addLayout(lock_row)

        # ---- 门铃控制行 ----
        bell_row = QHBoxLayout()
        bell_row.setSpacing(12)

        btn_doorbell = QPushButton("🔔 触发门铃")
        btn_doorbell.setObjectName("btn_doorbell")
        btn_doorbell.setToolTip("下发 {buzz=500}，蜂鸣器短响500ms+蓝灯闪5秒")
        btn_doorbell.clicked.connect(self._on_doorbell)
        bell_row.addWidget(btn_doorbell)

        btn_buzz_off = QPushButton("🔇 关闭蜂鸣器")
        btn_buzz_off.setToolTip("下发 {buzz=0}")
        btn_buzz_off.clicked.connect(self._on_buzz_off)
        bell_row.addWidget(btn_buzz_off)

        layout.addLayout(bell_row)

        # ---- 分隔线 ----
        separator = QFrame()
        separator.setFrameShape(QFrame.HLine)
        separator.setObjectName("separator")
        layout.addWidget(separator)

        # ---- 告警控制行 ----
        alert_row = QHBoxLayout()
        alert_row.setSpacing(12)

        btn_alarm = QPushButton("🚨 触发告警")
        btn_alarm.setObjectName("btn_alarm")
        btn_alarm.setToolTip("下发 {alert=2}，红灯+蜂鸣器长响")
        btn_alarm.clicked.connect(self._on_alarm)
        alert_row.addWidget(btn_alarm)

        btn_reset = QPushButton("✅ 解除告警")
        btn_reset.setObjectName("btn_reset")
        btn_reset.setToolTip("下发 {reset=1}，恢复安全状态+绿灯")
        btn_reset.clicked.connect(self._on_reset)
        alert_row.addWidget(btn_reset)

        layout.addLayout(alert_row)

        # ---- 夜间模式控制行 ----
        night_row = QHBoxLayout()
        night_row.setSpacing(12)

        btn_night_on = QPushButton("🌙 启用夜间模式")
        btn_night_on.setToolTip("下发 {night=1} 到 sensor-c")
        btn_night_on.clicked.connect(self._on_night_on)
        night_row.addWidget(btn_night_on)

        btn_night_off = QPushButton("☀️ 白天模式")
        btn_night_off.setToolTip("下发 {night=0} 到 sensor-c")
        btn_night_off.clicked.connect(self._on_night_off)
        night_row.addWidget(btn_night_off)

        layout.addLayout(night_row)

        # ---- 状态提示 ----
        self._status_label = QLabel("就绪")
        self._status_label.setAlignment(Qt.AlignCenter)
        self._status_label.setStyleSheet(
            "color: #808090; font-size: 11px; padding: 4px; background: transparent;"
        )
        layout.addWidget(self._status_label)

    # ---- 按钮点击处理 ----

    def _on_unlock(self):
        """远程开门：unlock=1 + RGB绿灯"""
        cmd = {"unlock": 1, "rgb": [0, 255, 0]}
        self._status_label.setText("✅ 已发送开门命令")
        self._status_label.setStyleSheet(
            "color: #2ECC71; font-size: 11px; padding: 4px; background: transparent;"
        )
        self.command_requested.emit(cmd)

    def _on_lock(self):
        """手动关门：unlock=0"""
        cmd = {"unlock": 0}
        self._status_label.setText("🔒 已发送关门命令")
        self._status_label.setStyleSheet(
            "color: #808090; font-size: 11px; padding: 4px; background: transparent;"
        )
        self.command_requested.emit(cmd)

    def _on_doorbell(self):
        """触发门铃：buzz=500 + RGB蓝灯"""
        cmd = {"buzz": 500, "rgb": [0, 0, 255]}
        self._status_label.setText("🔔 已发送门铃命令")
        self._status_label.setStyleSheet(
            "color: #3498DB; font-size: 11px; padding: 4px; background: transparent;"
        )
        self.command_requested.emit(cmd)

    def _on_buzz_off(self):
        """关闭蜂鸣器：buzz=0"""
        cmd = {"buzz": 0}
        self._status_label.setText("🔇 已关闭蜂鸣器")
        self._status_label.setStyleSheet(
            "color: #808090; font-size: 11px; padding: 4px; background: transparent;"
        )
        self.command_requested.emit(cmd)

    def _on_alarm(self):
        """触发告警：alert=2"""
        cmd = {"alert": 2}
        self._status_label.setText("🚨 已触发告警")
        self._status_label.setStyleSheet(
            "color: #E74C3C; font-size: 11px; padding: 4px; background: transparent;"
        )
        self.command_requested.emit(cmd)

    def _on_reset(self):
        """解除告警：reset=1"""
        cmd = {"reset": 1}
        self._status_label.setText("✅ 已解除告警")
        self._status_label.setStyleSheet(
            "color: #2ECC71; font-size: 11px; padding: 4px; background: transparent;"
        )
        self.command_requested.emit(cmd)

    def _on_night_on(self):
        """启用夜间模式：night=1（发往sensor-c）"""
        cmd = {"night": 1, "_target": "sensor-c"}
        self._status_label.setText("🌙 已启用夜间模式")
        self._status_label.setStyleSheet(
            "color: #9B59B6; font-size: 11px; padding: 4px; background: transparent;"
        )
        self.command_requested.emit(cmd)

    def _on_night_off(self):
        """白天模式：night=0（发往sensor-c）"""
        cmd = {"night": 0, "_target": "sensor-c"}
        self._status_label.setText("☀️ 已切换白天模式")
        self._status_label.setStyleSheet(
            "color: #F39C12; font-size: 11px; padding: 4px; background: transparent;"
        )
        self.command_requested.emit(cmd)

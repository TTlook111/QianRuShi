"""
remote_control_panel.py — 远程控制区
E同学：Python应用层负责人
v2.1: 详细悬停提示（模块/命令/反应）+ 美化UI
下发命令格式遵循通信接口说明
"""

from PyQt5.QtWidgets import (
    QGroupBox, QVBoxLayout, QHBoxLayout, QPushButton, QLabel,
    QFrame
)
from PyQt5.QtCore import Qt, pyqtSignal


def make_tooltip(title, target, command, reaction, detail=""):
    """生成格式化的悬停提示"""
    lines = [
        f"<b>{title}</b>",
        f"<hr style='color:#34425A;'>",
        f"<b>🎯 目标模块：</b>{target}",
        f"<b>📤 发送命令：</b><code>{command}</code>",
        f"<b>📥 硬件反应：</b>{reaction}",
    ]
    if detail:
        lines.append(f"<br><b>💡 说明：</b>{detail}")
    return "<br>".join(lines)


class RemoteControlPanel(QGroupBox):
    """
    远程控制区面板

    v2.1: 每个按钮都有详细悬停提示
    """

    command_requested = pyqtSignal(dict)

    def __init__(self, parent=None):
        super().__init__("🎮 远程控制", parent)
        self._init_ui()

    def _init_ui(self):
        layout = QVBoxLayout(self)
        layout.setSpacing(12)

        # ==================== sensor-b 控制区 ====================
        sensor_b_header = QLabel("━━━ sensor-b 控制执行节点 (602) ━━━")
        sensor_b_header.setAlignment(Qt.AlignCenter)
        sensor_b_header.setStyleSheet(
            "color: #3498DB; font-size: 12px; font-weight: bold; "
            "background: #171F2D; padding: 4px; border-radius: 4px;"
        )
        layout.addWidget(sensor_b_header)

        # ---- 门锁控制行 ----
        lock_label = QLabel("🔒 门锁控制")
        lock_label.setStyleSheet("color: #95A3B8; font-size: 11px; background: transparent;")
        layout.addWidget(lock_label)

        lock_row = QHBoxLayout()
        lock_row.setSpacing(12)

        btn_unlock = QPushButton("🔓 远程开门")
        btn_unlock.setObjectName("btn_unlock")
        btn_unlock.setToolTip(make_tooltip(
            "🔓 远程开门",
            "sensor-b (控制执行节点 602)",
            '{"unlock": 1, "rgb": [0, 255, 0]}',
            "继电器打开 → 门锁保持开启<br>RGB灯变绿色<br>需要点击“手动关门”才会关闭",
            "开门同时会记录到数据库，事件时间线会显示"
        ))
        btn_unlock.clicked.connect(self._on_unlock)
        lock_row.addWidget(btn_unlock)

        btn_lock = QPushButton("🔒 手动关门")
        btn_lock.setToolTip(make_tooltip(
            "🔒 手动关门",
            "sensor-b (控制执行节点 602)",
            '{"unlock": 0}',
            "继电器关闭 → 门锁锁定",
            "发送 unlock=0，立即关闭门锁继电器"
        ))
        btn_lock.clicked.connect(self._on_lock)
        lock_row.addWidget(btn_lock)

        layout.addLayout(lock_row)

        # ---- 门铃控制行 ----
        bell_label = QLabel("🔔 门铃控制")
        bell_label.setStyleSheet("color: #95A3B8; font-size: 11px; background: transparent;")
        layout.addWidget(bell_label)

        bell_row = QHBoxLayout()
        bell_row.setSpacing(12)

        btn_doorbell = QPushButton("🔔 触发门铃")
        btn_doorbell.setObjectName("btn_doorbell")
        btn_doorbell.setToolTip(make_tooltip(
            "🔔 触发门铃",
            "sensor-b (控制执行节点 602)",
            '{"buzz": 500, "rgb": [0, 0, 255]}',
            "蜂鸣器短响 500ms<br>RGB灯变蓝色闪烁 5 秒<br>5秒后恢复告警等级对应颜色",
            "模拟访客按门铃效果"
        ))
        btn_doorbell.clicked.connect(self._on_doorbell)
        bell_row.addWidget(btn_doorbell)

        btn_buzz_off = QPushButton("🔇 关闭蜂鸣器")
        btn_buzz_off.setToolTip(make_tooltip(
            "🔇 关闭蜂鸣器",
            "sensor-b (控制执行节点 602)",
            '{"buzz": 0}',
            "蜂鸣器立即关闭<br>设置用户静音标志（防止告警重新触发）",
            "切换告警等级时会清除静音标志"
        ))
        btn_buzz_off.clicked.connect(self._on_buzz_off)
        bell_row.addWidget(btn_buzz_off)

        layout.addLayout(bell_row)

        # ---- 分隔线 ----
        separator1 = QFrame()
        separator1.setFrameShape(QFrame.HLine)
        separator1.setObjectName("separator")
        layout.addWidget(separator1)

        # ---- 告警控制行 ----
        alert_label = QLabel("🚨 告警控制")
        alert_label.setStyleSheet("color: #95A3B8; font-size: 11px; background: transparent;")
        layout.addWidget(alert_label)

        alert_row = QHBoxLayout()
        alert_row.setSpacing(12)

        btn_alarm = QPushButton("🚨 触发告警")
        btn_alarm.setObjectName("btn_alarm")
        btn_alarm.setToolTip(make_tooltip(
            "🚨 触发告警",
            "sensor-b (控制执行节点 602)",
            '{"alert": 2}',
            "告警等级设为 2（报警）<br>RGB灯变红色<br>蜂鸣器持续长响",
            "最高告警等级，需要手动解除"
        ))
        btn_alarm.clicked.connect(self._on_alarm)
        alert_row.addWidget(btn_alarm)

        btn_reset = QPushButton("✅ 解除告警")
        btn_reset.setObjectName("btn_reset")
        btn_reset.setToolTip(make_tooltip(
            "✅ 解除告警",
            "sensor-b (控制执行节点 602)",
            '{"reset": 1}',
            "告警等级恢复为 0（安全）<br>关闭蜂鸣器<br>清除所有标志<br>关门<br>RGB灯变绿色",
            "系统完全复位到安全状态"
        ))
        btn_reset.clicked.connect(self._on_reset)
        alert_row.addWidget(btn_reset)

        layout.addLayout(alert_row)

        # ---- 分隔线 ----
        separator2 = QFrame()
        separator2.setFrameShape(QFrame.HLine)
        separator2.setObjectName("separator")
        layout.addWidget(separator2)

        # ==================== sensor-c 控制区 ====================
        sensor_c_header = QLabel("━━━ sensor-c 安防检测节点 (603) ━━━")
        sensor_c_header.setAlignment(Qt.AlignCenter)
        sensor_c_header.setStyleSheet(
            "color: #8E44AD; font-size: 12px; font-weight: bold; "
            "background: #171F2D; padding: 4px; border-radius: 4px;"
        )
        layout.addWidget(sensor_c_header)

        # ---- 夜间模式控制行 ----
        night_label = QLabel("🌙 夜间模式")
        night_label.setStyleSheet("color: #95A3B8; font-size: 11px; background: transparent;")
        layout.addWidget(night_label)

        night_row = QHBoxLayout()
        night_row.setSpacing(12)

        btn_night_on = QPushButton("🌙 启用夜间模式")
        btn_night_on.setObjectName("btn_night")
        btn_night_on.setToolTip(make_tooltip(
            "🌙 启用夜间模式",
            "sensor-c (安防检测节点 603)",
            '{"night": 1}',
            "启用 22:00-06:00 安防时段<br>夜间开门自动触发入侵告警<br>夜间震动触发注意告警",
            "夜间模式下安防灵敏度更高"
        ))
        btn_night_on.clicked.connect(self._on_night_on)
        night_row.addWidget(btn_night_on)

        btn_night_off = QPushButton("☀️ 白天模式")
        btn_night_off.setToolTip(make_tooltip(
            "☀️ 白天模式",
            "sensor-c (安防检测节点 603)",
            '{"night": 0}',
            "关闭夜间安防时段<br>降低告警灵敏度",
            "白天模式下开门不会触发入侵告警"
        ))
        btn_night_off.clicked.connect(self._on_night_off)
        night_row.addWidget(btn_night_off)

        layout.addLayout(night_row)

        # ---- 状态提示 ----
        layout.addStretch()
        self._status_label = QLabel("就绪")
        self._status_label.setAlignment(Qt.AlignCenter)
        self._status_label.setStyleSheet(
            "color: #808090; font-size: 11px; padding: 4px; background: transparent;"
        )
        layout.addWidget(self._status_label)

    # ==================== sensor-b 按钮点击处理 ====================

    def _on_unlock(self):
        """远程开门：unlock=1 + RGB绿灯"""
        cmd = {"unlock": 1, "rgb": [0, 255, 0]}
        self._set_status("✅ 已发送开门命令 → sensor-b", "#2ECC71")
        self.command_requested.emit(cmd)

    def _on_lock(self):
        """手动关门：unlock=0"""
        cmd = {"unlock": 0}
        self._set_status("🔒 已发送关门命令 → sensor-b", "#808090")
        self.command_requested.emit(cmd)

    def _on_doorbell(self):
        """触发门铃：buzz=500 + RGB蓝灯"""
        cmd = {"buzz": 500, "rgb": [0, 0, 255]}
        self._set_status("🔔 已发送门铃命令 → sensor-b", "#3498DB")
        self.command_requested.emit(cmd)

    def _on_buzz_off(self):
        """关闭蜂鸣器：buzz=0"""
        cmd = {"buzz": 0}
        self._set_status("🔇 已关闭蜂鸣器 → sensor-b", "#808090")
        self.command_requested.emit(cmd)

    def _on_alarm(self):
        """触发告警：alert=2"""
        cmd = {"alert": 2}
        self._set_status("🚨 已触发告警 → sensor-b", "#E74C3C")
        self.command_requested.emit(cmd)

    def _on_reset(self):
        """解除告警：reset=1"""
        cmd = {"reset": 1}
        self._set_status("✅ 已解除告警 → sensor-b", "#2ECC71")
        self.command_requested.emit(cmd)

    # ==================== sensor-c 按钮点击处理 ====================

    def _on_night_on(self):
        """启用夜间模式：night=1"""
        cmd = {"night": 1, "_target": "sensor-c"}
        self._set_status("🌙 已启用夜间模式 → sensor-c", "#8E44AD")
        self.command_requested.emit(cmd)

    def _on_night_off(self):
        """白天模式：night=0"""
        cmd = {"night": 0, "_target": "sensor-c"}
        self._set_status("☀️ 已切换白天模式 → sensor-c", "#F39C12")
        self.command_requested.emit(cmd)

    # ==================== 工具方法 ====================

    def _set_status(self, text, color):
        """设置状态文字和颜色"""
        self._status_label.setText(text)
        self._status_label.setStyleSheet(
            f"color: {color}; font-size: 11px; padding: 4px; background: transparent;"
        )

    def set_send_result(self, success: bool):
        """显示通信层发送结果。"""
        if success:
            self._set_status("✅ 平台已接收命令", "#2ECC71")
        else:
            self._set_status("❌ 命令发送失败", "#E74C3C")

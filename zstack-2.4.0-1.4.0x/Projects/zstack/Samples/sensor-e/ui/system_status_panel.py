"""
system_status_panel.py — 运行诊断面板
显示智云连接、节点收包、数据库摘要和最近解析数据。
"""

from datetime import datetime

from PyQt5.QtWidgets import (
    QGroupBox, QVBoxLayout, QGridLayout, QLabel, QPlainTextEdit, QFrame,
    QSizePolicy
)
from PyQt5.QtCore import Qt

import config


class MetricCard(QFrame):
    """紧凑指标卡。"""

    def __init__(self, title, value="--", accent="#3A7BDB", parent=None):
        super().__init__(parent)
        self.setObjectName("metric_card")
        self.setMinimumHeight(58)
        self.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Fixed)
        self._accent = accent

        layout = QVBoxLayout(self)
        layout.setContentsMargins(10, 8, 10, 8)
        layout.setSpacing(4)

        title_label = QLabel(title)
        title_label.setObjectName("metric_title")
        title_label.setAlignment(Qt.AlignCenter)
        layout.addWidget(title_label)

        self._value_label = QLabel(value)
        self._value_label.setObjectName("metric_value")
        self._value_label.setAlignment(Qt.AlignCenter)
        self._value_label.setStyleSheet(f"color: {accent};")
        layout.addWidget(self._value_label)

    def set_value(self, value, accent=None):
        self._value_label.setText(str(value))
        if accent:
            self._accent = accent
        self._value_label.setStyleSheet(f"color: {self._accent};")


class SystemStatusPanel(QGroupBox):
    """运行诊断：连接、节点、数据库、最近数据。"""

    def __init__(self, parent=None):
        super().__init__("🧭 运行诊断", parent)
        self._node_counts = {name: 0 for name in config.NODE_MAC}
        self._init_ui()

    def _init_ui(self):
        layout = QVBoxLayout(self)
        layout.setSpacing(8)

        cards = QGridLayout()
        cards.setSpacing(8)

        self._cloud_card = MetricCard("智云", "连接中", "#F39C12")
        self._sensor_a_card = MetricCard("sensor-a", "0 包", "#42A5F5")
        self._sensor_b_card = MetricCard("sensor-b", "0 包", "#2ECC71")
        self._sensor_c_card = MetricCard("sensor-c", "0 包", "#E67E22")
        self._db_card = MetricCard("数据库", "--", "#B0B0D0")

        for idx, card in enumerate((
            self._cloud_card,
            self._sensor_a_card,
            self._sensor_b_card,
            self._sensor_c_card,
            self._db_card,
        )):
            cards.addWidget(card, idx // 3, idx % 3)

        for col in range(3):
            cards.setColumnStretch(col, 1)

        layout.addLayout(cards)

        self._log = QPlainTextEdit()
        self._log.setObjectName("diagnostic_log")
        self._log.setReadOnly(True)
        self._log.setMaximumBlockCount(120)
        self._log.setPlaceholderText("等待智云数据...")
        layout.addWidget(self._log)

    def set_cloud_state(self, text, ok=False):
        self._cloud_card.set_value(text, "#2ECC71" if ok else "#F39C12")

    def set_disconnected(self):
        self._cloud_card.set_value("已断开", "#E74C3C")

    def update_database_counts(self, access_count, env_count):
        self._db_card.set_value(f"事件 {access_count} / 环境 {env_count}", "#B0B0D0")

    def append_data(self, data):
        addr = data.get("_addr", "")
        node_name = self._node_name(addr)
        if node_name:
            self._node_counts[node_name] += 1
            getattr(self, f"_{node_name.replace('-', '_')}_card").set_value(
                f"{self._node_counts[node_name]} 包",
                "#2ECC71"
            )

        visible = {
            k: v for k, v in data.items()
            if not k.startswith("_")
        }
        ts = datetime.now().strftime("%H:%M:%S")
        source = node_name or addr or "unknown"
        self._log.appendPlainText(f"[{ts}] {source}  {visible}")

    def append_command(self, target, cmd):
        ts = datetime.now().strftime("%H:%M:%S")
        self._log.appendPlainText(f"[{ts}] -> {target}  {cmd}")

    def _node_name(self, addr):
        for name, mac in config.NODE_MAC.items():
            if addr == mac:
                return name
        return ""

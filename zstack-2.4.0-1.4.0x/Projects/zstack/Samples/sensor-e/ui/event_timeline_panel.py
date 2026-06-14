"""
event_timeline_panel.py — 访客事件时间线
E同学：Python应用层负责人
使用 QTableWidget 显示访客事件日志
列：时间、事件类型、传感器、详情、告警等级
"""

from datetime import datetime

from PyQt5.QtWidgets import (
    QGroupBox, QVBoxLayout, QTableWidget, QTableWidgetItem,
    QHeaderView, QAbstractItemView
)
from PyQt5.QtCore import Qt
from PyQt5.QtGui import QColor, QBrush

import config


# 事件类型中文映射
EVENT_TYPE_NAMES = {
    config.EVENT_DOORBELL: "🔔 门铃",
    config.EVENT_PIR_DETECT: "👤 检测到人",
    config.EVENT_DOOR_OPEN: "🚪 门已打开",
    config.EVENT_DOOR_CLOSE: "🚪 门已关闭",
    config.EVENT_LOITER: "⚠️ 有人徘徊",
    config.EVENT_INTRUSION: "🚨 夜间入侵",
    config.EVENT_REMOTE_UNLOCK: "🔓 远程开门",
    config.EVENT_ALERT_RESET: "✅ 告警解除",
}

# 告警等级颜色
ALERT_COLORS = {
    0: QColor("#2ECC71"),   # 安全-绿
    1: QColor("#3498DB"),   # 注意-蓝
    2: QColor("#E74C3C"),   # 报警-红
}

ALERT_NAMES = {
    0: "安全",
    1: "注意",
    2: "报警",
}


class EventTimelinePanel(QGroupBox):
    """
    访客事件时间线面板

    实时显示门禁相关事件：
        - 门铃按下
        - PIR检测到人
        - 门开/关
        - 有人徘徊
        - 夜间入侵告警
        - 远程开门
        - 告警解除
    """

    MAX_ROWS = 200  # 最大显示行数

    def __init__(self, parent=None):
        super().__init__("📋 访客事件时间线", parent)
        self._init_ui()

    def _init_ui(self):
        layout = QVBoxLayout(self)

        # 创建表格
        self._table = QTableWidget(0, 5)
        self._table.setHorizontalHeaderLabels(
            ["时间", "事件类型", "传感器", "详情", "告警等级"]
        )

        # 表格样式
        self._table.setAlternatingRowColors(True)
        self._table.setSelectionBehavior(QAbstractItemView.SelectRows)
        self._table.setEditTriggers(QAbstractItemView.NoEditTriggers)
        self._table.verticalHeader().setVisible(False)

        # 列宽设置
        header = self._table.horizontalHeader()
        header.setSectionResizeMode(0, QHeaderView.Fixed)
        header.resizeSection(0, 150)
        header.setSectionResizeMode(1, QHeaderView.Fixed)
        header.resizeSection(1, 120)
        header.setSectionResizeMode(2, QHeaderView.Fixed)
        header.resizeSection(2, 80)
        header.setSectionResizeMode(3, QHeaderView.Stretch)
        header.setSectionResizeMode(4, QHeaderView.Fixed)
        header.resizeSection(4, 80)

        layout.addWidget(self._table)

    def add_event(self, event_type, sensor="", detail="", alert=0, timestamp=None):
        """
        添加一条事件到时间线（插入到最上方）

        参数：
            event_type — 事件类型（config.EVENT_xxx）
            sensor     — 触发传感器名称
            detail     — 详情描述
            alert      — 告警等级 0/1/2
            timestamp  — 事件时间（默认当前时间）
        """
        # 限制行数
        if self._table.rowCount() >= self.MAX_ROWS:
            self._table.removeRow(self._table.rowCount() - 1)

        # 在表格顶部插入新行
        self._table.insertRow(0)

        # 时间
        ts = timestamp or datetime.now()
        time_item = QTableWidgetItem(ts.strftime("%m-%d %H:%M:%S"))
        time_item.setTextAlignment(Qt.AlignCenter)
        self._table.setItem(0, 0, time_item)

        # 事件类型
        type_name = EVENT_TYPE_NAMES.get(event_type, event_type)
        type_item = QTableWidgetItem(type_name)
        type_item.setTextAlignment(Qt.AlignCenter)
        self._table.setItem(0, 1, type_item)

        # 传感器
        sensor_item = QTableWidgetItem(sensor)
        sensor_item.setTextAlignment(Qt.AlignCenter)
        self._table.setItem(0, 2, sensor_item)

        # 详情
        detail_item = QTableWidgetItem(detail)
        self._table.setItem(0, 3, detail_item)

        # 告警等级（带颜色）
        alert_name = ALERT_NAMES.get(alert, str(alert))
        alert_item = QTableWidgetItem(alert_name)
        alert_item.setTextAlignment(Qt.AlignCenter)
        color = ALERT_COLORS.get(alert, QColor("#808090"))
        alert_item.setForeground(QBrush(color))
        self._table.setItem(0, 4, alert_item)

        # 如果是告警事件，整行高亮
        if alert >= 2:
            for col in range(5):
                item = self._table.item(0, col)
                if item:
                    item.setBackground(QBrush(QColor(230, 76, 60, 30)))

    def process_sensor_data(self, data: dict):
        """
        处理传感器数据，自动判断并生成事件

        参数：
            data — 传感器数据字典
        """
        # 门铃触发
        if data.get(config.FIELD_TCH) == 1:
            self.add_event(
                config.EVENT_DOORBELL,
                sensor="sensor-c",
                detail="有人按下门铃",
                alert=0
            )

        # PIR 检测到人
        if data.get(config.FIELD_PIR) == 1:
            stay = data.get(config.FIELD_STAY, 0)
            if stay > 0:
                self.add_event(
                    config.EVENT_LOITER,
                    sensor="sensor-c",
                    detail=f"有人徘徊 {stay} 秒",
                    alert=1
                )
            else:
                self.add_event(
                    config.EVENT_PIR_DETECT,
                    sensor="sensor-c",
                    detail="门口检测到人",
                    alert=0
                )

        # 门状态变化
        if config.FIELD_DOOR in data:
            door = int(data[config.FIELD_DOOR])
            if door:
                alert = int(data.get(config.FIELD_ALERT, 0))
                night = int(data.get(config.FIELD_NIGHT, 0))
                if night and alert >= 2:
                    self.add_event(
                        config.EVENT_INTRUSION,
                        sensor="sensor-c",
                        detail="夜间未授权开门",
                        alert=2
                    )
                else:
                    self.add_event(
                        config.EVENT_DOOR_OPEN,
                        sensor="sensor-c",
                        detail="门被打开",
                        alert=alert
                    )

    def load_from_db(self, logs):
        """从数据库加载历史事件"""
        for log in reversed(logs):
            self.add_event(
                event_type=log.event_type,
                sensor=log.sensor,
                detail=f"value={log.value}",
                alert=log.alert,
                timestamp=log.create_time
            )

"""
env_info_panel.py — 环境信息面板
E同学：Python应用层负责人
v2.0: LCD数字显示 + 实时数据曲线图
显示字段：temp（温度）、humi（湿度）、lux（光照）
"""

from collections import deque

from PyQt5.QtWidgets import (
    QGroupBox, QGridLayout, QVBoxLayout, QHBoxLayout,
    QLabel, QLCDNumber, QFrame, QSizePolicy, QTabWidget, QWidget
)
from PyQt5.QtCore import Qt
from PyQt5.QtGui import QFont

import config

# matplotlib 嵌入 PyQt5
try:
    from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg
    from matplotlib.figure import Figure
    import matplotlib.pyplot as plt
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False


class EnvGauge(QFrame):
    """单个环境指标仪表：图标 + LCD数字 + 单位"""

    def __init__(self, icon, title, unit, color="#00E676", parent=None):
        super().__init__(parent)
        self.setObjectName("env_card")
        self.setMinimumHeight(112)
        self.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Preferred)
        layout = QVBoxLayout(self)
        layout.setAlignment(Qt.AlignCenter)
        layout.setContentsMargins(12, 10, 12, 10)
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
        self._lcd.setMinimumHeight(48)
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


class RealTimeChart(FigureCanvasQTAgg if HAS_MATPLOTLIB else QWidget):
    """实时数据曲线图（基于matplotlib）"""

    def __init__(self, title="", ylabel="", color="#3498DB", max_points=60, parent=None):
        if not HAS_MATPLOTLIB:
            super().__init__(parent)
            return

        plt.style.use('dark_background')
        self.fig = Figure(figsize=(6, 2.5), dpi=80, facecolor='#171F2D')
        self.axes = self.fig.add_subplot(111)
        self.axes.set_facecolor('#111722')
        self.fig.subplots_adjust(left=0.12, right=0.95, top=0.88, bottom=0.18)

        super().__init__(self.fig)
        self.setParent(parent)

        self._title = title
        self._ylabel = ylabel
        self._color = color
        self._max_points = max_points
        self._x_data = deque(maxlen=max_points)
        self._y_data = deque(maxlen=max_points)
        self._counter = 0

        # 设置样式
        self._setup_axes()

    def _setup_axes(self):
        """设置坐标轴样式"""
        self.axes.clear()
        self.axes.set_title(self._title, color='#C9D3E2', fontsize=11, fontweight='bold', pad=8)
        self.axes.set_ylabel(self._ylabel, color='#95A3B8', fontsize=9)
        self.axes.tick_params(colors='#6A7A8C', labelsize=8)
        self.axes.spines['bottom'].set_color('#34425A')
        self.axes.spines['left'].set_color('#34425A')
        self.axes.spines['top'].set_visible(False)
        self.axes.spines['right'].set_visible(False)
        self.axes.grid(True, color='#2E3A4D', linewidth=0.5, alpha=0.5)

    def add_point(self, value):
        """添加一个数据点并刷新图表"""
        if not HAS_MATPLOTLIB:
            return

        self._counter += 1
        self._x_data.append(self._counter)
        self._y_data.append(value)

        self.axes.clear()
        self._setup_axes()

        x = list(self._x_data)
        y = list(self._y_data)

        # 绘制曲线
        self.axes.plot(x, y, color=self._color, linewidth=1.5, antialiased=True)

        # 填充曲线下方
        if len(x) > 1:
            self.axes.fill_between(x, y, alpha=0.15, color=self._color)

        # 最新值标注
        if y:
            self.axes.annotate(
                f'{y[-1]:.1f}',
                xy=(x[-1], y[-1]),
                xytext=(5, 5),
                textcoords='offset points',
                color=self._color,
                fontsize=9,
                fontweight='bold'
            )

        self.draw()


class EnvInfoPanel(QGroupBox):
    """
    环境信息面板

    v2.0: 数字仪表 + 实时曲线图（双Tab切换）

    显示三个环境指标：
        - 🌡️ 温度（℃）
        - 💧 湿度（%RH）
        - ☀️ 光照（lux）
    """

    def __init__(self, parent=None):
        super().__init__("🌍 环境信息", parent)
        self._init_ui()

    def _init_ui(self):
        layout = QVBoxLayout(self)
        layout.setSpacing(8)

        # ---- 数字仪表行 ----
        gauge_grid = QGridLayout()
        gauge_grid.setSpacing(10)

        self._temp_gauge = EnvGauge("🌡️", "温度", "℃", "#FF7043")
        self._humi_gauge = EnvGauge("💧", "湿度", "%RH", "#42A5F5")
        self._lux_gauge = EnvGauge("☀️", "光照", "lux", "#FFCA28")

        gauge_grid.addWidget(self._temp_gauge, 0, 0)
        gauge_grid.addWidget(self._humi_gauge, 0, 1)
        gauge_grid.addWidget(self._lux_gauge, 0, 2)
        for col in range(3):
            gauge_grid.setColumnStretch(col, 1)

        layout.addLayout(gauge_grid)

        # ---- 实时曲线图（如果matplotlib可用） ----
        if HAS_MATPLOTLIB:
            self._tab_widget = QTabWidget()
            self._tab_widget.setObjectName("chart_tabs")

            self._temp_chart = RealTimeChart("温度变化", "℃", "#FF7043")
            self._humi_chart = RealTimeChart("湿度变化", "%RH", "#42A5F5")
            self._lux_chart = RealTimeChart("光照变化", "lux", "#FFCA28")

            self._tab_widget.addTab(self._temp_chart, "📈 温度")
            self._tab_widget.addTab(self._humi_chart, "📈 湿度")
            self._tab_widget.addTab(self._lux_chart, "📈 光照")

            layout.addWidget(self._tab_widget)
        else:
            no_chart_label = QLabel("💡 安装 matplotlib 可显示实时曲线图: pip install matplotlib")
            no_chart_label.setStyleSheet("color: #95A3B8; font-size: 11px; padding: 8px;")
            no_chart_label.setAlignment(Qt.AlignCenter)
            layout.addWidget(no_chart_label)

    def update_data(self, data: dict):
        """
        更新环境信息面板

        参数：
            data — 传感器数据字典，包含 temp/humi/lux 字段
        """
        if config.FIELD_TEMP in data:
            val = float(data[config.FIELD_TEMP])
            self._temp_gauge.set_value(val)
            if HAS_MATPLOTLIB:
                self._temp_chart.add_point(val)

        if config.FIELD_HUMI in data:
            val = float(data[config.FIELD_HUMI])
            self._humi_gauge.set_value(val)
            if HAS_MATPLOTLIB:
                self._humi_chart.add_point(val)

        if config.FIELD_LUX in data:
            val = int(data[config.FIELD_LUX])
            self._lux_gauge.set_value(val)
            if HAS_MATPLOTLIB:
                self._lux_chart.add_point(val)

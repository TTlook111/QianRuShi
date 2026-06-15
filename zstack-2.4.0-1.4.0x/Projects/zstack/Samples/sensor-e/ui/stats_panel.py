"""
stats_panel.py — 访客统计图表
E同学：Python应用层负责人
使用 matplotlib 嵌入 PyQt5 绘制日/周访客统计图
"""

from PyQt5.QtWidgets import QGroupBox, QVBoxLayout, QGridLayout, QPushButton
from PyQt5.QtCore import pyqtSignal

from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.figure import Figure
import matplotlib

# 设置 matplotlib 中文字体
matplotlib.rcParams["font.sans-serif"] = ["SimHei", "Microsoft YaHei", "Arial"]
matplotlib.rcParams["axes.unicode_minus"] = False


class StatsPanel(QGroupBox):
    """
    访客统计图表面板

    功能：
        - 日访客次数条形图（最近7天）
        - 事件类型分布饼图
        - 支持手动刷新
    """
    export_requested = pyqtSignal(str)

    def __init__(self, parent=None):
        super().__init__("📊 访客统计", parent)
        self._db = None
        self._init_ui()

    def set_database(self, db):
        """设置数据库引用"""
        self._db = db

    def _init_ui(self):
        layout = QVBoxLayout(self)

        # 按钮行
        btn_row = QGridLayout()
        btn_row.setSpacing(8)

        btn_daily = QPushButton("📅 日统计")
        btn_daily.clicked.connect(self._show_daily_chart)
        btn_row.addWidget(btn_daily, 0, 0)

        btn_event = QPushButton("📊 事件统计")
        btn_event.clicked.connect(self._show_event_chart)
        btn_row.addWidget(btn_event, 0, 1)

        btn_refresh = QPushButton("🔄 刷新")
        btn_refresh.clicked.connect(self._show_daily_chart)
        btn_row.addWidget(btn_refresh, 0, 2)

        btn_export_log = QPushButton("⬇️ 导出日志")
        btn_export_log.clicked.connect(lambda: self.export_requested.emit("access_log"))
        btn_row.addWidget(btn_export_log, 1, 0)

        btn_export_env = QPushButton("⬇️ 导出环境")
        btn_export_env.clicked.connect(lambda: self.export_requested.emit("door_env"))
        btn_row.addWidget(btn_export_env, 1, 1)

        for col in range(3):
            btn_row.setColumnStretch(col, 1)

        layout.addLayout(btn_row)

        # matplotlib 画布
        self._figure = Figure(figsize=(7, 2.8), facecolor="#171F2D")
        self._canvas = FigureCanvas(self._figure)
        self._canvas.setMinimumHeight(230)
        layout.addWidget(self._canvas)

        # 默认显示空图表
        self._show_empty_chart()

    def _show_empty_chart(self, message="暂无统计数据\n点击上方按钮刷新"):
        """显示空白占位图表"""
        self._figure.clear()
        ax = self._figure.add_subplot(111)
        ax.set_facecolor("#171F2D")
        ax.text(0.5, 0.5, message,
                ha="center", va="center", fontsize=14,
                color="#808090", transform=ax.transAxes)
        ax.set_xticks([])
        ax.set_yticks([])
        for spine in ax.spines.values():
            spine.set_visible(False)
        self._canvas.draw()

    def _show_daily_chart(self):
        """显示最近7天每日访客次数条形图"""
        self._figure.clear()
        ax = self._figure.add_subplot(111)
        ax.set_facecolor("#171F2D")

        if self._db:
            data = self._db.get_daily_visitor_count(days=7)
        else:
            data = []

        if not data:
            self._show_empty_chart("暂无访客统计数据\n触发门铃/PIR/门磁后刷新")
            return

        days = [d[0][-5:] for d in data]      # 只显示 MM-DD
        counts = [d[1] for d in data]

        # 渐变色条形图
        colors = ["#3A7BDB", "#4A8BEB", "#5A9BFB", "#6AABFF",
                  "#7ABBFF", "#8ACBFF", "#9ADBFF"]
        bar_colors = colors[:len(days)]
        if len(bar_colors) < len(days):
            bar_colors = bar_colors + [colors[-1]] * (len(days) - len(bar_colors))

        bars = ax.bar(days, counts, color=bar_colors, width=0.6, edgecolor="none")
        ax.set_ylim(0, max(counts) * 1.25 + 1)

        # 在柱子上方显示数值
        for bar, count in zip(bars, counts):
            ax.text(bar.get_x() + bar.get_width() / 2, bar.get_height() + 0.3,
                    str(count), ha="center", va="bottom", color="#E0E0E0", fontsize=10)

        ax.set_title("最近7天访客次数", color="#E0E0E0", fontsize=13, pad=10)
        ax.set_xlabel("日期", color="#B0B0D0", fontsize=10)
        ax.set_ylabel("次数", color="#B0B0D0", fontsize=10)
        ax.tick_params(colors="#808090")
        ax.spines["bottom"].set_color("#3A3A5C")
        ax.spines["left"].set_color("#3A3A5C")
        ax.spines["top"].set_visible(False)
        ax.spines["right"].set_visible(False)

        self._figure.tight_layout()
        self._canvas.draw()

    def _show_event_chart(self):
        """显示事件类型分布饼图"""
        self._figure.clear()
        ax = self._figure.add_subplot(111)
        ax.set_facecolor("#171F2D")

        if self._db:
            data = self._db.get_event_type_stats(days=7)
        else:
            data = []

        if not data:
            self._show_empty_chart("暂无事件分类数据\n触发门禁事件后刷新")
            return

        labels = [d[0] for d in data]
        sizes = [d[1] for d in data]
        colors_pie = ["#3498DB", "#2ECC71", "#E67E22", "#9B59B6", "#E74C3C",
                      "#1ABC9C", "#F39C12", "#34495E"]

        wedges, texts, autotexts = ax.pie(
            sizes, labels=labels, autopct="%1.0f%%",
            colors=colors_pie[:len(sizes)],
            textprops={"color": "#E0E0E0", "fontsize": 10},
            pctdistance=0.75,
            startangle=90
        )

        for autotext in autotexts:
            autotext.set_fontsize(9)
            autotext.set_color("#FFFFFF")

        ax.set_title("事件类型分布（近7天）", color="#E0E0E0", fontsize=13, pad=10)

        self._figure.tight_layout()
        self._canvas.draw()

    def refresh(self):
        """刷新当前图表"""
        self._show_daily_chart()

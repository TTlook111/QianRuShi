"""
stats_panel.py — 访客统计图表
E同学：Python应用层负责人
v3.0: 全新UI设计，渐变色、阴影、圆角、现代配色
使用 matplotlib 嵌入 PyQt5 绘制日/周访客统计图
"""

from PyQt5.QtWidgets import QGroupBox, QVBoxLayout, QHBoxLayout, QPushButton, QLabel
from PyQt5.QtCore import pyqtSignal, Qt
from PyQt5.QtGui import QFont

from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.figure import Figure
import matplotlib
import matplotlib.patheffects as pe
import numpy as np

# 设置 matplotlib 中文字体
matplotlib.rcParams["font.sans-serif"] = ["SimHei", "Microsoft YaHei", "Arial"]
matplotlib.rcParams["axes.unicode_minus"] = False

# 现代化配色方案
MODERN_COLORS = {
    "blue": ["#4F8EF7", "#3B7BEB", "#2D6BD9", "#1F5BC7"],
    "purple": ["#A78BFA", "#8B6CF0", "#7C5CE0", "#6D4DD0"],
    "green": ["#34D399", "#2DD4A0", "#26C99A", "#1FBE94"],
    "orange": ["#FB923C", "#F97316", "#EA580C", "#DC4B04"],
    "pink": ["#F472B6", "#EC4899", "#DB2777", "#C0176A"],
    "cyan": ["#22D3EE", "#06B6D4", "#0891B2", "#0E7490"],
    "gradient_blue": ["#667EEA", "#5A72E0", "#4E66D6", "#425ACC"],
    "gradient_purple": ["#764BA2", "#6B42A0", "#603A9E", "#55329C"],
}

# 现代化图表背景色
CHART_BG = "#171F2D"
AXIS_BG = "#111722"
GRID_COLOR = "#1E2A3A"
TEXT_COLOR = "#E8ECF3"
SUBTITLE_COLOR = "#95A3B8"
ACCENT_COLOR = "#4F8EF7"


class StatsPanel(QGroupBox):
    """
    访客统计图表面板

    v2.1: 更美观的图表样式
    功能：
        - 日访客次数条形图（最近7天）
        - 事件类型分布环形图
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
        layout.setSpacing(10)

        # ---- 标题栏 ----
        header_row = QHBoxLayout()
        title = QLabel("📈 数据可视化")
        title.setStyleSheet(
            "color: #E8ECF3; font-size: 14px; font-weight: bold; background: transparent;"
        )
        header_row.addWidget(title)
        header_row.addStretch()

        # 当前图表类型标签
        self._chart_type_label = QLabel("📅 日统计")
        self._chart_type_label.setStyleSheet(
            "color: #3498DB; font-size: 12px; background: #171F2D; "
            "padding: 4px 12px; border-radius: 4px;"
        )
        header_row.addWidget(self._chart_type_label)
        layout.addLayout(header_row)

        # ---- 按钮行 ----
        btn_row = QHBoxLayout()
        btn_row.setSpacing(10)

        # 按钮样式
        btn_style = """
            QPushButton {
                background-color: #1E2A3A;
                color: #C9D3E2;
                border: 1px solid #2E3A4D;
                border-radius: 6px;
                padding: 8px 16px;
                font-size: 12px;
                font-weight: 500;
                min-width: 80px;
            }
            QPushButton:hover {
                background-color: #2E3A4D;
                border-color: #4F8EF7;
                color: #FFFFFF;
            }
            QPushButton:pressed {
                background-color: #1E2A3A;
                border-color: #4F8EF7;
            }
        """

        btn_daily = QPushButton("📅 日统计")
        btn_daily.setStyleSheet(btn_style)
        btn_daily.setToolTip(
            "<b>日访客统计</b><br>"
            "显示最近7天每天的访客事件次数<br>"
            "数据来源：access_log 表"
        )
        btn_daily.clicked.connect(self._show_daily_chart)
        btn_row.addWidget(btn_daily)

        btn_event = QPushButton("📊 事件分布")
        btn_event.setStyleSheet(btn_style)
        btn_event.setToolTip(
            "<b>事件类型分布</b><br>"
            "显示近7天各类型事件的占比<br>"
            "包含：门铃、PIR、开门、告警等"
        )
        btn_event.clicked.connect(self._show_event_chart)
        btn_row.addWidget(btn_event)

        btn_alert = QPushButton("🚨 告警趋势")
        btn_alert.setStyleSheet(btn_style)
        btn_alert.setToolTip(
            "<b>告警趋势图</b><br>"
            "显示近7天告警等级变化趋势<br>"
            "安全/注意/报警 三级分布"
        )
        btn_alert.clicked.connect(self._show_alert_trend)
        btn_row.addWidget(btn_alert)

        btn_refresh = QPushButton("🔄 刷新")
        btn_refresh.setStyleSheet(btn_style)
        btn_refresh.setToolTip("刷新当前图表数据")
        btn_refresh.clicked.connect(self._refresh_current)
        btn_row.addWidget(btn_refresh)

        layout.addLayout(btn_row)

        # ---- 导出按钮行 ----
        export_row = QHBoxLayout()
        export_row.setSpacing(10)

        # 导出按钮样式
        export_btn_style = """
            QPushButton {
                background-color: #2E3A4D;
                color: #95A3B8;
                border: 1px solid #3A4A5E;
                border-radius: 6px;
                padding: 7px 14px;
                font-size: 11px;
                font-weight: 500;
                min-width: 100px;
            }
            QPushButton:hover {
                background-color: #3A4A5E;
                border-color: #4F8EF7;
                color: #FFFFFF;
            }
            QPushButton:pressed {
                background-color: #2E3A4D;
                border-color: #4F8EF7;
            }
        """

        btn_export_log = QPushButton("⬇️ 导出日志CSV")
        btn_export_log.setStyleSheet(export_btn_style)
        btn_export_log.setToolTip(
            "<b>导出访客日志</b><br>"
            "将 access_log 表导出为 CSV 文件<br>"
            "包含：时间、事件类型、传感器、告警等级"
        )
        btn_export_log.clicked.connect(lambda: self.export_requested.emit("access_log"))
        export_row.addWidget(btn_export_log)

        btn_export_env = QPushButton("⬇️ 导出环境CSV")
        btn_export_env.setStyleSheet(export_btn_style)
        btn_export_env.setToolTip(
            "<b>导出环境数据</b><br>"
            "将 door_env 表导出为 CSV 文件<br>"
            "包含：温度、湿度、光照"
        )
        btn_export_env.clicked.connect(lambda: self.export_requested.emit("door_env"))
        export_row.addWidget(btn_export_env)

        layout.addLayout(export_row)

        # ---- matplotlib 画布 ----
        self._figure = Figure(figsize=(8, 3.2), facecolor="#171F2D")
        self._canvas = FigureCanvas(self._figure)
        self._canvas.setMinimumHeight(260)
        layout.addWidget(self._canvas)

        # 当前图表类型
        self._current_chart = "daily"

        # 默认显示日统计
        self._show_daily_chart()

    def _setup_axes(self, ax):
        """统一设置坐标轴样式 - 现代化设计"""
        ax.set_facecolor(AXIS_BG)
        ax.tick_params(colors=SUBTITLE_COLOR, labelsize=9, length=0, width=0)
        ax.spines["bottom"].set_color(GRID_COLOR)
        ax.spines["left"].set_color(GRID_COLOR)
        ax.spines["top"].set_visible(False)
        ax.spines["right"].set_visible(False)
        ax.grid(True, axis="y", color=GRID_COLOR, linewidth=0.8, alpha=0.6, linestyle="--")

    def _show_empty_chart(self, message="暂无统计数据", subtitle="点击上方按钮刷新"):
        """显示空白占位图表 - 现代化设计"""
        self._figure.clear()
        ax = self._figure.add_subplot(111)
        ax.set_facecolor(CHART_BG)

        # 主提示 - 带阴影效果
        ax.text(0.5, 0.55, message,
                ha="center", va="center", fontsize=18,
                color=TEXT_COLOR, transform=ax.transAxes,
                fontweight="bold",
                path_effects=[pe.withStroke(linewidth=2, foreground="#111722")])

        # 副提示
        ax.text(0.5, 0.42, subtitle,
                ha="center", va="center", fontsize=12,
                color=SUBTITLE_COLOR, transform=ax.transAxes)

        # 装饰性图标
        ax.text(0.5, 0.65, "📊",
                ha="center", va="center", fontsize=48,
                color=SUBTITLE_COLOR, transform=ax.transAxes,
                alpha=0.3)

        ax.set_xticks([])
        ax.set_yticks([])
        for spine in ax.spines.values():
            spine.set_visible(False)

        self._figure.subplots_adjust(left=0.1, right=0.9, top=0.85, bottom=0.15)
        self._canvas.draw()

    def _refresh_current(self):
        """刷新当前图表"""
        if self._current_chart == "daily":
            self._show_daily_chart()
        elif self._current_chart == "event":
            self._show_event_chart()
        elif self._current_chart == "alert":
            self._show_alert_trend()

    def _show_daily_chart(self):
        """显示最近7天每日访客次数条形图 - 现代化设计"""
        self._current_chart = "daily"
        self._chart_type_label.setText("📅 日统计")
        self._figure.clear()
        ax = self._figure.add_subplot(111)
        self._setup_axes(ax)

        if self._db:
            data = self._db.get_daily_visitor_count(days=7)
        else:
            data = []

        if not data:
            self._show_empty_chart("暂无访客统计", "触发门铃/PIR/门磁后刷新")
            return

        days = [d[0][-5:] for d in data]
        counts = [d[1] for d in data]

        # 渐变色条形图 - 蓝色系
        gradient_colors = MODERN_COLORS["blue"]

        # 创建圆角条形图
        bar_width = 0.6
        x = np.arange(len(days))
        bars = ax.bar(x, counts, width=bar_width, color=gradient_colors[0],
                      edgecolor="none", zorder=3, alpha=0.95)

        # 为每个柱子添加渐变效果
        for i, bar in enumerate(bars):
            # 设置柱子颜色（从浅到深渐变）
            color_idx = min(i, len(gradient_colors) - 1)
            bar.set_facecolor(gradient_colors[color_idx])

            # 添加顶部圆角效果
            bar.set_zorder(3)

        max_count = max(counts) if counts else 1
        ax.set_ylim(0, max_count * 1.35 + 1)
        ax.set_xlim(-0.5, len(days) - 0.5)

        # 在柱子上方显示数值（带阴影和背景框）
        for i, (bar, count) in enumerate(zip(bars, counts)):
            # 数值文字
            ax.text(bar.get_x() + bar.get_width() / 2, bar.get_height() + max_count * 0.05,
                    str(count), ha="center", va="bottom",
                    color="#FFFFFF", fontsize=12, fontweight="bold",
                    path_effects=[pe.withStroke(linewidth=3, foreground="#111722")])

        # 设置x轴标签
        ax.set_xticks(x)
        ax.set_xticklabels(days, fontsize=10, color=SUBTITLE_COLOR)

        # 标题和标签
        ax.set_title("最近 7 天访客次数", color=TEXT_COLOR, fontsize=15,
                     fontweight="bold", pad=15)
        ax.set_xlabel("日期", color=SUBTITLE_COLOR, fontsize=10, labelpad=8)
        ax.set_ylabel("次数", color=SUBTITLE_COLOR, fontsize=10, labelpad=8)

        # 调整布局
        self._figure.subplots_adjust(left=0.1, right=0.95, top=0.88, bottom=0.15)
        self._canvas.draw()

    def _show_event_chart(self):
        """显示事件类型分布环形图 - 现代化设计"""
        self._current_chart = "event"
        self._chart_type_label.setText("📊 事件分布")
        self._figure.clear()

        # 创建单子图
        ax = self._figure.add_subplot(111)
        ax.set_facecolor(CHART_BG)

        if self._db:
            data = self._db.get_event_type_stats(days=7)
        else:
            data = []

        if not data:
            self._show_empty_chart("暂无事件数据", "触发门禁事件后刷新")
            return

        labels = [d[0] for d in data]
        sizes = [d[1] for d in data]

        # 现代化配色方案 - 渐变色系
        donut_colors = [
            MODERN_COLORS["blue"][0],
            MODERN_COLORS["green"][0],
            MODERN_COLORS["orange"][0],
            MODERN_COLORS["purple"][0],
            MODERN_COLORS["pink"][0],
            MODERN_COLORS["cyan"][0],
            MODERN_COLORS["gradient_blue"][0],
            MODERN_COLORS["gradient_purple"][0],
        ]

        # 环形图（donut chart）- 现代化设计
        wedges, texts, autotexts = ax.pie(
            sizes, labels=labels, autopct="%1.0f%%",
            colors=donut_colors[:len(sizes)],
            textprops={"color": TEXT_COLOR, "fontsize": 10, "fontweight": "500"},
            pctdistance=0.82,
            startangle=90,
            wedgeprops=dict(
                width=0.35,
                edgecolor=CHART_BG,
                linewidth=3,
                antialiased=True
            )
        )

        # 百分比文字样式 - 更醒目
        for autotext in autotexts:
            autotext.set_fontsize(9)
            autotext.set_color("#FFFFFF")
            autotext.set_fontweight("bold")
            autotext.set_path_effects([pe.withStroke(linewidth=2, foreground="#111722")])

        # 标签样式
        for text in texts:
            text.set_fontsize(9)
            text.set_color(SUBTITLE_COLOR)
            text.set_fontweight("500")

        # 中心文字 - 更现代的设计
        total = sum(sizes)
        center_text = f"共 {total}\n条事件"
        ax.text(0, 0, center_text,
                ha="center", va="center", fontsize=13,
                color=TEXT_COLOR, fontweight="bold",
                linespacing=1.5)

        # 添加中心装饰圆
        center_circle = matplotlib.patches.Circle((0, 0), 0.25,
                                                   facecolor=CHART_BG,
                                                   edgecolor="#2E3A4D",
                                                   linewidth=1.5,
                                                   zorder=10)
        ax.add_patch(center_circle)

        ax.set_title("事件类型分布（近7天）", color=TEXT_COLOR, fontsize=15,
                     fontweight="bold", pad=18)

        # 调整布局
        self._figure.subplots_adjust(left=0.05, right=0.95, top=0.85, bottom=0.1)
        self._canvas.draw()

    def _show_alert_trend(self):
        """显示告警等级趋势图 - 现代化设计"""
        self._current_chart = "alert"
        self._chart_type_label.setText("🚨 告警趋势")
        self._figure.clear()

        # 创建双子图
        ax1 = self._figure.add_subplot(121)
        ax2 = self._figure.add_subplot(122)
        self._setup_axes(ax1)
        self._setup_axes(ax2)

        if self._db:
            daily_data = self._db.get_daily_visitor_count(days=7)
            event_data = self._db.get_event_type_stats(days=7)
        else:
            daily_data = []
            event_data = []

        if not daily_data and not event_data:
            self._show_empty_chart("暂无告警数据", "触发告警事件后刷新")
            return

        # 左图：每日事件数量折线图 - 现代化设计
        if daily_data:
            days = [d[0][-5:] for d in daily_data]
            counts = [d[1] for d in daily_data]

            # 绘制折线 - 蓝色渐变
            line, = ax1.plot(days, counts, color=MODERN_COLORS["blue"][0], linewidth=3,
                             marker="o", markersize=8, markerfacecolor="#FFFFFF",
                             markeredgecolor=MODERN_COLORS["blue"][0], markeredgewidth=2.5,
                             zorder=5, antialiased=True)

            # 填充曲线下方 - 渐变效果
            ax1.fill_between(days, counts, alpha=0.15, color=MODERN_COLORS["blue"][0])

            # 添加装饰性阴影线
            ax1.fill_between(days, counts, alpha=0.05, color=MODERN_COLORS["blue"][1])

            # 数据点标注 - 带背景框
            for i, (day, count) in enumerate(zip(days, counts)):
                ax1.annotate(str(count), (day, count),
                             textcoords="offset points", xytext=(0, 12),
                             ha="center", fontsize=10, color="#FFFFFF",
                             fontweight="bold",
                             path_effects=[pe.withStroke(linewidth=3, foreground="#111722")])

            max_count = max(counts) if counts else 1
            ax1.set_ylim(0, max_count * 1.5 + 1)
            ax1.set_xlim(-0.3, len(days) - 0.7)

        ax1.set_title("每日事件趋势", color=TEXT_COLOR, fontsize=13,
                      fontweight="bold", pad=12)
        ax1.set_xlabel("日期", color=SUBTITLE_COLOR, fontsize=10, labelpad=8)
        ax1.set_ylabel("次数", color=SUBTITLE_COLOR, fontsize=10, labelpad=8)

        # 右图：事件类型横向条形图 - 现代化设计
        if event_data:
            event_labels = [d[0] for d in event_data][::-1]
            event_counts = [d[1] for d in event_data][::-1]

            # 使用渐变色系
            bar_colors_list = [
                MODERN_COLORS["blue"][0],
                MODERN_COLORS["green"][0],
                MODERN_COLORS["orange"][0],
                MODERN_COLORS["purple"][0],
                MODERN_COLORS["pink"][0],
                MODERN_COLORS["cyan"][0],
                MODERN_COLORS["gradient_blue"][0],
                MODERN_COLORS["gradient_purple"][0],
            ]

            bar_colors = [bar_colors_list[i % len(bar_colors_list)] for i in range(len(event_labels))]

            bars = ax2.barh(event_labels, event_counts, color=bar_colors,
                           height=0.65, edgecolor="none", alpha=0.95,
                           zorder=3)

            # 数值标注 - 带阴影效果
            for bar, count in zip(bars, event_counts):
                ax2.text(bar.get_width() + max(event_counts) * 0.02,
                         bar.get_y() + bar.get_height() / 2,
                         str(count), va="center", ha="left",
                         color="#FFFFFF", fontsize=11, fontweight="bold",
                         path_effects=[pe.withStroke(linewidth=2, foreground="#111722")])

            max_count = max(event_counts) if event_counts else 1
            ax2.set_xlim(0, max_count * 1.25 + 1)

        ax2.set_title("事件类型分布", color=TEXT_COLOR, fontsize=13,
                      fontweight="bold", pad=12)

        # 调整布局
        self._figure.subplots_adjust(left=0.12, right=0.95, top=0.85, bottom=0.15, wspace=0.3)
        self._canvas.draw()

    def refresh(self):
        """刷新当前图表"""
        self._refresh_current()

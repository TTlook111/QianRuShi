# QT页面美化方案

## 一、设计理念

### 1.1 参考前端设计思路

借鉴现代前端框架（如Ant Design、Element Plus、Material Design）的设计理念：

- **深色主题**: 减少视觉疲劳，突出数据显示
- **卡片式布局**: 信息分组清晰，视觉层次分明
- **渐变色与阴影**: 增加立体感和现代感
- **动画过渡**: 状态变化平滑自然
- **图标与Emoji**: 直观表达功能含义

### 1.2 配色方案

```python
# 主色调
PRIMARY = "#3498DB"      # 蓝色 - 主要操作
SUCCESS = "#2ECC71"      # 绿色 - 成功/安全
WARNING = "#F39C12"      # 橙色 - 警告/注意
DANGER = "#E74C3C"       # 红色 - 危险/报警
INFO = "#9B59B6"         # 紫色 - 信息

# 背景色
BG_DARK = "#1A1A2E"      # 深色背景
BG_CARD = "#16213E"      # 卡片背景
BG_INPUT = "#0F3460"     # 输入框背景

# 文字色
TEXT_PRIMARY = "#EAEAEA"  # 主要文字
TEXT_SECONDARY = "#808090" # 次要文字
TEXT_MUTED = "#555555"    # 灰色文字
```

---

## 二、QSS样式表设计

### 2.1 全局样式

```qss
/* style.qss - 智能门禁系统深色主题 */

/* 全局背景 */
QWidget {
    background-color: #1A1A2E;
    color: #EAEAEA;
    font-family: "Microsoft YaHei", "Segoe UI", sans-serif;
}

/* 主窗口 */
QMainWindow {
    background-color: #1A1A2E;
}

/* 滚动区域 */
QScrollArea {
    border: none;
    background-color: #1A1A2E;
}

/* 分组框 (卡片) */
QGroupBox {
    background-color: #16213E;
    border: 1px solid #0F3460;
    border-radius: 12px;
    margin-top: 20px;
    padding: 20px 15px 15px 15px;
    font-size: 14px;
    font-weight: bold;
    color: #EAEAEA;
}

QGroupBox::title {
    subcontrol-origin: margin;
    subcontrol-position: top left;
    padding: 5px 15px;
    background-color: #0F3460;
    border-radius: 8px;
    color: #3498DB;
}

/* 标签 */
QLabel {
    color: #EAEAEA;
    background: transparent;
}

QLabel#title_label {
    font-size: 24px;
    font-weight: bold;
    color: #3498DB;
}

QLabel#status_label {
    font-size: 12px;
    color: #808090;
}

/* 按钮基础样式 */
QPushButton {
    background-color: #0F3460;
    color: #EAEAEA;
    border: 1px solid #3498DB;
    border-radius: 8px;
    padding: 10px 20px;
    font-size: 13px;
    font-weight: bold;
    min-height: 40px;
}

QPushButton:hover {
    background-color: #3498DB;
    border-color: #5DADE2;
}

QPushButton:pressed {
    background-color: #2980B9;
}

QPushButton:disabled {
    background-color: #555555;
    border-color: #666666;
    color: #888888;
}

/* 特殊按钮样式 */
QPushButton#btn_unlock {
    background-color: #27AE60;
    border-color: #2ECC71;
}

QPushButton#btn_unlock:hover {
    background-color: #2ECC71;
}

QPushButton#btn_lock {
    background-color: #7F8C8D;
    border-color: #95A5A6;
}

QPushButton#btn_doorbell {
    background-color: #2980B9;
    border-color: #3498DB;
}

QPushButton#btn_alarm {
    background-color: #C0392B;
    border-color: #E74C3C;
}

QPushButton#btn_alarm:hover {
    background-color: #E74C3C;
}

QPushButton#btn_reset {
    background-color: #16A085;
    border-color: #1ABC9C;
}

/* 状态卡片 */
QFrame#status_card {
    background-color: #0F3460;
    border: 1px solid #1A1A4E;
    border-radius: 10px;
    padding: 15px;
}

QFrame#status_card:hover {
    border-color: #3498DB;
    background-color: #16213E;
}

/* 输入框 */
QLineEdit, QSpinBox, QComboBox {
    background-color: #0F3460;
    color: #EAEAEA;
    border: 1px solid #1A1A4E;
    border-radius: 6px;
    padding: 8px 12px;
    font-size: 13px;
}

QLineEdit:focus, QSpinBox:focus, QComboBox:focus {
    border-color: #3498DB;
}

/* 表格 */
QTableWidget {
    background-color: #16213E;
    alternate-background-color: #1A1A4E;
    color: #EAEAEA;
    gridline-color: #0F3460;
    border: 1px solid #0F3460;
    border-radius: 8px;
}

QTableWidget::item {
    padding: 8px;
    border-bottom: 1px solid #0F3460;
}

QTableWidget::item:selected {
    background-color: #3498DB;
}

QHeaderView::section {
    background-color: #0F3460;
    color: #EAEAEA;
    padding: 10px;
    border: none;
    border-right: 1px solid #1A1A4E;
    border-bottom: 2px solid #3498DB;
    font-weight: bold;
}

/* 状态栏 */
QStatusBar {
    background-color: #0F3460;
    color: #808090;
    border-top: 1px solid #1A1A4E;
    font-size: 12px;
}

/* 分隔线 */
QFrame#separator {
    background-color: #1A1A4E;
    max-height: 1px;
}

/* 滚动条 */
QScrollBar:vertical {
    background-color: #1A1A2E;
    width: 10px;
    border-radius: 5px;
}

QScrollBar::handle:vertical {
    background-color: #3498DB;
    border-radius: 5px;
    min-height: 30px;
}

QScrollBar::handle:vertical:hover {
    background-color: #5DADE2;
}

QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
    height: 0px;
}

QScrollBar:horizontal {
    background-color: #1A1A2E;
    height: 10px;
    border-radius: 5px;
}

QScrollBar::handle:horizontal {
    background-color: #3498DB;
    border-radius: 5px;
    min-width: 30px;
}
```

---

## 三、UI组件美化建议

### 3.1 状态指示器美化

```python
class StatusIndicator(QFrame):
    """美化后的状态指示器"""

    def __init__(self, title, icon="", parent=None):
        super().__init__(parent)
        self.setObjectName("status_card")
        self.setMinimumHeight(120)
        self.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Preferred)

        layout = QVBoxLayout(self)
        layout.setAlignment(Qt.AlignCenter)
        layout.setContentsMargins(15, 15, 15, 15)
        layout.setSpacing(8)

        # 图标（使用Emoji或SVG）
        self._icon_label = QLabel(icon)
        self._icon_label.setAlignment(Qt.AlignCenter)
        self._icon_label.setFont(QFont("Segoe UI Emoji", 28))
        self._icon_label.setStyleSheet("background: transparent;")
        layout.addWidget(self._icon_label)

        # 标题
        title_label = QLabel(title)
        title_label.setAlignment(Qt.AlignCenter)
        title_label.setFont(QFont("Microsoft YaHei", 11, QFont.Bold))
        title_label.setStyleSheet("color: #808090; background: transparent;")
        layout.addWidget(title_label)

        # 状态文字
        self._status = QLabel("--")
        self._status.setAlignment(Qt.AlignCenter)
        self._status.setFont(QFont("Microsoft YaHei", 14, QFont.Bold))
        self._status.setStyleSheet("color: #EAEAEA; background: transparent;")
        layout.addWidget(self._status)

        # 添加动画效果
        self._animation = QPropertyAnimation(self, b"geometry")
        self._animation.setDuration(200)
        self._animation.setEasingCurve(QEasingCurve.OutCubic)

    def set_status(self, color, text, icon=None):
        """设置状态（带动画）"""
        self._status.setText(text)
        self._status.setStyleSheet(f"color: {color}; background: transparent;")
        self._icon_label.setStyleSheet(f"color: {color}; background: transparent;")
        if icon:
            self._icon_label.setText(icon)

        # 闪烁动画效果
        self._flash_animation()

    def _flash_animation(self):
        """状态变化时的闪烁动画"""
        animation = QPropertyAnimation(self, b"windowOpacity")
        animation.setDuration(300)
        animation.setStartValue(0.7)
        animation.setEndValue(1.0)
        animation.setEasingCurve(QEasingCurve.InOutQuad)
        animation.start()
```

### 3.2 控制按钮美化

```python
class ControlButton(QPushButton):
    """美化后的控制按钮"""

    def __init__(self, text, icon="", color="#3498DB", parent=None):
        super().__init__(f"{icon} {text}", parent)
        self._color = color
        self._setup_style()

    def _setup_style(self):
        self.setMinimumHeight(50)
        self.setFont(QFont("Microsoft YaHei", 12, QFont.Bold))
        self.setCursor(Qt.PointingHandCursor)

        # 动态样式
        self.setStyleSheet(f"""
            QPushButton {{
                background-color: {self._color}22;
                color: {self._color};
                border: 2px solid {self._color};
                border-radius: 10px;
                padding: 12px 24px;
            }}
            QPushButton:hover {{
                background-color: {self._color}44;
                border-color: {self._color};
            }}
            QPushButton:pressed {{
                background-color: {self._color}66;
            }}
        """)

    def set_loading(self, loading=True):
        """加载状态"""
        if loading:
            self.setEnabled(False)
            self.setText("⏳ 发送中...")
        else:
            self.setEnabled(True)
            # 恢复原始文本
```

### 3.3 实时数据曲线

```python
from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg
from matplotlib.figure import Figure
import matplotlib.pyplot as plt

class RealTimeChart(FigureCanvasQTAgg):
    """实时数据曲线图"""

    def __init__(self, parent=None, width=5, height=3, dpi=100):
        plt.style.use('dark_background')
        self.fig = Figure(figsize=(width, height), dpi=dpi, facecolor='#16213E')
        self.axes = self.fig.add_subplot(111)
        self.axes.set_facecolor('#0F3460')

        super().__init__(self.fig)
        self.setParent(parent)

        # 数据缓存
        self.x_data = []
        self.y_data = []
        self.max_points = 50

        # 设置样式
        self.axes.tick_params(colors='#808090')
        self.axes.spines['bottom'].set_color('#1A1A4E')
        self.axes.spines['left'].set_color('#1A1A4E')
        self.axes.spines['top'].set_visible(False)
        self.axes.spines['right'].set_visible(False)

    def update_data(self, value):
        """更新数据点"""
        self.x_data.append(len(self.x_data))
        self.y_data.append(value)

        # 限制数据点数量
        if len(self.x_data) > self.max_points:
            self.x_data = self.x_data[-self.max_points:]
            self.y_data = self.y_data[-self.max_points:]

        # 绘制曲线
        self.axes.clear()
        self.axes.plot(self.x_data, self.y_data, color='#3498DB', linewidth=2)
        self.axes.fill_between(self.x_data, self.y_data, alpha=0.3, color='#3498DB')

        self.axes.set_facecolor('#0F3460')
        self.axes.tick_params(colors='#808090')
        self.draw()
```

---

## 四、动画效果实现

### 4.1 状态变化动画

```python
from PyQt5.QtCore import QPropertyAnimation, QEasingCurve

class AnimatedWidget(QWidget):
    """带动画效果的组件"""

    def animate_color_change(self, target_color, duration=300):
        """颜色渐变动画"""
        animation = QPropertyAnimation(self, b"styleSheet")
        animation.setDuration(duration)
        animation.setStartValue(self.styleSheet())
        animation.setEndValue(f"background-color: {target_color};")
        animation.setEasingCurve(QEasingCurve.InOutQuad)
        animation.start()
        return animation

    def animate_pulse(self, duration=500):
        """脉冲动画（用于告警）"""
        animation = QPropertyAnimation(self, b"geometry")
        animation.setDuration(duration)
        animation.setLoopCount(3)

        original = self.geometry()
        animation.setStartValue(original)
        animation.setKeyValueAt(0.5, original.adjusted(-5, -5, 5, 5))
        animation.setEndValue(original)

        animation.start()
        return animation
```

### 4.2 告警闪烁效果

```python
class AlertBlinker(QObject):
    """告警闪烁控制器"""

    def __init__(self, widget):
        super().__init__()
        self.widget = widget
        self.timer = QTimer()
        self.timer.timeout.connect(self._toggle)
        self.is_on = False

    def start_blink(self, color="#E74C3C", interval=500):
        """开始闪烁"""
        self.color = color
        self.timer.start(interval)

    def stop_blink(self):
        """停止闪烁"""
        self.timer.stop()
        self.widget.setStyleSheet("")

    def _toggle(self):
        """切换状态"""
        if self.is_on:
            self.widget.setStyleSheet(f"border: 2px solid {self.color};")
        else:
            self.widget.setStyleSheet(f"border: 2px solid transparent;")
        self.is_on = not self.is_on
```

---

## 五、完整美化后的主窗口布局

```python
class MainWindow(QMainWindow):
    """美化后的主窗口"""

    def _init_ui(self):
        # 设置深色主题
        self.setStyleSheet(open("resources/style.qss", "r").read())

        # 主布局
        scroll = QScrollArea()
        scroll.setWidgetResizable(True)
        scroll.setFrameShape(QScrollArea.NoFrame)
        self.setCentralWidget(scroll)

        central = QWidget()
        central.setMinimumWidth(1000)
        scroll.setWidget(central)

        main_layout = QVBoxLayout(central)
        main_layout.setSpacing(16)
        main_layout.setContentsMargins(20, 16, 20, 16)

        # 标题栏（带渐变背景）
        title_bar = self._create_title_bar()
        main_layout.addWidget(title_bar)

        # 上半区：状态看板 + 远程控制
        top_splitter = QSplitter(Qt.Horizontal)
        top_splitter.setChildrenCollapsible(False)

        # 左侧：状态卡片
        left_panel = self._create_status_panel()
        top_splitter.addWidget(left_panel)

        # 右侧：控制面板
        right_panel = self._create_control_panel()
        top_splitter.addWidget(right_panel)

        top_splitter.setSizes([600, 400])
        main_layout.addWidget(top_splitter, stretch=3)

        # 中间：事件时间线
        timeline_panel = self._create_timeline_panel()
        main_layout.addWidget(timeline_panel, stretch=2)

        # 下半区：统计图表
        stats_panel = self._create_stats_panel()
        main_layout.addWidget(stats_panel, stretch=1)

    def _create_title_bar(self):
        """创建标题栏"""
        title_widget = QWidget()
        title_widget.setStyleSheet("""
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                                        stop:0 #1A1A2E, stop:1 #16213E);
            border-radius: 12px;
            padding: 20px;
        """)

        layout = QHBoxLayout(title_widget)

        # 标题
        title = QLabel("🏠 智能门禁访客管理系统")
        title.setFont(QFont("Microsoft YaHei", 20, QFont.Bold))
        title.setStyleSheet("color: #3498DB; background: transparent;")
        layout.addWidget(title)

        layout.addStretch()

        # 连接状态
        self._conn_label = QLabel("● 连接中...")
        self._conn_label.setStyleSheet("""
            color: #F39C12;
            font-size: 14px;
            font-weight: bold;
            background: transparent;
        """)
        layout.addWidget(self._conn_label)

        # 时间
        self._time_label = QLabel()
        self._time_label.setStyleSheet("color: #808090; background: transparent;")
        layout.addWidget(self._time_label)

        return title_widget
```

---

## 六、实施建议

### 6.1 分阶段实施

| 阶段 | 任务 | 预计时间 |
|------|------|----------|
| 第一阶段 | 创建QSS样式文件，应用基础深色主题 | 1小时 |
| 第二阶段 | 美化状态指示器和按钮组件 | 2小时 |
| 第三阶段 | 添加实时数据曲线图 | 2小时 |
| 第四阶段 | 实现动画效果和告警闪烁 | 1小时 |
| 第五阶段 | 完善布局和细节调整 | 1小时 |

### 6.2 关键文件修改

```
sensor-e/
├── resources/
│   └── style.qss              # 新增：QSS样式表
├── ui/
│   ├── main_window.py          # 修改：应用新样式
│   ├── door_status_panel.py    # 修改：美化状态卡片
│   ├── env_info_panel.py       # 修改：添加曲线图
│   ├── remote_control_panel.py # 修改：美化按钮
│   ├── event_timeline_panel.py # 修改：美化时间线
│   ├── stats_panel.py          # 修改：美化图表
│   └── system_status_panel.py  # 修改：美化系统状态
└── widgets/                    # 新增：自定义组件目录
    ├── animated_widget.py      # 动画组件
    ├── real_time_chart.py      # 实时曲线图
    └── alert_blinker.py        # 告警闪烁器
```

---

## 七、效果预览

### 美化前
```
┌─────────────────────────────────────────────┐
│  智能门禁访客管理系统                          │
├─────────────────────────────────────────────┤
│  [状态卡片] [状态卡片] [状态卡片] [状态卡片]    │
│  [开门] [关门] [门铃] [静音]                  │
│  [告警] [解除] [夜间] [白天]                  │
├─────────────────────────────────────────────┤
│  事件时间线...                                │
├─────────────────────────────────────────────┤
│  统计图表...                                  │
└─────────────────────────────────────────────┘
```

### 美化后
```
╔═══════════════════════════════════════════════════════════════════╗
║  🏠 智能门禁访客管理系统                    ● 已连接  2026-06-15  ║
╚═══════════════════════════════════════════════════════════════════╝

┌─────────────────────────────────────────────────────────────────┐
│  🚪 门禁状态看板                                                 │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐          │
│  │   👤     │ │   🚪     │ │   🔔     │ │   🛡️     │          │
│  │ 访客状态  │ │  门状态   │ │ 门铃状态  │ │ 安全模式  │          │
│  │   无人    │ │   已关   │ │   静默   │ │   安全   │          │
│  └──────────┘ └──────────┘ └──────────┘ └──────────┘          │
└─────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│  🎮 远程控制                                                     │
│  ┌─────────────────┐ ┌─────────────────┐                       │
│  │   🔓 远程开门    │ │   🔒 手动关门    │                       │
│  └─────────────────┘ └─────────────────┘                       │
│  ┌─────────────────┐ ┌─────────────────┐                       │
│  │   🔔 触发门铃    │ │   🔇 关闭蜂鸣器  │                       │
│  └─────────────────┘ └─────────────────┘                       │
│  ─────────────────────────────────────────                      │
│  ┌─────────────────┐ ┌─────────────────┐                       │
│  │   🚨 触发告警    │ │   ✅ 解除告警    │                       │
│  └─────────────────┘ └─────────────────┘                       │
└─────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│  📊 实时数据                                                     │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │      📈 温度变化曲线 (最近50个数据点)                      │   │
│  │   30℃ ┤                          ___                    │   │
│  │       │                    ___/    \___                 │   │
│  │   25℃ ┤              ___/                          │   │
│  │       │        ___/                                   │   │
│  │   20℃ ┤___/                                        │   │
│  │       └────────────────────────────────────────────   │   │
│  │        10:00   10:05   10:10   10:15   10:20         │   │
│  └─────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
```

---

## 八、总结

通过以上美化方案，QT上位机将具备：

1. **现代化深色主题** - 减少视觉疲劳，专业感强
2. **卡片式布局** - 信息分组清晰，层次分明
3. **动画效果** - 状态变化平滑自然
4. **实时曲线图** - 数据可视化更直观
5. **告警闪烁** - 重要状态一目了然

整个美化工作预计需要**7小时**左右，可以分阶段实施，优先完成深色主题和基础组件美化。

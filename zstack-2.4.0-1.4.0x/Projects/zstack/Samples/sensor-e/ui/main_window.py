"""
main_window.py — 主窗口
E同学：Python应用层负责人
整合门禁看板、环境面板、远程控制、事件时间线、统计图表
"""

import logging
from datetime import datetime

from PyQt5.QtWidgets import (
    QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
    QSplitter, QLabel, QStatusBar, QFrame
)
from PyQt5.QtCore import Qt, QTimer
from PyQt5.QtGui import QFont

import config
from data.database import Database
from data.models import AccessLog, DoorEnv, SensorState
from communication.cloud_api import CloudAPI
from communication.websocket_client import WebSocketClient
from ui.door_status_panel import DoorStatusPanel
from ui.env_info_panel import EnvInfoPanel
from ui.remote_control_panel import RemoteControlPanel
from ui.event_timeline_panel import EventTimelinePanel
from ui.stats_panel import StatsPanel

logger = logging.getLogger(__name__)


class MainWindow(QMainWindow):
    """
    智能门禁访客管理系统 — 主窗口

    布局结构：
    ┌─────────────────────────────────────────────┐
    │              🏠 标题栏 + 连接状态              │
    ├─────────────────────┬───────────────────────┤
    │  门禁状态看板         │  远程控制区              │
    ├─────────────────────┤                       │
    │  环境信息面板         │                       │
    ├─────────────────────┴───────────────────────┤
    │              访客事件时间线                     │
    ├─────────────────────────────────────────────┤
    │              访客统计图表                      │
    ├─────────────────────────────────────────────┤
    │              状态栏                           │
    └─────────────────────────────────────────────┘
    """

    def __init__(self):
        super().__init__()
        self.setWindowTitle(config.WINDOW_TITLE)
        self.resize(config.WINDOW_WIDTH, config.WINDOW_HEIGHT)

        # 初始化数据层
        self._db = Database()
        self._sensor_state = SensorState()

        # 初始化通信层
        self._cloud_api = CloudAPI(self)
        self._ws_client = WebSocketClient(self)

        # 初始化UI
        self._init_ui()

        # 连接信号
        self._connect_signals()

        # 加载历史数据
        self._load_history()

        # 启动数据轮询与 WebSocket 监听
        self._cloud_api.start_polling()
        self._ws_client.start()

        logger.info("主窗口初始化完成")

    def _init_ui(self):
        """构建UI布局"""
        central = QWidget()
        self.setCentralWidget(central)
        main_layout = QVBoxLayout(central)
        main_layout.setSpacing(8)
        main_layout.setContentsMargins(12, 8, 12, 8)

        # ---- 标题栏 ----
        title_row = QHBoxLayout()

        title_label = QLabel("🏠 智能门禁访客管理系统")
        title_label.setObjectName("title_label")
        title_label.setFont(QFont("Microsoft YaHei", 18, QFont.Bold))
        title_row.addWidget(title_label)

        title_row.addStretch()

        # 连接状态指示
        self._conn_label = QLabel("● 模拟模式")
        self._conn_label.setStyleSheet(
            "color: #F39C12; font-size: 13px; font-weight: bold; background: transparent;"
        )
        title_row.addWidget(self._conn_label)

        # 当前时间
        self._time_label = QLabel("")
        self._time_label.setStyleSheet(
            "color: #808090; font-size: 12px; background: transparent;"
        )
        title_row.addWidget(self._time_label)

        main_layout.addLayout(title_row)

        # ---- 上半区：状态看板 + 远程控制 ----
        top_splitter = QSplitter(Qt.Horizontal)

        # 左侧：门禁看板 + 环境面板（纵向堆叠）
        left_widget = QWidget()
        left_layout = QVBoxLayout(left_widget)
        left_layout.setContentsMargins(0, 0, 0, 0)
        left_layout.setSpacing(8)

        self._door_panel = DoorStatusPanel()
        left_layout.addWidget(self._door_panel)

        self._env_panel = EnvInfoPanel()
        left_layout.addWidget(self._env_panel)

        top_splitter.addWidget(left_widget)

        # 右侧：远程控制区
        self._control_panel = RemoteControlPanel()
        top_splitter.addWidget(self._control_panel)

        top_splitter.setSizes([700, 500])

        # ---- 创建垂直分割器，容纳：上半区、中间时间线、下半区统计 ----
        v_splitter = QSplitter(Qt.Vertical)

        # 1. 上半区放入垂直分割器
        v_splitter.addWidget(top_splitter)

        # 2. 中间区：事件时间线
        self._timeline_panel = EventTimelinePanel()
        v_splitter.addWidget(self._timeline_panel)

        # 3. 下半区：统计图表
        self._stats_panel = StatsPanel()
        self._stats_panel.set_database(self._db)
        v_splitter.addWidget(self._stats_panel)

        # 设置初始高度像素比例：上半区380px，中间时间线220px，下半区图表200px
        v_splitter.setSizes([380, 220, 200])

        # 将垂直分割器添加到主垂直布局中
        main_layout.addWidget(v_splitter)

        # ---- 状态栏 ----
        status_bar = QStatusBar()
        self.setStatusBar(status_bar)
        self._status_msg = QLabel("系统就绪 | E同学：Python应用层")
        status_bar.addWidget(self._status_msg)

        # ---- 时钟定时器 ----
        self._clock_timer = QTimer(self)
        self._clock_timer.timeout.connect(self._update_clock)
        self._clock_timer.start(1000)
        self._update_clock()

    def _connect_signals(self):
        """连接信号与槽"""
        # 云API数据 → 更新UI
        self._cloud_api.data_received.connect(self._on_data_received)
        self._cloud_api.error_occurred.connect(self._on_error)
        self._cloud_api.command_sent.connect(self._on_command_sent)

        # WebSocket连接与数据 -> 更新UI
        self._ws_client.data_received.connect(self._on_data_received)
        self._ws_client.error_occurred.connect(self._on_error)
        self._ws_client.connected.connect(self._on_ws_connected)
        self._ws_client.disconnected.connect(self._on_ws_disconnected)

        # 远程控制按钮 → 发送命令
        self._control_panel.command_requested.connect(self._on_command_requested)

    def _on_ws_connected(self):
        """WebSocket 连线认证成功回调"""
        self._conn_label.setText("● 云端已连接")
        self._conn_label.setStyleSheet(
            "color: #2ECC71; font-size: 13px; font-weight: bold; background: transparent;"
        )
        self._status_msg.setText("✅ 智云 WebSocket 认证成功并在线")

    def _on_ws_disconnected(self):
        """WebSocket 连线断开回调"""
        self._conn_label.setText("● 云端离线")
        self._conn_label.setStyleSheet(
            "color: #E74C3C; font-size: 13px; font-weight: bold; background: transparent;"
        )
        self._status_msg.setText("❌ 智云 WebSocket 连线断开")

    def _load_history(self):
        """从数据库加载历史事件"""
        try:
            logs = self._db.select_access_logs(limit=50)
            self._timeline_panel.load_from_db(logs)
        except Exception as e:
            logger.warning("加载历史数据失败: %s", e)

    # ======================== 数据处理 ========================

    def _on_data_received(self, data: dict):
        """
        收到传感器数据后的统一处理入口

        流程：
            1. 兼容解析带 mac 硬件地址的智云上报数据
            2. 更新内存中的 SensorState
            3. 刷新门禁看板
            4. 刷新环境面板
            5. 生成事件记录（事件时间线）
            6. 存储到数据库
        """
        # 兼容处理带 addr 物理地址标识的智云上报数据
        import json
        if isinstance(data, dict) and "addr" in data:
            mac_addr = data.get("addr", "").replace(":", "").upper()

            payload = data.get("data", {})
            if isinstance(payload, str):
                try:
                    payload = json.loads(payload)
                except Exception:
                    payload = {}
            
            # 建立 MAC 地址与系统统一业务字段的映射关系
            normalized_data = {}
            if mac_addr == "00124B001C45BD01":   # sensor-a (环境采集)
                normalized_data = {
                    config.FIELD_TEMP: payload.get("temp"),
                    config.FIELD_HUMI: payload.get("humi"),
                    config.FIELD_LUX: payload.get("lux")
                }
            elif mac_addr == "00124B001C4665DE": # sensor-c (安防检测)
                normalized_data = {
                    config.FIELD_PIR: payload.get("pir"),
                    config.FIELD_DOOR: payload.get("door"),
                    config.FIELD_TCH: payload.get("tch"),
                    config.FIELD_ALERT: payload.get("alert"),
                    config.FIELD_STAY: payload.get("stay"),
                    config.FIELD_NIGHT: payload.get("night")
                }
            elif mac_addr == "00124B001C45BB54": # sensor-b (控制执行)
                normalized_data = {
                    config.FIELD_UNLOCK: payload.get("unlock"),
                    config.FIELD_BUZZ: payload.get("buzz"),
                    config.FIELD_RGB: payload.get("rgb")
                }
            
            # 只保留非 None 字段更新
            data = {k: v for k, v in normalized_data.items() if v is not None}
            if not data:
                return # 无有效字段更新，直接返回

        # 1. 更新内存状态
        self._update_sensor_state(data)

        # 2. 刷新门禁看板
        self._door_panel.update_data(data)

        # 3. 刷新环境面板
        self._env_panel.update_data(data)

        # 4. 事件时间线处理
        self._timeline_panel.process_sensor_data(data)

        # 5. 数据库存储
        self._save_to_db(data)

        # 更新状态栏
        self._status_msg.setText(
            f"最后更新: {datetime.now().strftime('%H:%M:%S')} | "
            f"temp={self._sensor_state.temp}℃ "
            f"humi={self._sensor_state.humi}% "
            f"alert={self._sensor_state.alert}"
        )

    def _update_sensor_state(self, data: dict):
        """更新内存中的传感器状态"""
        if config.FIELD_TEMP in data:
            self._sensor_state.temp = float(data[config.FIELD_TEMP])
        if config.FIELD_HUMI in data:
            self._sensor_state.humi = float(data[config.FIELD_HUMI])
        if config.FIELD_LUX in data:
            self._sensor_state.lux = int(data[config.FIELD_LUX])
        if config.FIELD_PIR in data:
            self._sensor_state.pir = int(data[config.FIELD_PIR])
        if config.FIELD_DOOR in data:
            self._sensor_state.door = int(data[config.FIELD_DOOR])
        if config.FIELD_TCH in data:
            self._sensor_state.tch = int(data[config.FIELD_TCH])
        if config.FIELD_ALERT in data:
            self._sensor_state.alert = int(data[config.FIELD_ALERT])
        if config.FIELD_STAY in data:
            self._sensor_state.stay = int(data[config.FIELD_STAY])
        if config.FIELD_NIGHT in data:
            self._sensor_state.night = int(data[config.FIELD_NIGHT])
        if config.FIELD_UNLOCK in data:
            self._sensor_state.unlock = int(data[config.FIELD_UNLOCK])
        if config.FIELD_BUZZ in data:
            self._sensor_state.buzz = int(data[config.FIELD_BUZZ])
        if config.FIELD_RGB in data:
            self._sensor_state.rgb = data[config.FIELD_RGB]
        self._sensor_state.last_update = datetime.now()

    def _save_to_db(self, data: dict):
        """将传感器数据保存到数据库"""
        try:
            # 保存环境数据
            if config.FIELD_TEMP in data:
                env = DoorEnv(
                    temp=float(data.get(config.FIELD_TEMP, 0)),
                    humi=float(data.get(config.FIELD_HUMI, 0)),
                    lux=int(data.get(config.FIELD_LUX, 0))
                )
                self._db.insert_door_env(env)

            # 保存安防事件
            if data.get(config.FIELD_TCH) == 1:
                log = AccessLog(
                    sensor="tch",
                    event_type=config.EVENT_DOORBELL,
                    value=1,
                    alert=int(data.get(config.FIELD_ALERT, 0))
                )
                self._db.insert_access_log(log)

            if data.get(config.FIELD_PIR) == 1:
                stay = int(data.get(config.FIELD_STAY, 0))
                event_type = config.EVENT_LOITER if stay > 10 else config.EVENT_PIR_DETECT
                log = AccessLog(
                    sensor="pir",
                    event_type=event_type,
                    value=1,
                    alert=int(data.get(config.FIELD_ALERT, 0))
                )
                self._db.insert_access_log(log)

        except Exception as e:
            logger.warning("数据库写入失败: %s", e)

    # ======================== 命令发送 ========================

    def _on_command_requested(self, cmd: dict):
        """
        处理远程控制面板发出的命令

        参数：
            cmd — 命令字典（如 {"unlock": 1, "rgb": [0, 255, 0]}）
                  特殊字段 _target 指定目标节点（默认 sensor-b）
        """
        target = cmd.pop("_target", "sensor-b")

        # 优先使用带有节点 MAC 封装的 WebSocket 进行实时控制下发
        mac_addr = config.NODE_MAC.get(target)
        if mac_addr:
            self._ws_client.send_command(mac_addr, cmd)
            self._on_command_sent(True)
        else:
            logger.warning("未找到节点 %s 的 MAC 地址，无法发送控制指令", target)
            self._on_command_sent(False)

        logger.info("发送命令到 %s (MAC: %s): %s", target, mac_addr, cmd)

        # 记录远程开门事件
        if cmd.get("unlock") == 1:
            log = AccessLog(
                sensor="remote",
                event_type=config.EVENT_REMOTE_UNLOCK,
                value=1,
                is_remote_unlock=True
            )
            self._db.insert_access_log(log)
            self._timeline_panel.add_event(
                config.EVENT_REMOTE_UNLOCK,
                sensor="remote",
                detail="用户远程开门",
                alert=0
            )

        # 记录告警解除事件
        if cmd.get("reset") == 1:
            self._timeline_panel.add_event(
                config.EVENT_ALERT_RESET,
                sensor="remote",
                detail="用户解除告警",
                alert=0
            )

    def _on_command_sent(self, success: bool):
        """命令发送结果回调"""
        if success:
            self._status_msg.setText(
                f"✅ 命令发送成功 | {datetime.now().strftime('%H:%M:%S')}"
            )
        else:
            self._status_msg.setText(
                f"❌ 命令发送失败 | {datetime.now().strftime('%H:%M:%S')}"
            )

    def _on_error(self, msg: str):
        """错误处理"""
        self._status_msg.setText(f"⚠️ {msg}")
        logger.error(msg)

    # ======================== 工具方法 ========================

    def _update_clock(self):
        """更新时钟显示"""
        self._time_label.setText(datetime.now().strftime("%Y-%m-%d %H:%M:%S"))

    def closeEvent(self, event):
        """窗口关闭时清理资源"""
        self._cloud_api.stop_polling()
        self._ws_client.stop()
        self._clock_timer.stop()
        logger.info("主窗口已关闭")
        event.accept()

"""
main_window.py — 主窗口
E同学：Python应用层负责人
整合门禁看板、环境面板、远程控制、事件时间线、统计图表
"""

import logging
from datetime import datetime

from PyQt5.QtWidgets import (
    QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
    QSplitter, QLabel, QStatusBar, QFileDialog, QMessageBox,
    QScrollArea, QSizePolicy
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
from ui.system_status_panel import SystemStatusPanel

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
        self._last_db_values = {}
        self._loiter_db_reported = False

        # 初始化通信层
        if config.USE_REAL_CLOUD:
            self._cloud_api = WebSocketClient(self)
        else:
            self._cloud_api = CloudAPI(self)

        # 初始化UI
        self._init_ui()

        # 连接信号
        self._connect_signals()

        # 加载历史数据
        self._load_history()
        self._refresh_database_counts()

        # 启动通信
        if config.USE_REAL_CLOUD:
            self._cloud_api.start()
        else:
            self._cloud_api.start_polling()

        logger.info("主窗口初始化完成")

    def _init_ui(self):
        """构建UI布局"""
        scroll = QScrollArea()
        scroll.setWidgetResizable(True)
        scroll.setFrameShape(QScrollArea.NoFrame)
        scroll.setHorizontalScrollBarPolicy(Qt.ScrollBarAlwaysOff)
        self.setCentralWidget(scroll)

        central = QWidget()
        central.setMinimumWidth(980)
        scroll.setWidget(central)

        main_layout = QVBoxLayout(central)
        main_layout.setSpacing(8)
        main_layout.setContentsMargins(12, 8, 12, 8)

        # ---- 标题栏 ----
        title_row = QHBoxLayout()

        title_label = QLabel("🏠 智能门禁访客管理系统")
        title_label.setObjectName("title_label")
        title_label.setFont(QFont("Microsoft YaHei", 18, QFont.Bold))
        title_label.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Preferred)
        title_row.addWidget(title_label)

        title_row.addStretch()

        # 连接状态指示
        mode_text = "● 智云连接中" if config.USE_REAL_CLOUD else "● 模拟模式"
        self._conn_label = QLabel(mode_text)
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
        top_splitter.setChildrenCollapsible(False)

        # 左侧：门禁看板 + 环境面板（纵向堆叠）
        left_widget = QWidget()
        left_layout = QVBoxLayout(left_widget)
        left_layout.setContentsMargins(0, 0, 0, 0)
        left_layout.setSpacing(8)

        self._door_panel = DoorStatusPanel()
        left_layout.addWidget(self._door_panel, stretch=1)

        self._env_panel = EnvInfoPanel()
        left_layout.addWidget(self._env_panel, stretch=1)

        top_splitter.addWidget(left_widget)

        # 右侧：远程控制 + 运行诊断
        right_widget = QWidget()
        right_layout = QVBoxLayout(right_widget)
        right_layout.setContentsMargins(0, 0, 0, 0)
        right_layout.setSpacing(8)

        self._control_panel = RemoteControlPanel()
        right_layout.addWidget(self._control_panel, stretch=1)

        self._system_panel = SystemStatusPanel()
        right_layout.addWidget(self._system_panel, stretch=1)

        top_splitter.addWidget(right_widget)

        top_splitter.setSizes([700, 500])
        main_layout.addWidget(top_splitter, stretch=3)

        # ---- 中间区：事件时间线 ----
        self._timeline_panel = EventTimelinePanel()
        main_layout.addWidget(self._timeline_panel, stretch=2)

        # ---- 下半区：统计图表 ----
        self._stats_panel = StatsPanel()
        self._stats_panel.set_database(self._db)
        self._stats_panel.export_requested.connect(self._on_export_requested)
        main_layout.addWidget(self._stats_panel, stretch=1)

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
        if config.USE_REAL_CLOUD:
            self._cloud_api.connected.connect(self._on_cloud_connected)
            self._cloud_api.disconnected.connect(self._on_cloud_disconnected)

        # 远程控制按钮 → 发送命令
        self._control_panel.command_requested.connect(self._on_command_requested)

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
            1. 更新内存中的 SensorState
            2. 刷新门禁看板
            3. 刷新环境面板
            4. 生成事件记录（事件时间线）
            5. 存储到数据库
        """
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
        self._system_panel.append_data(data)
        self._refresh_database_counts()

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
            if config.FIELD_TCH in data:
                tch = int(data.get(config.FIELD_TCH, 0))
                last_tch = self._last_db_values.get(config.FIELD_TCH)
                self._last_db_values[config.FIELD_TCH] = tch
            else:
                tch = None
                last_tch = None

            if tch == 1 and last_tch != 1:
                self._insert_event_if_new(
                    sensor="tch",
                    event_type=config.EVENT_DOORBELL,
                    value=1,
                    alert=int(data.get(config.FIELD_ALERT, 0))
                )

            if config.FIELD_PIR in data:
                pir = int(data.get(config.FIELD_PIR, 0))
                last_pir = self._last_db_values.get(config.FIELD_PIR)
                self._last_db_values[config.FIELD_PIR] = pir
                stay = int(data.get(config.FIELD_STAY, 0))
                if pir == 0:
                    self._loiter_db_reported = False
                elif stay > 10 and not self._loiter_db_reported:
                    self._loiter_db_reported = True
                    self._insert_event_if_new(
                        sensor="pir",
                        event_type=config.EVENT_LOITER,
                        value=1,
                        alert=max(1, int(data.get(config.FIELD_ALERT, 0)))
                    )
                elif pir == 1 and last_pir != 1:
                    self._insert_event_if_new(
                        sensor="pir",
                        event_type=config.EVENT_PIR_DETECT,
                        value=1,
                        alert=int(data.get(config.FIELD_ALERT, 0))
                    )

            if config.FIELD_DOOR in data:
                door = int(data.get(config.FIELD_DOOR, 0))
                last_door = self._last_db_values.get(config.FIELD_DOOR)
                self._last_db_values[config.FIELD_DOOR] = door
                if last_door is None or door == last_door:
                    return
                alert = int(data.get(config.FIELD_ALERT, 0))
                night = int(data.get(config.FIELD_NIGHT, 0))
                if door == 1:
                    event_type = config.EVENT_INTRUSION if alert == config.ALERT_ALARM or night == 1 else config.EVENT_DOOR_OPEN
                else:
                    event_type = config.EVENT_DOOR_CLOSE
                self._insert_event_if_new(
                    sensor="door",
                    event_type=event_type,
                    value=door,
                    alert=alert
                )

        except Exception as e:
            logger.warning("数据库写入失败: %s", e)

    def _insert_event_if_new(self, sensor, event_type, value, alert):
        """短时间内同类事件只记一次，避免持续上报刷库。"""
        if self._db.has_recent_access_event(sensor, event_type, value, seconds=3):
            return
        log = AccessLog(
            sensor=sensor,
            event_type=event_type,
            value=value,
            alert=alert
        )
        self._db.insert_access_log(log)

    # ======================== 命令发送 ========================

    def _on_command_requested(self, cmd: dict):
        """
        处理远程控制面板发出的命令

        参数：
            cmd — 命令字典（如 {"unlock": 1, "rgb": [0, 255, 0]}）
                  特殊字段 _target 指定目标节点（默认 sensor-b）
        """
        target = cmd.get("_target", "sensor-b")
        send_cmd = dict(cmd)
        send_cmd.pop("_target", None)

        if target == "sensor-c":
            self._cloud_api.send_to_sensor_c(send_cmd)
        else:
            self._cloud_api.send_to_sensor_b(send_cmd)

        logger.info("发送命令到 %s: %s", target, send_cmd)
        self._system_panel.append_command(target, send_cmd)

        # 记录远程开门事件
        if send_cmd.get("unlock") == 1:
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
            self._refresh_database_counts()

        # 记录告警解除事件
        if send_cmd.get("reset") == 1:
            self._timeline_panel.add_event(
                config.EVENT_ALERT_RESET,
                sensor="remote",
                detail="用户解除告警",
                alert=0
            )

    def _on_command_sent(self, success: bool):
        """命令发送结果回调"""
        self._control_panel.set_send_result(success)
        if success:
            self._status_msg.setText(
                f"✅ 命令发送成功 | {datetime.now().strftime('%H:%M:%S')}"
            )
        else:
            self._status_msg.setText(
                f"❌ 命令发送失败 | {datetime.now().strftime('%H:%M:%S')}"
            )

    def _on_cloud_connected(self):
        """智云 WebSocket 连接成功"""
        self._conn_label.setText("● 智云已连接")
        self._conn_label.setStyleSheet(
            "color: #2ECC71; font-size: 13px; font-weight: bold; background: transparent;"
        )
        self._system_panel.set_cloud_state("已认证", ok=True)

    def _on_cloud_disconnected(self):
        """智云 WebSocket 断开"""
        self._conn_label.setText("● 智云已断开")
        self._conn_label.setStyleSheet(
            "color: #E74C3C; font-size: 13px; font-weight: bold; background: transparent;"
        )
        self._system_panel.set_disconnected()

    def _on_error(self, msg: str):
        """错误处理"""
        self._status_msg.setText(f"⚠️ {msg}")
        logger.error(msg)

    def _on_export_requested(self, table_name):
        """导出数据库表为 CSV。"""
        default_name = f"{table_name}_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv"
        file_path, _ = QFileDialog.getSaveFileName(
            self,
            "导出 CSV",
            default_name,
            "CSV Files (*.csv)"
        )
        if not file_path:
            return
        try:
            count = self._db.export_table_csv(table_name, file_path)
            QMessageBox.information(self, "导出完成", f"已导出 {count} 条记录。")
        except Exception as e:
            QMessageBox.warning(self, "导出失败", str(e))

    def _refresh_database_counts(self):
        """刷新诊断面板数据库摘要。"""
        try:
            access_count, env_count = self._db.get_summary_counts()
            if hasattr(self, "_system_panel"):
                self._system_panel.update_database_counts(access_count, env_count)
        except Exception as e:
            logger.debug("刷新数据库摘要失败: %s", e)

    # ======================== 工具方法 ========================

    def _update_clock(self):
        """更新时钟显示"""
        self._time_label.setText(datetime.now().strftime("%Y-%m-%d %H:%M:%S"))

    def closeEvent(self, event):
        """窗口关闭时清理资源"""
        if config.USE_REAL_CLOUD:
            self._cloud_api.stop()
        else:
            self._cloud_api.stop_polling()
        self._clock_timer.stop()
        logger.info("主窗口已关闭")
        event.accept()

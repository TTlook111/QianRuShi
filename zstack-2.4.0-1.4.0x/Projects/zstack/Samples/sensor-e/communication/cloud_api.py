"""
cloud_api.py — 智云平台 HTTP API 封装
E同学：Python应用层负责人
通过 HTTP REST API 与智云平台通信，获取传感器数据 / 下发控制命令
"""

import json
import logging
from datetime import datetime

from PyQt5.QtCore import QObject, QTimer, pyqtSignal

import config
from data.models import SensorState

logger = logging.getLogger(__name__)


class CloudAPI(QObject):
    """
    智云平台 HTTP API 客户端

    信号：
        data_received(dict)  — 收到新的传感器数据
        command_sent(bool)   — 命令发送结果（成功/失败）
        error_occurred(str)  — 发生错误
    """

    data_received = pyqtSignal(dict)
    command_sent = pyqtSignal(bool)
    error_occurred = pyqtSignal(str)

    def __init__(self, parent=None):
        super().__init__(parent)
        self.base_url = config.CLOUD_API_BASE_URL
        self.api_key = config.CLOUD_API_KEY
        self.device_id = config.CLOUD_DEVICE_ID
        self._poll_timer = QTimer(self)
        self._poll_timer.timeout.connect(self._poll_data)
        self._connected = False

    def start_polling(self, interval_ms=None):
        """启动定时轮询获取传感器数据"""
        interval = interval_ms or config.CLOUD_POLL_INTERVAL_MS
        self._poll_timer.start(interval)
        self._connected = True
        logger.info("智云平台轮询已启动, 间隔=%dms", interval)

    def stop_polling(self):
        """停止轮询"""
        self._poll_timer.stop()
        self._connected = False
        logger.info("智云平台轮询已停止")

    def is_connected(self):
        """返回连接状态"""
        return self._connected

    def _poll_data(self):
        """
        轮询获取传感器数据
        TODO: 替换为实际智云平台 API 调用
        当前使用模拟数据用于UI测试
        """
        try:
            # ---- 实际 API 调用（取消注释后使用）----
            # import requests
            # headers = {"Authorization": f"Bearer {self.api_key}"}
            # resp = requests.get(
            #     f"{self.base_url}/devices/{self.device_id}/data",
            #     headers=headers, timeout=5
            # )
            # if resp.status_code == 200:
            #     data = resp.json()
            #     self.data_received.emit(data)
            # else:
            #     self.error_occurred.emit(f"API返回错误: {resp.status_code}")

            # ---- 模拟数据（用于开发测试）----
            import random
            mock_data = {
                # sensor-a 环境数据
                "temp": round(20.0 + random.uniform(-3, 8), 1),
                "humi": round(40.0 + random.uniform(-10, 30), 1),
                "lux": random.randint(50, 800),
                # sensor-c 安防数据
                "pir": random.choice([0, 0, 0, 0, 1]),
                "door": random.choice([0, 0, 0, 0, 0, 1]),
                "tch": 0,
                "alert": 0,
                "stay": 0,
                "night": 0,
                # sensor-b 控制状态
                "unlock": 0,
                "buzz": 0,
                "rgb": [0, 255, 0],
            }
            self.data_received.emit(mock_data)

        except Exception as e:
            self.error_occurred.emit(f"数据获取失败: {e}")
            logger.error("轮询数据失败: %s", e)

    def send_command(self, node_name, cmd_dict):
        """
        向指定节点下发控制命令

        参数：
            node_name — 节点名称（如 "sensor-b"）
            cmd_dict  — 命令字典（如 {"unlock": 1}）

        命令格式遵循通信接口说明：
            远程开门：{"unlock": 1, "rgb": [0, 255, 0]}
            门铃触发：{"buzz": 500, "rgb": [0, 0, 255]}
            声光告警：{"buzz": 1, "rgb": [255, 0, 0]}
            告警解除：{"reset": 1}
        """
        try:
            # ---- 实际 API 调用（取消注释后使用）----
            # import requests
            # headers = {
            #     "Authorization": f"Bearer {self.api_key}",
            #     "Content-Type": "application/json"
            # }
            # payload = {
            #     "device_id": self.device_id,
            #     "node": node_name,
            #     "command": cmd_dict
            # }
            # resp = requests.post(
            #     f"{self.base_url}/devices/{self.device_id}/command",
            #     headers=headers, json=payload, timeout=5
            # )
            # success = resp.status_code == 200
            # self.command_sent.emit(success)

            # ---- 模拟发送（开发测试）----
            cmd_str = json.dumps(cmd_dict, ensure_ascii=False)
            logger.info("发送命令到 %s: %s", node_name, cmd_str)
            self.command_sent.emit(True)

        except Exception as e:
            self.error_occurred.emit(f"命令发送失败: {e}")
            self.command_sent.emit(False)
            logger.error("发送命令失败: %s", e)

    def send_to_sensor_b(self, cmd_dict):
        """快捷方法：向 sensor-b（控制执行节点）发送命令"""
        self.send_command("sensor-b", cmd_dict)

    def send_to_sensor_c(self, cmd_dict):
        """快捷方法：向 sensor-c（安防检测节点）发送命令"""
        self.send_command("sensor-c", cmd_dict)

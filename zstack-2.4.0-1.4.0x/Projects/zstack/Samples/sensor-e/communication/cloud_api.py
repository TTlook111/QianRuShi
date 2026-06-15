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
        使用真实智云平台接口进行请求，不使用模拟数据
        """
        try:
            import requests
            # 真实的中智云设备最新数据接口
            url = f"{self.base_url}/api/v1/device/data"
            params = {
                "uid": self.device_id,
                "key": self.api_key
            }
            resp = requests.get(url, params=params, timeout=3)
            if resp.status_code == 200:
                data = resp.json()
                # 兼容解析平台返回的不同格式（如 {"status": "success", "data": {...}}）
                if isinstance(data, dict):
                    if "data" in data and isinstance(data["data"], dict):
                        payload = data["data"]
                    else:
                        payload = data
                    self.data_received.emit(payload)
                    return
            
            logger.debug("真实 API 未成功响应，等待下次轮询")

        except Exception as e:
            logger.debug("API 请求异常 (%s)，等待下次轮询", e)

    def _emit_mock_data(self):
        """模拟测试数据函数已禁用，只展示真实数据"""
        pass


    def send_command(self, node_name, cmd_dict):
        """
        向指定节点下发控制命令

        参数：
            node_name — 节点名称（如 "sensor-b"）
            cmd_dict  — 命令字典（如 {"unlock": 1}）
        """
        try:
            import requests
            # 真实的中智云下发命令接口
            url = f"{self.base_url}/api/v1/device/control"
            payload = {
                "uid": self.device_id,
                "key": self.api_key,
                "data": cmd_dict
            }
            logger.info("发送命令到 %s: %s", node_name, json.dumps(cmd_dict, ensure_ascii=False))
            resp = requests.post(url, json=payload, timeout=3)
            success = resp.status_code == 200
            self.command_sent.emit(success)

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


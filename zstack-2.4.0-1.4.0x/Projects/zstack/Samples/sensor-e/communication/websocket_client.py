"""
websocket_client.py — WebSocket 实时通信客户端
E同学：Python应用层负责人
通过 WebSocket 实时接收智云平台推送的传感器事件
"""

import json
import logging
import threading
import time

from PyQt5.QtCore import QObject, pyqtSignal

import config

logger = logging.getLogger(__name__)


class WebSocketClient(QObject):
    """
    WebSocket 实时通信客户端

    信号：
        data_received(dict) — 收到实时传感器数据
        connected()         — WebSocket 已连接
        disconnected()      — WebSocket 已断开
        command_sent(bool)  — 控制命令发送结果
        error_occurred(str) — 发生错误
    """

    data_received = pyqtSignal(dict)
    connected = pyqtSignal()
    disconnected = pyqtSignal()
    command_sent = pyqtSignal(bool)
    error_occurred = pyqtSignal(str)

    def __init__(self, parent=None):
        super().__init__(parent)
        self._ws = None
        self._thread = None
        self._running = False
        self._ws_url = config.CLOUD_WS_URL
        self._authenticated = False

    def start(self):
        """启动 WebSocket 连接（在后台线程中运行）"""
        if self._running:
            return
        self._running = True
        self._thread = threading.Thread(target=self._run, daemon=True)
        self._thread.start()
        logger.info("WebSocket 客户端已启动")

    def stop(self):
        """停止 WebSocket 连接"""
        self._running = False
        if self._ws:
            try:
                self._ws.close()
            except Exception:
                pass
        logger.info("WebSocket 客户端已停止")

    def is_connected(self):
        """返回连接状态"""
        return self._running and self._ws is not None and self._authenticated

    def _run(self):
        """WebSocket 连接主循环"""
        try:
            import websocket

            def on_open(ws):
                logger.info("WebSocket 已连接: %s", self._ws_url)
                self._send_authenticate(ws)

            def on_message(ws, message):
                self._handle_message(ws, message)

            def on_error(ws, error):
                error_msg = f"WebSocket 错误: {error}"
                logger.error(error_msg)
                self.error_occurred.emit(error_msg)

            def on_close(ws, close_code, close_msg):
                logger.info("WebSocket 已断开 (code=%s)", close_code)
                self._authenticated = False
                self.disconnected.emit()

            self._ws = websocket.WebSocketApp(
                self._ws_url,
                on_open=on_open,
                on_message=on_message,
                on_error=on_error,
                on_close=on_close
            )

            # 运行，自动重连
            while self._running:
                self._ws.run_forever(ping_interval=30, ping_timeout=10)
                if self._running:
                    logger.info("WebSocket 断开，5秒后重连...")
                    time.sleep(5)

        except ImportError:
            msg = "websocket-client 未安装，请运行: pip install websocket-client"
            logger.error(msg)
            self.error_occurred.emit(msg)
        except Exception as e:
            logger.error("WebSocket 运行异常: %s", e)
            self.error_occurred.emit(f"WebSocket 异常: {e}")
        finally:
            self._authenticated = False
            self._running = False

    def _send_json(self, ws, payload):
        """发送智云 WebSocket JSON 文本帧。"""
        msg = json.dumps(payload, ensure_ascii=False, separators=(",", ":"))
        ws.send(msg)
        logger.info("WebSocket 发送: %s", msg)

    def _send_authenticate(self, ws):
        payload = {
            "method": "authenticate",
            "uid": config.CLOUD_UID,
            "key": config.CLOUD_KEY,
            "version": "0.1.0",
            "autodb": True,
        }
        self._send_json(ws, payload)

    def _handle_message(self, ws, message):
        """处理智云 authenticate/echo/sensor/message 包。"""
        if isinstance(message, bytes):
            message = message.decode("utf-8", errors="ignore")

        for raw in str(message).split("\x00"):
            raw = raw.strip()
            if not raw:
                continue
            logger.info("WebSocket 收到: %s", raw)

            try:
                packet = json.loads(raw)
            except json.JSONDecodeError as e:
                logger.warning("WebSocket 消息解析失败: %s raw=%s", e, raw)
                continue

            method = packet.get("method")
            if method == "authenticate_rsp":
                self._authenticated = packet.get("status") == "ok"
                if self._authenticated:
                    logger.info("智云平台认证成功")
                    self.connected.emit()
                else:
                    self.error_occurred.emit(f"智云认证失败: {packet}")
                continue

            if method == "echo":
                try:
                    self._send_json(ws, packet)
                except Exception as e:
                    self.error_occurred.emit(f"心跳回复失败: {e}")
                continue

            if method in ("sensor", "message"):
                data = self._parse_sensor_data(packet.get("data", ""))
                if data:
                    addr = packet.get("addr", "")
                    data = self._normalize_fields(addr, data)
                    data["_addr"] = addr
                    data["_method"] = method
                    self.data_received.emit(data)
                continue

            logger.info("收到未处理的智云消息: %s", packet)

    def _parse_sensor_data(self, data):
        """
        兼容智云常见数据格式：
        {"temp":27.0} / {temp:27.0} / {temp=27.0,humi=65,rgb=[0,255,0]}
        """
        if isinstance(data, dict):
            return data

        text = str(data).strip().strip("\x00")
        if not text:
            return {}

        try:
            return json.loads(text)
        except json.JSONDecodeError:
            pass

        json_start = text.find("{")
        json_end = text.rfind("}")
        if 0 <= json_start < json_end:
            json_text = text[json_start:json_end + 1]
            try:
                return json.loads(json_text)
            except json.JSONDecodeError:
                text = json_text

        if text.startswith("{") and text.endswith("}"):
            text = text[1:-1]

        result = {}
        for item in self._split_top_level(text):
            if not item:
                continue
            sep = "=" if "=" in item else ":"
            if sep not in item:
                continue
            key, value = item.split(sep, 1)
            key = key.strip().strip('"\'')
            result[key] = self._parse_value(value.strip())
        return result

    def _split_top_level(self, text):
        parts = []
        start = 0
        depth = 0
        for idx, ch in enumerate(text):
            if ch in "[{(":
                depth += 1
            elif ch in "]})" and depth > 0:
                depth -= 1
            elif ch == "," and depth == 0:
                parts.append(text[start:idx].strip())
                start = idx + 1
        parts.append(text[start:].strip())
        return parts

    def _parse_value(self, value):
        if value in ("?", ""):
            return value
        try:
            return json.loads(value)
        except json.JSONDecodeError:
            pass
        try:
            number = float(value)
            return int(number) if number.is_integer() else number
        except ValueError:
            return value.strip('"\'')

    def _normalize_fields(self, addr, data):
        """兼容硬件旧字段 A0/A1/A2，统一给 UI 项目字段。"""
        normalized = dict(data)
        if addr == config.NODE_MAC.get("sensor-a"):
            mapping = {"A0": config.FIELD_TEMP, "A1": config.FIELD_HUMI, "A2": config.FIELD_LUX}
        elif addr == config.NODE_MAC.get("sensor-c"):
            mapping = {"A0": config.FIELD_PIR, "A1": config.FIELD_TCH, "A2": config.FIELD_DOOR, "A3": config.FIELD_ALERT}
        else:
            mapping = {}

        for old_key, new_key in mapping.items():
            if old_key in normalized and new_key not in normalized:
                normalized[new_key] = normalized[old_key]
        return normalized

    def _format_command_data(self, cmd_dict):
        parts = []
        for key, value in cmd_dict.items():
            if isinstance(value, list):
                val = "[" + ",".join(str(v) for v in value) + "]"
            else:
                val = str(value)
            parts.append(f"{key}={val}")
        return "{" + ",".join(parts) + "}"

    def send_command(self, node_name, cmd_dict):
        """
        通过 WebSocket 发送控制命令
        参数：
            node_name — 节点名称（如 "sensor-b"）
            cmd_dict  — 命令字典（如 {"unlock": 1}）
        """
        if not self._ws or not self._running:
            self.error_occurred.emit("WebSocket 未连接")
            self.command_sent.emit(False)
            return
        if not self._authenticated:
            self.error_occurred.emit("智云尚未认证成功")
            self.command_sent.emit(False)
            return

        node_mac = config.NODE_MAC.get(node_name)
        if not node_mac:
            self.error_occurred.emit(f"未配置节点 MAC: {node_name}")
            self.command_sent.emit(False)
            return

        try:
            payload = {
                "method": "control",
                "addr": node_mac,
                "data": self._format_command_data(cmd_dict),
            }
            self._send_json(self._ws, payload)
            self.command_sent.emit(True)
        except Exception as e:
            logger.error("WebSocket 发送失败: %s", e)
            self.error_occurred.emit(f"发送失败: {e}")
            self.command_sent.emit(False)

    def send_to_sensor_b(self, cmd_dict):
        """快捷方法：向 sensor-b（控制执行节点）发送命令"""
        self.send_command("sensor-b", cmd_dict)

    def send_to_sensor_c(self, cmd_dict):
        """快捷方法：向 sensor-c（安防检测节点）发送命令"""
        self.send_command("sensor-c", cmd_dict)

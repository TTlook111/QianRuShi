"""
websocket_client.py — WebSocket 实时通信客户端
E同学：Python应用层负责人
通过 WebSocket 实时接收智云平台推送的传感器事件
"""

import json
import logging
import threading

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
        error_occurred(str) — 发生错误
    """

    data_received = pyqtSignal(dict)
    connected = pyqtSignal()
    disconnected = pyqtSignal()
    error_occurred = pyqtSignal(str)

    def __init__(self, parent=None):
        super().__init__(parent)
        self._ws = None
        self._thread = None
        self._running = False
        self._ws_url = config.CLOUD_WS_URL

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
        return self._running and self._ws is not None

    def _run(self):
        """WebSocket 连接主循环"""
        try:
            import websocket

            def on_open(ws):
                logger.info("WebSocket 已连接: %s", self._ws_url)
                self.connected.emit()

            def on_message(ws, message):
                try:
                    data = json.loads(message)
                    self.data_received.emit(data)
                except json.JSONDecodeError as e:
                    logger.warning("WebSocket 消息解析失败: %s", e)

            def on_error(ws, error):
                error_msg = f"WebSocket 错误: {error}"
                logger.error(error_msg)
                self.error_occurred.emit(error_msg)

            def on_close(ws, close_code, close_msg):
                logger.info("WebSocket 已断开 (code=%s)", close_code)
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
                    import time
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
            self._running = False

    def send_command(self, cmd_dict):
        """
        通过 WebSocket 发送控制命令
        参数：cmd_dict — 命令字典（如 {"unlock": 1}）
        """
        if self._ws and self._running:
            try:
                msg = json.dumps(cmd_dict, ensure_ascii=False)
                self._ws.send(msg)
                logger.info("WebSocket 发送: %s", msg)
            except Exception as e:
                logger.error("WebSocket 发送失败: %s", e)
                self.error_occurred.emit(f"发送失败: {e}")

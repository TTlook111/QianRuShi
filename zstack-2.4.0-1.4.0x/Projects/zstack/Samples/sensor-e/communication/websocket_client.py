"""
websocket_client.py — WebSocket 实时通信客户端
E同学：Python应用层负责人
通过 WebSocket 实时接收智云平台推送的传感器事件
"""

import json
import ssl
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
                logger.info("WebSocket 已连接: %s, 正在进行身份认证...", self._ws_url)
                # 智云物联平台 WebSocket 登录授权包
                reg_payload = {
                    "method": "authenticate",
                    "uid": config.CLOUD_DEVICE_ID,
                    "key": config.CLOUD_API_KEY
                }
                try:
                    ws.send(json.dumps(reg_payload))
                    logger.info("智云平台注册认证请求发送成功: uid=%s", config.CLOUD_DEVICE_ID)
                    self.connected.emit()
                except Exception as e:
                    logger.error("智云平台注册认证请求发送失败: %s", e)

            def on_message(ws, message):
                try:
                    data = json.loads(message)
                    # 过滤并记录认证回复帧
                    if isinstance(data, dict):
                        if data.get("method") == "authenticate":
                            logger.info("智云平台认证成功回复: %s", data.get("status", "success"))
                            return
                        # 兼容官方 WSNRTConnect.js 协议推送格式 (method='message' 且包含 'addr' 和 'data')
                        if data.get("method") == "message" and "addr" in data:
                            payload_str = data.get("data", "")
                            payload = {}
                            if isinstance(payload_str, str):
                                import re
                                # 提取 ZXBee 原始帧中包裹在 { } 内的 JSON 数据段
                                match = re.search(r'(\{.*\})', payload_str)
                                if match:
                                    try:
                                        payload = json.loads(match.group(1))
                                    except Exception as e:
                                        logger.warning("WebSocket 提取 JSON 解析失败: %s, 原始串: %s", e, payload_str)
                            elif isinstance(payload_str, dict):
                                payload = payload_str

                            # 重新打包为 addr/data 结构投递给 UI 统一处理
                            data = {
                                "addr": data.get("addr"),
                                "data": payload
                            }

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
                # 开启安全绕过，使用 ssl.CERT_NONE 规避 28090 加密端口的 SSL 证书链报错
                self._ws.run_forever(
                    ping_interval=30,
                    ping_timeout=10,
                    sslopt={"cert_reqs": ssl.CERT_NONE}
                )

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

    def send_command(self, mac_addr, cmd_dict):
        """
        通过 WebSocket 发送控制命令给特定 MAC 地址的硬件设备
        参数：
            mac_addr — 目标子设备的 MAC 地址
            cmd_dict — 控制命令字典（如 {"unlock": 1, "rgb": [0, 255, 0]}）
        """
        if self._ws and self._running:
            try:
                # 针对单片机解析能力的局限性，将多字段指令拆分成单条独立指令发送
                # 例如：将 {"unlock":1, "rgb":[0,255,0]} 拆分为 "{unlock=1}" 和 "{rgb=[0,255,0]}" 分开发送
                # 每次发送间隔 150ms，防止单片机串口及射频缓冲区发生粘包或溢出
                import time
                for k, v in cmd_dict.items():
                    if isinstance(v, list):
                        val_str = "[" + ",".join(map(str, v)) + "]"
                    else:
                        val_str = str(v)
                    payload_str = f"{{{k}={val_str}}}"

                    # 按照中智云平台官方 WSNRTConnect.js 的 sendMessage 协议封装
                    msg_dict = {
                        "method": "control",
                        "addr": mac_addr,
                        "data": payload_str
                    }
                    msg = json.dumps(msg_dict, ensure_ascii=False)
                    self._ws.send(msg)
                    logger.info("WebSocket 发送控制到 MAC(%s): %s", mac_addr, msg)
                    
                    # 延时 150ms
                    time.sleep(0.15)
            except Exception as e:
                logger.error("WebSocket 发送至 MAC(%s) 失败: %s", mac_addr, e)
                self.error_occurred.emit(f"发送失败: {e}")






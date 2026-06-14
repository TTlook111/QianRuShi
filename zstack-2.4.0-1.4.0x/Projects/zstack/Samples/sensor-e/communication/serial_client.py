"""
serial_client.py — 串口直连通信模式（备用）
E同学：Python应用层负责人
直接通过串口与协调器通信，解析 ZXBee 帧格式
可在无智云平台时使用
"""

import json
import logging
import threading

from PyQt5.QtCore import QObject, pyqtSignal

import config

logger = logging.getLogger(__name__)


class SerialClient(QObject):
    """
    串口直连通信客户端

    信号：
        data_received(dict) — 收到解析后的传感器数据
        connected()         — 串口已连接
        disconnected()      — 串口已断开
        error_occurred(str) — 发生错误
    """

    data_received = pyqtSignal(dict)
    connected = pyqtSignal()
    disconnected = pyqtSignal()
    error_occurred = pyqtSignal(str)

    def __init__(self, parent=None):
        super().__init__(parent)
        self._serial = None
        self._thread = None
        self._running = False

    def start(self, port=None, baudrate=None):
        """
        打开串口并启动接收线程
        参数：
            port     — 串口号（默认使用config.SERIAL_PORT）
            baudrate — 波特率（默认使用config.SERIAL_BAUDRATE）
        """
        if self._running:
            return

        port = port or config.SERIAL_PORT
        baudrate = baudrate or config.SERIAL_BAUDRATE

        try:
            import serial
            self._serial = serial.Serial(
                port=port,
                baudrate=baudrate,
                bytesize=config.SERIAL_DATABITS,
                stopbits=config.SERIAL_STOPBITS,
                parity=config.SERIAL_PARITY,
                timeout=config.SERIAL_TIMEOUT
            )
            self._running = True
            self._thread = threading.Thread(target=self._recv_loop, daemon=True)
            self._thread.start()
            self.connected.emit()
            logger.info("串口已连接: %s @ %d", port, baudrate)

        except ImportError:
            msg = "pyserial 未安装，请运行: pip install pyserial"
            logger.error(msg)
            self.error_occurred.emit(msg)
        except Exception as e:
            msg = f"串口打开失败: {e}"
            logger.error(msg)
            self.error_occurred.emit(msg)

    def stop(self):
        """关闭串口"""
        self._running = False
        if self._serial and self._serial.is_open:
            try:
                self._serial.close()
            except Exception:
                pass
        self.disconnected.emit()
        logger.info("串口已关闭")

    def is_connected(self):
        """返回连接状态"""
        return self._running and self._serial is not None and self._serial.is_open

    def _recv_loop(self):
        """串口接收主循环，解析 ZXBee 帧"""
        buf = bytearray()

        while self._running:
            try:
                if not self._serial or not self._serial.is_open:
                    break

                # 读取可用数据
                data = self._serial.read(256)
                if not data:
                    continue

                buf.extend(data)

                # 尝试从缓冲区解析 ZXBee 帧
                while len(buf) >= 9:
                    # 查找帧头 SOF = 0xAA
                    sof_idx = -1
                    for i in range(len(buf)):
                        if buf[i] == config.ZXBEE_SOF:
                            sof_idx = i
                            break

                    if sof_idx < 0:
                        buf.clear()
                        break

                    # 丢弃 SOF 之前的无效字节
                    if sof_idx > 0:
                        buf = buf[sof_idx:]

                    # 检查是否有足够数据读取 LEN 字段
                    if len(buf) < 7:
                        break

                    payload_len = buf[6]
                    frame_len = payload_len + 9  # SOF(1) + DST(2) + SRC(2) + CMD(1) + LEN(1) + PAYLOAD(N) + CHK(1) + EOF(1)

                    if len(buf) < frame_len:
                        break

                    # 检查帧尾
                    if buf[frame_len - 1] != config.ZXBEE_EOF:
                        buf = buf[1:]
                        continue

                    # 校验和验证
                    calc_chk = sum(buf[:7 + payload_len]) & 0xFF
                    if calc_chk != buf[7 + payload_len]:
                        logger.warning("ZXBee 校验和不匹配")
                        buf = buf[1:]
                        continue

                    # 解析帧内容
                    frame = buf[:frame_len]
                    buf = buf[frame_len:]

                    dst = (frame[1] << 8) | frame[2]
                    src = (frame[3] << 8) | frame[4]
                    cmd = frame[5]
                    payload_bytes = frame[7:7 + payload_len]

                    try:
                        payload_str = payload_bytes.decode("utf-8")
                        payload_dict = json.loads(payload_str)

                        # 添加来源元数据
                        payload_dict["_src"] = src
                        payload_dict["_dst"] = dst
                        payload_dict["_cmd"] = cmd

                        logger.info("收到 ZXBee 帧: src=0x%04X cmd=0x%02X %s",
                                    src, cmd, payload_str)
                        self.data_received.emit(payload_dict)

                    except (UnicodeDecodeError, json.JSONDecodeError) as e:
                        logger.warning("ZXBee Payload 解析失败: %s", e)

            except Exception as e:
                if self._running:
                    logger.error("串口接收异常: %s", e)
                break

        self._running = False
        self.disconnected.emit()

    def send_command(self, dst_addr, cmd_dict):
        """
        通过串口发送 ZXBee 帧到指定节点

        参数：
            dst_addr — 目标节点地址（如 0x0002）
            cmd_dict — 命令字典（如 {"unlock": 1}）
        """
        if not self._serial or not self._serial.is_open:
            self.error_occurred.emit("串口未连接")
            return

        try:
            payload = json.dumps(cmd_dict, separators=(",", ":"), ensure_ascii=False)
            payload_bytes = payload.encode("utf-8")
            plen = len(payload_bytes)

            # 构建 ZXBee 帧
            frame = bytearray()
            frame.append(config.ZXBEE_SOF)                 # SOF
            frame.append((dst_addr >> 8) & 0xFF)            # DST 高字节
            frame.append(dst_addr & 0xFF)                   # DST 低字节
            frame.append(0x00)                              # SRC 高字节（PC端 = 0x0000）
            frame.append(0x00)                              # SRC 低字节
            frame.append(config.CMD_WRITE)                  # CMD = 写入命令
            frame.append(plen)                              # LEN
            frame.extend(payload_bytes)                     # PAYLOAD
            chk = sum(frame) & 0xFF
            frame.append(chk)                               # CHK
            frame.append(config.ZXBEE_EOF)                  # EOF

            self._serial.write(frame)
            logger.info("串口发送 ZXBee 帧到 0x%04X: %s", dst_addr, payload)

        except Exception as e:
            msg = f"串口发送失败: {e}"
            logger.error(msg)
            self.error_occurred.emit(msg)

    def send_to_sensor_b(self, cmd_dict):
        """快捷方法：向 sensor-b 发送命令"""
        self.send_command(config.NODE_ADDR["sensor-b"], cmd_dict)

    def send_to_sensor_c(self, cmd_dict):
        """快捷方法：向 sensor-c 发送命令"""
        self.send_command(config.NODE_ADDR["sensor-c"], cmd_dict)

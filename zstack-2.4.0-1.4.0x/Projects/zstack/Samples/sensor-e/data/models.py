"""
models.py — 数据模型定义
E同学：Python应用层负责人
对应数据库表：access_log / door_env
"""

from dataclasses import dataclass, field
from datetime import datetime


@dataclass
class AccessLog:
    """
    访客事件日志（对应 access_log 表）

    字段说明：
        id              — 自增主键
        student_id      — 学生编号
        sensor          — 触发传感器（pir / door / tch）
        event_type      — 事件类型（doorbell / 徘徊 / 开门 / 告警）
        value           — 传感器值
        alert           — 告警等级（0正常 / 1注意 / 2报警）
        is_remote_unlock — 是否远程开门
        create_time     — 记录创建时间
    """
    id: int = 0
    student_id: str = ""
    sensor: str = ""
    event_type: str = ""
    value: int = 0
    alert: int = 0
    is_remote_unlock: bool = False
    create_time: datetime = field(default_factory=datetime.now)


@dataclass
class DoorEnv:
    """
    门口环境数据（对应 door_env 表）

    字段说明：
        id          — 自增主键
        student_id  — 学生编号
        temp        — 温度（℃）
        humi        — 湿度（%RH）
        lux         — 光照强度（lux）
        create_time — 记录创建时间
    """
    id: int = 0
    student_id: str = ""
    temp: float = 0.0
    humi: float = 0.0
    lux: int = 0
    create_time: datetime = field(default_factory=datetime.now)


@dataclass
class SensorState:
    """
    实时传感器状态（内存中维护，用于UI刷新）

    汇聚 sensor-a / sensor-b / sensor-c 所有上行字段
    """
    # sensor-a 环境数据
    temp: float = 0.0
    humi: float = 0.0
    lux: int = 0

    # sensor-c 安防数据
    pir: int = 0
    door: int = 0
    tch: int = 0
    alert: int = 0
    stay: int = 0
    night: int = 0

    # sensor-b 控制状态
    unlock: int = 0
    buzz: int = 0
    rgb: list = field(default_factory=lambda: [0, 255, 0])

    # 元数据
    last_update: datetime = field(default_factory=datetime.now)

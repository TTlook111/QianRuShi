"""
config.py — sensor-e 系统配置
E同学：Python应用层负责人
智能门禁访客管理系统 — PyQt5 上位机配置文件
"""

# ======================== 智云平台配置 ========================
# TODO: 替换为课程提供的实际智云平台 API 地址
CLOUD_API_BASE_URL = "http://cloud.example.com/api"
CLOUD_API_KEY = ""                      # 智云平台 API Key
CLOUD_DEVICE_ID = ""                    # 智云平台设备 ID
CLOUD_POLL_INTERVAL_MS = 3000           # HTTP 轮询间隔（毫秒）

# WebSocket 实时推送地址
CLOUD_WS_URL = "ws://cloud.example.com/ws"

# ======================== 串口配置（备用直连模式）========================
SERIAL_PORT = "COM3"                    # 串口号，根据实际修改
SERIAL_BAUDRATE = 38400                 # 波特率（与协调器一致）
SERIAL_DATABITS = 8
SERIAL_STOPBITS = 1
SERIAL_PARITY = "N"                     # 无校验
SERIAL_TIMEOUT = 1                      # 读超时（秒）

# ======================== 数据库配置 ========================
DATABASE_TYPE = "mysql"                # "sqlite" 或 "mysql"
SQLITE_DB_PATH = "door_access.db"       # SQLite 数据库文件路径

# MySQL 配置（B同学数据库，暂留接口）
MYSQL_HOST = "localhost"
MYSQL_PORT = 3306
MYSQL_USER = "root"
MYSQL_PASSWORD = "123456yhj"
MYSQL_DATABASE = "door_access"

# ======================== ZigBee 节点地址映射 ========================
NODE_ADDR = {
    "coordinator": 0x0000,              # 协调器
    "sensor-a":    0x0001,              # 环境采集（601）
    "sensor-b":    0x0002,              # 控制执行（602）
    "sensor-c":    0x0003,              # 安防检测（603）
}

NODE_NAME = {
    0x0000: "协调器",
    0x0001: "环境采集(601)",
    0x0002: "控制执行(602)",
    0x0003: "安防检测(603)",
}

# ======================== 项目统一字段定义 ========================
# 上行字段（硬件 -> 智云 -> Python）
FIELD_PIR = "pir"                       # 人体红外 0/1
FIELD_DOOR = "door"                     # 门磁 0/1
FIELD_TCH = "tch"                       # 门铃/触摸 0/1
FIELD_TEMP = "temp"                     # 温度 float
FIELD_HUMI = "humi"                     # 湿度 float
FIELD_LUX = "lux"                       # 光照 int
FIELD_ALERT = "alert"                   # 告警等级 0/1/2
FIELD_STAY = "stay"                     # 停留时间 int(秒)
FIELD_NIGHT = "night"                   # 夜间模式 0/1

# 下行字段（Python -> 智云 -> 硬件）
FIELD_UNLOCK = "unlock"                 # 远程开门 0/1
FIELD_BUZZ = "buzz"                     # 蜂鸣器 0/1/500
FIELD_RGB = "rgb"                       # RGB [R,G,B]
FIELD_RESET = "reset"                   # 告警解除 0/1

# ======================== 告警等级定义 ========================
ALERT_SAFE = 0                          # 安全（绿色）
ALERT_ATTENTION = 1                     # 注意（蓝色）——有人徘徊
ALERT_ALARM = 2                         # 报警（红色）——夜间入侵

# ======================== 事件类型定义 ========================
EVENT_DOORBELL = "doorbell"             # 门铃
EVENT_PIR_DETECT = "pir_detect"         # PIR检测到人
EVENT_DOOR_OPEN = "door_open"           # 门被打开
EVENT_DOOR_CLOSE = "door_close"         # 门被关闭
EVENT_LOITER = "loiter"                 # 有人徘徊
EVENT_INTRUSION = "intrusion"           # 夜间入侵
EVENT_REMOTE_UNLOCK = "remote_unlock"   # 远程开门
EVENT_ALERT_RESET = "alert_reset"       # 告警解除

# ======================== ZXBee 协议常量 ========================
ZXBEE_SOF = 0xAA
ZXBEE_EOF = 0x55
CMD_REPORT = 0x01                       # 数据上报
CMD_ALARM = 0x02                        # 告警上报
CMD_WRITE = 0x03                        # 下发控制
CMD_RESET = 0x06                        # 告警解除

# ======================== UI 配置 ========================
WINDOW_TITLE = "智能门禁访客管理系统"
WINDOW_WIDTH = 1280
WINDOW_HEIGHT = 800

"""
config.py — sensor-e 系统配置
E同学：Python应用层负责人
智能门禁访客管理系统 — PyQt5 上位机配置文件
"""

# ======================== 智云平台配置 ========================
USE_REAL_CLOUD = True                   # True=连接智云平台，False=使用模拟数据
CLOUD_HOST = "api.zhiyun360.com"
CLOUD_UID = "736952991135"              # 智云应用 ID
CLOUD_KEY = "AAECDgYAAQsBUFJUAQADWFNTXAwcUwsHVB4CWwYDFAwGVAMYUwpVC1dTDgsJBwAMXA"
CLOUD_WS_URL = f"wss://{CLOUD_HOST}:28090"
CLOUD_TCP_HOST = CLOUD_HOST
CLOUD_TCP_PORT = 28082

# HTTP 轮询配置（备用模拟/HTTP模式）
CLOUD_API_BASE_URL = f"http://{CLOUD_HOST}/api"
CLOUD_API_KEY = CLOUD_KEY
CLOUD_DEVICE_ID = CLOUD_UID
CLOUD_POLL_INTERVAL_MS = 3000           # HTTP 轮询间隔（毫秒）

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
MYSQL_PASSWORD = "123456"
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

# ======================== 智云节点 MAC 地址映射 ========================
NODE_MAC = {
    "sensor-a": "00:12:4B:00:1C:45:BD:01",
    "sensor-b": "00:12:4B:00:1C:45:BB:54",
    "sensor-c": "00:12:4B:00:1C:46:65:DE",
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
FIELD_FLAME = "flame"                   # 火焰检测 0/1 (sensor-c)
FIELD_GAS = "gas"                       # 可燃气体 0/1 (sensor-c)
FIELD_GRATING = "grating"               # 红外光栅 0/1 (sensor-c)
FIELD_TOUCH = "touch"                   # 触摸按键 0/1 (sensor-c Mode2)

# 下行字段（Python -> 智云 -> 硬件）
FIELD_UNLOCK = "unlock"                 # 远程开门 0/1
FIELD_BUZZ = "buzz"                     # 蜂鸣器 0/1/500
FIELD_RGB = "rgb"                       # RGB [R,G,B]
FIELD_RESET = "reset"                   # 告警解除 0/1
FIELD_ARM = "arm"                       # 布防模式 0/1/2 (sensor-c)
FIELD_VOICE = "V1"                      # 语音播报 (sensor-c, hex数据)

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
EVENT_FLAME_DETECT = "flame_detect"     # 火焰检测 (sensor-c)
EVENT_GAS_ALARM = "gas_alarm"           # 可燃气体超标 (sensor-c)
EVENT_GRATING_BREAK = "grating_break"   # 红外光栅遮断 (sensor-c)
EVENT_VOICE_PLAY = "voice_play"         # 语音播报 (sensor-c)
EVENT_ARM_CHANGE = "arm_change"         # 布防模式变更 (sensor-c)

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
WINDOW_HEIGHT = 900

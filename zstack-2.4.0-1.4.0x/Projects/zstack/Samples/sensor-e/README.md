# sensor-e — 智能门禁访客管理系统 PyQt5 上位机

## 项目信息

| 项目 | 说明 |
|------|------|
| 负责人 | E同学 |
| 模块 | Python应用层 |
| 技术栈 | Python 3.7+ / PyQt5 / matplotlib / SQLite |
| 节点工程 | sensor-e（非CC2530硬件节点，为PC端上位机程序） |

## 功能概述

本程序是智能门禁访客管理系统的 **PC 端上位机**，通过智云平台与 ZigBee 硬件节点通信，实现：

1. **门禁状态看板** — 实时显示 PIR 访客状态、门磁状态、门铃状态、告警等级
2. **环境信息面板** — 显示温度、湿度、光照数据（LCD 数字仪表盘）
3. **远程控制区** — 远程开门、门铃触发、告警控制、夜间模式切换
4. **访客事件时间线** — 实时滚动显示门禁事件日志
5. **访客统计图表** — 日/周访客统计条形图、事件类型分布饼图

## 快速启动

```bash
# 1. 安装依赖
pip install -r requirements.txt

# 2. 启动程序
python main.py
```

> 首次启动默认使用 **模拟数据模式**，无需连接硬件即可看到完整界面。

## 项目结构

```
sensor-e/
├── main.py                     # 程序入口
├── config.py                   # 系统配置（API地址、串口参数、字段定义）
├── requirements.txt            # Python 依赖
├── README.md                   # 本文档
├── ui/                         # 界面层
│   ├── main_window.py          # 主窗口（整合所有面板）
│   ├── door_status_panel.py    # 门禁状态看板
│   ├── env_info_panel.py       # 环境信息面板
│   ├── remote_control_panel.py # 远程控制区
│   ├── event_timeline_panel.py # 事件时间线
│   └── stats_panel.py          # 统计图表
├── communication/              # 通信层
│   ├── cloud_api.py            # 智云平台 HTTP API
│   ├── websocket_client.py     # WebSocket 实时通信
│   └── serial_client.py        # 串口直连模式（备用）
├── data/                       # 数据层
│   ├── models.py               # 数据模型
│   └── database.py             # SQLite 本地存储
└── resources/
    └── style.qss               # PyQt5 深色主题样式表
```

## 通信字段

本程序使用的所有数据字段均遵循项目统一规范：

### 上行字段（硬件 → 智云 → Python）

| 字段 | 类型 | 来源 | 说明 |
|------|------|------|------|
| `pir` | int 0/1 | sensor-c | 人体红外 |
| `door` | int 0/1 | sensor-c | 门磁状态 |
| `tch` | int 0/1 | sensor-c | 门铃/触摸 |
| `temp` | float | sensor-a | 温度(℃) |
| `humi` | float | sensor-a | 湿度(%RH) |
| `lux` | int | sensor-a | 光照(lux) |
| `alert` | int 0/1/2 | sensor-c | 告警等级 |
| `stay` | int | sensor-c | 停留时间(秒) |
| `night` | int 0/1 | sensor-c | 夜间模式 |

### 下行字段（Python → 智云 → 硬件）

| 字段 | 类型 | 目标 | 说明 |
|------|------|------|------|
| `unlock` | int 0/1 | sensor-b | 远程开门 |
| `buzz` | int | sensor-b | 蜂鸣器(0关/1长响/500短响) |
| `rgb` | [R,G,B] | sensor-b | RGB灯颜色 |
| `reset` | int 0/1 | sensor-b | 告警解除 |

## 连接智云平台

编辑 `config.py`，修改以下配置：

```python
CLOUD_API_BASE_URL = "http://实际智云平台地址/api"
CLOUD_API_KEY = "你的API Key"
CLOUD_DEVICE_ID = "你的设备ID"
```

然后在 `communication/cloud_api.py` 中取消实际 API 调用部分的注释，注释掉模拟数据部分。

## 数据库

默认使用 **SQLite** 本地存储（`door_access.db`），表结构与课程设计文档一致：

- `access_log` — 访客事件日志（INSERT/SELECT/UPDATE/DELETE）
- `door_env` — 环境数据记录

后续可在 `config.py` 中切换为 MySQL（B同学负责数据库）。

## 答辩要点

E同学答辩时重点介绍：
1. PyQt5 界面设计与各面板功能
2. HTTP API / WebSocket 通信模块如何与智云平台对接
3. 数据可视化方案（matplotlib 图表）
4. 数据库 CRUD 操作实现
5. 远程控制命令的下发流程

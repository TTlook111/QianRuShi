# D部分：通信协议与 ZigBee 任务说明

## 1. 你到底负责什么

你负责的是智能门禁访客管理系统中的通信传输部分。

简单说，你的作用是：

```text
把硬件节点的数据上传到智云平台
把智云/Python 下发的控制命令送回硬件节点
```

也就是连接这三部分：

```text
硬件节点  <->  ZigBee/ZXBee通信  <->  智云平台  <->  Python/PyQt
```

你不是主要写传感器驱动的人，也不是主要写 PyQt 界面的人。你主要负责让这些模块之间能按照统一协议通信。

## 2. 协议是不是自己设计的

不是全部自己设计。

外层协议格式是课程规定的 ZXBee 帧格式，不能随便改：

```text
SOF + DST + SRC + CMD + LEN + PAYLOAD + CHK + EOF
```

字段含义：

| 字段 | 含义 |
| --- | --- |
| `SOF` | 帧头，固定 `0xAA` |
| `DST` | 目的地址 |
| `SRC` | 源地址 |
| `CMD` | 命令字 |
| `LEN` | 数据长度 |
| `PAYLOAD` | 数据内容 |
| `CHK` | 校验和 |
| `EOF` | 帧尾，固定 `0x55` |

校验和规则：

```text
从 SOF 到 PAYLOAD 最后一个字节累加，取低 8 位
```

你们自己设计的是 `PAYLOAD` 里面的字段，比如：

```json
{"pir":1,"door":0,"tch":1}
```

可以理解成：

```text
老师规定信封格式
你们设计信封里写什么内容
```

## 3. 项目通信链路

### 3.1 上行链路

硬件检测到数据变化后，数据上传流程是：

```text
sensor-a / sensor-c
  -> 生成 JSON Payload
  -> ZXBee 协议封包
  -> ZigBee 发送给协调器
  -> 协调器上传智云平台
  -> Python/PyQt 从智云平台读取或实时接收
```

例如有人靠近门口：

```json
{"pir":1,"door":0,"tch":0}
```

### 3.2 下行链路

Python/PyQt 控制硬件时，命令下发流程是：

```text
Python/PyQt 点击按钮
  -> 智云平台下发命令
  -> 协调器接收
  -> ZigBee 发给终端节点
  -> sensor-b 或 sensor-c 解析命令
  -> 执行开门、蜂鸣器、RGB、解除告警
```

例如远程开门：

```json
{"unlock":1,"rgb":[0,255,0]}
```

## 4. 三个工程分别对应什么

当前工程里有三个节点：

```text
zstack-2.4.0-1.4.0x/Projects/zstack/Samples/sensor-a
zstack-2.4.0-1.4.0x/Projects/zstack/Samples/sensor-b
zstack-2.4.0-1.4.0x/Projects/zstack/Samples/sensor-c
```

### 4.1 `sensor-a`：环境采集节点

路径：

```text
zstack-2.4.0-1.4.0x/Projects/zstack/Samples/sensor-a
```

节点名：

```text
601
```

主要作用：

```text
采集温度、湿度、光照
```

项目字段：

| 原字段 | 项目字段 | 含义 |
| --- | --- | --- |
| `A0` | `temp` | 温度 |
| `A1` | `humi` | 湿度 |
| `A2` | `lux` | 光照 |

推荐上报格式：

```json
{"temp":27.0,"humi":65.0,"lux":200}
```

### 4.2 `sensor-b`：控制执行节点

路径：

```text
zstack-2.4.0-1.4.0x/Projects/zstack/Samples/sensor-b
```

节点名：

```text
602
```

主要作用：

```text
控制蜂鸣器、RGB、继电器/模拟门锁
```

项目字段：

| 字段 | 含义 |
| --- | --- |
| `unlock` | 远程开门 |
| `buzz` | 蜂鸣器 |
| `rgb` | RGB灯 |
| `reset` | 解除告警 |

推荐下发格式：

```json
{"unlock":1,"rgb":[0,255,0]}
{"buzz":500,"rgb":[0,0,255]}
{"buzz":1,"rgb":[255,0,0]}
{"reset":1}
```

### 4.3 `sensor-c`：安防检测节点

路径：

```text
zstack-2.4.0-1.4.0x/Projects/zstack/Samples/sensor-c
```

节点名：

```text
603
```

主要作用：

```text
检测人体红外、门磁、门铃/触摸、告警事件
```

项目字段：

| 原字段 | 项目字段 | 含义 |
| --- | --- | --- |
| `A0` | `pir` | 人体红外 |
| `A1` | `tch` | 门铃/触摸/震动 |
| `A2` | `door` | 门磁/门状态 |
| 自定义 | `alert` | 告警等级 |

推荐上报格式：

```json
{"pir":0,"door":0,"tch":0}
{"tch":1,"door":0}
{"pir":1,"stay":15,"alert":1}
{"door":1,"night":1,"alert":2}
```

## 5. 你主要改哪些文件

### 5.1 公共协议文件

这些是你最核心的 D 部分代码：

```text
zstack-2.4.0-1.4.0x/Projects/zstack/Samples/common/zxbee.h
zstack-2.4.0-1.4.0x/Projects/zstack/Samples/common/zxbee.c
zstack-2.4.0-1.4.0x/Projects/zstack/Samples/common/zxbee-inf.c
```

分别负责：

| 文件 | 作用 |
| --- | --- |
| `zxbee.h` | 定义协议常量和函数声明 |
| `zxbee.c` | 封包、解析、校验和 |
| `zxbee-inf.c` | ZigBee 发送和接收接口 |

### 5.2 三个节点的业务文件

你还需要和这三个文件对接字段：

```text
sensor-a/Source/sensor.c
sensor-b/Source/sensor.c
sensor-c/Source/sensor.c
```

分别处理：

| 文件 | 你要关注的函数 |
| --- | --- |
| `sensor-a/Source/sensor.c` | `sensorUpdate()` |
| `sensor-b/Source/sensor.c` | `ZXBeeUserProcess()`、`sensorControl()` |
| `sensor-c/Source/sensor.c` | `sensorUpdate()`、`sensorCheck()` |

## 6. 建议实现顺序

### 第一步：统一字段名

先确定大家都用这些字段：

```text
pir, door, tch, temp, humi, lux, alert, unlock, buzz, rgb, reset
```

不要一会儿用 `A0`，一会儿用 `pir`。最终给智云和 Python 的字段最好用项目文档里的英文缩写。

### 第二步：整理 `zxbee.h`

建议定义：

```c
#define ZXBEE_SOF 0xAA
#define ZXBEE_EOF 0x55

#define CMD_REPORT 0x01
#define CMD_ALARM  0x02
#define CMD_WRITE  0x03
#define CMD_RESET  0x06
```

### 第三步：整理 `zxbee.c`

实现：

```text
ZXBee_CheckSum()
ZXBee_BuildFrame()
ZXBeeDecodePackage()
```

核心目标是生成完整帧：

```text
AA + DST + SRC + CMD + LEN + JSON Payload + CHK + 55
```

### 第四步：整理 `zxbee-inf.c`

确认发送函数最终调用：

```c
zb_SendDataRequest(...)
```

收到数据时进入：

```text
ZXBeeInfRecv()
  -> ZXBeeDecodePackage()
  -> ZXBeeUserProcess()
```

### 第五步：对接三个 `sensor.c`

`sensor-a`：

```json
{"temp":27.0,"humi":65.0,"lux":200}
```

`sensor-c`：

```json
{"pir":1,"door":0,"tch":0}
{"tch":1,"door":0}
{"pir":1,"stay":15,"alert":1}
```

`sensor-b`：

```json
{"unlock":1,"rgb":[0,255,0]}
{"buzz":500,"rgb":[0,0,255]}
{"reset":1}
```

## 7. 典型帧示例

环境上报：

```text
AA 00 00 00 01 01 LEN {"temp":27,"humi":65,"lux":200} CHK 55
```

门铃事件：

```text
AA 00 00 00 03 02 LEN {"tch":1,"door":0} CHK 55
```

远程开门：

```text
AA 00 03 00 00 03 LEN {"unlock":1,"rgb":[0,255,0]} CHK 55
```

## 8. 你答辩时可以这样说

我负责项目的通信协议与 ZigBee 传输层。外层 ZXBee 帧格式按照课程规范实现，包括帧头、目的地址、源地址、命令字、长度、Payload、校验和和帧尾。Payload 字段根据智能门禁访客管理系统统一设计为 `pir`、`door`、`tch`、`temp`、`humi`、`lux`、`alert`、`unlock`、`buzz`、`rgb`、`reset`。我负责把 `sensor-a` 的环境数据、`sensor-c` 的门禁安防事件和 `sensor-b` 的控制执行命令统一到同一套协议中，并通过 ZigBee 实现终端节点、协调器、智云平台和 Python/PyQt 之间的数据传输。

## 9. 最小交付内容

你至少要能拿出：

- `zxbee.c/.h` 协议封包、解析、校验和代码。
- `zxbee-inf.c` ZigBee 发送和接收接口说明。
- `sensor-a` 环境数据上报说明。
- `sensor-c` 门禁/安防事件上报说明。
- `sensor-b` 下行控制命令解析说明。
- 智云平台字段配置截图。
- 至少一组上行数据测试。
- 至少一组下行控制测试。
- 网络拓扑图。

## 10. 最终结论

你的 D 部分就是：

```text
按老师规定的 ZXBee 外层协议
+ 放入你们门禁项目自己设计的 JSON 字段
+ 让 sensor-a / sensor-b / sensor-c 三个节点都能和智云平台通信
+ 保证 Python/PyQt 能显示数据并远程控制硬件
```

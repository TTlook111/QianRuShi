# D 部分通信协议与 ZigBee 完成说明

## 1. 已完成内容

本次完成了智能门禁访客管理系统 D 部分的通信协议与 ZigBee 传输对接，核心目标是：

```text
硬件节点 <-> ZXBee 帧协议 <-> ZigBee 协调器 <-> 智云平台 <-> Python/PyQt
```

外层协议统一为课程要求的 ZXBee 帧格式：

```text
SOF + DST + SRC + CMD + LEN + PAYLOAD + CHK + EOF
```

其中：

```text
SOF = 0xAA
EOF = 0x55
CHK = 从 SOF 到 PAYLOAD 最后一个字节累加后取低 8 位
PAYLOAD = 项目自定义 JSON 字段
```

## 2. 公共协议层

修改文件：

```text
zstack-2.4.0-1.4.0x/Projects/zstack/Samples/common/zxbee.h
zstack-2.4.0-1.4.0x/Projects/zstack/Samples/common/zxbee.c
zstack-2.4.0-1.4.0x/Projects/zstack/Samples/common/zxbee-inf.h
zstack-2.4.0-1.4.0x/Projects/zstack/Samples/common/zxbee-inf.c
```

已实现：

```text
ZXBee_CheckSum()
ZXBee_BuildFrame()
ZXBee_ParseFrame()
ZXBeeDecodePackage()
ZXBeeInfSend()
ZXBeeInfRecv()
ZXBeeInfSetLocalAddr()
```

命令字定义：

```c
#define CMD_REPORT 0x01
#define CMD_ALARM  0x02
#define CMD_WRITE  0x03
#define CMD_RESET  0x06
```

节点地址定义：

```c
#define ZXBEE_ADDR_COORD     0x00
#define ZXBEE_ADDR_SENSOR_A  0x01
#define ZXBEE_ADDR_SENSOR_B  0x02
#define ZXBEE_ADDR_SENSOR_C  0x03
```

## 3. sensor-a 环境采集节点

修改文件：

```text
zstack-2.4.0-1.4.0x/Projects/zstack/Samples/sensor-a/Source/sensor.c
```

节点地址：

```text
SRC = 0x01
节点名 = 601
```

字段映射：

| 原字段 | 项目字段 | 含义 |
| --- | --- | --- |
| A0 | temp | 温度 |
| A1 | humi | 湿度 |
| A2 | lux | 光照 |

典型 Payload：

```json
{"temp":27.0,"humi":65.0,"lux":200}
```

## 4. sensor-c 安防检测节点

修改文件：

```text
zstack-2.4.0-1.4.0x/Projects/zstack/Samples/sensor-c/Source/sensor.c
```

节点地址：

```text
SRC = 0x03
节点名 = 603
```

字段映射：

| 原字段 | 项目字段 | 含义 |
| --- | --- | --- |
| A0 | pir | 人体红外 |
| A1 | tch | 门铃/触摸/震动 |
| A2 | door | 门磁/门状态 |
| A3 | alert | 告警等级 |

典型 Payload：

```json
{"pir":1,"door":0,"tch":0}
{"tch":1,"door":0}
{"pir":1,"alert":1}
```

## 5. sensor-b 控制执行节点

修改文件：

```text
zstack-2.4.0-1.4.0x/Projects/zstack/Samples/sensor-b/Source/sensor.c
```

节点地址：

```text
SRC = 0x02
节点名 = 602
```

已支持下行字段：

| 字段 | 含义 | 处理方式 |
| --- | --- | --- |
| unlock | 远程开门 | 映射继电器 1 控制位 |
| buzz | 蜂鸣器 | 映射蜂鸣器控制位 |
| rgb | RGB 灯 | 解析 `[r,g,b]` 并映射原 RGB 控制位 |
| reset | 解除告警/复位 | 清空控制状态 |

典型下行 Payload：

```json
{"unlock":1,"rgb":[0,255,0]}
{"buzz":500,"rgb":[0,0,255]}
{"reset":1}
```

## 6. 测试帧示例

环境数据上行：

```text
AA 00 01 01 23 7B 22 74 65 6D 70 22 3A 32 37 2E 30 2C 22 68 75 6D 69 22 3A 36 35 2E 30 2C 22 6C 75 78 22 3A 32 30 30 7D 7D 55
```

对应 Payload：

```json
{"temp":27.0,"humi":65.0,"lux":200}
```

远程开门下行：

```text
AA 02 00 03 1C 7B 22 75 6E 6C 6F 63 6B 22 3A 31 2C 22 72 67 62 22 3A 5B 30 2C 32 35 35 2C 30 5D 7D EF 55
```

对应 Payload：

```json
{"unlock":1,"rgb":[0,255,0]}
```

## 7. 网络拓扑

```text
sensor-a(601, 环境采集, SRC=0x01)
        \
         \
sensor-b(602, 控制执行, SRC=0x02)  <->  Coordinator(SRC=0x00)  <->  智云平台  <->  Python/PyQt
         /
        /
sensor-c(603, 安防检测, SRC=0x03)
```

## 8. 答辩说明

我负责项目的通信协议与 ZigBee 传输层。外层采用课程规定的 ZXBee 帧格式，包含帧头、目的地址、源地址、命令字、长度、JSON Payload、校验和和帧尾。Payload 字段按智能门禁项目统一为 `temp`、`humi`、`lux`、`pir`、`door`、`tch`、`alert`、`unlock`、`buzz`、`rgb`、`reset`。三个节点通过统一协议完成上行数据采集和下行控制命令解析，保证硬件节点、协调器、智云平台和 Python/PyQt 之间能够按同一套字段通信。

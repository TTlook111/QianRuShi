#ifndef __ZXBEE_H__
#define __ZXBEE_H__

#define ZXBEE_SOF               0xAA // 帧头 Start of Frame
#define ZXBEE_EOF               0x55 // 帧尾 End of Frame

#define ZXBEE_ADDR_COORD        0x0000 // 协调器
#define ZXBEE_ADDR_SENSOR_A     0x0001 // 环境采集
#define ZXBEE_ADDR_SENSOR_B     0x0002 // 控制执行
#define ZXBEE_ADDR_SENSOR_C     0x0003 // 安防检测
#define ZXBEE_ADDR_BROADCAST    0xFFFF

#define CMD_REPORT              0x01 // 数据上报
#define CMD_ALARM               0x02 // 告警上报
#define CMD_WRITE               0x03 // 控制写入
#define CMD_RESET               0x06 // 复位命令

#define ZXBEE_MAX_PAYLOAD_LEN   96
#define ZXBEE_MAX_FRAME_LEN     (ZXBEE_MAX_PAYLOAD_LEN + 9)

typedef struct {
  uint16 dst; // 目的地址
  uint16 src; // 源地址
  uint8 cmd; // 命令字
  uint8 len; // 负载长度
  char *payload; // 负载指针
} ZXBeeFrame;

int8 ZXBeeBegin(void);
int8 ZXBeeAdd(char* tag, char* val);
char* ZXBeeEnd(void);
char* ZXBeeDecodePackage(char *pkg, int len);
uint8 ZXBee_CheckSum(uint8 *buf, int len);
int ZXBee_BuildFrame(uint16 dst, uint16 src, uint8 cmd, char *payload, uint8 *out);
int ZXBee_ParseFrame(char *pkg, int len, ZXBeeFrame *frame);

#endif

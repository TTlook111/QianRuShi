#ifndef __ZXBEE_H__
#define __ZXBEE_H__

#define ZXBEE_SOF               0xAA
#define ZXBEE_EOF               0x55

#define ZXBEE_ADDR_COORD        0x00
#define ZXBEE_ADDR_SENSOR_A     0x01
#define ZXBEE_ADDR_SENSOR_B     0x02
#define ZXBEE_ADDR_SENSOR_C     0x03
#define ZXBEE_ADDR_BROADCAST    0xFF

#define CMD_REPORT              0x01
#define CMD_ALARM               0x02
#define CMD_WRITE               0x03
#define CMD_RESET               0x06

#define ZXBEE_MAX_PAYLOAD_LEN   109
#define ZXBEE_MAX_FRAME_LEN     (ZXBEE_MAX_PAYLOAD_LEN + 7)

typedef struct {
  uint8 dst;
  uint8 src;
  uint8 cmd;
  uint8 len;
  char *payload;
} ZXBeeFrame;

int8 ZXBeeBegin(void);
int8 ZXBeeAdd(char* tag, char* val);
char* ZXBeeEnd(void);
char* ZXBeeDecodePackage(char *pkg, int len);
uint8 ZXBee_CheckSum(uint8 *buf, int len);
int ZXBee_BuildFrame(uint8 dst, uint8 src, uint8 cmd, char *payload, uint8 *out);
int ZXBee_ParseFrame(char *pkg, int len, ZXBeeFrame *frame);

#endif

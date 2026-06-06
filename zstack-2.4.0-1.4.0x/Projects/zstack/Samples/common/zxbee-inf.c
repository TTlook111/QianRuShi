#include <string.h>
#include <stdlib.h>
#include "hal_types.h"
#include "AppCommon.h"
#include "hal_led.h"
#include "ZDApp.h"
#include "zxbee.h"
#include "sapi.h"
#include "stdio.h"
#include "zxbee-inf.h"

#define DEBUG 0
#if DEBUG
#define Debug printf
#else
#define Debug(...)
#endif

static uint8 localAddr = ZXBEE_ADDR_COORD;

void ZXBeeInfSetLocalAddr(uint8 addr)
{
  localAddr = addr;
}

static uint8 ZXBeeLocalAddr(void)
{
  return localAddr;
}

static uint8 ZXBeeCmdFromPayload(char *p)
{
  if (p == NULL) return CMD_REPORT;
  if (strstr(p, "\"reset\"") != NULL) return CMD_RESET;
  if (strstr(p, "\"unlock\"") != NULL || strstr(p, "\"buzz\"") != NULL || strstr(p, "\"rgb\"") != NULL) return CMD_WRITE;
  if (strstr(p, "\"alert\"") != NULL) return CMD_ALARM;
  return CMD_REPORT;
}

void ZXBeeInfInit(void)
{
}

void ZXBeeSendConfirm(uint8 h, uint8 st)
{
  uint8 GetLinkStatus(void);
  static int8 txError = 0;
  if (h == 0xaa) {
    if (GetLinkStatus()) {
      if (st == 0) {
        txError = 0;
      } else {
        txError += 1;
      }
      if (txError >= 5) {
        void myReset(void);
        myReset();
      }
    }
  }
}

void ZXBeeInfSend(char *p, int len)
{
  uint8 frame[ZXBEE_MAX_FRAME_LEN];
  uint8 *sendBuf;
  int sendLen;
  int i;

  if (p == NULL || len <= 0) return;

  HalLedSet(HAL_LED_1, HAL_LED_MODE_OFF);
  HalLedSet(HAL_LED_1, HAL_LED_MODE_BLINK);

  if ((uint8)p[0] == ZXBEE_SOF && (uint8)p[len - 1] == ZXBEE_EOF) {
    sendBuf = (uint8*)p;
    sendLen = len;
  } else {
    sendLen = ZXBee_BuildFrame(ZXBEE_ADDR_COORD, ZXBeeLocalAddr(), ZXBeeCmdFromPayload(p), p, frame);
    sendBuf = frame;
  }

  if (sendLen <= 0) return;

#if DEBUG
  Debug("Debug send:");
  for (i = 0; i < sendLen; i++) {
    Debug("%02X ", sendBuf[i]);
  }
  Debug("\r\n");
#endif
  zb_SendDataRequest(0, 0, sendLen, sendBuf, 0xaa, AF_ACK_REQUEST, AF_DEFAULT_RADIUS);
}

void ZXBeeInfRecv(char *pkg, int len)
{
  uint8 frame[ZXBEE_MAX_FRAME_LEN];
  char *p = ZXBeeDecodePackage(pkg, len);
  int frameLen;

  if (p != NULL) {
    frameLen = ZXBee_BuildFrame(ZXBEE_ADDR_COORD, ZXBeeLocalAddr(), CMD_REPORT, p, frame);
    if (frameLen > 0) {
      ZXBeeInfSend((char*)frame, frameLen);
    }
  }
}


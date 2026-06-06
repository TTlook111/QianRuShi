#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hal_types.h"
#include "zxbee.h"

static char wbuf[ZXBEE_MAX_PAYLOAD_LEN];
static char payloadBuf[ZXBEE_MAX_PAYLOAD_LEN];

int ZXBeeSysCommandProc(char *ptag, char *pval);
int ZXBeeUserProcess(char *ptag, char *pval);

static uint8 valueNeedsQuote(char *val)
{
  char c;
  if (val == NULL || val[0] == 0) return 1;
  c = val[0];
  if ((c >= '0' && c <= '9') || c == '-' || c == '[' || c == '{' ) return 0;
  return 1;
}

static void trimValue(char *s)
{
  char *p;
  int len;
  while (*s == ' ' || *s == '\t' || *s == '"') {
    memmove(s, s + 1, strlen(s));
  }
  len = strlen(s);
  while (len > 0) {
    p = s + len - 1;
    if (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n' || *p == '"') {
      *p = 0;
      len--;
    } else {
      break;
    }
  }
}

int8 ZXBeeBegin(void)
{
  wbuf[0] = '{';
  wbuf[1] = '\0';
  return 1;
}

int8 ZXBeeAdd(char* tag, char* val)
{
  int offset;
  int left;
  if (tag == NULL || val == NULL) return 0;
  offset = strlen(wbuf);
  left = ZXBEE_MAX_PAYLOAD_LEN - offset;
  if (left <= 2) return 0;
  if (valueNeedsQuote(val)) {
    sprintf(&wbuf[offset], "\"%s\":\"%s\",", tag, val);
  } else {
    sprintf(&wbuf[offset], "\"%s\":%s,", tag, val);
  }
  return strlen(wbuf);
}

char* ZXBeeEnd(void)
{
  int offset = strlen(wbuf);
  if (offset <= 1) return NULL;
  wbuf[offset - 1] = '}';
  wbuf[offset] = '\0';
  if (offset > 2) return wbuf;
  return NULL;
}

uint8 ZXBee_CheckSum(uint8 *buf, int len)
{
  uint16 sum = 0;
  int i;
  for (i = 0; i < len; i++) {
    sum += buf[i];
  }
  return (uint8)(sum & 0xFF);
}

int ZXBee_BuildFrame(uint8 dst, uint8 src, uint8 cmd, char *payload, uint8 *out)
{
  uint8 plen;
  if (payload == NULL || out == NULL) return 0;
  plen = (uint8)strlen(payload);
  if (plen >= ZXBEE_MAX_PAYLOAD_LEN) return 0;

  out[0] = ZXBEE_SOF;
  out[1] = dst;
  out[2] = src;
  out[3] = cmd;
  out[4] = plen;
  memcpy(out + 5, payload, plen);
  out[5 + plen] = ZXBee_CheckSum(out, 5 + plen);
  out[6 + plen] = ZXBEE_EOF;
  return plen + 7;
}

int ZXBee_ParseFrame(char *pkg, int len, ZXBeeFrame *frame)
{
  uint8 plen;
  if (pkg == NULL || frame == NULL || len < 7) return 0;
  if ((uint8)pkg[0] != ZXBEE_SOF || (uint8)pkg[len - 1] != ZXBEE_EOF) return 0;
  plen = (uint8)pkg[4];
  if (len != (int)plen + 7) return 0;
  if (ZXBee_CheckSum((uint8*)pkg, 5 + plen) != (uint8)pkg[5 + plen]) return 0;

  if (plen >= ZXBEE_MAX_PAYLOAD_LEN) return 0;
  memcpy(payloadBuf, pkg + 5, plen);
  payloadBuf[plen] = 0;

  frame->dst = (uint8)pkg[1];
  frame->src = (uint8)pkg[2];
  frame->cmd = (uint8)pkg[3];
  frame->len = plen;
  frame->payload = payloadBuf;
  return 1;
}

static int ZXBeeDecodeJsonPayload(char *payload)
{
  char *p;
  char *key;
  char *val;
  int handled = 0;

  if (payload == NULL) return 0;
  if (payload[0] != '{') return 0;
  p = payload + 1;

  while (*p != 0 && *p != '}') {
    int bracket = 0;
    char *end;
    while (*p == ' ' || *p == ',' || *p == '\t' || *p == '\r' || *p == '\n') p++;
    if (*p == '"') p++;
    key = p;
    while (*p != 0 && *p != '"' && *p != ':' && *p != '=') p++;
    if (*p == '"') *p++ = 0;
    while (*p == ' ' || *p == ':') p++;
    val = p;
    end = p;
    while (*end != 0) {
      if (*end == '[' || *end == '{') bracket++;
      else if (*end == ']' || *end == '}') {
        if (bracket > 0) bracket--;
        else break;
      } else if (*end == ',' && bracket == 0) {
        break;
      }
      end++;
    }
    if (*end == ',') {
      *end = 0;
      p = end + 1;
    } else {
      *end = 0;
      p = end;
    }
    trimValue(key);
    trimValue(val);
    if (key[0] != 0) {
      int ret = ZXBeeSysCommandProc(key, val);
      if (ret < 0) {
#ifndef CC2530_Serial
        ret = ZXBeeUserProcess(key, val);
#endif
      }
      if (ret >= 0) handled++;
    }
  }
  return handled;
}

static int ZXBeeDecodeLegacyPayload(char *pkg, int len)
{
  char *p;
  char *ptag = NULL;
  char *pval = NULL;
  int handled = 0;

  if (pkg[0] != '{' || pkg[len - 1] != '}') return 0;
  pkg[len - 1] = 0;
  p = pkg + 1;
  do {
    ptag = p;
    p = strchr(p, '=');
    if (p != NULL) {
      int ret;
      *p++ = 0;
      pval = p;
      p = strchr(p, ',');
      if (p != NULL) *p++ = 0;
      ret = ZXBeeSysCommandProc(ptag, pval);
      if (ret < 0) {
#ifndef CC2530_Serial
        ret = ZXBeeUserProcess(ptag, pval);
#endif
      }
      if (ret >= 0) handled++;
    }
  } while (p != NULL);
  return handled;
}

char* ZXBeeDecodePackage(char *pkg, int len)
{
  ZXBeeFrame frame;
  int handled;

  if (pkg == NULL || len <= 0) return NULL;
  ZXBeeBegin();

  if (ZXBee_ParseFrame(pkg, len, &frame)) {
    handled = ZXBeeDecodeJsonPayload(frame.payload);
  } else {
    handled = ZXBeeDecodeLegacyPayload(pkg, len);
  }

  if (handled <= 0) return NULL;
  return ZXBeeEnd();
}


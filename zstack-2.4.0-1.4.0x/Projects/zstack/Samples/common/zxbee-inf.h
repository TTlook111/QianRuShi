#ifndef __ZXBEE_INF_H__
#define __ZXBEE_INF_H__

#include "hal_types.h"

void ZXBeeInfInit(void);
void ZXBeeInfSetLocalAddr(uint16 addr);
void ZXBeeInfSend(char *p, int len);
void ZXBeeInfRecv(char *pkg, int len);
#endif

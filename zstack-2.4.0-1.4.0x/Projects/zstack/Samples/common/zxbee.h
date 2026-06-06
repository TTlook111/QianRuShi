#ifndef __ZXBEE_H__
#define __ZXBEE_H__

int8 ZXBeeBegin(void);
int8 ZXBeeAdd(char* tag, char* val);
char* ZXBeeEnd(void);
char* ZXBeeDecodePackage(char *pkg, int len);

#endif 
#ifndef __APPCOMMON_H__
#define __APPCOMMON_H__

int ZXBeeUserProcess(char *ptag, char *pval);

extern void Uart_SendData(uint8 *buf);

uint8 _get_at_event(void);

uint8 GetLinkStatus(void);
uint16 GetPanId(void);
void SetPanId(uint16 id);
uint8 GetChannel(void);
void SetChannel(uint8 val);
uint8 GetLogicalType(void);
void SetLogicalType(uint8 t);
uint8 GetCurrentLogicalType(void);
#endif
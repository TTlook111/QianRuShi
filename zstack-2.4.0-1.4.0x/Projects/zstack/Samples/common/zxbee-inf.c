/*********************************************************************************************
* 文件：zxbee-inf.c
* 作者：Xuzhy 2018.5.16
* 说明：ZXBee通信协议数据包收发
* 修改：
* 注释：
*********************************************************************************************/

/*********************************************************************************************
* 头文件
*********************************************************************************************/
#include<string.h>
#include "hal_types.h"
#include "AppCommon.h"
#include "hal_led.h"
#include "ZDApp.h"
#include "zxbee.h"
#include "sapi.h"
#include "stdio.h"
#include "zxbee-inf.h"
/*********************************************************************************************
* 宏定义
*********************************************************************************************/
#define DEBUG 0
#if DEBUG
#define Debug   printf
#else
#define Debug(...)
#endif
/*********************************************************************************************
* 名称：ZXBeeInfInit()
* 功能：ZXBee接口初始化
* 参数：
* 返回：
* 修改：
* 注释：
*********************************************************************************************/
void ZXBeeInfInit(void)
{

}

void ZXBeeSendConfirm(uint8 h, uint8 st)
{
  uint8 GetLinkStatus(void);
  static int8 txError = 0;
  if (h == 0xaa){
    if (GetLinkStatus()) {
      if (st == 0) {
        txError = 0;
      } else  txError += 1;
      
      if (txError >= 5) {
        void myReset(void);
        //zb_SystemReset();
        myReset();
      }
    }
  }
}
/*********************************************************************************************
* 名称：ZXBeeInfSend()
* 功能：节点发送无线数据包给汇集节点
* 参数：*p -- 要发送的无线数据包
* 返回：
* 修改：
* 注释：
*********************************************************************************************/
void ZXBeeInfSend(char *p, int len)
{
    HalLedSet( HAL_LED_1, HAL_LED_MODE_OFF );
    HalLedSet( HAL_LED_1, HAL_LED_MODE_BLINK );
#if DEBUG
    Debug("Debug send:");
    for (int i=0; i<len; i++) {
      Debug("%c", p[i]);
    }
    Debug("\r\n");
#endif
    zb_SendDataRequest( 0, 0, len, (uint8*)p, 0xaa, AF_ACK_REQUEST, AF_DEFAULT_RADIUS );
}
/*********************************************************************************************
* 名称：ZXBeeInfRecv()
* 功能：节点收到无线数据包
* 参数：*pkg -- 收到的无线数据包
* 返回：
* 修改：
* 注释：
*********************************************************************************************/
void ZXBeeInfRecv(char *pkg, int len)
{
  char *p = ZXBeeDecodePackage(pkg, len);
  if (p != NULL) {
    ZXBeeInfSend(p, strlen(p));
  }
}

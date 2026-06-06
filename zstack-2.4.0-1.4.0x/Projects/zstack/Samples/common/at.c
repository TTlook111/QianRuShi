/*********************************************************************************************
* 文件：at.c
* 作者：Xuzhy 2018.5.16
* 说明：节点串口at指令
* 修改：fuyou 增加透传驱动
* 注释：
*********************************************************************************************/

/*********************************************************************************************
* 头文件
*********************************************************************************************/
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "ZComDef.h"
#include "sapi.h"
#include "hal_uart.h"
#include "hal_led.h"
#include "at.h"
#include "AppCommon.h"
#include "at-uart.h"
#include "hal_flash.h"
#include "iap_config.h"
#include "hal_types.h"
#include "hal_mcu.h"

/*********************************************************************************************
* 宏定义
*********************************************************************************************/
#define AT_BUFF_SIZE    96                                      //AT缓存长度
#define AT_BUFF_NUM     2                                       //AT缓存个数
/*********************************************************************************************
* 全局变量
*********************************************************************************************/
static uint8 __AT_EVT;                                          //AT接口事件
static char at_echo = 0;
static int at_datalen = 0;                                     // 指示用于接收发送的数据长度
static int at_recvdata = 0;
static char atbuff[AT_BUFF_NUM][AT_BUFF_SIZE];                  //AT指令接收缓存
static char bufferbit = 0;
static char *pAtCommand = NULL;                                 // 存放接收到的at指令或数据
sys_parameter_t sys_parameter;
/*********************************************************************************************
* 函数原型说明
*********************************************************************************************/
static char* at_quebuffer_get(void);
static void at_quebuffer_put(char *buf);
static int _at_get_ch(char ch); 
uint8 _get_at_event(void);                                      // at 事件标识
void at_recv_ch(char ch);
int8 getLastPkgRssi(void);
/*********************************************************************************************
* 名称：at_quebuffer_put()
* 功能：at缓存释放
* 参数：
* 返回：
* 修改：
* 注释：
*********************************************************************************************/
void at_quebuffer_put(char *buf)
{
  for (int i=0; i<AT_BUFF_NUM; i++) {
    if (atbuff[i] == buf){
      bufferbit &= ~(1<<i);
      return;
    }
  }
}
/*********************************************************************************************
* 名称：at_quebuffer_get()
* 功能：缓存获取
* 参数：
* 返回：
* 修改：
* 注释：
*********************************************************************************************/
char* at_quebuffer_get(void)
{
  for (int i=0; i<AT_BUFF_NUM; i++) {
    if (((bufferbit>>i) & 1) == 0) {
      bufferbit |= 1<<i;
        return atbuff[i];
    }
  }
  return NULL;
}
/*********************************************************************************************
* 名称：at_put_ch()
* 功能：通过at端口发送一个字符
* 参数：ch: 待发送的字符
* 返回：无
* 修改：
* 注释：
*********************************************************************************************/
static void _at_put_ch(uint8 ch)
{
  at_uart_write(ch);
}
/*********************************************************************************************
* 名称：_at_get_ch()
* 功能：处理at接口接收到的数据
* 参数：
* 返回：
* 修改：
* 注释：
*********************************************************************************************/
static int _at_get_ch(char ch)
{
  static char idx = 0;
  static char *pbuf = NULL;
  if (pbuf == NULL) {
    pbuf = at_quebuffer_get();
    if (pbuf == NULL) { 
      return -1;
    }
  }
  if (at_datalen == 0) {
    if (at_echo) {
      _at_put_ch(ch);
    }
    if (idx < AT_BUFF_SIZE-1) {
      pbuf[idx++] = ch;
      if (idx >= 2 && pbuf[idx-2]=='\r' && pbuf[idx-1]=='\n'){
        idx -= 2;
        pbuf[idx] = '\0';
        if (idx > 0) {
          /* at指令接收完成 */
          if (pAtCommand == NULL) {
            pAtCommand = pbuf;
            osal_set_event(sapi_TaskID, __AT_EVT);
          } else {
            at_quebuffer_put(pbuf); //丢弃数据
          }
           
          pbuf = NULL;
          idx = 0;
        } else {
           //丢弃数据
          idx = 0;
        }
      }
    } else {
      /*缓存溢出丢弃所有数据*/
      idx = 0;
    }
  } else {
    pbuf[at_recvdata++] = ch;
    if (at_recvdata == at_datalen) {
      /*数据接收完成*/
      if (pAtCommand == NULL) {
        pAtCommand = pbuf;
        osal_set_event(sapi_TaskID, __AT_EVT);
      }else {
         at_quebuffer_put(pbuf); //丢弃数据
         at_recvdata = 0;
      }
      at_datalen = 0;
      pbuf = NULL;
    }
  }
  return idx;
}
/*********************************************************************************************
* 名称：at_response_buf()
* 功能：通过at接口发送一段数据
* 参数：s:数据开始地址
*       len：数据长度
* 返回：
* 修改：
* 注释：
*********************************************************************************************/
void at_response_buf(char *s, int len)
{
  for (int i=0; i<len; i++) {
    _at_put_ch(s[i]);
  }
}
/*********************************************************************************************
* 名称：at_response()
* 功能：通过at接口发送字符串
* 参数：s:待发送的字符串
* 返回：
* 修改：
* 注释：
*********************************************************************************************/
void at_response(char *s)
{
  at_response_buf(s, strlen(s));
}
/*********************************************************************************************
* 名称：at_notify_data()
* 功能：at接口接收到数据通知
* 参数：buf:接收到的数据
*       len:接收到数据长度
* 返回：
* 修改：
* 注释：
*********************************************************************************************/
void at_notify_data(char *buf, int len)
{
  char sc[16];
#ifdef CC2530_Serial
  sprintf(sc, "+RECV:%u,%d\r\n", len, getLastPkgRssi());
#else
  sprintf(sc, "+RECV:%u\r\n", len);
#endif
  at_response(sc);
  at_response_buf(buf, len);
}
/*********************************************************************************************
* 名称：at_proc()
* 功能：处理at指令
* 参数：
* 返回：
* 修改：
* 注释：
*********************************************************************************************/
void at_proc(void)
{
  char buf[32];
  if (pAtCommand == NULL) 
    return;
  char *p_msg = pAtCommand;
  
  if (at_recvdata != 0) {
    /*处理发送数据*/
    at_response(ATOK);
   
    zb_SendDataRequest( 0, 0/*cmd*/, at_recvdata, (uint8*)p_msg, 0xaa, AF_ACK_REQUEST, AF_DEFAULT_RADIUS );
    HalLedSet( HAL_LED_1, HAL_LED_MODE_OFF );
    HalLedSet( HAL_LED_1, HAL_LED_MODE_BLINK );
    
    sprintf(buf, "+SEND:%u\r\n", at_recvdata);
    at_response(buf);
    at_recvdata = 0;
  } else {
    int msg_size = strlen(p_msg);
    for (int i=0; i<msg_size; i++){
      if (p_msg[i] == '?' || p_msg[i] == '=') break;
      p_msg[i] = toupper(p_msg[i]);
    }
    
    if (strcmp(p_msg, "AT") == 0) {
      at_response(ATOK);
    }
    else if (strcmp(p_msg, "ATE1") == 0) {
      at_echo = 1;
      at_response(ATOK);
    }
    else if (strcmp(p_msg, "ATE0") == 0) {
      at_echo = 0;
      at_response(ATOK);
    }  
    else if (memcmp(p_msg, "AT+HW?", 7) == 0) {
      at_response("+HW:CC2530\r\n");
      at_response(ATOK);
    }
    else if (memcmp(p_msg, "AT+MAC?", 7) == 0) {
      uint8 *mac = (uint8 *)(P_INFOPAGE+HAL_INFOP_IEEE_OSET);
      sprintf(buf, "+MAC:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X\r\n",
              mac[7],mac[6],mac[5],mac[4],mac[3],mac[2],mac[1],mac[0]); 
      at_response(buf);
      at_response(ATOK);
    } 
    else if (memcmp(p_msg, "AT+PANID", 8) == 0) {
      if (p_msg[8] == '?') {
        sprintf(buf, "+PANID:%u\r\n", GetPanId());
        at_response(buf);
        at_response(ATOK);
      } else if (p_msg[8] == '=') {
        uint16 id = atoi(&p_msg[9]);
        SetPanId(id);
        at_response(ATOK);
      } else {
        at_response(ATERROR);
      }
    }
    else if (memcmp(p_msg, "AT+CHANNEL", 10) == 0) {
      if (p_msg[10] == '?') {
        sprintf(buf, "+CHANNEL:%u\r\n", GetChannel());
        at_response(buf);
        at_response(ATOK);
      } else if (p_msg[10] == '=') {
        uint16 id = atoi(&p_msg[11]);
        SetChannel(id);
        at_response(ATOK);
      } else {
        at_response(ATERROR);
      }
    }
    else if (memcmp(p_msg, "AT+LOGICALTYPE", 14) == 0) {
      if (p_msg[14] == '?') {
        sprintf(buf, "+LOGICALTYPE:%u\r\n", GetLogicalType());
        at_response(buf);
        at_response(ATOK);
      } else if (p_msg[14] == '=') {
        uint16 id = atoi(&p_msg[15]);
        SetLogicalType(id);
        at_response(ATOK);
      } else {
        at_response(ATERROR);
      }
    }
    else if (memcmp(p_msg, "AT+LINK?", 8) == 0) {
      sprintf(buf, "+LINK:%u\r\n", GetLinkStatus());
      at_response(buf);
      at_response(ATOK);
    }
    /*else if (memcmp(p_msg, "AT+ENVSAVE", 10) == 0) {
      //ucfg_save();
      at_response(ATOK);
    } */
    else if (memcmp(p_msg, "AT+RSSI?", 8) == 0) {
      
      sprintf(buf, "+RSSI:%d\r\n", getLastPkgRssi());
      at_response(buf);
      at_response(ATOK);
    }
    else if (memcmp(p_msg, "AT+LQI?", 7) == 0) {
      uint8 getLastPkgLQI(void);
      sprintf(buf, "+LQI:%d\r\n", getLastPkgLQI());
      at_response(buf);
      at_response(ATOK);
    }
    
    else if (memcmp(p_msg, "AT+RESET", 8)==0) {
       at_response(ATOK);
       zb_SystemReset();
    }
    else if (memcmp(p_msg, "AT+SEND=", 8) == 0) {
      int len = atoi(&p_msg[8]);
      if (len > 0 && len<=AT_BUFF_SIZE) {
        at_response(">");
        at_datalen = len;
      } else {
        at_response(ATERROR);
      }
    }
#ifdef CC2530_USE_IAP 
    else if (memcmp(p_msg, "AT+IAP=", 7) == 0) {
      char* p=p_msg+7;
      if(strstr(p,"IAP")){
        SYS_PARAMETER_READ;
        sys_parameter.current_part=0xff;
        HalFlashErase(PARA_PARTITION_PAGE);
        SYS_PARAMETER_WRITE;
        at_response(ATOK);
        HAL_SYSTEM_RESET();
      }else if(strstr(p,"APP")){
        SYS_PARAMETER_READ;
        sys_parameter.current_part=0x01;
        HalFlashErase(PARA_PARTITION_PAGE);
        SYS_PARAMETER_WRITE;
        at_response(ATOK);
      }
      else {
        at_response(ATERROR);
      }
    }
#endif  
    else {
      if (user_at_proc(p_msg)<0) {
        at_response(ATERROR);
      }
    }
    
  }
  at_quebuffer_put(p_msg);
  pAtCommand = NULL;
}

/* 主动上报link status */
void AT_reportedLinkStatus()
{
    char buf[16]={0};
    sprintf(buf, "+LINK:%u\r\n", GetLinkStatus());
    at_response(buf);
}


/*********************************************************************************************
* 名称：at_init()
* 功能：AT指令初始化
* 参数：
* 返回：
* 修改：
* 注释：
*********************************************************************************************/
void at_init(void)
{
    __AT_EVT = _get_at_event();

    at_uart_init();
    at_uart_set_input_call(at_recv_ch);

    at_response("+HW:CC2530\r\n");
    at_response("+RDY\r\n");
}
/*********************************************************************************************
* 名称：at_recv_ch()
* 功能：at接收到字符处理
* 参数：
* 返回：
* 修改：
* 注释：
*********************************************************************************************/
void at_recv_ch(char ch)
{
  _at_get_ch(ch);
}

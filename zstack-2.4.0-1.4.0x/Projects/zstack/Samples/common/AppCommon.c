/*********************************************************************************************
* 文件： AppCommon.c
* 作者： Xuzhy 2018.5.16
* 说明：基于Sapi架构的应用层文件，参考：SimpleSwitch.c
* 修改：fuyou 增加透传驱动部分
* 注释：
*********************************************************************************************/

/*********************************************************************************************
* 头文件
*********************************************************************************************/
#include "ZComDef.h"
#include "OSAL.h"
#include "sapi.h"
#include "hal_key.h"
#include "hal_led.h"
#include "hal_adc.h"
#include "SimpleApp.h"
#include "mt.h"
#include "ZDApp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sensor.h"
#include "osal_nv.h"
#include "AddrMgr.h"
#include "rtg.h"
#include "nwk_util.h"
#include "AppCommon.h"
#include "zxbee.h"
#include "at.h"
#include "zxbee-inf.h"
#include "OnBoard.h"
/*********************************************************************************************
* 宏定义
*********************************************************************************************/
// Application States
#define APP_INIT                           0                    // Initial state
#define APP_START                          1                    // Device has started/joined network

// Application osal event identifiers
#define __START_EVT                        0x0010
#define __REPORT_EVT                       0x0020
#define __AT_EVT                           0x0040

#define REPORT_DELAY                      30

#define NUM_IN_CMD_SWITCH                 2
#define NUM_OUT_CMD_SWITCH                2
/*********************************************************************************************
* 全局变量
*********************************************************************************************/
static uint8 mLinkStatus = 0;                                   // zigbee入网状态，0：未入网，1：人网
static uint16 panid;
static uint8 logicalType;

static uint32 chs[] = {0x00000800, 0x00001000, 0x00002000, 0x00004000, 0x00008000,
        0x00010000, 0x00020000, 0x00040000,0x00080000,0x00100000,0x00200000,
        0x00400000,0x00800000,0x01000000,0x02000000,0x04000000}; 

const cId_t zb_InCmdList[NUM_IN_CMD_SWITCH] =
{
  ID_CMD_READ_REQ,
  ID_CMD_WRITE_REQ,
};
const cId_t zb_OutCmdList[NUM_OUT_CMD_SWITCH] =
{
  ID_CMD_READ_RES,
  ID_CMD_WRITE_RES,
};
// Define SimpleDescriptor for Switch device
const SimpleDescriptionFormat_t zb_SimpleDesc =
{
  MY_ENDPOINT_ID,             //  Endpoint
  MY_PROFILE_ID,              //  Profile ID
  DEV_ID_SENSOR,              //  Device ID
  DEVICE_VERSION_SWITCH,      //  Device Version
  0,                          //  Reserved
  NUM_IN_CMD_SWITCH,          //  Number of Input Commands
  (cId_t *) zb_InCmdList,     //  Input Command List
  NUM_OUT_CMD_SWITCH,         //  Number of Output Commands
  (cId_t *) zb_OutCmdList     //  Output Command List
};

void myReset(void)
{
  HAL_DISABLE_INTERRUPTS();
  WatchDogEnable( WDTISH );
  while(1);
}

/*********************************************************************************************
* 函数原型说明
*********************************************************************************************/
void zb_HanderMsg(osal_event_hdr_t *msg);
void sensorLinkOn(void);
static void my_report_proc(void);
static char* read_nb(char *buf, int len);
static uint16 _tm_report_delay, _tm_report_cnt;
void starReportTPN(uint16 t, uint16 n);
/*********************************************************************************************
* 名称：_get_at_event()
* 功能：导出osal at事件标识
* 参数：
* 返回：
* 修改：
* 注释：
*********************************************************************************************/
uint8 _get_at_event(void)
{
  return __AT_EVT;
}
/*********************************************************************************************
* 名称：zb_HandleOsalEvent()
* 功能：sapi事件处理函数，当一个任务事件发生了之后，调用这个函数
* 参数：event - 产生的任务事件
* 返回：
* 修改：
* 注释：
*********************************************************************************************/
void zb_HandleOsalEvent( uint16 event )
{
  if (event & ZB_ENTRY_EVENT) {
    uint8 startOptions;
    uint8 selType = NODE_TYPE;
    
    at_init();

    zb_ReadConfiguration( ZCD_NV_LOGICAL_TYPE, sizeof(uint8), &logicalType );
    if ( logicalType !=ZG_DEVICETYPE_ENDDEVICE && logicalType !=ZG_DEVICETYPE_ROUTER ) {
      zb_WriteConfiguration(ZCD_NV_LOGICAL_TYPE, sizeof(uint8), &selType);
      zb_SystemReset();
    }
	
    // Do more configuration if necessary and then restart device with auto-start bit set
    // write endpoint to simple desc...dont pass it in start req..then reset
    zb_ReadConfiguration( ZCD_NV_STARTUP_OPTION, sizeof(uint8), &startOptions );
    if (startOptions != ZCD_STARTOPT_AUTO_START) {
      startOptions = ZCD_STARTOPT_AUTO_START;
      zb_WriteConfiguration( ZCD_NV_STARTUP_OPTION, sizeof(uint8), &startOptions );
      zb_SystemReset();
    } 
    osal_nv_read( ZCD_NV_PANID, 0, sizeof( panid ), &panid );    
    HalLedSet( HAL_LED_2, HAL_LED_MODE_FLASH );			        //网络灯开始闪烁
    
    ZXBeeInfInit();                                             // ZXBee通信协议初始化
#ifndef CC2530_Serial
        sensorInit();                                               // 传感器初始化
#endif
#ifdef CC2530_Serial
     WatchDogEnable( WDTIMX );
     
#endif        
    osal_start_timerEx( sapi_TaskID, __START_EVT, 500);     
  }
  
  
  static uint16 tick = 0;
  static uint16 at_tick = 0;
  
  if (event & __START_EVT) {
    
#ifdef CC2530_Serial
    uint8 f = EA;
    EA = 0;
    WDCTL = WDCLP1;
    WDCTL = WDCLP2;
    EA = f;
    
    if (mLinkStatus && ((int16)tick-(int16)at_tick) > 5*60*2){
      myReset();
    }
#endif    
    
    tick += 1;
    if (mLinkStatus == 0 && tick>110) {
      //zb_SystemReset();
      myReset();
    }
   
    osal_start_timerEx( sapi_TaskID, __START_EVT, 500); 
  }
  if (event & __REPORT_EVT) {                                   // 触发上报网络参数事件
    if (_tm_report_cnt > 0) {
      my_report_proc();
      osal_start_timerEx( sapi_TaskID, __REPORT_EVT, _tm_report_delay * 1000);
      _tm_report_cnt--;
    }
  }
  if (event & __AT_EVT) {                                       // 触发AT指令事件
#ifdef CC2530_Serial
    at_tick = tick;
#endif
    at_proc();
  }
  if (event & 0x000F) {                                         // 触发用户自定义事件
#ifndef CC2530_Serial
        MyEventProcess( event );
#endif
  }
}
/*********************************************************************************************
* 名称：zb_HandleKeys()
* 功能：处理节点产生的按键事件
* 参数：shift：转移标志；keys : 按下的按键
* 返回：
* 修改：
* 注释：
*********************************************************************************************/
void zb_HandleKeys( uint8 shift, uint8 keys )
{
}
/*********************************************************************************************
* 名称：zb_StartConfirm()
* 功能：当zstack协议栈启动完成后，执行这个函数
* 参数：status：启动完成后的状态
* 返回：
* 修改：
* 注释：
*********************************************************************************************/
void zb_StartConfirm( uint8 status )
{
  // If the device sucessfully started, change state to running
  if ( status == ZB_SUCCESS )                                   // 入网成功
  {
    HalLedSet( HAL_LED_2, HAL_LED_MODE_ON );
    mLinkStatus = 1;
#ifndef CC2530_Serial
        sensorLinkOn();                                             // 入网成功后调用
#else
    AT_reportedLinkStatus();
#endif
  }
  else
  {
  }
}
/*********************************************************************************************
* 名称：zb_SendDataConfirm()
* 功能：发送数据确认
* 参数：
* 返回：
* 修改：
* 注释：
*********************************************************************************************/
void zb_SendDataConfirm( uint8 handle, uint8 status )
{
  void ZXBeeSendConfirm(uint8 h, uint8 st);
  ZXBeeSendConfirm(handle, status);
}
/*********************************************************************************************
* 名称：zb_BindConfirm()
* 功能：绑定确认
* 参数：
* 返回：
* 修改：
* 注释：
*********************************************************************************************/
void zb_BindConfirm( uint16 commandId, uint8 status )
{  
}
/*********************************************************************************************
* 名称：zb_AllowBindConfirm()
* 功能：允许其他设备绑定
* 参数：
* 返回：
* 修改：
* 注释：
*********************************************************************************************/
void zb_AllowBindConfirm( uint16 source )
{
}
/*********************************************************************************************
* 名称：zb_FindDeviceConfirm()
* 功能：查找设备完成后确定函数
* 参数：
* 返回：
* 修改：
* 注释：
*********************************************************************************************/
void zb_FindDeviceConfirm( uint8 searchType, uint8 *searchKey, uint8 *result )
{
}
/*********************************************************************************************
* 名称：zb_HanderMsg()
* 功能：MT串口处理函数
* 参数：
* 返回：
* 修改：
* 注释：
*********************************************************************************************/
void zb_HanderMsg(osal_event_hdr_t *msg)
{
}
/*********************************************************************************************
* 名称：zb_ReceiveDataIndication()
* 功能：当zigbee接受到节点发送的数据后，调用这个函数
* 参数：source：源地址；commandID：命令ID；len：收到数据的长度；pData：收到的数据
* 返回：
* 修改：
* 注释：
*********************************************************************************************/
void zb_ReceiveDataIndication( uint16 source, uint16 command, uint16 len, uint8 *pData  )
{
  uint16 pAddr = NLME_GetCoordShortAddr();
  
  /* 接收到数据处理 */
  HalLedSet( HAL_LED_1, HAL_LED_MODE_OFF );
  HalLedSet( HAL_LED_1, HAL_LED_MODE_BLINK );  
  
 
  // 处理接收到的无线数据包APP_DATA
  if (command == 0) { //如果command==0 说明是zxbee数据
    if (logicalType != ZG_DEVICETYPE_COORDINATOR) { //通过at指令发送到串口
      at_notify_data((char *)pData, len);
    }
    ZXBeeInfRecv((char*)pData, len);   //交给ZXBee接口处理接收数据
  }
}
/*********************************************************************************************
* 名称：my_report_proc()
* 功能：节点网络信息上报处理
* 参数：
* 返回：
* 修改：
* 注释：
*********************************************************************************************/
static void my_report_proc(void)
{
  unsigned char mac[Z_EXTADDR_LEN];
  static char wbuf[96];
  ZXBeeBegin();

  NLME_GetCoordExtAddr(mac);
  sprintf(wbuf, "%02X%02X", mac[1],mac[0]);

  read_nb(wbuf+strlen(wbuf), -1);
  ZXBeeAdd("PN", wbuf);
  
#ifndef CC2530_Serial
    sprintf(wbuf, "%d%d%s", NODE_CATEGORY, logicalType, NODE_NAME);
    ZXBeeAdd("TYPE", wbuf);
#else
    at_notify_data("AT+TYPE?\r\n",strlen("AT+TYPE?\r\n"));
#endif
  char *p = ZXBeeEnd();
  if (p != NULL) {
    ZXBeeInfSend(p, strlen(p));
  }
}
/*********************************************************************************************
* 名称：read_nb()
* 功能：查询邻居节点地址
* 参数：
* 返回：
* 修改：
* 注释：
*********************************************************************************************/
static char* read_nb(char *buf, int len)
{
  int i;
  char *p;
  
  buf[0] = 0;
  p = buf;     
  for (i=0; i<MAX_NEIGHBOR_ENTRIES; i++) {
    neighborEntry_t *pnb = &neighborTable[i];
    if (pnb->panId == panid 
        && memcmp(pnb->neighborExtAddr,"\x00\x00\x00\x00\x00\x00\x00\x00", 8)!=0 
        && pnb->age <= NWK_ROUTE_AGE_LIMIT) {
          sprintf(p, "%02X%02X", pnb->neighborExtAddr[1], pnb->neighborExtAddr[0]);
          p = p + strlen(p);         
    }
  }
  return buf;
}
/*********************************************************************************************
* 名称：GetCurrentLogicalType()
* 功能：获取当前节点类型
* 参数：
* 返回：
* 修改：
* 注释：
*********************************************************************************************/
uint8 GetCurrentLogicalType(void)
{
  return logicalType;
}
/*********************************************************************************************
* 名称：GetLinkStatus()
* 功能：获取节点入网状态
* 参数：
* 返回：
* 修改：
* 注释：
*********************************************************************************************/
uint8 GetLinkStatus(void)
{
  return mLinkStatus;
}
/*********************************************************************************************
* 名称：GetPanId()
* 功能：获取节点PANID
* 参数：
* 返回：
* 修改：
* 注释：
*********************************************************************************************/
uint16 GetPanId(void)
{
   uint16 tmp16;
   osal_nv_read( ZCD_NV_PANID, 0, sizeof( tmp16 ), &tmp16 );
   return tmp16;
}
/*********************************************************************************************
* 名称：SetPanId()
* 功能：设置节点PANID
* 参数：
* 返回：
* 修改：
* 注释：
*********************************************************************************************/
void SetPanId(uint16 id)
{
    uint8 startOptions = ZCD_STARTOPT_DEFAULT_NETWORK_STATE;
    uint16 tmp16;
    tmp16 = GetPanId();
    if (tmp16 != id) {
        osal_nv_write(ZCD_NV_PANID, 0, osal_nv_item_len( ZCD_NV_PANID ), &id);
        zb_WriteConfiguration( ZCD_NV_STARTUP_OPTION, sizeof(uint8), &startOptions ); //标记网络状态发生改变
    }
}
/*********************************************************************************************
* 名称：GetChannel()
* 功能：获取节点信道
* 参数：
* 返回：
* 修改：
* 注释：
*********************************************************************************************/
uint8 GetChannel(void)
{
  uint32 tmp32;
  uint8 i;
		
  osal_nv_read( ZCD_NV_CHANLIST, 0, sizeof( tmp32 ), &tmp32 );
     
  for (i=0; i<16; i++) {
    if (tmp32 == chs[i]) break;
  }
  i += 11;
  return i;
}
/*********************************************************************************************
* 名称：SetChannel()
* 功能：设置节点信道
* 参数：
* 返回：
* 修改：
* 注释：
*********************************************************************************************/
void SetChannel(uint8 val)
{
  uint8 startOptions = ZCD_STARTOPT_DEFAULT_NETWORK_STATE;
  uint32 tmp32, t32;
  tmp32 = val - 11;
  osal_nv_read( ZCD_NV_CHANLIST, 0, sizeof( tmp32 ), &t32 );
  if (tmp32 < 16) {
    if (t32 != chs[tmp32]) {
      osal_nv_write(ZCD_NV_CHANLIST, 0, osal_nv_item_len( ZCD_NV_CHANLIST ), &chs[tmp32]);
      zb_WriteConfiguration( ZCD_NV_STARTUP_OPTION, sizeof(uint8), &startOptions ); //标记网络状态发生改变
    }
  }
}
/*********************************************************************************************
* 名称：GetLogicalType()
* 功能：获取节点类型
* 参数：
* 返回：
* 修改：
* 注释：
*********************************************************************************************/
uint8 GetLogicalType(void)
{
  uint8 st;
  zb_ReadConfiguration( ZCD_NV_LOGICAL_TYPE, sizeof(uint8), &st );
  return st;
}
/*********************************************************************************************
* 名称：SetLogicalType()
* 功能：设置节点类型
* 参数：
* 返回：
* 修改：
* 注释：
*********************************************************************************************/
void SetLogicalType(uint8 t)
{
  uint8 st;
  uint8 startOptions = ZCD_STARTOPT_DEFAULT_NETWORK_STATE;
  if (logicalType == ZG_DEVICETYPE_ROUTER || logicalType == ZG_DEVICETYPE_ENDDEVICE) {
    st = GetLogicalType();
    if (t != st && (t==ZG_DEVICETYPE_ROUTER || t == ZG_DEVICETYPE_ENDDEVICE)) {
      zb_WriteConfiguration(ZCD_NV_LOGICAL_TYPE, sizeof(uint8), &t);
      zb_WriteConfiguration( ZCD_NV_STARTUP_OPTION, sizeof(uint8), &startOptions ); //标记网络状态发生改变
    } 
  }
}
/*********************************************************************************************
* 名称：starReportTPN()
* 功能：设置节点网络参数循环上报时间参数
* 参数：
* 返回：
* 修改：
* 注释：
*********************************************************************************************/
void starReportTPN(uint16 t, uint16 n)
{
  _tm_report_delay = t;
  _tm_report_cnt = n;
  if (_tm_report_cnt != 0) {
    osal_start_timerEx( sapi_TaskID, __REPORT_EVT, (osal_rand()%_tm_report_delay) * (osal_rand()%1000));
  }
}

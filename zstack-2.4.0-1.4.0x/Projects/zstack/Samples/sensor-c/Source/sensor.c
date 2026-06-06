/*********************************************************************************************
* 文件：sensor.c
* 作者：Xuzhy 2018.5.16
* 说明：xLab Sensor-C传感器应用程序
* 修改：fuyou 2018.08.03 添加传感器检测功能
* 注释：
*********************************************************************************************/
/*
 * mode工作模式说明：
 *  1: 正常工作模式，包含人体红外&振动&门磁&火焰检测
 *  2：语音播放模式，用于播放语音提示
 *
 */
/*********************************************************************************************
* 头文件
*********************************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sapi.h"
#include "osal_nv.h"
#include "addrmgr.h"
#include "mt.h"
#include "hal_led.h"
#include "hal_adc.h"
#include "hal_uart.h"
#include "sensor.h"
#include "Flame.h"
#include "grating.h"
#include "hall.h"
#include "relay.h"
#include "infrared.h"
#include "MP-4.h"
#include "Touch.h"
#include "vibration.h"
#include "syn6288.h"
#include "zxbee.h"
#include "zxbee-inf.h"
/*********************************************************************************************
* 宏定义
*********************************************************************************************/

/*********************************************************************************************
* 变量
*********************************************************************************************/
static uint8  D0 = 0xFF;                                        // D0传感器数据主动上报使能
static uint8  D1 = 0;                                           // D1控制继电器的变量
static uint8  A0 = 0;                                           // mode=1 人体红外检测 mode=2 语音播放
static uint8  A1 = 0;                                           // mode=1 振动检测 mode=2 语音播放状态
static uint8  A2 = 0;                                           // mode=1 门磁检测 mode=2 门磁状态
static uint8  A3 = 0;                                           // mode=1 火焰检测 mode=2 火焰状态
static uint8  A4 = 0;                                           // 可燃气体检测
static uint8  A5 = 0;                                           // 光栅检测
static uint16 V0 = 30;                                          // V0数据上报周期，默认30s
static char*  V1;                                               // 语音播放数据指针
static uint8 mode = 1;
/*********************************************************************************************
* 名称：updateV0()
* 功能：更新V0的值
* 参数：*val -- 指向存放数据的指针
* 返回：
* 修改：
* 注释：
*********************************************************************************************/
void updateV0(char *val)
{
  //将字符串变量val转换为整型变量
  V0 = atoi(val);                                               // 将字符串变量转换为整型
}
/*********************************************************************************************
* 名称：updateA0()
* 功能：更新A0的值
* 参数：
* 返回：
* 修改：
* 注释：
*********************************************************************************************/
void updateA0(void)
{
   static uint32 ct = 0;

  if (mode == 1) {                                               // 正常工作模式下进行检测
    A0 = get_infrared_status();
    if (A0 != 0) {
      ct = osal_GetSystemClock();
    } else if (osal_GetSystemClock() > ct+1000) {
      ct = 0;
      A0 = 0;
    } else {
      A0 = 1;
    }
  }
}
/*********************************************************************************************
* 名称：updateA1()
* 功能：更新A1的值
* 参数：
* 返回：
* 修改：
* 注释：
*********************************************************************************************/
void updateA1(void)
{
   static uint32 ct = 0;

  if (mode == 1) {                                              // 正常工作模式
    A1 = get_vibration_status();
    if (A1 != 0) {
      ct = osal_GetSystemClock();
    } else if (osal_GetSystemClock() > ct+2000) {
      ct = 0;
      A1 = 0;
    } else {
      A1 = 1;
    }
  }
}
/*********************************************************************************************
* 名称：updateA2()
* 功能：更新A2的值
* 参数：
* 返回：
* 修改：
* 注释：
*********************************************************************************************/
void updateA2(void)
{
  if (mode == 1) {
    A2 = get_hall_status();                                     // 获取门磁状态
  }
}
/*********************************************************************************************
* 名称：updateA3()
* 功能：更新A3的值
* 参数：
* 返回：
* 修改：
* 注释：
*********************************************************************************************/
void updateA3(void)
{
  static uint32 ct = 0;

  if (mode == 1) {
    A3 = get_flame_status();                                    // 获取火焰状态
    if (A3 != 0) {
      ct = osal_GetSystemClock();
    } else if (osal_GetSystemClock() > ct+2000) {
      ct = 0;
      A3 = 0;
    } else {
      A3 = 1;
    }
  }
}
/*********************************************************************************************
* 名称：updateA4()
* 功能：更新A4的值
* 参数：
* 返回：
* 修改：
* 注释：
*********************************************************************************************/
void updateA4(void)
{
  A4 = get_combustiblegas_data();                               // 获取可燃气体数据
}
/*********************************************************************************************
* 名称：updateA5()
* 功能：更新A5的值
* 参数：
* 返回：
* 修改：
* 注释：
*********************************************************************************************/
void updateA5(void)                                             // 获取光栅状态
{
  A5 = get_grating_status();
}
/*********************************************************************************************
* 名称：sensorInit()
* 功能：初始化传感器及定时器
* 参数：
* 返回：
* 修改：
* 注释：
*********************************************************************************************/
void sensorInit(void)
{
  ZXBeeInfSetLocalAddr(ZXBEE_ADDR_SENSOR_C);
  // 传感器初始化
  vibration_init();                                             // 振动传感器初始化，包含定时器配置
  infrared_init();                                              // 人体红外传感器初始化
  flame_init();                                                 // 火焰传感器初始化
  hall_init();                                                  // 门磁传感器初始化
  combustiblegas_init();                                        // 可燃气体传感器初始化
  grating_init();                                               // 光栅传感器初始化
  relay_init();                                                 // 继电器初始化

  // 启动定时器，触发事件MY_REPORT_EVT
  osal_start_timerEx(sapi_TaskID, MY_REPORT_EVT, (uint16)((osal_rand()%10) * 1000));
}
/*********************************************************************************************
* 名称：sensorLinkOn()
* 功能：当传感器节点加入网络后执行的操作
* 参数：
* 返回：
* 修改：
* 注释：
*********************************************************************************************/
void sensorLinkOn(void)
{
  sensorUpdate();
}
/*********************************************************************************************
* 名称：sensorUpdate()
* 功能：定时上报传感器数据
* 参数：
* 返回：
* 修改：
* 注释：
*********************************************************************************************/
void sensorUpdate(void)
{
  char pData[16];
  char *p = pData;

  ZXBeeBegin();                                                 // 智云数据帧头

  // 根据D0的位判断是否上报数据
  if ((D0 & 0x01) == 0x01){                                     // 上报人体红外数据到pData缓冲区
    updateA0();
    sprintf(p, "%u", A0);
    ZXBeeAdd("pir", p);
  }
  if (mode == 1) {
    if ((D0 & 0x02) == 0x02){                                     // 上报振动数据到pData缓冲区
      updateA1();
      sprintf(p, "%u", A1);
      ZXBeeAdd("tch", p);
    }
    if ((D0 & 0x04) == 0x04){                                     // 上报门磁数据到pData缓冲区
      updateA2();
      sprintf(p, "%u", A2);
      ZXBeeAdd("door", p);
    }
    if ((D0 & 0x08) == 0x08){                                     // 上报火焰数据到pData缓冲区
      updateA3();
      sprintf(p, "%u", A3);
      ZXBeeAdd("alert", p);
    }
  }
  if ((D0 & 0x10) == 0x10){                                     // 上报可燃气体数据到pData缓冲区
    updateA4();
    sprintf(p, "%u", A4);
    ZXBeeAdd("A4", p);
  }
  if ((D0 & 0x10) == 0x10){                                     // 上报光栅数据到pData缓冲区
    updateA5();
    sprintf(p, "%u", A5);
    ZXBeeAdd("A5", p);
  }

  sprintf(p, "%u", D1);                                  // 控制命令
  ZXBeeAdd("D1", p);

  p = ZXBeeEnd();                                               // 智云数据帧尾
  if (p != NULL) {
    ZXBeeInfSend(p, strlen(p));	                                // 将数据通过zb_SendDataRequest()发送出去
  }
}
/*********************************************************************************************
* 名称：sensorCheck()
* 功能：检测传感器数据
* 参数：
* 返回：
* 修改：
* 注释：
*********************************************************************************************/
void sensorCheck(void)
{
  static char lastA0 = 0,lastA1=0,lastA2=0,lastA3=0,lastA4=0,lastA5=0;
  static uint32 ct0=0, ct1=0, ct2=0, ct3=0, ct4=0, ct5=0;
  char pData[16];
  char *p = pData;

  updateA0();
  updateA1();
  updateA2();
  updateA3();
  updateA4();
  updateA5();

  ZXBeeBegin();

  if (lastA0 != A0
      || ( //模式1时，当检测到变化后3S内持续上报；模式2时，当检测到变化后持续上报1次
          (mode == 1)&&ct0 != 0 && osal_GetSystemClock() > (ct0+3000)) //超过3秒
      ) {
    if (D0 & 0x01) {
      sprintf(p, "%u", A0);
      ZXBeeAdd("pir", p);
      ct0 = osal_GetSystemClock();
    }
    if (A0 == 0) {
        ct0 = 0;
    }
    lastA0 = A0;
  }
  if (mode == 1) {   // 模式1时进行振动检测
    if (lastA1 != A1 || (ct1 != 0 && osal_GetSystemClock() > (ct1+3000))) {  // 振动检测
      if (D0 & 0x02) {
        sprintf(p, "%u", A1);
        ZXBeeAdd("tch", p);
        ct1 = osal_GetSystemClock();
      }
      if (A1 == 0) {
        ct1 = 0;
      }
      lastA1 = A1;
    }
    if (lastA2 != A2 || (ct2 != 0 && osal_GetSystemClock() > (ct2+3000))) {  // 门磁检测
      if (D0 & 0x04) {
        sprintf(p, "%u", A2);
        ZXBeeAdd("door", p);
        ct2 = osal_GetSystemClock();
      }
      if (A2 == 0) {
        ct2 = 0;
      }
      lastA2 = A2;
    }

    if (lastA3 != A3 || (ct3 != 0 && osal_GetSystemClock() > (ct3+3000))) {  // 火焰检测
      if (D0 & 0x08) {
        sprintf(p, "%u", A3);
        ZXBeeAdd("alert", p);
        ct3 = osal_GetSystemClock();
      }
      if (A3 == 0) {
        ct3 = 0;
      }
      lastA3 = A3;
    }
  }
  if (lastA4 != A4 || (ct4 != 0 && osal_GetSystemClock() > (ct4+3000))) {  // 可燃气体检测
    if (D0 & 0x10) {
      sprintf(p, "%u", A4);
      ZXBeeAdd("A4", p);
      ct4 = osal_GetSystemClock();
    }
    if (A4 == 0) {
      ct4 = 0;
    }
    lastA4 = A4;
  }
  if (lastA5 != A5 || (ct5 != 0 && osal_GetSystemClock() > (ct5+3000))) {  // 光栅检测
    if (D0 & 0x01) {
      sprintf(p, "%u", A5);
      ZXBeeAdd("A5", p);
      ct5 = osal_GetSystemClock();
    }
    if (A5 == 0) {
      ct5 = 0;
    }
    lastA5 = A5;
  }

  p = ZXBeeEnd();
  if (p != NULL) {
    int len = strlen(p);
    ZXBeeInfSend(p, len);
  }
}
/*********************************************************************************************
* 名称：sensorControl()
* 功能：控制传感器
* 参数：cmd - 控制命令
* 返回：
* 修改：
* 注释：
*********************************************************************************************/
void sensorControl(uint8 cmd)
{
  // 根据cmd参数控制继电器
  if(cmd & 0x40){
    RELAY1 = ON;                                                 // 开启继电器1
  }
  else{
    RELAY1 = OFF;                                                 // 关闭继电器1
  }
  if(cmd & 0x80){
    RELAY2 = ON;                                                 // 开启继电器2
  }
  else{
    RELAY2 = OFF;                                                 // 关闭继电器2
  }
}
/*********************************************************************************************
* 名称：ZXBeeUserProcess()
* 功能：处理用户数据
* 参数：*ptag -- 标签
*       *pval -- 数据
* 返回：ret -- 数据长度
* 修改：
* 注释：
*********************************************************************************************/
int ZXBeeUserProcess(char *ptag, char *pval)
{
  int val;
  int ret = 0;
  char pData[16];
  char *p = pData;

  // 将字符串变量pval转换为整型变量
  val = atoi(pval);
  // 控制命令
  if (0 == strcmp("CD0", ptag)){                                // 清D0的某位，用CD0标签名
    D0 &= ~val;
    ret = sprintf(p, "%u", D0);
    ZXBeeAdd("D0", p);
  }
  if (0 == strcmp("OD0", ptag)){                                // 置D0的某位，用OD0标签名
    D0 |= val;
    ret = sprintf(p, "%u", D0);
    ZXBeeAdd("D0", p);
  }
  if (0 == strcmp("D0", ptag)){                                 // 查询D0的值
    if (0 == strcmp("?", pval)){
      ret = sprintf(p, "%u", D0);
      ZXBeeAdd("D0", p);
    }
  }
  if (0 == strcmp("CD1", ptag)){                                // 清D1的某位，用CD1标签名
    D1 &= ~val;
    sensorControl(D1);                                          // 控制继电器
    ret = sprintf(p, "%u", D1);
    ZXBeeAdd("D1", p);
  }
  if (0 == strcmp("OD1", ptag)){                                // 置D1的某位，用OD1标签名
    D1 |= val;
    sensorControl(D1);                                          // 控制继电器
    ret = sprintf(p, "%u", D1);
    ZXBeeAdd("D1", p);
  }
  if (0 == strcmp("D1", ptag)){                                 // 查询D1的值
    if (0 == strcmp("?", pval)){
      ret = sprintf(p, "%u", D1);
      ZXBeeAdd("D1", p);
    }
  }
  if (0 == strcmp("A0", ptag) || 0 == strcmp("pir", ptag)){
    if (0 == strcmp("?", pval)){
      updateA0();
      ret = sprintf(p, "%u", A0);
      ZXBeeAdd("pir", p);
    }
  }
  if (0 == strcmp("A1", ptag) || 0 == strcmp("tch", ptag)){
    if (0 == strcmp("?", pval)){
      updateA1();
      ret = sprintf(p, "%u", A1);
      ZXBeeAdd("tch", p);
    }
  }
  if (0 == strcmp("A2", ptag) || 0 == strcmp("door", ptag)){
    if (0 == strcmp("?", pval)){
      updateA2();
      ret = sprintf(p, "%u", A2);
      ZXBeeAdd("door", p);
    }
  }
  if (0 == strcmp("A3", ptag) || 0 == strcmp("alert", ptag)){
    if (0 == strcmp("?", pval)){
      updateA3();
      ret = sprintf(p, "%u", A3);
      ZXBeeAdd("alert", p);
    }
  }
  if (0 == strcmp("A4", ptag)){
    if (0 == strcmp("?", pval)){
      updateA4();
      ret = sprintf(p, "%u", A4);
      ZXBeeAdd("A4", p);
    }
  }
  if (0 == strcmp("A5", ptag)){
    if (0 == strcmp("?", pval)){
      updateA5();
      ret = sprintf(p, "%u", A5);
      ZXBeeAdd("A5", p);
    }
  }
  if (0 == strcmp("V0", ptag)){
    if (0 == strcmp("?", pval)){
      ret = sprintf(p, "%u", V0);                         	// 查询上报周期
      ZXBeeAdd("V0", p);
    }else{
      updateV0(pval);
    }
  }
  if (0 == strcmp("V1", ptag)){
    if (0 == strcmp("?", pval)){
      ret = sprintf(p, "%s", V1);
      ZXBeeAdd("V1", p);
    }else{
      if(mode == 2){
        int n = strlen(pval)/2;
        syn6288_play_unicode(hex2unicode(pval),n);
      }
    }
  }
  return ret;
}
/*********************************************************************************************
* 名称：MyEventProcess()
* 功能：事件处理函数
* 参数：event -- 事件
* 返回：
* 修改：
* 注释：
*********************************************************************************************/
void MyEventProcess( uint16 event )
{
  //首次收到事件时启动检测定时器
  static char check_start_flag = 0;
  if(check_start_flag == 0) {
    if (event & MY_REPORT_EVT) {
      // 启动检测定时器，触发事件MY_CHECK_EVT
      osal_start_timerEx(sapi_TaskID, MY_CHECK_EVT, 100);
      check_start_flag = 1;
    }
  }

  if (event & MY_REPORT_EVT) {
    sensorUpdate();
    //启动定时器，触发事件MY_REPORT_EVT
    osal_start_timerEx(sapi_TaskID, MY_REPORT_EVT, V0*1000);
  }
  if (event & MY_CHECK_EVT) {
    sensorCheck();
    // 启动检测定时器，触发事件MY_CHECK_EVT
    osal_start_timerEx(sapi_TaskID, MY_CHECK_EVT, 100);
  }
}

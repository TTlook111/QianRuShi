/*********************************************************************************************
* 文件：sensor.c
* 作者：Xuzhy 2018.5.16
* 说明：xLab Sensor-A传感器应用程序
* 修改：
* 注释：
*********************************************************************************************/

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
#include "htu21d.h"
#include "fbm320.h"
#include "BH1750.h"
#include "iic.h"
#include "lis3dh.h"
#include "MP-503.h"
#include "stadiometry.h"
#include "relay.h"
#include "ld3320.h"
#include "math.h"
#include "zxbee.h"
#include "zxbee-inf.h"
/*********************************************************************************************
* 宏定义
*********************************************************************************************/

/*********************************************************************************************
* 变量
*********************************************************************************************/
static uint8 D0 = 0xFF;                                         // D0传感器数据主动上报使能
static uint8 D1 = 0;                                            // D1控制继电器的变量
static float A0 = 0.0;                                          // A0温度值
static float A1 = 0.0;                                          // A1湿度值
static float A2 = 0.0;                                          // A2光照值
static int A3 = 0;                                              // A3空气质量值
static float A4 = 0.0;                                          // A4气压值
static int A5 = 0;                                              // A5跌倒检测值
static float A6 = 0.0;                                          // A6测距值
static int A7 = 0;                                              // A7语音识别值
static uint16 V0 = 30;                                          // V0数据上报周期，默认30s
static uint8 lis3dh_status = 0;                                 // 加速度传感器状态
/*********************************************************************************************
* 名称：fallDect()
* 功能：检测加速度判断是否跌倒
* 参数：x,y,z - 三轴加速度的x,y,z值
* 返回：int - 检测结果
* 修改：
* 注释：
*********************************************************************************************/
int fallDect(float x, float y, float z)
{
  float x2 = x * x;
  float y2 = y * y;
  float z2 = z * z;
  float a;
  
  a = sqrt(x2 + y2 + z2);
  if (a<5) {
    return 1;
  }    
  return 0;
}
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
  V0 = atoi(val);                                 // 将字符串变量转换为整型
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
  // 读取温度传感器数据赋给A0
  A0 = (htu21d_get_data(TEMP)/100.0f);
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
  // 读取湿度传感器数据赋给A1
  A1 = (htu21d_get_data(HUMI)/100.0f);
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
  // 读取光照传感器数据赋给A2
  A2 = bh1750_get_data();
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
  // 读取空气质量传感器数据赋给A3
  A3 = get_airgas_data();
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
  // 读取气压传感器数据赋给A4
  float temperature = 0;                                        // 定义温度变量
  long pressure = 0;                                            // 定义气压变量

  fbm320_data_get(&temperature,&pressure);                      // 获取气压传感器数据
  A4 = pressure/100.0f;                                         // 计算气压值

}
/*********************************************************************************************
* 名称：updateA5()
* 功能：更新A5跌倒检测的值
* 参数：
* 返回：
* 修改：
* 注释：
*********************************************************************************************/
void updateA5(void)
{
  static uint32 ct = 0;
  float accx,accy,accz;
  
  if (A5 == 0) {
    lis3dh_read_data(&accx,&accy,&accz);
    if(!(accx==0&&accy==0&&accz==0)) {
      A5 = fallDect(accx,accy,accz);
      ct = osal_GetSystemClock();
    } else {
      A5 = 0;
    }
  } else if (A5!=0 && osal_GetSystemClock()>(ct+2000)) {
    A5 = 0;
  }
}
/*********************************************************************************************
* 名称：updateA6()
* 功能：更新A6的值
* 参数：
* 返回：
* 修改：
* 注释：
*********************************************************************************************/
void updateA6(void)
{
  // 读取测距传感器数据赋给A6
  A6 = get_stadiometry_data();
}
/*********************************************************************************************
* 名称：updateA7()
* 功能：读取语音识别传感器数据赋给A7的值
* 参数：
* 返回：
* 修改：
* 注释：
*********************************************************************************************/
void updateA7(void)
{
  A7 = ld3320_read();                                       // 读取语音识别传感器数据
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
  ZXBeeInfSetLocalAddr(ZXBEE_ADDR_SENSOR_A);
  // 传感器初始化
  htu21d_init();                                                // 温湿度传感器初始化
  bh1750_init();                                                // 光照传感器初始化
  airgas_init();                                                // 空气质量传感器初始化
  fbm320_init();                                                // 气压传感器初始化
  lis3dh_status = lis3dh_init();                                // 加速度传感器初始化
  stadiometry_init();                                           // 测距传感器初始化
  ld3320_init();                                                // 语音识别传感器初始化
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

  // 根据D0的位判断需要上报的数据
  if ((D0 & 0x01) == 0x01){                                     // 若需要则读取传感器数据，并通过pData上报数据
    updateA0();
    sprintf(p, "%.1f", A0);
    ZXBeeAdd("temp", p);
  }
  if ((D0 & 0x02) == 0x02){                                     // 若需要则读取传感器数据，并通过pData上报数据
    updateA1();
    sprintf(p, "%.1f", A1);
    ZXBeeAdd("humi", p);
  }
  if ((D0 & 0x04) == 0x04){                                     // 若需要则读取传感器数据，并通过pData上报数据
    updateA2();
    sprintf(p, "%.1f", A2);
    ZXBeeAdd("lux", p);
  }
  if ((D0 & 0x08) == 0x08){                                     // 若需要则读取传感器数据，并通过pData上报数据
    updateA3();
    sprintf(p, "%u", A3);
    ZXBeeAdd("A3", p);
  }
  if ((D0 & 0x10) == 0x10){                                     // 若需要则读取传感器数据，并通过pData上报数据
    updateA4();
    sprintf(p, "%.1f", A4);
    ZXBeeAdd("A4", p);
  }
  if ((D0 & 0x10) == 0x10){                                     // 若需要则读取传感器数据，并通过pData上报数据
    updateA5();
    sprintf(p, "%u", A5);
    ZXBeeAdd("A5", p);
  }
  if ((D0 & 0x40) == 0x40){                                     // 若需要则读取传感器数据，并通过pData上报数据
    updateA6();
    sprintf(p, "%.1f", A6);
    ZXBeeAdd("A6", p);
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
  char pData[16];
  char *p = pData;
  static int lastA5 = 0;

  if(lis3dh_status == 0) updateA5();                            // 跌倒检测
  updateA7();                                                   // 语音识别

  ZXBeeBegin();

  if (A5 != lastA5) {                                           // A5变化后2秒内重复上报
    sprintf(p,"%u",A5);
    ZXBeeAdd("A5", p);
    lastA5 = A5;
  }

  if (A7 != 0){                                                 // 语音识别后上报A7
    sprintf(p,"%u",A7);
    ZXBeeAdd("A7", p);
  }

  p = ZXBeeEnd();
  if (p != NULL) {
    ZXBeeInfSend(p, strlen(p));
  }
}
/*********************************************************************************************
* 名称：sensorControl()
* 功能：控制继电器操作
* 参数：cmd - 控制命令
* 返回：
* 修改：
* 注释：
*********************************************************************************************/
void sensorControl(uint8 cmd)
{
  // 根据cmd值控制继电器开关
  if(cmd & 0x40){
    RELAY1 = ON;                                                 // 打开继电器1
  }
  else{
    RELAY1 = OFF;                                                 // 关闭继电器1
  }
  if(cmd & 0x80){
    RELAY2 = ON;                                                 // 打开继电器2
  }
  else{
    RELAY2 = OFF;                                                 // 关闭继电器2
  }
}
/*********************************************************************************************
* 名称：ZXBeeUserProcess()
* 功能：解析接收到的数据并执行相应操作
* 参数：*ptag -- 数据标签
*       *pval -- 数据内容
* 返回：<0：失败；>=0 成功
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
  // 数据处理
  if (0 == strcmp("CD0", ptag)){                                // 对D0位清零操作，CD0为标签名
    D0 &= ~val;
    ret = sprintf(p, "%u", D0);
    ZXBeeAdd("D0", p);
  }
  if (0 == strcmp("OD0", ptag)){                                // 对D0位置一操作，OD0为标签名
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
  if (0 == strcmp("CD1", ptag)){                                // 对D1位清零操作，CD1为标签名
    D1 &= ~val;
    sensorControl(D1);                                          // 控制继电器
    ret = sprintf(p, "%u", D1);
    ZXBeeAdd("D1", p);
  }
  if (0 == strcmp("OD1", ptag)){                                // 对D1位置一操作，OD1为标签名
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
  if (0 == strcmp("A0", ptag) || 0 == strcmp("temp", ptag)){
    if (0 == strcmp("?", pval)){
      updateA0();
      ret = sprintf(p, "%.1f", A0);
      ZXBeeAdd("temp", p);
    }
  }
  if (0 == strcmp("A1", ptag) || 0 == strcmp("humi", ptag)){
    if (0 == strcmp("?", pval)){
      updateA1();
      ret = sprintf(p, "%.1f", A1);
      ZXBeeAdd("humi", p);
    }
  }
  if (0 == strcmp("A2", ptag) || 0 == strcmp("lux", ptag)){
    if (0 == strcmp("?", pval)){
      updateA2();
      ret = sprintf(p, "%.1f", A2);
      ZXBeeAdd("lux", p);
    }
  }
  if (0 == strcmp("A3", ptag)){
    if (0 == strcmp("?", pval)){
      updateA3();
      ret = sprintf(p, "%u", A3);
      ZXBeeAdd("A3", p);
    }
  }
  if (0 == strcmp("A4", ptag)){
    if (0 == strcmp("?", pval)){
      updateA4();
      ret = sprintf(p, "%.1f", A4);
      ZXBeeAdd("A4", p);
    }
  }
  if (0 == strcmp("A6", ptag)){
    if (0 == strcmp("?", pval)){
      updateA6();
      ret = sprintf(p, "%.1f", A6);
      ZXBeeAdd("A6", p);
    }
  }
  if (0 == strcmp("V0", ptag)){
    if (0 == strcmp("?", pval)){
      ret = sprintf(p, "%u", V0);                         	// 查询V0的值
      ZXBeeAdd("V0", p);
    }else{
      updateV0(pval);
    }
  }
  return ret;
}
/*********************************************************************************************
* 名称：MyEventProcess()
* 功能：事件处理函数
* 参数：event -- 事件编号
* 返回：
* 修改：
* 注释：
*********************************************************************************************/
void MyEventProcess( uint16 event )
{
  //首次启动定时器，触发事件
  static char check_start_flag = 0;
  if(check_start_flag == 0) {
    if (event & MY_REPORT_EVT) {
      // 首次启动定时器，触发事件MY_CHECK_EVT
      osal_start_timerEx(sapi_TaskID, MY_CHECK_EVT, 100);
      check_start_flag = 1;
    }
  }

  if (event & MY_REPORT_EVT) {
    sensorUpdate();
    //设置定时器，触发事件MY_REPORT_EVT
    osal_start_timerEx(sapi_TaskID, MY_REPORT_EVT, V0*1000);
  }
  if (event & MY_CHECK_EVT) {
    sensorCheck();
    // 设置定时器，触发事件MY_CHECK_EVT
    osal_start_timerEx(sapi_TaskID, MY_CHECK_EVT, 100);
  }
}


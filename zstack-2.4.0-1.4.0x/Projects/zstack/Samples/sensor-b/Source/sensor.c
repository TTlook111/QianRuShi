/*********************************************************************************************
* 文件：sensor.c
* 作者：Xuzhy 2018.5.16
* 说明：xLab Sensor-B传感器应用程序
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
#include "led.h"
#include "rgb.h"
#include "fan.h"
#include "beep.h"
#include "stepmotor.h"
#include "relay.h"
#include "timer.h"
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
static uint16 V0 = 30;                                          // V0数据上报周期，默认30s
static uint8  mode = 0;                                         // 工作模式
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
* 名称：sensorInit()
* 功能：初始化传感器及定时器
* 参数：
* 返回：
* 修改：
* 注释：
*********************************************************************************************/
void sensorInit(void)
{
  ZXBeeInfSetLocalAddr(ZXBEE_ADDR_SENSOR_B);
  // 传感器初始化
  // 检测P0_3 引脚电平，高电平时为模式1，低电平时为模式2
  P0SEL &= ~0x08;                                               // 设置P0_3为IO口
  P0DIR &= !0x08;

  if(P0_3 == 0){
    mode = 1;
    stepmotor_init();                                           // 步进电机初始化
    fan_init();                                                 // 风扇初始化
  }else{
    mode = 2;
    rgb_init();                                                 // RGB灯初始化
    beep_init();                                                // 蜂鸣器初始化
  }

  led_init();                                                   // LED灯初始化
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

  ZXBeeBegin();

  sprintf(p, "%u", D1);                                  // 控制命令
  ZXBeeAdd("state", p);

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
  static uint8 stepmotor_flag = 0;
  // 根据cmd参数控制继电器
  if(mode == 1){                                                  // 模式1控制
    if ((cmd & 0x04) == 0x04) {                                   // 控制步进电机，bit2
      if(stepmotor_flag != 1) {                                   // 正转
        stepmotor_flag = 1;
        forward(5000);
      }
    } else {                                                      // 反转
      if(stepmotor_flag != 0) {
        stepmotor_flag = 0;
        reversion(5000);
      }
    }

	if(cmd & 0x08) {                                               // 控制风扇，bit3
      FANIO = FAN_ON;                                                  // 开启风扇
    } else {
      FANIO = FAN_OFF;                                                 // 关闭风扇
    }
  }

  if(mode == 2){                                                  // 模式2控制
    if ((cmd & 0x01) == 0x01){                                    // RGB灯控制，bit0~bit1
      if ((cmd & 0x02) == 0x02){                                  // cmd为3时亮蓝灯
        RGB_R = OFF;
        RGB_G = OFF;
        RGB_B = ON;
      }
      else{                                                       // cmd为1时亮红灯
        RGB_R = ON;
        RGB_G = OFF;
        RGB_B = OFF;
      }
    }
    else if ((cmd & 0x02) == 0x02){                               // cmd为2时亮绿灯
      RGB_R = OFF;
      RGB_G = ON;
      RGB_B = OFF;
    }
    else{                                                         // cmd为0时关闭灯
      RGB_R = OFF;
      RGB_G = OFF;
      RGB_B = OFF;
    }

	if(cmd & 0x08) {                                              // 控制蜂鸣器，bit3
      BEEPIO = ON;                                                // 开启蜂鸣器
    } else {
      BEEPIO = OFF;                                               // 关闭蜂鸣器
    }
  }


  if(cmd & 0x20){                                               // LED2控制，bit5
    LED2 = ON;                                                  // 开启LED2
  }
  else{
    LED2 = OFF;                                                 // 关闭LED2
  }
  if(cmd & 0x10){                                               // LED1控制，bit4
    LED1 = ON;                                                  // 开启LED1
  }
  else{
    LED1 = OFF;                                                 // 关闭LED1
  }

  if(cmd & 0x80){                                               // 继电器2控制，bit7
    RELAY2 = ON;                                                // 开启继电器2
  }
  else{
    RELAY2 = OFF;                                               // 关闭继电器2
  }
  if(cmd & 0x40){                                               // 继电器1控制，bit6
    RELAY1 = ON;                                                // 开启继电器1
  }
  else{
    RELAY1 = OFF;                                               // 关闭继电器1
  }
}
/*********************************************************************************************
* 名称：sensorSetRgb()
* 功能：设置RGB灯颜色
* 参数：*val -- 颜色数据，格式为[r,g,b]
* 返回：
* 修改：
* 注释：
*********************************************************************************************/
static void sensorSetRgb(char *val)
{
  int r = 0;
  int g = 0;
  int b = 0;
  if (val == NULL) return;
  sscanf(val, "[%d,%d,%d]", &r, &g, &b);
  D1 &= ~0x03;
  if (b > 0) {
    D1 |= 0x03;
  } else if (g > 0) {
    D1 |= 0x02;
  } else if (r > 0) {
    D1 |= 0x01;
  }
  sensorControl(D1);
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
  if (0 == strcmp("unlock", ptag)){
    if (val) D1 |= 0x40;
    else D1 &= ~0x40;
    sensorControl(D1);
    ret = sprintf(p, "%u", D1);
    ZXBeeAdd("state", p);
  }
  if (0 == strcmp("buzz", ptag)){
    if (val) D1 |= 0x08;
    else D1 &= ~0x08;
    sensorControl(D1);
    ret = sprintf(p, "%u", D1);
    ZXBeeAdd("state", p);
  }
  if (0 == strcmp("rgb", ptag)){
    sensorSetRgb(pval);
    ret = sprintf(p, "%u", D1);
    ZXBeeAdd("state", p);
  }
  if (0 == strcmp("reset", ptag)){
    D1 = 0;
    sensorControl(D1);
    ret = sprintf(p, "%u", D1);
    ZXBeeAdd("reset", p);
  }
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
    ZXBeeAdd("state", p);
  }
  if (0 == strcmp("OD1", ptag)){                                // 置D1的某位，用OD1标签名
    D1 |= val;
    sensorControl(D1);                                          // 控制继电器
    ret = sprintf(p, "%u", D1);
    ZXBeeAdd("state", p);
  }
  if (0 == strcmp("D1", ptag)){                                 // 查询D1的值
    if (0 == strcmp("?", pval)){
      ret = sprintf(p, "%u", D1);
      ZXBeeAdd("state", p);
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
  if (event & MY_REPORT_EVT) {
    sensorUpdate();
    //启动定时器，触发事件MY_REPORT_EVT
    osal_start_timerEx(sapi_TaskID, MY_REPORT_EVT, V0*1000);
  }
}

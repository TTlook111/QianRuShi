/*********************************************************************************************
* 文件：sensor.c
* 作者：Xuzhy 2018.5.16
* 说明：xLab Sensor-B传感器程序
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
* 全局变量
*********************************************************************************************/
static uint8  D0 = 0xFF;                                        // 默认打开主动上报功能
static uint8  D1 = 0;                                           // 继电器初始状态为全关
static uint16 V0 = 30;                                          // V0设置为上报时间间隔，默认为30s
static uint8  mode = 0;                                         // 控制标识符
/*********************************************************************************************
* 名称：updateV0()
* 功能：更新V0的值
* 参数：*val -- 待更新的变量
* 返回：
* 修改：
* 注释：
*********************************************************************************************/
void updateV0(char *val)
{
  //将字符串变量val解析转换为整型变量赋值
  V0 = atoi(val);                                 // 获取数据上报时间更改值
}
/*********************************************************************************************
* 名称：sensorInit()
* 功能：传感器硬件初始化
* 参数：无
* 返回：无
* 修改：
* 注释：
*********************************************************************************************/
void sensorInit(void)
{
  // 初始化传感器代码
  // 通过检测P0_3 来判断传感器跳线模式，并初始化对应的传感器
  P0SEL &= ~0x08;                                               // 配置管脚为通用IO模式
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
  
  // 启动定时器，触发传感器上报数据事件：MY_REPORT_EVT
  osal_start_timerEx(sapi_TaskID, MY_REPORT_EVT, (uint16)((osal_rand()%10) * 1000));
}
/*********************************************************************************************
* 名称：sensorLinkOn()
* 功能：传感器节点入网成功调用函数
* 参数：无
* 返回：无
* 修改：
* 注释：
*********************************************************************************************/
void sensorLinkOn(void)
{
  sensorUpdate();
}
/*********************************************************************************************
* 名称：sensorUpdate()
* 功能：处理主动上报的数据
* 参数：无
* 返回：无
* 修改：
* 注释：
*********************************************************************************************/
void sensorUpdate(void)
{ 
  char pData[16];
  char *p = pData;
  
  ZXBeeBegin();
  
  sprintf(p, "%u", D1);                                  // 上报控制编码 
  ZXBeeAdd("D1", p);
  
  p = ZXBeeEnd();                                               // 智云数据帧格式包尾
  if (p != NULL) {												
    ZXBeeInfSend(p, strlen(p));	                                // 将需要上传的数据进行打包操作，并通过zb_SendDataRequest()发送到协调器
  }
}
/*********************************************************************************************
* 名称：sensorCheck()
* 功能：传感器监测
* 参数：无
* 返回：无
* 修改：
* 注释：
*********************************************************************************************/
void sensorCheck(void)
{
  
}
/*********************************************************************************************
* 名称：sensorControl()
* 功能：传感器控制
* 参数：cmd - 控制命令
* 返回：无
* 修改：
* 注释：
*********************************************************************************************/
void sensorControl(uint8 cmd)
{
  static uint8 stepmotor_flag = 0;
  // 根据cmd参数处理对应的控制程序
  if(mode == 1){                                                  // 传感器模式一跳线
    if ((cmd & 0x04) == 0x04) {                                   // 步进电机控制位：bit2
      if(stepmotor_flag != 1) {                                   // 步进电机正转
        stepmotor_flag = 1; 
        forward(5000);
      }
    } else {                                                      // 步进电机反转
      if(stepmotor_flag != 0) {
        stepmotor_flag = 0; 
        reversion(5000);
      }
    }
	
	if(cmd & 0x08) {                                               // 风扇控制位：bit3
      FANIO = FAN_ON;                                                  // 开启风扇
    } else {
      FANIO = FAN_OFF;                                                 // 关闭风扇       
    }
  }
  
  if(mode == 2){                                                  // 传感器模式二跳线
    if ((cmd & 0x01) == 0x01){                                    // RGB灯控制位：bit0~bit1
      if ((cmd & 0x02) == 0x02){                                  // cmd为3，亮蓝灯
        RGB_R = OFF;
        RGB_G = OFF;
        RGB_B = ON;
      }
      else{                                                       // cmd为1，亮红灯
        RGB_R = ON;
        RGB_G = OFF;
        RGB_B = OFF;
      }
    }
    else if ((cmd & 0x02) == 0x02){                               // cmd为2，亮绿灯
      RGB_R = OFF;
      RGB_G = ON;
      RGB_B = OFF;
    }
    else{                                                         // cmd为1，灯全灭
      RGB_R = OFF;
      RGB_G = OFF;
      RGB_B = OFF;
    }
		
	if(cmd & 0x08) {                                              // 蜂鸣器控制位：bit3
      BEEPIO = ON;                                                // 开启蜂鸣器
    } else {
      BEEPIO = OFF;                                               // 关闭蜂鸣器      
    }
  }
  

  if(cmd & 0x20){                                               // LED2灯控制位：bit5
    LED2 = ON;                                                  // 开启LED2
  }
  else{
    LED2 = OFF;                                                 // 关闭LED2        
  }
  if(cmd & 0x10){                                               // LED1灯控制位：bit4
    LED1 = ON;                                                  // 开启LED1
  }
  else{
    LED1 = OFF;                                                 // 关闭LED1
  }
  
  if(cmd & 0x80){                                               // 继电器2控制位：bit7
    RELAY2 = ON;                                                // 开启继电器2
  }
  else{
    RELAY2 = OFF;                                               // 关闭继电器2        
  }
  if(cmd & 0x40){                                               // 继电器1控制位：bit6
    RELAY1 = ON;                                                // 开启继电器1
  }
  else{
    RELAY1 = OFF;                                               // 关闭继电器1
  }
}
/*********************************************************************************************
* 名称：ZXBeeUserProcess()
* 功能：解析收到的控制命令
* 参数：*ptag -- 控制命令名称
*       *pval -- 控制命令参数
* 返回：ret -- p字符串长度
* 修改：
* 注释：
*********************************************************************************************/
int ZXBeeUserProcess(char *ptag, char *pval)
{ 
  int val;
  int ret = 0;	
  char pData[16];
  char *p = pData;
  
  // 将字符串变量pval解析转换为整型变量赋值
  val = atoi(pval);	
  // 控制命令解析
  if (0 == strcmp("CD0", ptag)){                                // 对D0的位进行操作，CD0表示位清零操作
    D0 &= ~val;
    ret = sprintf(p, "%u", D0);
    ZXBeeAdd("D0", p);
  }
  if (0 == strcmp("OD0", ptag)){                                // 对D0的位进行操作，OD0表示位置一操作
    D0 |= val;
    ret = sprintf(p, "%u", D0);
    ZXBeeAdd("D0", p);
  }
  if (0 == strcmp("D0", ptag)){                                 // 查询上报使能编码
    if (0 == strcmp("?", pval)){
      ret = sprintf(p, "%u", D0);
      ZXBeeAdd("D0", p);
    } 
  }
  if (0 == strcmp("CD1", ptag)){                                // 对D1的位进行操作，CD1表示位清零操作
    D1 &= ~val;
    sensorControl(D1);                                          // 处理执行命令
    ret = sprintf(p, "%u", D1);
    ZXBeeAdd("D1", p);
  }
  if (0 == strcmp("OD1", ptag)){                                // 对D1的位进行操作，OD1表示位置一操作
    D1 |= val;
    sensorControl(D1);                                          // 处理执行命令
    ret = sprintf(p, "%u", D1);
    ZXBeeAdd("D1", p);
  }
  if (0 == strcmp("D1", ptag)){                                 // 查询执行器命令编码
    if (0 == strcmp("?", pval)){
      ret = sprintf(p, "%u", D1);
      ZXBeeAdd("D1", p);
    } 
  }
  if (0 == strcmp("V0", ptag)){
    if (0 == strcmp("?", pval)){
      ret = sprintf(p, "%u", V0);                         	// 上报时间间隔
      ZXBeeAdd("V0", p);
    }else{
      updateV0(pval);
    }
  } 
  return ret;
}
/*********************************************************************************************
* 名称：MyEventProcess()
* 功能：自定义事件处理
* 参数：event -- 事件编号
* 返回：无
* 修改：
* 注释：
*********************************************************************************************/
void MyEventProcess( uint16 event )
{
  if (event & MY_REPORT_EVT) { 
    sensorUpdate();
    //启动定时器，触发事件：MY_REPORT_EVT 
    osal_start_timerEx(sapi_TaskID, MY_REPORT_EVT, V0*1000);
  }  
}
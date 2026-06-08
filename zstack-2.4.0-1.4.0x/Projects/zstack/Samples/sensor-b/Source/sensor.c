/*********************************************************************************************
* 文件：sensor.c
* 作者：Xuzhy 2018.5.16
* 说明：xLab Sensor-B传感器应用程序（C同学：控制模块与联动逻辑）
* 修改：2026.06 补全联动逻辑、RGB真彩色、蜂鸣器定时、继电器自动回弹
* 修改：2026.06 修复8个review问题：事件冲突/V0溢出/定时器竞争/RGB覆盖/蜂鸣器静音/state desync/D1兼容/重复初始化
* 注释：智能门禁访客管理系统 - 控制执行节点(602)
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
#include "beep.h"
#include "relay.h"
#include "zxbee.h"
#include "zxbee-inf.h"
#include <ioCC2530.h>

/*********************************************************************************************
* 宏定义 - 告警等级
*********************************************************************************************/
#define ALERT_SAFE      0                                       // 安全状态
#define ALERT_ATTENTION 1                                       // 注意状态（有人徘徊等）
#define ALERT_ALARM     2                                       // 报警状态（夜间入侵等）

/* 修复#2: V0最大值限制，避免 V0*1000 在16位int上溢出 */
#define V0_MAX          65                                      // 65*1000=65000 < 65535

/*********************************************************************************************
* 变量
*********************************************************************************************/
static uint8  D0 = 0xFF;                                        // D0传感器数据主动上报使能
static uint8  D1 = 0;                                           // 修复#7: 恢复D1变量，兼容旧协议
static uint16 V0 = 30;                                          // V0数据上报周期，默认30s

/* RGB三通道状态（0-255，>0表示点亮） */
static uint8  rgb_r = 0;                                        // 红色通道值
static uint8  rgb_g = 0;                                        // 绿色通道值
static uint8  rgb_b = 0;                                        // 蓝色通道值

/* 蜂鸣器状态 */
static uint8  buzz_mode = 0;                                    // 0=关闭 1=长响 2=短响(定时)

/* 继电器/门锁状态 */
static uint8  unlock_state = 0;                                 // 0=锁定 1=解锁
static uint8  relay_auto_flag = 0;                              // 继电器自动关闭标志

/* 告警状态 */
static uint8  alert_level = ALERT_SAFE;                         // 当前告警等级

/* 门铃状态 */
static uint8  doorbell_active = 0;                              // 门铃模式激活标志
static uint32 doorbell_start_time = 0;                          // 修复#9: 门铃触发时间戳，用于5秒蓝灯超时

/* 修复#5: 蜂鸣器用户静音标志（允许在告警模式下单独静音蜂鸣器） */
static uint8  buzz_user_muted = 0;                              // 0=未静音 1=用户手动静音

/*********************************************************************************************
* 名称：rgbSet()
* 功能：直接设置RGB灯三通道
* 参数：r - 红色值(0-255, >0点亮)
*       g - 绿色值(0-255, >0点亮)
*       b - 蓝色值(0-255, >0点亮)
* 返回：无
* 修改：
* 注释：CC2530无PWM，>0即点亮，==0即关闭
*********************************************************************************************/
static void rgbSet(uint8 r, uint8 g, uint8 b)
{
  rgb_r = r;
  rgb_g = g;
  rgb_b = b;

  /* 控制红色通道 */
  if (r > 0) {
    RGB_R = ON;
  } else {
    RGB_R = OFF;
  }

  /* 控制绿色通道 */
  if (g > 0) {
    RGB_G = ON;
  } else {
    RGB_G = OFF;
  }

  /* 控制蓝色通道 */
  if (b > 0) {
    RGB_B = ON;
  } else {
    RGB_B = OFF;
  }
}

/*********************************************************************************************
* 名称：buzzerOn()
* 功能：开启蜂鸣器
* 参数：无
* 返回：无
* 修改：
* 注释：
*********************************************************************************************/
static void buzzerOn(void)
{
  BEEPIO = ON;                                                  // 蜂鸣器响（ON=0，低电平触发）
}

/*********************************************************************************************
* 名称：buzzerOff()
* 功能：关闭蜂鸣器
* 参数：无
* 返回：无
* 修改：
* 注释：
*********************************************************************************************/
static void buzzerOff(void)
{
  BEEPIO = OFF;                                                 // 蜂鸣器停
  buzz_mode = 0;                                                // 清除蜂鸣器模式
}

/*********************************************************************************************
* 名称：buzzerShort()
* 功能：蜂鸣器短响指定时间后自动关闭
* 参数：ms - 响声持续时间(毫秒)
* 返回：无
* 修改：
* 注释：使用osal_start_timerEx实现定时关闭
*********************************************************************************************/
static void buzzerShort(uint16 ms)
{
  buzz_mode = 2;                                                // 标记为短响模式
  buzzerOn();                                                   // 开启蜂鸣器
  osal_start_timerEx(sapi_TaskID, MY_BUZZER_EVT, ms);           // 启动定时关闭
}

/*********************************************************************************************
* 名称：relayUnlock()
* 功能：远程开门（继电器打开 + 自动3秒后关闭）
* 参数：无
* 返回：无
* 修改：
* 注释：unlock=1时调用，开门后RGB绿灯亮
*********************************************************************************************/
static void relayUnlock(void)
{
  unlock_state = 1;                                             // 标记为解锁状态
  RELAY1 = ON;                                                  // 继电器1打开（模拟开锁）
  relay_auto_flag = 1;                                          // 标记需要自动关闭
  osal_start_timerEx(sapi_TaskID, MY_RELAY_EVT, 3000);          // 3秒后自动关闭
}

/*********************************************************************************************
* 名称：relayLock()
* 功能：关门/锁定（继电器关闭）
* 参数：无
* 返回：无
* 修改：
* 注释：
*********************************************************************************************/
static void relayLock(void)
{
  unlock_state = 0;                                             // 标记为锁定状态
  RELAY1 = OFF;                                                 // 继电器1关闭
  relay_auto_flag = 0;                                          // 清除自动关闭标志
}

/*********************************************************************************************
* 名称：doorbellTrigger()
* 功能：门铃触发（蜂鸣器短响500ms + RGB蓝灯亮5秒）
* 参数：无
* 返回：无
* 修改：修复#1 - 不再使用MY_DOORBELL_EVT(0x0010)，改用MY_BUZZER_EVT+doorbell_active标志
* 注释：门铃蓝灯超时由MY_BUZZER_EVT处理（buzzerShort 500ms结束后检查doorbell_active）
*********************************************************************************************/
static void doorbellTrigger(void)
{
  doorbell_active = 1;                                          // 标记门铃激活
  doorbell_start_time = osal_GetSystemClock();                  // 修复#9: 记录触发时间戳
  buzzerShort(500);                                             // 蜂鸣器短响500ms（同时启动MY_BUZZER_EVT）
  rgbSet(0, 0, 255);                                            // RGB蓝灯亮
}

/*********************************************************************************************
* 名称：updateV0()
* 功能：更新V0的值（带上限保护）
* 参数：*val -- 指向存放数据的指针
* 返回：
* 修改：修复#2 - 限制V0最大值为65，避免V0*1000在16位int上溢出
* 注释：CC2530的int为16位，65*1000=65000 < 65535
*********************************************************************************************/
void updateV0(char *val)
{
  uint16 tmp = (uint16)atoi(val);                               // 将字符串变量转换为整型
  if (tmp > V0_MAX) {
    tmp = V0_MAX;                                               // 限制最大值，避免溢出
  }
  V0 = tmp;
}

/*********************************************************************************************
* 名称：sensorInit()
* 功能：初始化传感器及定时器
* 参数：
* 返回：
* 修改：修复#8 - 删除重复的P0_3手动初始化，仅保留beep_init()
* 注释：门禁项目使用mode=2（RGB+蜂鸣器）
*********************************************************************************************/
void sensorInit(void)
{
  ZXBeeInfSetLocalAddr(ZXBEE_ADDR_SENSOR_B);                    // 设置节点地址为0x02

  /* 传感器初始化（各驱动内部已完成GPIO配置） */
  rgb_init();                                                    // RGB灯初始化
  beep_init();                                                   // 蜂鸣器初始化（内部配置P0_3）
  led_init();                                                    // LED灯初始化
  relay_init();                                                  // 继电器初始化

  /* 初始状态：绿灯亮表示系统正常 */
  rgbSet(0, 255, 0);                                             // RGB绿灯

  /* 启动定时上报 */
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
  sensorUpdate();                                                // 入网后立即上报一次
}

/*********************************************************************************************
* 名称：sensorUpdate()
* 功能：定时上报传感器控制状态
* 参数：
* 返回：
* 修改：
* 注释：上报格式符合项目规范：{"unlock":0,"buzz":0,"rgb":[0,255,0]}
*********************************************************************************************/
void sensorUpdate(void)
{
  char pData[64];
  char *p = pData;

  ZXBeeBegin();                                                  // 智云数据帧头

  /* 上报门锁状态 */
  sprintf(p, "%u", unlock_state);
  ZXBeeAdd("unlock", p);

  /* 上报蜂鸣器状态 */
  sprintf(p, "%u", buzz_mode);
  ZXBeeAdd("buzz", p);

  /* 上报RGB状态（数组格式） */
  sprintf(p, "[%u,%u,%u]", rgb_r, rgb_g, rgb_b);
  ZXBeeAdd("rgb", p);

  /* 上报告警等级 */
  sprintf(p, "%u", alert_level);
  ZXBeeAdd("alert", p);

  p = ZXBeeEnd();                                                // 智云数据帧尾
  if (p != NULL) {
    ZXBeeInfSend(p, strlen(p));                                  // 通过ZigBee发送
  }
}

/*********************************************************************************************
* 名称：sensorCheck()
* 功能：周期性检测状态变化
* 参数：
* 返回：
* 修改：修复#4 - 仅在告警模式下覆盖RGB，安全模式不覆盖远程设置
*       修复#5 - 告警模式下尊重用户静音标志
* 注释：每100ms调用一次，管理告警状态机
*********************************************************************************************/
void sensorCheck(void)
{
  /* 门铃蓝灯5秒超时检查 */
  if (doorbell_active) {
    if (osal_GetSystemClock() - doorbell_start_time >= 5000) {
      doorbell_active = 0;                                       // 5秒后清除门铃标志
      /* 恢复告警等级对应颜色 */
      switch (alert_level) {
        case ALERT_SAFE:      rgbSet(0, 255, 0); break;         // 绿灯
        case ALERT_ATTENTION: rgbSet(0, 0, 255); break;         // 蓝灯
        case ALERT_ALARM:     rgbSet(255, 0, 0); break;         // 红灯
      }
    }
  }

  /* 短响模式期间不做告警处理 */
  if (buzz_mode == 2) return;

  /* 根据告警等级维护蜂鸣器状态（始终执行，不被门铃抑制） */
  switch (alert_level) {
    case ALERT_SAFE:
      /* 安全模式下不主动覆盖RGB，允许远程rgb命令生效 */
      break;

    case ALERT_ATTENTION:
      /* 门铃期间不覆盖蓝灯（已经是蓝色），非门铃时设置蓝灯 */
      if (!doorbell_active) {
        if (!(rgb_r == 0 && rgb_g == 0 && rgb_b == 255)) {
          rgbSet(0, 0, 255);                                     // 蓝灯
        }
      }
      break;

    case ALERT_ALARM:
      /* 门铃期间不覆盖蓝灯，但蜂鸣器告警始终生效（安全优先） */
      if (!doorbell_active) {
        if (!(rgb_r == 255 && rgb_g == 0 && rgb_b == 0)) {
          rgbSet(255, 0, 0);                                     // 红灯
        }
      }
      /* 蜂鸣器告警不受门铃抑制（安全优先） */
      if (buzz_mode == 0 && !buzz_user_muted) {
        buzzerOn();                                              // 蜂鸣器长响
        buzz_mode = 1;
      }
      break;
  }
}

/*********************************************************************************************
* 名称：syncD1()
* 功能：从当前状态变量重建D1位掩码（修复: 保持D1与高级命令同步）
* 参数：无
* 返回：无
* 修改：
* 注释：D1位定义: bit0=R, bit1=G, bit2=B, bit3=蜂鸣器, bit4=LED1, bit5=LED2, bit6=继电器1, bit7=继电器2
*********************************************************************************************/
static void syncD1(void)
{
  D1 = 0;
  if (rgb_r > 0) D1 |= 0x01;
  if (rgb_g > 0) D1 |= 0x02;
  if (rgb_b > 0) D1 |= 0x04;
  if (buzz_mode > 0) D1 |= 0x08;
  if (unlock_state) D1 |= 0x40;
}

/*********************************************************************************************
* 名称：sensorControl()
* 功能：根据D1位控制硬件（兼容旧协议）
* 参数：cmd - 控制命令（位操作）
* 返回：
* 修改：修复#6 - 操作GPIO的同时同步更新状态变量
* 注释：保留D0/D1位操作兼容，门禁项目主要通过ZXBeeUserProcess控制
*********************************************************************************************/
void sensorControl(uint8 cmd)
{
  /* 修复#6: 同步更新状态变量，避免GPIO/state desync */
  D1 = cmd;                                                      // 更新D1状态

  /* RGB控制 - 同步rgb_r/g/b */
  if (cmd & 0x01) {
    RGB_R = ON; rgb_r = 255;
  } else {
    RGB_R = OFF; rgb_r = 0;
  }
  if (cmd & 0x02) {
    RGB_G = ON; rgb_g = 255;
  } else {
    RGB_G = OFF; rgb_g = 0;
  }
  if (cmd & 0x04) {
    RGB_B = ON; rgb_b = 255;
  } else {
    RGB_B = OFF; rgb_b = 0;
  }

  /* 蜂鸣器控制 - 同步buzz_mode */
  if (cmd & 0x08) {
    BEEPIO = ON; buzz_mode = 1;
  } else {
    BEEPIO = OFF; buzz_mode = 0;
  }

  /* LED控制 */
  if (cmd & 0x10) {
    LED1 = ON;
  } else {
    LED1 = OFF;
  }
  if (cmd & 0x20) {
    LED2 = ON;
  } else {
    LED2 = OFF;
  }

  /* 继电器控制 - 修复: 使用relayUnlock()确保3秒安全定时器 */
  if (cmd & 0x40) {
    relayUnlock();                                               // 包含自动关闭定时器
  } else {
    relayLock();
  }
  if (cmd & 0x80) {
    RELAY2 = ON;
  } else {
    RELAY2 = OFF;
  }
}

/*********************************************************************************************
* 名称：sensorSetRgb()
* 功能：解析RGB数组字符串并设置RGB灯
* 参数：*val -- RGB数据，格式为[r,g,b]，如[0,255,0]
* 返回：无
* 修改：
* 注释：支持[255,0,0]、[0,255,0]、[0,0,255]等真彩色
*********************************************************************************************/
static void sensorSetRgb(char *val)
{
  int r = 0, g = 0, b = 0;

  if (val == NULL) return;

  /* 解析 [R,G,B] 格式 */
  if (val[0] == '[') {
    sscanf(val, "[%d,%d,%d]", &r, &g, &b);
  } else {
    /* 兼容旧格式：单个数字表示颜色索引 */
    int idx = atoi(val);
    switch (idx) {
      case 1: r = 255; break;                                    // 红
      case 2: g = 255; break;                                    // 绿
      case 3: b = 255; break;                                    // 蓝
      default: break;
    }
  }

  /* 限制范围 0-255 */
  if (r < 0) r = 0; if (r > 255) r = 255;
  if (g < 0) g = 0; if (g > 255) g = 255;
  if (b < 0) b = 0; if (b > 255) b = 255;

  rgbSet((uint8)r, (uint8)g, (uint8)b);                         // 设置RGB灯
}

/*********************************************************************************************
* 名称：ZXBeeUserProcess()
* 功能：处理下行控制命令
* 参数：*ptag -- 标签（字段名）
*       *pval -- 数据（字段值）
* 返回：ret -- 数据长度
* 修改：修复#3 - buzz=1时取消pending的buzzerShort定时器
*       修复#5 - buzz=0时设置用户静音标志
*       修复#7 - 恢复CD1/OD1/D1处理，桥接到新状态变量
* 注释：支持 unlock/buzz/rgb/reset 等门禁控制字段
*********************************************************************************************/
int ZXBeeUserProcess(char *ptag, char *pval)
{
  int val;
  int ret = 0;
  char pData[64];
  char *p = pData;

  val = atoi(pval);                                              // 字符串转整型

  /* ========== 远程开门 ========== */
  if (0 == strcmp("unlock", ptag)) {
    if (val) {
      relayUnlock();                                             // 开门 + 3秒自动关门
      buzzerOff();                                               // 修复: 关闭告警蜂鸣器
      alert_level = ALERT_SAFE;                                  // 开门后恢复正常
      rgbSet(0, 255, 0);                                         // 绿灯表示开门成功
    } else {
      relayLock();                                               // 手动关门
    }
    sprintf(p, "%u", unlock_state);
    ZXBeeAdd("unlock", p);
    syncD1();                                                    // 同步D1
    ret = 1;
  }

  /* ========== 蜂鸣器控制 ========== */
  if (0 == strcmp("buzz", ptag)) {
    if (val == 0) {
      /* 修复#5: 设置用户静音标志，防止sensorCheck()在告警模式下重新触发 */
      buzz_user_muted = 1;
      buzzerOff();
    } else if (val == 500) {
      /* 短响500ms + 蓝灯（门铃模式） */
      buzz_user_muted = 0;                                       // 门铃触发时清除静音
      doorbellTrigger();
    } else if (val == 1) {
      /* 修复#3: 长响前取消pending的buzzerShort定时器，避免提前关闭 */
      osal_stop_timerEx(sapi_TaskID, MY_BUZZER_EVT);
      buzz_user_muted = 0;                                       // 长响时清除静音
      buzz_mode = 1;
      buzzerOn();
    } else {
      /* 其他值：短响指定毫秒 */
      buzz_user_muted = 0;                                       // 短响时清除静音
      buzzerShort((uint16)val);
    }
    sprintf(p, "%u", buzz_mode);
    ZXBeeAdd("buzz", p);
    syncD1();                                                    // 同步D1
    ret = 1;
  }

  /* ========== RGB灯控制 ========== */
  if (0 == strcmp("rgb", ptag)) {
    sensorSetRgb(pval);                                          // 解析并设置RGB
    sprintf(p, "[%u,%u,%u]", rgb_r, rgb_g, rgb_b);
    ZXBeeAdd("rgb", p);
    syncD1();                                                    // 同步D1
    ret = 1;
  }

  /* ========== 告警解除/系统复位 ========== */
  if (0 == strcmp("reset", ptag)) {
    if (val) {
      alert_level = ALERT_SAFE;                                  // 恢复安全等级
      buzzerOff();                                               // 关闭蜂鸣器
      buzz_user_muted = 0;                                       // 清除静音标志
      doorbell_active = 0;                                       // 清除门铃状态
      relay_auto_flag = 0;                                       // 清除继电器自动标志
      if (unlock_state) {
        relayLock();                                              // 关门
      }
      rgbSet(0, 255, 0);                                         // 绿灯表示安全
    }
    sprintf(p, "%u", 1);
    ZXBeeAdd("reset", p);
    syncD1();                                                    // 同步D1
    ret = 1;
  }

  /* ========== 告警等级设置（来自sensor-c或Python） ========== */
  if (0 == strcmp("alert", ptag)) {
    /* 修复: 范围校验，只接受0/1/2 */
    if (val < 0) val = 0;
    if (val > ALERT_ALARM) val = ALERT_ALARM;
    alert_level = (uint8)val;
    buzz_user_muted = 0;                                         // 切换告警等级时清除静音
    /* 修复: 根据新等级立即更新硬件状态 */
    switch (alert_level) {
      case ALERT_SAFE:
        buzzerOff();                                             // 关闭蜂鸣器
        rgbSet(0, 255, 0);                                       // 绿灯
        break;
      case ALERT_ATTENTION:
        buzzerOff();                                             // 关闭蜂鸣器
        rgbSet(0, 0, 255);                                       // 蓝灯
        break;
      case ALERT_ALARM:
        rgbSet(255, 0, 0);                                       // 红灯
        if (buzz_mode == 0) { buzzerOn(); buzz_mode = 1; }      // 蜂鸣器响
        break;
    }
    sprintf(p, "%u", alert_level);
    ZXBeeAdd("alert", p);
    ret = 1;
  }

  /* ========== 修复#7: D1系统命令兼容（桥接到新状态变量） ========== */
  if (0 == strcmp("CD0", ptag)) {
    D0 &= ~val;
    ret = sprintf(p, "%u", D0);
    ZXBeeAdd("D0", p);
  }
  if (0 == strcmp("OD0", ptag)) {
    D0 |= val;
    ret = sprintf(p, "%u", D0);
    ZXBeeAdd("D0", p);
  }
  if (0 == strcmp("D0", ptag)) {
    if (0 == strcmp("?", pval)) {
      ret = sprintf(p, "%u", D0);
      ZXBeeAdd("D0", p);
    }
  }
  /* 修复#7: 恢复CD1/OD1/D1处理，通过sensorControl()桥接到状态变量 */
  if (0 == strcmp("CD1", ptag)) {
    D1 &= ~val;
    sensorControl(D1);                                           // 同步更新GPIO+状态变量
    ret = sprintf(p, "%u", D1);
    ZXBeeAdd("D1", p);
  }
  if (0 == strcmp("OD1", ptag)) {
    D1 |= val;
    sensorControl(D1);                                           // 同步更新GPIO+状态变量
    ret = sprintf(p, "%u", D1);
    ZXBeeAdd("D1", p);
  }
  if (0 == strcmp("D1", ptag)) {
    if (0 == strcmp("?", pval)) {
      ret = sprintf(p, "%u", D1);
      ZXBeeAdd("D1", p);
    }
  }

  /* ========== V0上报周期查询/设置 ========== */
  if (0 == strcmp("V0", ptag)) {
    if (0 == strcmp("?", pval)) {
      ret = sprintf(p, "%u", V0);
      ZXBeeAdd("V0", p);
    } else {
      updateV0(pval);
    }
  }

  return ret;
}

/*********************************************************************************************
* 名称：MyEventProcess()
* 功能：事件处理函数
* 参数：event -- 事件标志
* 返回：
* 修改：修复#1 - 删除MY_DOORBELL_EVT处理，门铃超时复用MY_BUZZER_EVT
*       修复#2 - V0*1000改为(uint32)V0*1000并截断
* 注释：处理定时上报、状态检测、蜂鸣器定时、继电器定时
*********************************************************************************************/
void MyEventProcess(uint16 event)
{
  /* 首次启动检测定时器 */
  static char check_start_flag = 0;
  if (check_start_flag == 0) {
    if (event & MY_REPORT_EVT) {
      osal_start_timerEx(sapi_TaskID, MY_CHECK_EVT, 100);       // 启动100ms检测
      check_start_flag = 1;
    }
  }

  /* ========== 定时上报事件 ========== */
  if (event & MY_REPORT_EVT) {
    sensorUpdate();                                              // 上报当前状态
    /* 修复#2: 使用uint32中间结果避免V0*1000溢出，再截断为uint16传给osal */
    {
      uint32 period = (uint32)V0 * 1000;
      if (period > 65535) period = 65535;                        // osal_start_timerEx参数为uint16
      osal_start_timerEx(sapi_TaskID, MY_REPORT_EVT, (uint16)period);
    }
  }

  /* ========== 状态检测事件（每100ms） ========== */
  if (event & MY_CHECK_EVT) {
    sensorCheck();                                               // 周期性状态检测
    osal_start_timerEx(sapi_TaskID, MY_CHECK_EVT, 100);         // 100ms后再检测
  }

  /* ========== 蜂鸣器定时关闭事件 ========== */
  if (event & MY_BUZZER_EVT) {
    buzzerOff();                                                 // 关闭蜂鸣器

    /* 修复#9: 门铃蓝灯超时由sensorCheck()时间戳管理，此处不处理doorbell_active。
       蜂鸣器关闭后，如果门铃仍激活则保持蓝灯不动，sensorCheck()会在5秒后恢复 */
    if (!doorbell_active && alert_level == ALERT_SAFE) {
      /* 非门铃场景，安全模式下恢复绿灯 */
      rgbSet(0, 255, 0);
    }
  }

  /* ========== 继电器自动关闭事件（开门3秒后） ========== */
  if (event & MY_RELAY_EVT) {
    if (relay_auto_flag) {
      relayLock();                                               // 自动关门
      /* 开门结束后，安全模式下恢复绿灯 */
      if (alert_level == ALERT_SAFE) {
        rgbSet(0, 255, 0);                                       // 恢复绿灯
      }
    }
  }
}

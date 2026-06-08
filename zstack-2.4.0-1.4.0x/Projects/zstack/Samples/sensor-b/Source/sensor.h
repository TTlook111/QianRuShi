/*********************************************************************************************
* 文件：sensor.h
* 作者：Xuzhy 2018.5.16
* 说明：sensor头文件
* 修改：
* 注释：
*********************************************************************************************/
#ifndef SENSOR_H
#define SENSOR_H
/*********************************************************************************************
* 宏定义
*********************************************************************************************/
#define MY_REPORT_EVT           0x0001
#define MY_CHECK_EVT           	0x0002
#define MY_BUZZER_EVT           0x0004                          // 蜂鸣器/门铃定时关闭事件（复用）
#define MY_RELAY_EVT            0x0008                          // 继电器自动关闭事件
/* 修复#1: 删除MY_DOORBELL_EVT(0x0010)，因其与AppCommon.c的__START_EVT冲突
   且被0x000F转发掩码排除。门铃超时改由MY_BUZZER_EVT+doorbell_active标志处理 */
#define NODE_NAME               "602"                           // 节点类型
#define NODE_CATEGORY           1
#define NODE_TYPE               NODE_ENDDEVICE                  // 路由节点：NODE_ROUTER  终端节点：NODE_ENDDEVICE
/*********************************************************************************************
* 函数原型
*********************************************************************************************/
extern void sensorInit(void);
extern void sensorLinkOn(void);
extern void sensorUpdate(void);
extern void sensorCheck(void);
extern void sensorControl(uint8 cmd);
extern void MyEventProcess( uint16 event );

#endif // SENSOR_H
/*********************************************************************************************
* 文件：security_logic.h
* 作者：A同学 (安防检测模块)
* 说明：智能门禁安防逻辑模块 — 头文件
* 修改：[新增] 告警分级、PIR徘徊检测、夜间判断、夜间开门告警
* 注释：在 sensor.c 的 update 函数中调用，为上报 Payload 提供业务字段
*********************************************************************************************/

#ifndef __SECURITY_LOGIC_H__
#define __SECURITY_LOGIC_H__

/*********************************************************************************************
* 头文件
*********************************************************************************************/
#include "hal_types.h"

/*********************************************************************************************
* 宏定义
*********************************************************************************************/

/* 告警级别 */
#define ALERT_NONE      0    /* 正常，无告警 */
#define ALERT_WARNING   1    /* 注意：徘徊/可疑行为 */
#define ALERT_ALARM     2    /* 报警：夜间入侵/破坏 */

/* 时间阈值 (单位: 秒) */
#define LOITER_THRESHOLD    10   /* PIR 持续触发超过此值 → 徘徊告警 */
#define NIGHT_START_HOUR    22   /* 夜间开始 (22:00) */
#define NIGHT_END_HOUR      6    /* 夜间结束 (06:00) */

/* 事件类型 */
#define EVENT_NONE          0    /* 无事件 */
#define EVENT_DOORBELL      1    /* 门铃按下 */
#define EVENT_LOITER        2    /* 徘徊检测 */
#define EVENT_DOOR_OPEN     3    /* 门打开 */
#define EVENT_NIGHT_INTRUDE 4    /* 夜间入侵 */
#define EVENT_VIBRATION     5    /* 震动/破坏 */

/*********************************************************************************************
* 类型定义
*********************************************************************************************/

/**
 * 安防状态快照
 * 每次 update 后更新，供 sensorUpdate/sensorCheck 读取并组包
 */
typedef struct {
    uint8  alert;           /* 告警级别: 0=正常 1=注意 2=报警 */
    uint16 stay;            /* PIR 持续触发时间 (秒) */
    uint8  night;           /* 是否夜间: 0=白天 1=夜间 (22:00-06:00) */
    uint8  event;           /* 当前事件类型 (EVENT_xxx) */
    uint8  event_changed;   /* 自上次读取后事件是否变化 (1=有变化) */
} security_state_t;

/*********************************************************************************************
* 内部原型函数
*********************************************************************************************/

/**
 * @brief  初始化安防逻辑模块
 * @note   清零内部状态，应在 sensorInit() 中调用
 */
void security_logic_init(void);

/**
 * @brief  更新安防逻辑 (每个检测周期调用一次)
 * @param  pir     - 当前 PIR 状态 (0=无人, 1=有人)
 * @param  tch     - 当前门铃/触摸状态 (0=未按, 1=按下)
 * @param  door    - 当前门状态 (0=关闭, 1=打开)
 * @param  sys_clk - 系统时钟 (osal_GetSystemClock() 返回值, ms)
 * @note   每次 sensorUpdate/sensorCheck 周期调用
 *         内部自动计时、判断告警级别、识别事件类型
 */
void security_logic_update(uint8 pir, uint8 tch, uint8 door, uint32 sys_clk);

/**
 * @brief  获取当前安防状态快照
 * @return security_state_t 指针 (内部静态变量, 不可 free)
 */
const security_state_t* security_logic_get_state(void);

/**
 * @brief  确认/消费事件变化标志
 * @note   调用后 event_changed 复位为 0, 下次事件变化时再置 1
 */
void security_logic_ack_event(void);

/**
 * @brief  手动设置系统时间 (供外部校时使用)
 * @param  hour   - 小时 (0-23)
 * @param  minute - 分钟 (0-59)
 */
void security_logic_set_time(uint8 hour, uint8 minute);

/**
 * @brief  重置告警 (远程解除告警时调用)
 * @note   将 alert 清零, 事件清零
 */
void security_logic_reset_alert(void);

/**
 * @brief  设置是否为夜间模式
 * @param  night - 0=白天, 1=夜间
 * @note   供外部手动切换, 也可通过 set_time 自动判断
 */
void security_logic_set_night(uint8 night);

#endif /* __SECURITY_LOGIC_H__ */

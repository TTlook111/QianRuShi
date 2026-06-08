/*********************************************************************************************
* 文件：security_logic.c
* 作者：A同学 (安防检测模块)
* 说明：智能门禁安防逻辑模块 — 实现
* 修改：[新增] 告警分级、PIR徘徊检测、夜间判断、夜间开门告警
* 注释：
*
*   告警判定规则:
*   ┌──────────┬────────────────────────────────────┬──────────┐
*   │ 告警级别  │             触发条件                │  事  件   │
*   ├──────────┼────────────────────────────────────┼──────────┤
*   │ ALERT_1  │ PIR 持续触发 >10s 且未按门铃       │ 徘徊检测  │
*   │ (注意)   │ 震动/敲门 未授权                    │ 可疑行为  │
*   ├──────────┼────────────────────────────────────┼──────────┤
*   │ ALERT_2  │ 夜间 (22:00-06:00) + 门被打开      │ 夜间入侵  │
*   │ (报警)   │ PIR + 震动同时触发 (暴力破坏)       │ 入侵告警  │
*   └──────────┴────────────────────────────────────┴──────────┘
*
*   数据流:
*   updateA0/A1/A2 (读取传感器)
*       → security_logic_update(pir, tch, door, clk)  ← 每个周期调用
*           → 内部计时/判断
*       → security_logic_get_state()
*           → 读取 alert/stay/night/event
*       → sensorUpdate/sensorCheck (组包上报)
*********************************************************************************************/

/*********************************************************************************************
* 头文件
*********************************************************************************************/
#include "security_logic.h"
#include "OSAL.h"            /* osal_GetSystemClock() */

/*********************************************************************************************
* 全局变量
*********************************************************************************************/

/* 安防状态快照 */
static security_state_t g_sec_state = {
    0,    /* alert   = 正常 */
    0,    /* stay    = 0秒 */
    0,    /* night   = 白天 */
    0,    /* event   = 无事件 */
    0     /* event_changed = 无变化 */
};

/* 内部跟踪变量 */
static uint32 g_pir_start_ms  = 0;     /* PIR 首次触发的系统时钟 (ms) */
static uint8  g_pir_triggered = 0;     /* 当前 PIR 是否正在触发 */
static uint8  g_doorbell_ever = 0;     /* 本周期内是否有人按过门铃 */
static uint8  g_last_pir       = 0;    /* 上一周期的 PIR 值 */
static uint8  g_last_tch       = 0;    /* 上一周期的门铃值 */
static uint8  g_last_door      = 0;    /* 上一周期的门状态 */
static uint8  g_last_event     = 0;    /* 上一周期的事件类型 */

/* 时间跟踪 */
static uint8  g_sys_hour  = 12;        /* 系统小时 (默认 12:00) */
static uint8  g_sys_min   = 0;         /* 系统分钟 */
static uint8  g_night_override = 0;    /* 手动夜间模式: 0=自动, 1=强制夜间 */
static uint8  g_night_manual  = 0;    /* 手动设置值 */

/*********************************************************************************************
* ===== 内部辅助函数 =====
*********************************************************************************************/

/*********************************************************************************************
* 名称：_is_night_time()
* 功能：判断当前时间是否在夜间窗口
* 参数：无
* 返回：1=夜间, 0=白天
* 修改：
* 注释：夜间定义: 22:00 ~ 次日 06:00
*********************************************************************************************/
static uint8 _is_night_time(void)
{
    if (g_night_override) {
        return g_night_manual;       /* 手动模式 */
    }
    /* 自动判断: 22:00-23:59 或 00:00-05:59 */
    if (g_sys_hour >= NIGHT_START_HOUR || g_sys_hour < NIGHT_END_HOUR) {
        return 1;
    }
    return 0;
}

/*********************************************************************************************
* 名称：_determine_event()
* 功能：根据传感器状态和内部跟踪变量判定当前事件类型
* 参数：pir  - PIR 状态
*       tch  - 门铃状态
*       door - 门状态
* 返回：事件类型 (EVENT_xxx)
* 修改：
*********************************************************************************************/
static uint8 _determine_event(uint8 pir, uint8 tch, uint8 door)
{
    uint8 is_night = _is_night_time();

    /* 优先级从高到低判断 */

    /* 1. 夜间开门 → 最高告警 */
    if (is_night && door == 1 && g_last_door == 0) {
        return EVENT_NIGHT_INTRUDE;
    }

    /* 2. PIR 持续触发超过阈值 + 未按门铃 → 徘徊 */
    if (g_pir_triggered && !g_doorbell_ever &&
        g_sec_state.stay >= LOITER_THRESHOLD) {
        return EVENT_LOITER;
    }

    /* 3. 门铃按下 */
    if (tch == 1 && g_last_tch == 0) {
        return EVENT_DOORBELL;
    }

    /* 4. 门被打开 */
    if (door == 1 && g_last_door == 0) {
        return EVENT_DOOR_OPEN;
    }

    /* 5. 无事件 */
    return EVENT_NONE;
}

/*********************************************************************************************
* 名称：_calc_alert_level()
* 功能：根据事件类型计算告警级别
* 参数：event - 事件类型
*       pir   - PIR 状态
*       tch   - 门铃状态
* 返回：ALERT_NONE / ALERT_WARNING / ALERT_ALARM
* 修改：
*********************************************************************************************/
static uint8 _calc_alert_level(uint8 event, uint8 pir, uint8 tch)
{
    switch (event) {
        case EVENT_NIGHT_INTRUDE:
            return ALERT_ALARM;            /* 夜间入侵 → 报警 */

        case EVENT_LOITER:
            return ALERT_WARNING;          /* 徘徊 → 注意 */

        case EVENT_DOORBELL:
            return ALERT_NONE;             /* 门铃是正常事件 */

        case EVENT_DOOR_OPEN:
            if (_is_night_time()) {
                return ALERT_ALARM;        /* 夜间开门 → 报警 */
            }
            return ALERT_NONE;             /* 白天开门 → 正常 */

        case EVENT_VIBRATION:
            if (_is_night_time()) {
                return ALERT_WARNING;      /* 夜间震动 → 注意 */
            }
            return ALERT_NONE;             /* 白天震动 → 正常 */

        default:
            break;
    }

    /* 无事件但 PIR 持续触发 (尚未到徘徊阈值) */
    if (pir == 1 && g_sec_state.stay >= 5) {
        return ALERT_NONE;                 /* 正常停留 (如等开门) */
    }

    return ALERT_NONE;
}

/*********************************************************************************************
* ===== 公开 API 实现 =====
*********************************************************************************************/

/*********************************************************************************************
* 名称：security_logic_init()
* 功能：初始化安防逻辑模块
* 参数：无
* 返回：无
*********************************************************************************************/
void security_logic_init(void)
{
    g_sec_state.alert         = ALERT_NONE;
    g_sec_state.stay          = 0;
    g_sec_state.night         = 0;
    g_sec_state.event         = EVENT_NONE;
    g_sec_state.event_changed = 0;

    g_pir_start_ms  = 0;
    g_pir_triggered = 0;
    g_doorbell_ever = 0;
    g_last_pir      = 0;
    g_last_tch      = 0;
    g_last_door     = 0;
    g_last_event    = EVENT_NONE;
    g_sys_hour      = 12;
    g_sys_min       = 0;
    g_night_override = 0;
}

/*********************************************************************************************
* 名称：security_logic_update()
* 功能：核心更新函数 — 每个检测周期调用一次
* 参数：pir     - PIR 状态 (0/1)
*       tch     - 门铃/触摸状态 (0/1)
*       door    - 门状态 (0 关/1 开)
*       sys_clk - osal_GetSystemClock() 返回值 (ms)
* 返回：无
* 修改：
* 注释：
*   调用频率: 随 sensorUpdate (周期上报) 或 sensorCheck (100ms巡检) 调用
*   典型周期: 100ms ~ 30s 不等
*
*   状态机:
*   ┌─────────┐  pir=1   ┌──────────┐  stay>10s  ┌──────────┐
*   │ IDLE    │ ───────→ │ COUNTING │ ──────────→ │ LOITER   │
*   │ (无人)  │ ←─────── │ (计时中) │             │ (徘徊)    │
*   └─────────┘  pir=0   └──────────┘             └──────────┘
*                              │
*                         门铃按下 → 重置计时 (合法访客)
*********************************************************************************************/
void security_logic_update(uint8 pir, uint8 tch, uint8 door, uint32 sys_clk)
{
    uint8  new_event;
    uint8  new_alert;
    uint32 elapsed_s;

    /* === 1. 夜间判断 === */
    g_sec_state.night = _is_night_time();

    /* === 2. PIR 持续触发计时 === */
    if (pir == 1 && g_pir_triggered == 0) {
        /* 上升沿: 有人靠近, 开始计时 */
        g_pir_start_ms  = sys_clk;
        g_pir_triggered = 1;
        g_doorbell_ever = 0;             /* 新一次 PIR 事件, 重置门铃标记 */
    } else if (pir == 1 && g_pir_triggered == 1) {
        /* 持续触发: 更新停留时间 */
        if (sys_clk >= g_pir_start_ms) {
            elapsed_s = (sys_clk - g_pir_start_ms) / 1000;
            g_sec_state.stay = (uint16)elapsed_s;
        }
    } else if (pir == 0 && g_pir_triggered == 1) {
        /* 下降沿: 人离开, 结束计时 */
        g_pir_triggered = 0;
        /* stay 保持上一次的值, 供事件上报使用 */
    }

    /* === 3. 门铃标记 === */
    if (tch == 1) {
        g_doorbell_ever = 1;             /* 有人按过门铃 */
    }

    /* 如果人已离开, 重置门铃标记 */
    if (pir == 0 && g_pir_triggered == 0) {
        g_doorbell_ever = 0;
        g_sec_state.stay = 0;
    }

    /* === 4. 事件判定 === */
    new_event = _determine_event(pir, tch, door);

    if (new_event != EVENT_NONE && new_event != g_last_event) {
        g_sec_state.event = new_event;
        g_sec_state.event_changed = 1;
    } else if (new_event == EVENT_NONE && g_last_event != EVENT_NONE) {
        /* 事件结束 */
        g_sec_state.event = EVENT_NONE;
        g_sec_state.event_changed = 1;
    }

    g_last_event = g_sec_state.event;

    /* === 5. 告警级别计算 === */
    new_alert = _calc_alert_level(g_sec_state.event, pir, tch);
    if (new_alert != g_sec_state.alert) {
        g_sec_state.alert = new_alert;
        g_sec_state.event_changed = 1;
    }

    /* === 6. 保存上周期状态 (供下次边沿检测) === */
    g_last_pir  = pir;
    g_last_tch  = tch;
    g_last_door = door;
}

/*********************************************************************************************
* 名称：security_logic_get_state()
* 功能：获取当前安防状态快照
* 参数：无
* 返回：security_state_t 指针
*********************************************************************************************/
const security_state_t* security_logic_get_state(void)
{
    return &g_sec_state;
}

/*********************************************************************************************
* 名称：security_logic_ack_event()
* 功能：确认事件 (消费 event_changed 标志)
*********************************************************************************************/
void security_logic_ack_event(void)
{
    g_sec_state.event_changed = 0;
}

/*********************************************************************************************
* 名称：security_logic_set_time()
* 功能：设置系统时间
* 参数：hour   - 小时 (0-23)
*       minute - 分钟 (0-59)
*********************************************************************************************/
void security_logic_set_time(uint8 hour, uint8 minute)
{
    if (hour < 24)  g_sys_hour = hour;
    if (minute < 60) g_sys_min  = minute;
}

/*********************************************************************************************
* 名称：security_logic_reset_alert()
* 功能：远程解除告警
*********************************************************************************************/
void security_logic_reset_alert(void)
{
    g_sec_state.alert = ALERT_NONE;
    g_sec_state.event = EVENT_NONE;
    g_sec_state.event_changed = 1;
    g_pir_triggered  = 0;
    g_doorbell_ever  = 0;
    g_sec_state.stay = 0;
}

/*********************************************************************************************
* 名称：security_logic_set_night()
* 功能：手动设置夜间模式
* 参数：night - 0=白天/自动, 1=强制夜间
*********************************************************************************************/
void security_logic_set_night(uint8 night)
{
    g_night_override = 1;
    g_night_manual   = (night != 0) ? 1 : 0;
}

/*********************************************************************************************
* 文件：gpio_int.h
* 作者：Lixm 2017.10.17
* 说明：CC2530 GPIO中断配置模块 - 头文件
* 修改：[添加] GPIO中断初始化、ISR回调绑定、中断使能/禁止
* 注释：支持 P0/P1 端口独立引脚中断配置
*********************************************************************************************/

/*********************************************************************************************
* 条件编译
*********************************************************************************************/
#ifndef __GPIO_INT_H__
#define __GPIO_INT_H__

/*********************************************************************************************
* 头文件
*********************************************************************************************/
#include <ioCC2530.h>
#include "hal_types.h"

/*********************************************************************************************
* 宏定义
*********************************************************************************************/

/* 中断触发边沿类型 */
#define GPIO_INT_RISING_EDGE    0x00    /* 上升沿触发 */
#define GPIO_INT_FALLING_EDGE   0x01    /* 下降沿触发 */

/* P0 端口引脚定义 (用于 P0IEN 寄存器位映射) */
#define GPIO_INT_PIN_P0_0       0x01    /* P0.0 - PIR红外 / 触摸按键 */
#define GPIO_INT_PIN_P0_1       0x02    /* P0.1 - 震动传感器 */
#define GPIO_INT_PIN_P0_2       0x04    /* P0.2 - 霍尔门磁 */
#define GPIO_INT_PIN_P0_3       0x08    /* P0.3 - 火焰传感器 */
#define GPIO_INT_PIN_P0_4       0x10    /* P0.4 - 可燃气体 */
#define GPIO_INT_PIN_P0_5       0x20    /* P0.5 - 光栅传感器 */
#define GPIO_INT_PIN_P0_6       0x40    /* P0.6 */
#define GPIO_INT_PIN_P0_7       0x80    /* P0.7 */

/* 中断回调事件标志 — 供应用层 OSAL 事件处理使用 */
#define GPIO_INT_EVT_PIR        0x0001  /* PIR 红外触发事件 */
#define GPIO_INT_EVT_VIBRATION  0x0002  /* 震动触发事件 */
#define GPIO_INT_EVT_HALL       0x0004  /* 门磁触发事件 */
#define GPIO_INT_EVT_TOUCH      0x0008  /* 触摸按键触发事件 */
#define GPIO_INT_EVT_FLAME      0x0010  /* 火焰触发事件 */
#define GPIO_INT_EVT_GRATING    0x0020  /* 光栅触发事件 */

/*********************************************************************************************
* 类型定义
*********************************************************************************************/

/**
 * GPIO 中断回调函数类型
 * @param pin    - 触发中断的引脚编号 (0~7)
 * @param level  - 当前引脚电平 (0=低电平, 1=高电平)
 * @param pdata  - 用户自定义数据指针
 */
typedef void (*gpio_int_callback_t)(uint8 pin, uint8 level, void *pdata);

/**
 * GPIO 中断引脚配置结构体
 */
typedef struct {
    uint8  pin_mask;            /* 引脚掩码 (GPIO_INT_PIN_P0_x) */
    uint8  edge_type;           /* 触发边沿: RISING_EDGE / FALLING_EDGE */
    gpio_int_callback_t cb;     /* 中断回调函数指针 */
    void  *pdata;               /* 回调用户数据 */
    uint8  enabled;             /* 使能标志: 1=已使能, 0=未使能 */
} gpio_int_config_t;

/*********************************************************************************************
* 内部原型函数
*********************************************************************************************/

/**
 * @brief  初始化单个 GPIO 引脚中断
 * @param  pin_mask  - 引脚掩码 (使用 GPIO_INT_PIN_P0_x 宏)
 * @param  edge_type - 触发边沿类型
 * @param  callback  - 中断回调函数 (ISR 中调用，需保持简短)
 * @param  pdata     - 用户数据指针，传入回调
 */
void gpio_int_init_pin(uint8 pin_mask, uint8 edge_type,
                       gpio_int_callback_t callback, void *pdata);

/**
 * @brief  使能指定引脚的中断
 * @param  pin_mask - 引脚掩码
 */
void gpio_int_enable_pin(uint8 pin_mask);

/**
 * @brief  禁止指定引脚的中断
 * @param  pin_mask - 引脚掩码
 */
void gpio_int_disable_pin(uint8 pin_mask);

/**
 * @brief  全局 GPIO 中断初始化 (使能 P0/P1 端口中断)
 * @note   必须在各引脚初始化之前调用
 */
void gpio_int_init_all(void);

/**
 * @brief  P0 端口中断服务程序 (ISR)
 * @note   由 IAR 中断向量自动调用，不需要手动调用
 *         在 ISR 中遍历 P0IFG 标志位，调用对应回调
 */
__interrupt void P0_ISR(void);

/**
 * @brief  读取引脚当前电平 (用于中断回调中确认状态)
 * @param  pin_mask - 引脚掩码
 * @return 0 = 低电平, 1 = 高电平
 */
uint8 gpio_int_read_pin(uint8 pin_mask);

#endif /* __GPIO_INT_H__ */

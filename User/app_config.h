/*********************************************************************************************************************
* CH32V307VCT6 主控板《软件一轮考核》一份固件
* 文件名称          app_config
* 说明              全局配置：版本、调度周期、板级规模、参数编号与存储位置
*                   本文件不包含任何硬件头文件，纯逻辑模块（app_crc32 / app_params / app_menu）也包含它
********************************************************************************************************************/
#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <stdint.h>

/*--------------------------------------------------- 版本信息 ---------------------------------------------------*/
#define APP_VERSION_MAJOR           1
#define APP_VERSION_MINOR           0
#define APP_VERSION_PATCH           0
#define APP_VERSION_STRING          "1.0.0"

/*--------------------------------------------------- 调度节拍 ---------------------------------------------------*/
#define APP_TICK_HZ                 1000u       /* TIM6_PIT 1ms 系统节拍 */
#define APP_TASK_KEY_MS             10u         /* 按键扫描周期 */
#define APP_TASK_UI_MS              50u         /* 界面刷新周期 */
#define APP_TASK_DEMO_MS            100u        /* 演示功能 tick 周期 */
#define APP_TASK_SELFTEST_MS        100u        /* 自检轮询周期 */

/*--------------------------------------------------- 板级规模 ---------------------------------------------------*/
#define APP_LED_NUM                 4u          /* 受控 LED 个数：LED5..LED8 */
#define APP_LED_ISR_INDEX           3u          /* 定时器中断演示占用的 LED 下标 */
#define APP_PWM_CH_NUM              3u          /* 三路 PWM 输出：C6 / D14 / D15 */
#define APP_KEY_QUEUE_SIZE          8u          /* 按键事件队列长度 */

#define APP_DEMO_NUM                11u         /* 功能演示个数（与细则条目对应） */
#define APP_PARAM_NUM               8u          /* 可保存参数个数 */

/*--------------------------------------------------- 屏幕几何 ---------------------------------------------------*/
/* tft180 默认 TFT180_PORTAIT：x_max = 128, y_max = 160；8X16 字体 => 16 列 10 行 */
#define APP_SCREEN_W                128u
#define APP_SCREEN_H                160u
#define APP_SCREEN_ROW_H            16u
#define APP_SCREEN_ROWS             10u
#define APP_SCREEN_COLS             16u
#define APP_SCREEN_TEXT_BUF         40u

/*--------------------------------------------------- 参数定义 ---------------------------------------------------*/
typedef enum
{
    APP_PARAM_BOOT_DEMO = 0,            /* 上电行为：0=停在三级菜单，1..11=直接进对应演示 */
    APP_PARAM_BLINK_MS,                 /* 定时器中断控制 LED 的周期 */
    APP_PARAM_PWM_FREQ,                 /* 三路 PWM 频率 */
    APP_PARAM_PWM_DUTY,                 /* 三路 PWM 占空比（千分比） */
    APP_PARAM_BUZZER_FREQ,              /* 蜂鸣器频率 */
    APP_PARAM_BUZZER_MS,                /* 蜂鸣器鸣叫时长 */
    APP_PARAM_BACKLIGHT,                /* 背光常亮 */
    APP_PARAM_FLOW_SPEED,               /* 流水灯速度等级 1..10 */
} app_param_id_enum;

/* 参数取值范围（纯逻辑模块与菜单共用）
 * BOOT DEMO：0 = 上电停在三级菜单（默认，便于现场按键交互），1..APP_DEMO_NUM = 上电直接进第 (值-1) 号演示 */
#define APP_PARAM_BOOT_DEMO_MIN     0u
#define APP_PARAM_BOOT_DEMO_MAX     (APP_DEMO_NUM)
#define APP_PARAM_BLINK_MIN         100u
#define APP_PARAM_BLINK_MAX         2000u
#define APP_PARAM_BLINK_STEP        50u
#define APP_PARAM_PWM_FREQ_MIN      50u
#define APP_PARAM_PWM_FREQ_MAX      20000u
#define APP_PARAM_PWM_FREQ_STEP     50u
#define APP_PARAM_PWM_DUTY_MIN      0u
#define APP_PARAM_PWM_DUTY_MAX      1000u
#define APP_PARAM_PWM_DUTY_STEP     10u
#define APP_PARAM_BUZZER_FREQ_MIN   500u
#define APP_PARAM_BUZZER_FREQ_MAX   5000u
#define APP_PARAM_BUZZER_FREQ_STEP  50u
#define APP_PARAM_BUZZER_MS_MIN     20u
#define APP_PARAM_BUZZER_MS_MAX     2000u
#define APP_PARAM_BUZZER_MS_STEP    20u
#define APP_PARAM_BACKLIGHT_MIN     0u
#define APP_PARAM_BACKLIGHT_MAX     1u
#define APP_PARAM_BACKLIGHT_STEP    1u
#define APP_PARAM_FLOW_MIN          1u
#define APP_PARAM_FLOW_MAX          10u
#define APP_PARAM_FLOW_STEP         1u

/*--------------------------------------------------- 参数存储 ---------------------------------------------------*/
/* 逐飞 flash 驱动：扇区 0..63，每扇区 4KB，每页 1KB（flash_erase_sector 实际擦一页）
 * 工程 Link.ld 只使用前 256KB，因此用最后一扇区的页 0/1 做双备份，页 2 做 flash 读写演示。 */
#define APP_PARAMS_SECTOR           63u
#define APP_PARAMS_SLOT0_PAGE       0u
#define APP_PARAMS_SLOT1_PAGE       1u
#define APP_PARAMS_RAW_PAGE         2u
#define APP_PARAMS_SLOT_COUNT       2u
#define APP_PARAMS_SLOT_INVALID     0xFFu

/*--------------------------------------------------- 其它 -------------------------------------------------------*/
#define APP_TOAST_MS                1500u       /* 临时提示显示时长 */
#define APP_LONG_PRESS_MS           1000u       /* 长按判定 */
#define APP_REPEAT_MS               250u        /* 长按后的连发周期 */
#define APP_DEBOUNCE_MS             20u         /* 按键消抖时间（2 次 10ms 采样） */

#endif

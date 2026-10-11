/*********************************************************************************************************************
* 文件名称          app_key
* 说明              按键扫描状态机：消抖 + 短按 + 长按 + 长按重复（纯逻辑，可主机端单测）
*                   输入是"74HC148 三位编码"原始码，输出是事件，不直接碰硬件
********************************************************************************************************************/
#ifndef APP_KEY_H
#define APP_KEY_H

#include <stdint.h>
#include "app_config.h"

/*--------------------------------------------------- 按键编号 ---------------------------------------------------*/
typedef enum
{
    APP_KEY_NONE = 0,
    APP_KEY_UP,                             /* 编码 011：五向开关上(I0)与"无键"同码，硬件不可分辨，用 KEY1 代替 */
    APP_KEY_DOWN,                           /* 编码 110：五向开关下 */
    APP_KEY_ENTER,                          /* 编码 001：五向开关中键 */
    APP_KEY_BACK,                           /* 编码 101：五向开关左 */
    APP_KEY_INC,                            /* 编码 010：五向开关右 */
    APP_KEY_DEC,                            /* 编码 000：独立按键 KEY2 */
    APP_KEY_NUMBER
} app_key_enum;

/*--------------------------------------------------- 事件 -------------------------------------------------------*/
typedef struct
{
    app_key_enum key;
    uint8_t      press;                     /* 消抖后按下 */
    uint8_t      release;                   /* 松开 */
    uint8_t      click;                     /* 短按（松开时未达长按时间） */
    uint8_t      long_press;                /* 长按判定成立 */
    uint8_t      repeat;                    /* 长按后的周期重复 */
} app_key_event_t;

/*--------------------------------------------------- 扫描器 -----------------------------------------------------*/
typedef struct
{
    app_key_enum    stable;                 /* 消抖后的稳定按键 */
    app_key_enum    candidate;              /* 正在消抖的候选按键 */
    uint32_t        candidate_ms;
    uint32_t        press_ms;
    uint32_t        last_repeat_ms;
    uint8_t         long_sent;
    uint32_t        event_count;            /* 事件累计（自证/答辩用） */
    app_key_event_t pending;                /* 待取走的事件 */
} app_key_t;

app_key_enum    app_key_code_to_key (uint8_t code);
const char     *app_key_name        (app_key_enum key);

void            app_key_init        (app_key_t *k);
void            app_key_scan        (app_key_t *k, uint8_t raw_code, uint32_t now_ms);
app_key_event_t app_key_take_events (app_key_t *k);

#endif

/*********************************************************************************************************************
* 文件名称          app_key
* 说明              按键状态机实现（纯逻辑）
********************************************************************************************************************/
#include "app_key.h"

app_key_enum app_key_code_to_key (uint8_t code)
{
    /* 74HC148 真值表（EI 接地、输入低有效，A2 A1 A0 -> MCU 的 PB14/PB12/PD8）：
     *   111 = 无键，同时 111 也是"上(I0)"的编码（硬件缺陷：GS/EO 未接 MCU）-> 视为无键
     *   110 = 下(I1)    101 = 左(I2)     100 = 未接输入的编码 -> 忽略
     *   011 = KEY1      010 = 右(I5)     001 = 中键(I6)      000 = KEY2(I7) */
    static const app_key_enum s_map[8] =
    {
        APP_KEY_DEC,        /* 000 KEY2 */
        APP_KEY_ENTER,      /* 001 M    */
        APP_KEY_INC,        /* 010 R    */
        APP_KEY_UP,         /* 011 KEY1（替代不可分辨的"上"） */
        APP_KEY_NONE,       /* 100 忽略 */
        APP_KEY_BACK,       /* 101 L    */
        APP_KEY_DOWN,       /* 110 D    */
        APP_KEY_NONE        /* 111 无键 */
    };

    return s_map[code & 0x07u];
}

const char *app_key_name (app_key_enum key)
{
    switch(key)
    {
        case APP_KEY_UP:    return "UP";
        case APP_KEY_DOWN:  return "DOWN";
        case APP_KEY_ENTER: return "ENTER";
        case APP_KEY_BACK:  return "BACK";
        case APP_KEY_INC:   return "INC";
        case APP_KEY_DEC:   return "DEC";
        default:            return "NONE";
    }
}

void app_key_init (app_key_t *k)
{
    if(0 == k)
    {
        return;
    }
    k->stable          = APP_KEY_NONE;
    k->candidate       = APP_KEY_NONE;
    k->candidate_ms    = 0u;
    k->press_ms        = 0u;
    k->last_repeat_ms  = 0u;
    k->long_sent       = 0u;
    k->event_count     = 0u;
    k->pending.key     = APP_KEY_NONE;
    k->pending.press   = 0u;
    k->pending.release = 0u;
    k->pending.click   = 0u;
    k->pending.long_press = 0u;
    k->pending.repeat  = 0u;
}

void app_key_scan (app_key_t *k, uint8_t raw_code, uint32_t now_ms)
{
    app_key_enum code_key;

    if(0 == k)
    {
        return;
    }

    code_key = app_key_code_to_key(raw_code);

    /* 1. 消抖：候选值必须稳定保持 APP_DEBOUNCE_MS 才被承认 */
    if(code_key != k->candidate)
    {
        k->candidate    = code_key;
        k->candidate_ms = now_ms;
        return;
    }
    if((uint32_t)(now_ms - k->candidate_ms) < APP_DEBOUNCE_MS)
    {
        return;
    }
    if(code_key == k->stable)
    {
        /* 稳定值未变化：处理长按与重复 */
        if(APP_KEY_NONE == k->stable)
        {
            return;
        }
        if((0u == k->long_sent) && ((uint32_t)(now_ms - k->press_ms) >= APP_LONG_PRESS_MS))
        {
            k->long_sent          = 1u;
            k->last_repeat_ms     = now_ms;
            k->pending.key        = k->stable;
            k->pending.long_press = 1u;
            k->event_count ++;
            return;
        }
        if((k->long_sent) && ((uint32_t)(now_ms - k->last_repeat_ms) >= APP_REPEAT_MS))
        {
            k->last_repeat_ms  = now_ms;
            k->pending.key     = k->stable;
            k->pending.repeat  = 1u;
            k->event_count ++;
        }
        return;
    }

    /* 2. 稳定值发生变化：先报旧键松开，再报新键按下 */
    if(APP_KEY_NONE != k->stable)
    {
        k->pending.key     = k->stable;
        k->pending.release = 1u;
        if(0u == k->long_sent)
        {
            k->pending.click = 1u;          /* 未达长按时间 -> 短按 */
        }
        k->event_count ++;
    }
    k->stable = code_key;
    if(APP_KEY_NONE != code_key)
    {
        k->press_ms       = now_ms;
        k->last_repeat_ms = now_ms;
        k->long_sent      = 0u;
        k->pending.key    = code_key;
        k->pending.press  = 1u;
        k->event_count ++;
    }
}

app_key_event_t app_key_take_events (app_key_t *k)
{
    app_key_event_t event;

    event.key       = APP_KEY_NONE;
    event.press     = 0u;
    event.release   = 0u;
    event.click     = 0u;
    event.long_press = 0u;
    event.repeat    = 0u;

    if(0 == k)
    {
        return event;
    }

    event = k->pending;

    k->pending.key        = APP_KEY_NONE;
    k->pending.press      = 0u;
    k->pending.release    = 0u;
    k->pending.click      = 0u;
    k->pending.long_press = 0u;
    k->pending.repeat     = 0u;

    return event;
}

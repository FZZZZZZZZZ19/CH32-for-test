/*********************************************************************************************************************
* 文件名称          test_key
* 说明              app_key 单元测试：74HC148 编码映射、消抖、短按、长按、长按重复
********************************************************************************************************************/
#include "test_util.h"
#include "app_key.h"

#define RAW_NONE        7u                      /* 111 = 无键 */
#define RAW_DOWN        6u                      /* 110 = 五向开关下 */
#define RAW_LEFT        5u                      /* 101 = 五向开关左 */
#define RAW_UNUSED      4u                      /* 100 = 未接输入 */
#define RAW_KEY1        3u                      /* 011 = KEY1 */
#define RAW_RIGHT       2u                      /* 010 = 五向开关右 */
#define RAW_ENTER       1u                      /* 001 = 五向开关中键 */
#define RAW_KEY2        0u                      /* 000 = KEY2 */

/*--------------------------------------------------- 编码映射 ---------------------------------------------------*/
static int test_code_map (void)
{
    CHECK_EQ(app_key_code_to_key(RAW_KEY2),  APP_KEY_DEC);      /* 000 KEY2 */
    CHECK_EQ(app_key_code_to_key(RAW_ENTER), APP_KEY_ENTER);    /* 001 中键 */
    CHECK_EQ(app_key_code_to_key(RAW_RIGHT), APP_KEY_INC);      /* 010 右 */
    CHECK_EQ(app_key_code_to_key(RAW_KEY1),  APP_KEY_UP);       /* 011 KEY1 代替不可分辨的"上" */
    CHECK_EQ(app_key_code_to_key(RAW_UNUSED),APP_KEY_NONE);     /* 100 忽略 */
    CHECK_EQ(app_key_code_to_key(RAW_LEFT),  APP_KEY_BACK);     /* 101 左 */
    CHECK_EQ(app_key_code_to_key(RAW_DOWN),  APP_KEY_DOWN);     /* 110 下 */
    CHECK_EQ(app_key_code_to_key(RAW_NONE),  APP_KEY_NONE);     /* 111 无键（同时也应是"上"） */

    /* 高位应被屏蔽，只取低 3 位 */
    CHECK_EQ(app_key_code_to_key((uint8_t)(RAW_DOWN | 0xF8u)), APP_KEY_DOWN);

    /* 名称表可用 */
    CHECK(app_key_name(APP_KEY_UP)    != 0);
    CHECK(app_key_name(APP_KEY_DOWN)  != 0);
    CHECK(app_key_name(APP_KEY_ENTER) != 0);
    CHECK(app_key_name(APP_KEY_BACK)  != 0);
    CHECK(app_key_name(APP_KEY_INC)   != 0);
    CHECK(app_key_name(APP_KEY_DEC)   != 0);
    CHECK(app_key_name(APP_KEY_NONE)  != 0);

    return 0;
}

/*--------------------------------------------------- 消抖与短按 -------------------------------------------------*/
static int test_debounce_and_click (void)
{
    app_key_t       k;
    app_key_event_t e;

    app_key_init(&k);

    /* 第一次采样只记录候选值 */
    app_key_scan(&k, RAW_DOWN, 0u);
    e = app_key_take_events(&k);
    CHECK_EQ(e.key, APP_KEY_NONE);
    CHECK_EQ(e.press, 0u);

    /* 未满 20ms 消抖时间：不产生事件 */
    app_key_scan(&k, RAW_DOWN, 10u);
    e = app_key_take_events(&k);
    CHECK_EQ(e.press, 0u);

    /* 满 20ms：产生按下事件 */
    app_key_scan(&k, RAW_DOWN, 20u);
    e = app_key_take_events(&k);
    CHECK_EQ(e.key, APP_KEY_DOWN);
    CHECK_EQ(e.press, 1u);
    CHECK_EQ(e.click, 0u);
    CHECK_EQ(k.stable, APP_KEY_DOWN);

    /* 中途抖动到别的编码：候选值被改写，不产生按下 */
    app_key_scan(&k, RAW_NONE, 30u);
    e = app_key_take_events(&k);
    CHECK_EQ(e.release, 0u);
    CHECK_EQ(k.stable, APP_KEY_DOWN);

    /* 松开：先消抖再产生 松开 + 短按 */
    app_key_scan(&k, RAW_NONE, 40u);
    (void)app_key_take_events(&k);
    app_key_scan(&k, RAW_NONE, 60u);
    e = app_key_take_events(&k);
    CHECK_EQ(e.key, APP_KEY_DOWN);
    CHECK_EQ(e.release, 1u);
    CHECK_EQ(e.click, 1u);
    CHECK_EQ(e.long_press, 0u);
    CHECK_EQ(k.stable, APP_KEY_NONE);

    /* 事件取走后必须清空 */
    e = app_key_take_events(&k);
    CHECK_EQ(e.key, APP_KEY_NONE);
    CHECK_EQ(e.click, 0u);

    return 0;
}

/*--------------------------------------------------- 长按与重复 -------------------------------------------------*/
static int test_long_press_and_repeat (void)
{
    app_key_t       k;
    app_key_event_t e;

    app_key_init(&k);

    app_key_scan(&k, RAW_LEFT, 0u);                 /* 候选 */
    app_key_scan(&k, RAW_LEFT, 20u);                /* 按下 */
    e = app_key_take_events(&k);
    CHECK_EQ(e.key, APP_KEY_BACK);
    CHECK_EQ(e.press, 1u);

    /* 999ms 还不能算长按 */
    app_key_scan(&k, RAW_LEFT, 1019u);
    e = app_key_take_events(&k);
    CHECK_EQ(e.long_press, 0u);
    CHECK_EQ(e.repeat, 0u);

    /* 满 1000ms：长按 */
    app_key_scan(&k, RAW_LEFT, 1020u);
    e = app_key_take_events(&k);
    CHECK_EQ(e.key, APP_KEY_BACK);
    CHECK_EQ(e.long_press, 1u);
    CHECK_EQ(e.click, 0u);

    /* 长按成立后不再重复报长按 */
    app_key_scan(&k, RAW_LEFT, 1200u);
    e = app_key_take_events(&k);
    CHECK_EQ(e.long_press, 0u);

    /* 每 250ms 连发一次 */
    app_key_scan(&k, RAW_LEFT, 1269u);
    e = app_key_take_events(&k);
    CHECK_EQ(e.repeat, 0u);
    app_key_scan(&k, RAW_LEFT, 1270u);
    e = app_key_take_events(&k);
    CHECK_EQ(e.key, APP_KEY_BACK);
    CHECK_EQ(e.repeat, 1u);

    /* 长按后松开：只报松开，不再报短按 */
    app_key_scan(&k, RAW_NONE, 1280u);
    (void)app_key_take_events(&k);
    app_key_scan(&k, RAW_NONE, 1300u);
    e = app_key_take_events(&k);
    CHECK_EQ(e.key, APP_KEY_BACK);
    CHECK_EQ(e.release, 1u);
    CHECK_EQ(e.click, 0u);

    return 0;
}

/*--------------------------------------------------- 无键/忽略码 -------------------------------------------------*/
static int test_idle_codes (void)
{
    app_key_t       k;
    app_key_event_t e;
    uint32_t        t;

    app_key_init(&k);

    /* 无键(111) 与 未接输入(100) 都不应产生任何事件 */
    for(t = 0u; t < 3000u; t += 10u)
    {
        app_key_scan(&k, RAW_NONE, t);
        e = app_key_take_events(&k);
        CHECK_EQ(e.key, APP_KEY_NONE);
        CHECK_EQ(e.press, 0u);
        CHECK_EQ(e.long_press, 0u);
        CHECK_EQ(e.repeat, 0u);
    }

    for(t = 0u; t < 1000u; t += 10u)
    {
        app_key_scan(&k, RAW_UNUSED, t);
        e = app_key_take_events(&k);
        CHECK_EQ(e.key, APP_KEY_NONE);
        CHECK_EQ(e.press, 0u);
    }

    CHECK_EQ(k.stable, APP_KEY_NONE);

    /* 空指针安全 */
    app_key_init(0);
    app_key_scan(0, RAW_DOWN, 0u);
    e = app_key_take_events(0);
    CHECK_EQ(e.key, APP_KEY_NONE);

    return 0;
}

int test_key (void)
{
    test_code_map();
    test_debounce_and_click();
    test_long_press_and_repeat();
    test_idle_codes();
    return 0;
}

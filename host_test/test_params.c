/*********************************************************************************************************************
* 文件名称          test_params
* 说明              app_params 单元测试：元数据、默认值、夹取、序列化/反序列化、CRC 校验、A/B 槽选择
********************************************************************************************************************/
#include <string.h>
#include "test_util.h"
#include "app_params.h"
#include "app_crc32.h"

/*--------------------------------------------------- 结构比较 ---------------------------------------------------*/
static int params_equal (const app_params_t *a, const app_params_t *b)
{
    return (a->boot_demo         == b->boot_demo)         &&
           (a->blink_ms          == b->blink_ms)          &&
           (a->pwm_freq_hz       == b->pwm_freq_hz)       &&
           (a->pwm_duty_permille == b->pwm_duty_permille) &&
           (a->buzzer_freq_hz    == b->buzzer_freq_hz)    &&
           (a->buzzer_ms         == b->buzzer_ms)         &&
           (a->backlight         == b->backlight)         &&
           (a->flow_speed        == b->flow_speed)        &&
           (a->seq               == b->seq)               &&
           (a->save_count        == b->save_count)        &&
           (a->boot_count        == b->boot_count);
}

/*--------------------------------------------------- 元数据与默认值 ---------------------------------------------*/
static int test_meta_and_defaults (void)
{
    app_params_t p;
    uint8_t      id;

    CHECK_EQ(APP_PARAMS_BLOCK_LEN, 44u);
    CHECK_EQ(APP_PARAMS_BLOCK_WORDS, 11u);
    CHECK_EQ(APP_PARAMS_CRC_OFFSET, 40u);

    for(id = 0u; id < APP_PARAM_NUM; id ++)
    {
        CHECK(app_param_meta[id].name != 0);
        CHECK(app_param_meta[id].name[0] != '\0');
        CHECK(app_param_meta[id].min <= app_param_meta[id].max);
        CHECK(app_param_meta[id].step >= 1u);
        CHECK_EQ(app_param_is_valid_id(id), 1u);
    }
    CHECK_EQ(app_param_is_valid_id(APP_PARAM_NUM), 0u);
    CHECK_EQ(app_param_is_valid_id(255u), 0u);

    app_params_defaults(&p);
    CHECK_EQ(p.boot_demo,         0u);
    CHECK_EQ(p.blink_ms,          200u);
    CHECK_EQ(p.pwm_freq_hz,       1000u);
    CHECK_EQ(p.pwm_duty_permille, 500u);
    CHECK_EQ(p.buzzer_freq_hz,    2731u);
    CHECK_EQ(p.buzzer_ms,         200u);
    CHECK_EQ(p.backlight,         1u);
    CHECK_EQ(p.flow_speed,        5u);
    CHECK_EQ(p.seq,               0u);
    CHECK_EQ(p.save_count,        0u);
    CHECK_EQ(p.boot_count,        0u);

    /* 默认值必须落在各自区间内 */
    for(id = 0u; id < APP_PARAM_NUM; id ++)
    {
        uint16_t v = app_param_get(&p, id);
        CHECK(v >= app_param_meta[id].min);
        CHECK(v <= app_param_meta[id].max);
    }

    /* 空指针与非法 id 安全 */
    app_params_defaults(0);
    CHECK_EQ(app_param_get(0, 0u), 0u);
    CHECK_EQ(app_param_get(&p, 99u), 0u);
    app_param_set(0, 0u, 1u);
    app_param_set(&p, 99u, 1u);

    return 0;
}

/*--------------------------------------------------- 夹取 -------------------------------------------------------*/
static int test_clamp (void)
{
    app_params_t p;

    CHECK_EQ(app_param_clamp(APP_PARAM_PWM_DUTY, 5000u), 1000u);
    CHECK_EQ(app_param_clamp(APP_PARAM_PWM_DUTY, 0u),    0u);
    CHECK_EQ(app_param_clamp(APP_PARAM_PWM_DUTY, 777u),  777u);
    CHECK_EQ(app_param_clamp(APP_PARAM_BLINK_MS, 1u),    100u);
    CHECK_EQ(app_param_clamp(APP_PARAM_BLINK_MS, 60000u),2000u);
    CHECK_EQ(app_param_clamp(APP_PARAM_BLINK_MS, 350u),  350u);
    CHECK_EQ(app_param_clamp(APP_PARAM_FLOW_SPEED, 0u),  1u);
    CHECK_EQ(app_param_clamp(APP_PARAM_FLOW_SPEED, 99u), 10u);
    CHECK_EQ(app_param_clamp(99u, 123u), 0u);

    /* app_param_set 内部夹取：越界值不会写进结构 */
    app_params_defaults(&p);
    app_param_set(&p, APP_PARAM_PWM_DUTY, 9999u);
    CHECK_EQ(app_param_get(&p, APP_PARAM_PWM_DUTY), 1000u);
    app_param_set(&p, APP_PARAM_PWM_FREQ, 1u);
    CHECK_EQ(app_param_get(&p, APP_PARAM_PWM_FREQ), 50u);
    app_param_set(&p, APP_PARAM_BACKLIGHT, 7u);
    CHECK_EQ(app_param_get(&p, APP_PARAM_BACKLIGHT), 1u);

    return 0;
}

/*--------------------------------------------------- 序列化/反序列化 --------------------------------------------*/
static int test_serialize (void)
{
    app_params_t  src;
    app_params_t  dst;
    uint8_t       buf[APP_PARAMS_BLOCK_LEN];
    uint8_t       buf2[APP_PARAMS_BLOCK_LEN];
    uint16_t      written;

    app_params_defaults(&src);
    src.boot_demo         = 7u;
    src.blink_ms          = 350u;
    src.pwm_freq_hz       = 2500u;
    src.pwm_duty_permille = 640u;
    src.buzzer_freq_hz    = 3000u;
    src.buzzer_ms         = 120u;
    src.backlight         = 0u;
    src.flow_speed        = 9u;
    src.seq               = 0x11223344u;
    src.save_count        = 23u;
    src.boot_count        = 41u;

    memset(buf, 0xAA, sizeof(buf));
    written = app_params_serialize(&src, buf, sizeof(buf));
    CHECK_EQ(written, APP_PARAMS_BLOCK_LEN);

    /* 头部字段 */
    CHECK_EQ(buf[0], 0x57u);                    /* 'W' */
    CHECK_EQ(buf[1], 0x43u);                    /* 'C' */
    CHECK_EQ(buf[2], 0x48u);                    /* 'H' */
    CHECK_EQ(buf[3], 0x31u);                    /* '1' */
    CHECK_EQ(buf[4], 1u);
    CHECK_EQ(buf[6], APP_PARAMS_PAYLOAD_LEN);
    CHECK_EQ(buf[8],  0x44u);                   /* seq 小端 */
    CHECK_EQ(buf[9],  0x33u);
    CHECK_EQ(buf[10], 0x22u);
    CHECK_EQ(buf[11], 0x11u);
    CHECK_EQ(buf[16], 7u);                      /* boot_demo */
    CHECK_EQ(buf[28], 0u);                      /* backlight */
    CHECK_EQ(buf[29], 9u);                      /* flow_speed */
    CHECK_EQ(buf[30], 0u);                      /* 保留区清零 */
    CHECK_EQ(buf[39], 0u);

    /* CRC 字段与自算一致 */
    CHECK_EQ(buf[40] | (buf[41] << 8) | (buf[42] << 16) | ((uint32_t)buf[43] << 24),
             app_crc32(buf, APP_PARAMS_CRC_OFFSET));

    /* 校验通过并能还原全部字段 */
    CHECK_EQ(app_params_block_valid(buf, sizeof(buf)), 1u);
    memset(&dst, 0, sizeof(dst));
    CHECK_EQ(app_params_deserialize(buf, sizeof(buf), &dst), 1u);
    CHECK(params_equal(&src, &dst));

    /* 两次序列化结果完全一致 */
    memset(buf2, 0, sizeof(buf2));
    CHECK_EQ(app_params_serialize(&src, buf2, sizeof(buf2)), APP_PARAMS_BLOCK_LEN);
    CHECK_EQ(memcmp(buf, buf2, APP_PARAMS_BLOCK_LEN), 0);

    /* 容量不足 / 空指针 */
    CHECK_EQ(app_params_serialize(&src, buf, APP_PARAMS_BLOCK_LEN - 1u), 0u);
    CHECK_EQ(app_params_serialize(0, buf, sizeof(buf)), 0u);
    CHECK_EQ(app_params_serialize(&src, 0, sizeof(buf)), 0u);
    CHECK_EQ(app_params_deserialize(buf, sizeof(buf), 0), 0u);

    return 0;
}

/*--------------------------------------------------- 损坏数据 ---------------------------------------------------*/
static int test_corrupt (void)
{
    app_params_t src;
    app_params_t dst;
    uint8_t      good[APP_PARAMS_BLOCK_LEN];
    uint8_t      bad[APP_PARAMS_BLOCK_LEN];

    app_params_defaults(&src);
    src.seq = 5u;
    CHECK_EQ(app_params_serialize(&src, good, sizeof(good)), APP_PARAMS_BLOCK_LEN);

    /* magic 错 */
    memcpy(bad, good, sizeof(bad));
    bad[0] ^= 0x01u;
    CHECK_EQ(app_params_block_valid(bad, sizeof(bad)), 0u);

    /* 版本错 */
    memcpy(bad, good, sizeof(bad));
    bad[4] = 9u;
    CHECK_EQ(app_params_block_valid(bad, sizeof(bad)), 0u);

    /* 载荷长度错 */
    memcpy(bad, good, sizeof(bad));
    bad[6] = 99u;
    CHECK_EQ(app_params_block_valid(bad, sizeof(bad)), 0u);

    /* CRC 错 */
    memcpy(bad, good, sizeof(bad));
    bad[43] ^= 0xFFu;
    CHECK_EQ(app_params_block_valid(bad, sizeof(bad)), 0u);

    /* 数据位翻转（CRC 不匹配） */
    memcpy(bad, good, sizeof(bad));
    bad[20] ^= 0x08u;
    CHECK_EQ(app_params_block_valid(bad, sizeof(bad)), 0u);

    /* 长度不足 / 空指针 */
    CHECK_EQ(app_params_block_valid(good, APP_PARAMS_BLOCK_LEN - 1u), 0u);
    CHECK_EQ(app_params_block_valid(0, sizeof(good)), 0u);

    /* 反序列化失败时不能污染目标结构 */
    memset(&dst, 0, sizeof(dst));
    memcpy(bad, good, sizeof(bad));
    bad[43] ^= 0xFFu;
    CHECK_EQ(app_params_deserialize(bad, sizeof(bad), &dst), 0u);
    CHECK_EQ(dst.seq, 0u);
    CHECK_EQ(dst.blink_ms, 0u);

    return 0;
}

/*--------------------------------------------------- A/B 槽选择 -------------------------------------------------*/
static int test_pick_slot (void)
{
    CHECK_EQ(app_params_pick_slot(1u, 5u, 1u, 6u), 1u);             /* 两槽都有效取 seq 大的 */
    CHECK_EQ(app_params_pick_slot(1u, 7u, 1u, 6u), 0u);
    CHECK_EQ(app_params_pick_slot(1u, 3u, 1u, 3u), 1u);             /* 相等时取 B 槽（最新写入） */
    CHECK_EQ(app_params_pick_slot(1u, 1u, 0u, 99u), 0u);            /* 只有 A 有效 */
    CHECK_EQ(app_params_pick_slot(0u, 99u, 1u, 1u), 1u);            /* 只有 B 有效 */
    CHECK_EQ(app_params_pick_slot(0u, 0u, 0u, 0u), APP_PARAMS_SLOT_INVALID);

    return 0;
}

int test_params (void)
{
    test_meta_and_defaults();
    test_clamp();
    test_serialize();
    test_corrupt();
    test_pick_slot();
    return 0;
}

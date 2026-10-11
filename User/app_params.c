/*********************************************************************************************************************
* 文件名称          app_params
* 说明              参数默认值/夹取/序列化/CRC 校验/双槽选择 + 目标端 flash 存取
********************************************************************************************************************/
#include "app_params.h"
#include "app_crc32.h"

#ifndef APP_HOST_TEST
#include "zf_common_headfile.h"
#endif

/*--------------------------------------------------- 参数元数据 -------------------------------------------------*/
const app_param_meta_t app_param_meta[APP_PARAM_NUM] =
{
    /* 名称            最小值                      最大值                      步进                       开关 */
    { "BOOT DEMO",     APP_PARAM_BOOT_DEMO_MIN,     APP_PARAM_BOOT_DEMO_MAX,    1u,                        0u },
    { "BLINK MS",      APP_PARAM_BLINK_MIN,         APP_PARAM_BLINK_MAX,        APP_PARAM_BLINK_STEP,      0u },
    { "PWM FREQ",      APP_PARAM_PWM_FREQ_MIN,      APP_PARAM_PWM_FREQ_MAX,     APP_PARAM_PWM_FREQ_STEP,   0u },
    { "PWM DUTY",      APP_PARAM_PWM_DUTY_MIN,      APP_PARAM_PWM_DUTY_MAX,     APP_PARAM_PWM_DUTY_STEP,   0u },
    { "BUZZ FREQ",     APP_PARAM_BUZZER_FREQ_MIN,   APP_PARAM_BUZZER_FREQ_MAX,  APP_PARAM_BUZZER_FREQ_STEP, 0u },
    { "BUZZ MS",       APP_PARAM_BUZZER_MS_MIN,     APP_PARAM_BUZZER_MS_MAX,    APP_PARAM_BUZZER_MS_STEP,  0u },
    { "BACKLIGHT",     APP_PARAM_BACKLIGHT_MIN,     APP_PARAM_BACKLIGHT_MAX,    APP_PARAM_BACKLIGHT_STEP,  1u },
    { "FLOW SPD",      APP_PARAM_FLOW_MIN,          APP_PARAM_FLOW_MAX,         APP_PARAM_FLOW_STEP,       0u },
};

/*--------------------------------------------------- 纯逻辑部分 -------------------------------------------------*/
uint8_t app_param_is_valid_id (uint8_t id)
{
    return (id < APP_PARAM_NUM) ? 1u : 0u;
}

uint16_t app_param_clamp (uint8_t id, uint16_t value)
{
    if(0u == app_param_is_valid_id(id))
    {
        return 0u;
    }
    if(value < app_param_meta[id].min)
    {
        value = app_param_meta[id].min;
    }
    else if(value > app_param_meta[id].max)
    {
        value = app_param_meta[id].max;
    }
    return value;
}

void app_params_defaults (app_params_t *p)
{
    if(0 == p)
    {
        return;
    }
    p->boot_demo            = 0u;                       /* OVERVIEW */
    p->blink_ms             = 200u;
    p->pwm_freq_hz          = 1000u;
    p->pwm_duty_permille    = 500u;                     /* 50% */
    p->buzzer_freq_hz       = 2731u;                    /* 无源蜂鸣器谐振点 */
    p->buzzer_ms            = 200u;
    p->backlight            = 1u;
    p->flow_speed           = 5u;
    p->seq                  = 0u;
    p->save_count           = 0u;
    p->boot_count           = 0u;
}

uint16_t app_param_get (const app_params_t *p, uint8_t id)
{
    if((0 == p) || (0u == app_param_is_valid_id(id)))
    {
        return 0u;
    }
    switch(id)
    {
        case APP_PARAM_BOOT_DEMO:       return p->boot_demo;
        case APP_PARAM_BLINK_MS:        return p->blink_ms;
        case APP_PARAM_PWM_FREQ:        return p->pwm_freq_hz;
        case APP_PARAM_PWM_DUTY:        return p->pwm_duty_permille;
        case APP_PARAM_BUZZER_FREQ:     return p->buzzer_freq_hz;
        case APP_PARAM_BUZZER_MS:       return p->buzzer_ms;
        case APP_PARAM_BACKLIGHT:       return (uint16_t)p->backlight;
        case APP_PARAM_FLOW_SPEED:      return (uint16_t)p->flow_speed;
        default:                        return 0u;
    }
}

void app_param_set (app_params_t *p, uint8_t id, uint16_t value)
{
    if((0 == p) || (0u == app_param_is_valid_id(id)))
    {
        return;
    }
    value = app_param_clamp(id, value);
    switch(id)
    {
        case APP_PARAM_BOOT_DEMO:       p->boot_demo         = value;  break;
        case APP_PARAM_BLINK_MS:        p->blink_ms          = value;  break;
        case APP_PARAM_PWM_FREQ:        p->pwm_freq_hz       = value;  break;
        case APP_PARAM_PWM_DUTY:        p->pwm_duty_permille = value;  break;
        case APP_PARAM_BUZZER_FREQ:     p->buzzer_freq_hz    = value;  break;
        case APP_PARAM_BUZZER_MS:       p->buzzer_ms         = value;  break;
        case APP_PARAM_BACKLIGHT:       p->backlight         = (uint8_t)value; break;
        case APP_PARAM_FLOW_SPEED:      p->flow_speed        = (uint8_t)value; break;
        default: break;
    }
}

/*--------------------------------------------------- 小端读写 ---------------------------------------------------*/
static void put_u16 (uint8_t *buf, uint16_t off, uint16_t value)
{
    buf[off + 0] = (uint8_t)(value & 0xFFu);
    buf[off + 1] = (uint8_t)((value >> 8) & 0xFFu);
}

static void put_u32 (uint8_t *buf, uint16_t off, uint32_t value)
{
    buf[off + 0] = (uint8_t)(value & 0xFFu);
    buf[off + 1] = (uint8_t)((value >> 8) & 0xFFu);
    buf[off + 2] = (uint8_t)((value >> 16) & 0xFFu);
    buf[off + 3] = (uint8_t)((value >> 24) & 0xFFu);
}

static uint16_t get_u16 (const uint8_t *buf, uint16_t off)
{
    return (uint16_t)((uint16_t)buf[off] | ((uint16_t)buf[off + 1] << 8));
}

static uint32_t get_u32 (const uint8_t *buf, uint16_t off)
{
    return ((uint32_t)buf[off]) |
           ((uint32_t)buf[off + 1] << 8) |
           ((uint32_t)buf[off + 2] << 16) |
           ((uint32_t)buf[off + 3] << 24);
}

/*--------------------------------------------------- 序列化 -----------------------------------------------------*/
uint16_t app_params_serialize (const app_params_t *p, uint8_t *buf, uint16_t len)
{
    uint16_t i;
    uint32_t crc;

    if((0 == p) || (0 == buf) || (len < APP_PARAMS_BLOCK_LEN))
    {
        return 0u;
    }

    for(i = 0; i < APP_PARAMS_BLOCK_LEN; i ++)
    {
        buf[i] = 0u;
    }

    put_u32(buf, 0u,  APP_PARAMS_MAGIC);
    put_u16(buf, 4u,  (uint16_t)APP_PARAMS_VERSION);
    put_u16(buf, 6u,  APP_PARAMS_PAYLOAD_LEN);
    put_u32(buf, 8u,  p->seq);

    put_u16(buf, 12u, p->save_count);
    put_u16(buf, 14u, p->boot_count);
    put_u16(buf, 16u, p->boot_demo);
    put_u16(buf, 18u, p->blink_ms);
    put_u16(buf, 20u, p->pwm_freq_hz);
    put_u16(buf, 22u, p->pwm_duty_permille);
    put_u16(buf, 24u, p->buzzer_freq_hz);
    put_u16(buf, 26u, p->buzzer_ms);
    buf[28] = p->backlight;
    buf[29] = p->flow_speed;
    /* 30..39 保留，保持 0 */

    crc = app_crc32(buf, APP_PARAMS_CRC_OFFSET);
    put_u32(buf, APP_PARAMS_CRC_OFFSET, crc);

    return APP_PARAMS_BLOCK_LEN;
}

uint8_t app_params_block_valid (const uint8_t *buf, uint16_t len)
{
    uint32_t crc;

    if((0 == buf) || (len < APP_PARAMS_BLOCK_LEN))
    {
        return 0u;
    }
    if(get_u32(buf, 0u) != APP_PARAMS_MAGIC)
    {
        return 0u;
    }
    if(get_u16(buf, 4u) != (uint16_t)APP_PARAMS_VERSION)
    {
        return 0u;
    }
    if(get_u16(buf, 6u) != APP_PARAMS_PAYLOAD_LEN)
    {
        return 0u;
    }
    crc = app_crc32(buf, APP_PARAMS_CRC_OFFSET);
    if(crc != get_u32(buf, APP_PARAMS_CRC_OFFSET))
    {
        return 0u;
    }
    return 1u;
}

uint8_t app_params_deserialize (const uint8_t *buf, uint16_t len, app_params_t *p)
{
    if((0 == p) || (0u == app_params_block_valid(buf, len)))
    {
        return 0u;
    }

    p->seq                  = get_u32(buf, 8u);
    p->save_count           = get_u16(buf, 12u);
    p->boot_count           = get_u16(buf, 14u);
    p->boot_demo            = get_u16(buf, 16u);
    p->blink_ms             = get_u16(buf, 18u);
    p->pwm_freq_hz          = get_u16(buf, 20u);
    p->pwm_duty_permille    = get_u16(buf, 22u);
    p->buzzer_freq_hz       = get_u16(buf, 24u);
    p->buzzer_ms            = get_u16(buf, 26u);
    p->backlight            = buf[28];
    p->flow_speed           = buf[29];

    /* 反序列化后再夹取一次：即使 flash 数据被写坏到"CRC 正确但越界"，也不会让上层拿到非法值 */
    p->boot_demo            = app_param_clamp(APP_PARAM_BOOT_DEMO,    p->boot_demo);
    p->blink_ms             = app_param_clamp(APP_PARAM_BLINK_MS,     p->blink_ms);
    p->pwm_freq_hz          = app_param_clamp(APP_PARAM_PWM_FREQ,     p->pwm_freq_hz);
    p->pwm_duty_permille    = app_param_clamp(APP_PARAM_PWM_DUTY,     p->pwm_duty_permille);
    p->buzzer_freq_hz       = app_param_clamp(APP_PARAM_BUZZER_FREQ,  p->buzzer_freq_hz);
    p->buzzer_ms            = app_param_clamp(APP_PARAM_BUZZER_MS,    p->buzzer_ms);
    p->backlight            = (uint8_t)app_param_clamp(APP_PARAM_BACKLIGHT, p->backlight);
    p->flow_speed           = (uint8_t)app_param_clamp(APP_PARAM_FLOW_SPEED, p->flow_speed);

    return 1u;
}

uint8_t app_params_pick_slot (uint8_t valid0, uint32_t seq0, uint8_t valid1, uint32_t seq1)
{
    if(valid0 && valid1)
    {
        return (seq0 > seq1) ? 0u : 1u;
    }
    if(valid0)
    {
        return 0u;
    }
    if(valid1)
    {
        return 1u;
    }
    return APP_PARAMS_SLOT_INVALID;
}

/*--------------------------------------------------- 目标端 flash 存取 ------------------------------------------*/
#ifndef APP_HOST_TEST
app_params_t app_params;

static uint8_t  s_active_slot = APP_PARAMS_SLOT_INVALID;
static uint32_t s_word_buf[APP_PARAMS_BLOCK_WORDS];
static uint32_t s_read_buf[APP_PARAMS_BLOCK_WORDS];

static uint16_t app_params_slot_page (uint8_t slot)
{
    return (0u == slot) ? APP_PARAMS_SLOT0_PAGE : APP_PARAMS_SLOT1_PAGE;
}

uint8_t app_params_store_slot (void)
{
    return s_active_slot;
}

uint8_t app_params_store_load (app_params_t *out)
{
    uint8_t  valid[APP_PARAMS_SLOT_COUNT];
    uint32_t seq[APP_PARAMS_SLOT_COUNT];
    uint8_t  slot;
    uint8_t  i;

    for(i = 0; i < APP_PARAMS_SLOT_COUNT; i ++)
    {
        valid[i] = 0u;
        seq[i]   = 0u;

        flash_read_page(APP_PARAMS_SECTOR, app_params_slot_page(i), (uint32 *)s_read_buf, APP_PARAMS_BLOCK_WORDS);
        if(app_params_block_valid((const uint8_t *)s_read_buf, APP_PARAMS_BLOCK_LEN))
        {
            valid[i] = 1u;
            seq[i]   = ((uint32_t)s_read_buf[2]);   /* seq 位于偏移 8，即第 3 个字 */
        }
    }

    slot = app_params_pick_slot(valid[0], seq[0], valid[1], seq[1]);
    if(APP_PARAMS_SLOT_INVALID == slot)
    {
        s_active_slot = APP_PARAMS_SLOT_INVALID;
        return APP_PARAMS_SLOT_INVALID;
    }

    s_active_slot = slot;
    if(0u == app_params_deserialize((const uint8_t *)s_read_buf, APP_PARAMS_BLOCK_LEN, out))
    {
        /* 理论上不会走到这里：block_valid 已通过；保守回退为无效 */
        s_active_slot = APP_PARAMS_SLOT_INVALID;
        return APP_PARAMS_SLOT_INVALID;
    }
    (void)out;
    return slot;
}

uint8_t app_params_store_save (const app_params_t *p)
{
    app_params_t tmp;
    uint8_t      target;
    uint16_t     page;
    uint16_t     written;

    if(0 == p)
    {
        return 0u;
    }

    /* 双槽轮换：优先写"非当前有效槽"，这样即使写坏也有上一份可用 */
    target = (0u == s_active_slot) ? 1u : 0u;
    page   = app_params_slot_page(target);

    tmp = *p;
    tmp.seq = p->seq + 1u;

    written = app_params_serialize(&tmp, (uint8_t *)s_word_buf, APP_PARAMS_BLOCK_LEN);
    if(0u == written)
    {
        return 0u;
    }

    if(flash_erase_sector(APP_PARAMS_SECTOR, page))         /* 返回 1 = 失败 */
    {
        return 0u;
    }
    if(flash_write_page(APP_PARAMS_SECTOR, page, (const uint32 *)s_word_buf, APP_PARAMS_BLOCK_WORDS))
    {
        return 0u;
    }

    app_params.seq = tmp.seq;
    s_active_slot  = target;
    return 1u;
}

void app_params_store_erase_all (void)
{
    flash_erase_sector(APP_PARAMS_SECTOR, APP_PARAMS_SLOT0_PAGE);
    flash_erase_sector(APP_PARAMS_SECTOR, APP_PARAMS_SLOT1_PAGE);
    s_active_slot = APP_PARAMS_SLOT_INVALID;
}

void app_params_init (void)
{
    uint8_t slot;

    app_params_defaults(&app_params);
    slot = app_params_store_load(&app_params);

    app_params.boot_count ++;
    if(APP_PARAMS_SLOT_INVALID == slot)
    {
        /* 首次上电（或两份都坏了）：写入一份默认参数 */
        printf("[params] no valid slot, write defaults\r\n");
    }
    /* 每次上电都回写一次，boot_count / seq 才能在复位后体现出来 */
    if(0u == app_params_store_save(&app_params))
    {
        printf("[params] save failed\r\n");
    }
    printf("[params] slot=%d seq=%u boot=%u save=%u\r\n",
           (int)app_params_store_slot(), (unsigned)app_params.seq,
           (unsigned)app_params.boot_count, (unsigned)app_params.save_count);
}

void app_params_save (void)
{
    app_params.save_count ++;
    if(0u == app_params_store_save(&app_params))
    {
        printf("[params] save failed\r\n");
    }
    else
    {
        printf("[params] saved slot=%d seq=%u save=%u\r\n",
               (int)app_params_store_slot(), (unsigned)app_params.seq,
               (unsigned)app_params.save_count);
    }
}

void app_params_reset (void)
{
    app_params_defaults(&app_params);
    app_params_store_erase_all();
    app_params.boot_count = 1u;
    app_params_store_save(&app_params);
    printf("[params] factory reset\r\n");
}
#endif

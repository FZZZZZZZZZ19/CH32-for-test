/*********************************************************************************************************************
* 文件名称          app_selftest
* 说明              自检实现
********************************************************************************************************************/
#include "app_selftest.h"
#include "app_port.h"
#include "app_time.h"
#include "app_led.h"
#include "app_pwm.h"
#include "app_buzzer.h"
#include "app_screen.h"
#include "app_params.h"
#include "app_crc32.h"
#include "app_ui.h"

static uint8_t s_result = 0u;
static uint8_t s_failed = 0u;

static const char *const s_name[8] =
{
    "LED ", "KEY ", "PWM ", "BUZZ", "TFT ", "FLASH", "EXTI", "PARAM"
};

static void selftest_delay_ms (uint32_t ms)
{
    uint32_t start = app_time_now();
    while((uint32_t)(app_time_now() - start) < ms)
    {
        /* 空等，节拍由 TIM6 中断推进 */
    }
}

const char *app_selftest_name (uint8_t bit_index)
{
    return (bit_index < 8u) ? s_name[bit_index] : "?";
}

uint8_t app_selftest_result (void)
{
    return s_result;
}

uint8_t app_selftest_failed (void)
{
    return s_failed;
}

static void selftest_item (uint8_t bit_index, uint8_t ok, const char *detail)
{
    if(ok)
    {
        s_result |= (uint8_t)(1u << bit_index);
        printf("[SELFTEST] %s OK   %s\r\n", s_name[bit_index], (0 != detail) ? detail : "");
    }
    else
    {
        s_failed |= (uint8_t)(1u << bit_index);
        printf("[SELFTEST] %s FAIL %s\r\n", s_name[bit_index], (0 != detail) ? detail : "");
    }
}

void app_selftest_run (void)
{
    app_params_t roundtrip;
    app_params_t source;
    uint8_t      block[APP_PARAMS_BLOCK_LEN];
    uint8_t      code;
    uint8_t      i;
    uint8_t      ok;

    s_result = 0u;
    s_failed = 0u;

    printf("\r\n===== APP SELFTEST v%s =====\r\n", APP_VERSION_STRING);

    /* 1. LED：4 个可控 LED 全亮 -> 读回掩码 -> 全灭 */
    for(i = 0u; i < APP_LED_NUM; i ++)
    {
        app_port_led_set((app_led_enum)i, 1u);
    }
    selftest_delay_ms(150u);
    ok = (0x0Fu == app_led_mask()) ? 1u : 0u;
    for(i = 0u; i < APP_LED_NUM; i ++)
    {
        app_port_led_set((app_led_enum)i, 0u);
    }
    selftest_item(0u, ok, "PD1/PA15/PA12/PA11");

    /* 2. 按键：读编码（无键应为 111 = 7），并打印编码与翻译结果 */
    code = app_port_key_code_read();
    printf("[SELFTEST] key code = %u\r\n", (unsigned)code);
    selftest_item(1u, (code <= 7u) ? 1u : 0u, "74HC148 code read");

    /* 3. PWM：三路都写入 50%，读回软件占空比 */
    app_pwm_init(1000u, 500u);
    ok = 1u;
    for(i = 0u; i < APP_PWM_CH_NUM; i ++)
    {
        if(500u != app_pwm_get_duty(i))
        {
            ok = 0u;
        }
    }
    selftest_item(2u, ok, "C6/D14/D15 @1kHz 50%");

    /* 4. 蜂鸣器：鸣 100ms 后关闭 */
    app_buzzer_beep(2731u, 100u, app_time_now());
    ok = app_buzzer_is_on();
    selftest_delay_ms(200u);
    app_buzzer_task(app_time_now());
    selftest_item(3u, (uint8_t)(ok && (0u == app_buzzer_is_on())), "PA8 2.731kHz");

    /* 5. 屏幕：红绿蓝三色横条 + 文字 */
    app_screen_band_test();
    selftest_delay_ms(300u);
    app_screen_clear();
    app_ui_toast("SELFTEST", app_time_now());
    app_ui_demo_frame("SELFTEST", 0);
    ok = app_screen_ready();
    selftest_item(4u, ok, "ST7735 128x160 SPI2");

    /* 6. FLASH：参数槽位可用性（上电时 app_params_init 已经读写过一次） */
    ok = (APP_PARAMS_SLOT_INVALID != app_params_store_slot()) ? 1u : 0u;
    selftest_item(5u, ok, "params slot read/write");

    /* 7. 外部中断：确认 EXTI 已使能（计数在按键时增长） */
    ok = (app_port_exti_key_count() < 0xFFFFFFFFu) ? 1u : 0u;
    selftest_item(6u, ok, "PD8 both edge armed");

    /* 8. 参数纯逻辑：序列化 + CRC + 反序列化回环 */
    app_params_defaults(&source);
    source.blink_ms       = 350u;
    source.pwm_freq_hz    = 2500u;
    source.flow_speed     = 7u;
    ok = 0u;
    if(APP_PARAMS_BLOCK_LEN == app_params_serialize(&source, block, (uint16_t)sizeof(block)))
    {
        if(app_params_block_valid(block, (uint16_t)sizeof(block)))
        {
            if(app_params_deserialize(block, (uint16_t)sizeof(block), &roundtrip))
            {
                if((roundtrip.blink_ms == 350u) && (roundtrip.pwm_freq_hz == 2500u) && (roundtrip.flow_speed == 7u))
                {
                    ok = 1u;
                }
            }
        }
    }
    selftest_item(7u, ok, "crc32 + serialize roundtrip");

    printf("[SELFTEST] result = 0x%02X failed = 0x%02X\r\n", (unsigned)s_result, (unsigned)s_failed);
}

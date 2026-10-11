/*********************************************************************************************************************
* 文件名称          app_demo
* 说明              11 个演示功能实现（细则 3..10 + 门面演示）
********************************************************************************************************************/
#include "app_demo.h"
#include "app_port.h"
#include "app_time.h"
#include "app_led.h"
#include "app_pwm.h"
#include "app_buzzer.h"
#include "app_screen.h"
#include "app_params.h"
#include "app_crc32.h"

#define DEMO_FLASH_WORDS    8u

static uint8_t  s_active   = 0u;
static uint8_t  s_demo     = 0u;
static uint8_t  s_sub      = 0u;
static uint32_t s_counter  = 0u;
static uint32_t s_last_ms  = 0u;
static uint32_t s_start_ms = 0u;

static uint8_t  s_flash_done     = 0u;
static uint8_t  s_flash_ok       = 0u;
static uint32_t s_flash_crc_w    = 0u;
static uint32_t s_flash_crc_r    = 0u;
static uint32_t s_flash_first_r  = 0u;
static uint32_t s_flash_last_r   = 0u;

/*--------------------------------------------------- 内部小工具 -------------------------------------------------*/
static void demo_status_sync (app_ui_status_t *st)
{
    if(0 == st)
    {
        return;
    }
    st->led_mask    = app_led_mask();
    st->pwm_duty    = app_pwm_get_duty(0u);
    st->pwm_freq_hz = app_pwm_get_freq();
    st->buzzer_on   = app_buzzer_is_on();
    st->isr_toggles = app_led_isr_toggle_count();
}

/*--------------------------------------------------- 各演示初始化 -----------------------------------------------*/
static void demo_enter_flash_test (void)
{
    uint32_t buffer[DEMO_FLASH_WORDS];
    uint32_t readback[DEMO_FLASH_WORDS];
    uint8_t  i;
    uint8_t  same = 1u;

    for(i = 0u; i < DEMO_FLASH_WORDS; i ++)
    {
        buffer[i] = 0xA5A50000u + (uint32_t)i;
    }

    s_flash_crc_w = app_crc32((const uint8_t *)buffer, (uint16_t)(DEMO_FLASH_WORDS * 4u));

    flash_buffer_clear();
    flash_erase_sector(APP_PARAMS_SECTOR, APP_PARAMS_RAW_PAGE);
    flash_write_page(APP_PARAMS_SECTOR, APP_PARAMS_RAW_PAGE, (const uint32 *)buffer, DEMO_FLASH_WORDS);

    flash_buffer_clear();
    flash_read_page(APP_PARAMS_SECTOR, APP_PARAMS_RAW_PAGE, (uint32 *)readback, DEMO_FLASH_WORDS);

    for(i = 0u; i < DEMO_FLASH_WORDS; i ++)
    {
        if(readback[i] != buffer[i])
        {
            same = 0u;
        }
    }

    s_flash_first_r = readback[0];
    s_flash_last_r  = readback[DEMO_FLASH_WORDS - 1u];
    s_flash_crc_r   = app_crc32((const uint8_t *)readback, (uint16_t)(DEMO_FLASH_WORDS * 4u));
    s_flash_ok      = (same && (s_flash_crc_r == s_flash_crc_w)) ? 1u : 0u;
    s_flash_done    = 1u;
}

void app_demo_start (uint8_t demo_id, uint32_t now_ms)
{
    s_active   = 1u;
    s_demo     = demo_id;
    s_sub      = 0u;
    s_counter  = 0u;
    s_last_ms  = now_ms;
    s_start_ms = now_ms;

    app_buzzer_stop();
    app_led_set_period_ms(app_params.blink_ms);
    app_pwm_sweep_enable(0u);

    switch(demo_id)
    {
        case 1u:                            /* LED：细则 3 */
        {
            app_led_set_period_ms((uint16_t)(1000u / app_params.flow_speed));
            app_led_set_mode(APP_LED_MODE_FLOW);
            break;
        }
        case 5u:                            /* 定时器中断 LED：细则 6 */
        {
            app_led_set_mode(APP_LED_MODE_BLINK_ISR);
            break;
        }
        case 7u:                            /* flash 读写：细则 8 */
        {
            demo_enter_flash_test();
            break;
        }
        case 6u:                            /* 屏幕：细则 7 */
        {
            app_screen_band_test();
            break;
        }
        default:
        {
            app_led_set_mode(APP_LED_MODE_BINARY);
            break;
        }
    }
}

void app_demo_stop (void)
{
    s_active = 0u;
    app_led_set_mode(APP_LED_MODE_OFF);
    app_pwm_sweep_enable(0u);
    app_buzzer_stop();
    app_screen_clear();
}

uint8_t app_demo_active (void)
{
    return s_active;
}

uint8_t app_demo_id (void)
{
    return s_demo;
}

/*--------------------------------------------------- 各演示 tick -----------------------------------------------*/
static void demo_overview (const app_demo_ctx_t *ctx)
{
    /* 每 500ms 刷一次，4 个 LED 显示"运行秒数 & 0xF"，肉眼可见 GPIO 受控 */
    if((uint32_t)(ctx->now_ms - s_last_ms) < 500u)
    {
        return;
    }
    s_last_ms = ctx->now_ms;

    app_screen_clear();
    app_ui_demo_frame("WCH BOARD", ctx->status);
    app_ui_line(2u, "1 FIRMWARE 1.0.0");
    app_ui_line(3u, "2 LED  PD1/PA15");
    app_ui_line(4u, "3 KEYS 74HC148");
    app_ui_line(5u, "5 PWM  C6/D14/D15");
    app_ui_line(6u, "6 TIMER TIM6 1MS");
    app_ui_line(7u, "8 FLASH S63 P0/1");
    app_ui_kv(8u, "UP", (ctx->now_ms - s_start_ms) / 1000u, 5u);

    app_led_set_binary((uint8_t)(((ctx->now_ms - s_start_ms) / 1000u) & 0x0Fu));
}

static void demo_led (const app_demo_ctx_t *ctx)
{
    if(ctx->click)
    {
        s_sub = (uint8_t)((s_sub + 1u) % 3u);
        if(0u == s_sub)
        {
            app_led_set_mode(APP_LED_MODE_FLOW);
        }
        else if(1u == s_sub)
        {
            app_led_set_mode(APP_LED_MODE_ON);
        }
        else
        {
            app_led_set_mode(APP_LED_MODE_OFF);
        }
        app_ui_toast((0u == s_sub) ? "FLOW" : ((1u == s_sub) ? "ALL ON" : "ALL OFF"), ctx->now_ms);
    }

    if((uint32_t)(ctx->now_ms - s_last_ms) < 200u)
    {
        return;
    }
    s_last_ms = ctx->now_ms;
    s_counter ++;

    app_ui_demo_frame("3-LED GPIO", ctx->status);
    app_ui_kv(2u, "MODE", (uint32_t)app_led_mode(), 1u);
    app_ui_kv(3u, "MASK", app_led_mask(), 1u);
    app_ui_kv(4u, "PD1", app_led_get(0u), 1u);
    app_ui_kv(5u, "PA15", app_led_get(1u), 1u);
    app_ui_kv(6u, "PA12", app_led_get(2u), 1u);
    app_ui_kv(7u, "PA11", app_led_get(3u), 1u);
    app_ui_line(8u, "ENTER=SWITCH MODE");
}

static void demo_keys (const app_demo_ctx_t *ctx)
{
    if(ctx->click)
    {
        s_counter ++;
        app_buzzer_beep(app_params.buzzer_freq_hz, (uint16_t)(app_params.buzzer_ms / 2u), ctx->now_ms);
    }

    if((uint32_t)(ctx->now_ms - s_last_ms) < 100u)
    {
        return;
    }
    s_last_ms = ctx->now_ms;

    app_ui_demo_frame("4-KEY SCAN", ctx->status);
    app_ui_kv(2u, "CODE", ctx->key_code, 1u);
    app_ui_line(3u, app_key_name(ctx->key_id));
    app_ui_kv(4u, "CLICKS", s_counter, 1u);
    app_ui_kv(5u, "UPTIME", app_time_now(), 1u);
    app_ui_line(6u, "UP=KEY1 DOWN=D");
    app_ui_line(7u, "ENTER=M BACK=L");
    app_ui_line(8u, "INC=R   DEC=KEY2");
    app_led_set_binary(ctx->key_code);
}

static void demo_pwm (const app_demo_ctx_t *ctx)
{
    if(0u == app_pwm_sweep_enabled())
    {
        app_pwm_set_all(app_params.pwm_freq_hz, app_params.pwm_duty_permille);
        app_pwm_sweep_enable(1u);
    }
    if(ctx->click)
    {
        app_pwm_sweep_enable(app_pwm_sweep_enabled() ? 0u : 1u);
        app_ui_toast(app_pwm_sweep_enabled() ? "SWEEP ON" : "SWEEP OFF", ctx->now_ms);
    }

    if((uint32_t)(ctx->now_ms - s_last_ms) < 200u)
    {
        return;
    }
    s_last_ms = ctx->now_ms;

    app_ui_demo_frame("5-PWM OUT", ctx->status);
    app_ui_kv(2u, "FREQ", app_pwm_get_freq(), 1u);
    app_ui_kv(3u, "DUTY", app_pwm_get_duty(0u), 1u);
    app_ui_line(4u, "H1-1 C6  TIM3");
    app_ui_line(5u, "H1-2 D15 TIM4");
    app_ui_line(6u, "H1-3 D14 TIM4");
    app_ui_line(7u, "SCOPE ON HEADER");
    app_screen_bar(8u, app_pwm_get_duty(0u));
}

static void demo_buzzer (const app_demo_ctx_t *ctx)
{
    if(ctx->click || (0u == s_counter))
    {
        s_counter ++;
        app_buzzer_beep(app_params.buzzer_freq_hz, app_params.buzzer_ms, ctx->now_ms);
    }

    if((uint32_t)(ctx->now_ms - s_last_ms) < 100u)
    {
        return;
    }
    s_last_ms = ctx->now_ms;

    app_ui_demo_frame("5-BUZZER PA8", ctx->status);
    app_ui_kv(2u, "FREQ", app_params.buzzer_freq_hz, 1u);
    app_ui_kv(3u, "TIME", app_params.buzzer_ms, 1u);
    app_ui_kv(4u, "ON", app_buzzer_is_on(), 1u);
    app_ui_kv(5u, "BEEPS", app_buzzer_count(), 1u);
    app_ui_line(7u, "PASSIVE BUZZER");
    app_ui_line(8u, "ENTER=REPLAY");
}

static void demo_timer_led (const app_demo_ctx_t *ctx)
{
    if((uint32_t)(ctx->now_ms - s_last_ms) < 200u)
    {
        return;
    }
    s_last_ms = ctx->now_ms;

    app_ui_demo_frame("6-TIMER IRQ", ctx->status);
    app_ui_kv(2u, "TICK", app_time_now(), 1u);
    app_ui_kv(3u, "PERIOD", app_params.blink_ms, 1u);
    app_ui_kv(4u, "TOGGLE", app_led_isr_toggle_count(), 1u);
    app_ui_kv(5u, "LED", app_led_get(APP_LED_ISR_INDEX), 1u);
    app_ui_line(7u, "TIM6 1MS ISR");
    app_ui_line(8u, "PA11 LED8");
}

static void demo_screen (const app_demo_ctx_t *ctx)
{
    static uint16_t duty = 0u;

    if((uint32_t)(ctx->now_ms - s_last_ms) < 100u)
    {
        return;
    }
    s_last_ms = ctx->now_ms;

    duty = (uint16_t)((duty + 20u) % 1000u);
    app_ui_demo_frame("7-SPI TFT180", ctx->status);
    app_ui_line(2u, "ST7735 128X160");
    app_ui_line(3u, "SCK B13 MOSI B15");
    app_ui_line(4u, "RST B7 DC D7");
    app_ui_line(5u, "CS D4 BL D0");
    app_ui_line(6u, "REDRAW 10HZ");
    app_screen_bar(8u, duty);
}

static void demo_flash (const app_demo_ctx_t *ctx)
{
    if(ctx->click)
    {
        app_params_save();
        demo_enter_flash_test();
        app_ui_toast("SAVED + R/W TEST", ctx->now_ms);
    }

    if((uint32_t)(ctx->now_ms - s_last_ms) < 200u)
    {
        return;
    }
    s_last_ms = ctx->now_ms;

    app_ui_demo_frame("8-FLASH PARAMS", ctx->status);
    app_ui_kv(2u, "SLOT", app_params_store_slot(), 1u);
    app_ui_kv(3u, "SEQ", app_params.seq, 1u);
    app_ui_kv(4u, "SAVE", app_params.save_count, 1u);
    app_ui_kv(5u, "BOOT", app_params.boot_count, 1u);
    app_ui_kv(6u, "RAW", s_flash_first_r, 1u);
    app_ui_kv(7u, "CRC", s_flash_ok, 1u);
    app_ui_line(8u, "ENTER=SAVE+TEST");
}

static void demo_exti (const app_demo_ctx_t *ctx)
{
    if(ctx->click)
    {
        s_counter ++;
        if(1u == (s_counter & 0x01u))
        {
            app_port_exti_key_init();       /* 重新初始化（清计数） */
            app_ui_toast("EXTI RAW MODE", ctx->now_ms);
        }
        else
        {
            exti_disable(APP_PORT_PIN_KEY_IN1);
            app_ui_toast("EXTI OFF", ctx->now_ms);
        }
    }

    if((uint32_t)(ctx->now_ms - s_last_ms) < 100u)
    {
        return;
    }
    s_last_ms = ctx->now_ms;

    app_ui_demo_frame("9-EXTI PD8", ctx->status);
    app_ui_kv(2u, "COUNT", app_port_exti_key_count(), 1u);
    app_ui_kv(3u, "LAST", app_port_exti_key_last_ms(), 1u);
    app_ui_kv(4u, "LEVEL", app_port_exti_level(), 1u);
    app_ui_kv(5u, "CODE", ctx->key_code, 1u);
    app_ui_line(7u, "PD8 BOTH EDGE");
    app_ui_line(8u, "KEY EDGE COUNT");
}

static void demo_ui_panel (const app_demo_ctx_t *ctx)
{
    uint8_t i;
    static const char *const tile_name[4] = { "A", "B", "C", "D" };

    if(ctx->click)
    {
        s_sub = (uint8_t)((s_sub + 1u) % 4u);
        s_counter ^= (uint32_t)(1u << s_sub);
        app_buzzer_beep(2731u, 50u, ctx->now_ms);
    }

    if((uint32_t)(ctx->now_ms - s_last_ms) < 100u)
    {
        return;
    }
    s_last_ms = ctx->now_ms;

    app_ui_demo_frame("9-UI PANEL", ctx->status);
    for(i = 0u; i < 4u; i ++)
    {
        uint8_t row = (uint8_t)(2u + i);
        app_ui_kv(row, tile_name[i], (s_counter >> i) & 0x01u, 1u);
    }
    app_ui_line(7u, "ENTER=TOGGLE TILE");
    app_ui_line(8u, "BACK=EXIT DEMO");
}

static void demo_menu_help (const app_demo_ctx_t *ctx)
{
    if((uint32_t)(ctx->now_ms - s_last_ms) < 500u)
    {
        return;
    }
    s_last_ms = ctx->now_ms;

    app_ui_demo_frame("10-MENU 3 LEVEL", ctx->status);
    app_ui_line(2u, "MAIN > PARAM > EDIT");
    app_ui_line(3u, "MAIN > DEMO  > RUN");
    app_ui_line(4u, "MAIN > SYSTEM");
    app_ui_line(6u, "PARAMS SAVED IN");
    app_ui_line(7u, "FLASH S63 P0/P1");
    app_ui_line(8u, "BACK=EXIT LIST");
}

uint8_t app_demo_task (const app_demo_ctx_t *ctx)
{
    if((0u == s_active) || (0 == ctx))
    {
        return 0u;
    }

    demo_status_sync(ctx->status);

    switch(s_demo)
    {
        case 0u:  demo_overview(ctx);   break;
        case 1u:  demo_led(ctx);        break;
        case 2u:  demo_keys(ctx);       break;
        case 3u:  demo_pwm(ctx);        break;
        case 4u:  demo_buzzer(ctx);     break;
        case 5u:  demo_timer_led(ctx);  break;
        case 6u:  demo_screen(ctx);     break;
        case 7u:  demo_flash(ctx);      break;
        case 8u:  demo_exti(ctx);       break;
        case 9u:  demo_ui_panel(ctx);   break;
        default:  demo_menu_help(ctx);  break;
    }

    return 1u;
}

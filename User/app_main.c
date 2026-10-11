/*********************************************************************************************************************
* 文件名称          app_main
* 说明              应用调度层实现
*
* 设计要点（答辩用）：
*   1. 全工程只有一份固件，所有考核条目都是"运行时可选的功能"，通过屏幕三级菜单切换 => 细则 11
*   2. 中断只做两件事：TIM6 1ms 累加节拍并在中断里翻转 LED(细则 6)；PD8 外部中断只做计数和时间戳(细则 9)
*   3. 按键扫描、菜单状态机、界面绘制全部在主循环里做，中断与主循环之间只有 uint32 的读写
********************************************************************************************************************/
#include "zf_common_headfile.h"
#include "app_main.h"
#include "app_config.h"
#include "app_port.h"
#include "app_time.h"
#include "app_key.h"
#include "app_led.h"
#include "app_pwm.h"
#include "app_buzzer.h"
#include "app_screen.h"
#include "app_ui.h"
#include "app_menu.h"
#include "app_params.h"
#include "app_demo.h"
#include "app_selftest.h"

/*--------------------------------------------------- 运行模式 ---------------------------------------------------*/
typedef enum
{
    APP_MODE_MENU = 0,                      /* 三级菜单 + 参数编辑 */
    APP_MODE_DEMO                           /* 单个功能演示 */
} app_mode_enum;

/* 演示编号 -> 考核细则条目号（0 表示总览） */
static const uint8_t s_demo_task_no[APP_DEMO_NUM] =
{
    0u, 3u, 4u, 5u, 5u, 6u, 7u, 8u, 9u, 9u, 10u
};

static app_menu_t       s_menu;
static app_key_t        s_key;
static app_ui_status_t  s_status;
static app_mode_enum    s_mode = APP_MODE_MENU;

static uint32_t s_last_key_ms  = 0u;
static uint32_t s_last_ui_ms   = 0u;
static uint32_t s_last_demo_ms = 0u;
static uint32_t s_last_echo_ms = 0u;
static uint32_t s_last_click_ms = 0u;       /* 双击判定（存 now+1，0 表示"没有上一次短按"） */

static uint8_t  s_demo_click    = 0u;
static uint8_t  s_demo_long     = 0u;
static uint8_t  s_demo_key_code = 7u;

/*--------------------------------------------------- 参数实时生效 ------------------------------------------------*/
static void app_main_apply_params (void)
{
    app_led_set_period_ms((app_params.blink_ms < APP_PARAM_BLINK_MIN) ? APP_PARAM_BLINK_MIN : app_params.blink_ms);
    app_pwm_set_all(app_params.pwm_freq_hz, app_params.pwm_duty_permille);
    app_screen_backlight((0u != app_params.backlight) ? 1u : 0u);
}

/*--------------------------------------------------- 状态栏数据同步 ----------------------------------------------*/
static void app_main_status_sync (uint32_t now_ms)
{
    s_status.demo_id      = app_demo_id();
    s_status.demo_running = app_demo_active();
    s_status.led_mask     = app_led_mask();
    s_status.pwm_freq_hz  = app_pwm_get_freq();
    s_status.pwm_duty     = app_pwm_get_duty(0u);
    s_status.buzzer_on    = app_buzzer_is_on();
    s_status.exti_count   = app_port_exti_key_count();
    s_status.exti_last_ms = app_port_exti_key_last_ms();
    s_status.exti_level   = app_port_exti_level();
    s_status.isr_toggles  = app_led_isr_toggle_count();
    s_status.flash_slot   = app_params_store_slot();
    s_status.flash_seq    = app_params.seq;
    s_status.boot_count   = app_params.boot_count;
    s_status.uptime_ms    = now_ms;
    s_status.task_no      = (APP_MODE_DEMO == s_mode) ? s_demo_task_no[s_status.demo_id] : 10u;
    s_status.key_code     = s_demo_key_code;
}

/*--------------------------------------------------- 进入/退出演示 ----------------------------------------------*/
static void app_main_enter_demo (uint8_t demo_id, uint32_t now_ms)
{
    if(demo_id >= APP_DEMO_NUM)
    {
        return;
    }
    app_demo_start(demo_id, now_ms);
    s_mode          = APP_MODE_DEMO;
    s_demo_click    = 0u;
    s_demo_long     = 0u;
    s_last_demo_ms  = now_ms;
    printf("[demo] enter %u (%s) task=%u\r\n", (unsigned)demo_id, app_demo_name(demo_id),
           (unsigned)s_demo_task_no[demo_id]);
}

static void app_main_exit_demo (void)
{
    app_demo_stop();
    app_led_set_mode(APP_LED_MODE_BINARY);
    app_led_set_binary((uint8_t)(app_params.boot_demo & 0x0Fu));
    s_mode = APP_MODE_MENU;
    app_main_apply_params();
    printf("[demo] exit -> menu\r\n");
}

/*--------------------------------------------------- 菜单动作执行 -----------------------------------------------*/
static void app_main_run_action (uint32_t now_ms)
{
    app_menu_action_enum action = app_menu_take_action(&s_menu);

    switch(action)
    {
        case APP_ACTION_DEMO_START:
        {
            app_main_enter_demo(s_menu.demo_id, now_ms);
            break;
        }

        case APP_ACTION_PARAM_COMMIT:
        {
            /* 参数在三级菜单里改完后立刻生效并落盘：细则 10 的"记录参数" */
            app_main_apply_params();
            app_params_save();
            app_ui_toast("PARAM SAVED", now_ms);
            break;
        }

        case APP_ACTION_SAVE:
        {
            app_params_save();
            app_ui_toast("PARAMS SAVED", now_ms);
            break;
        }

        case APP_ACTION_RELOAD:
        {
            app_params_defaults(&app_params);
            if(APP_PARAMS_SLOT_INVALID == app_params_store_load(&app_params))
            {
                app_params_defaults(&app_params);
            }
            app_main_apply_params();
            app_ui_toast("RELOADED", now_ms);
            break;
        }

        case APP_ACTION_FACTORY:
        {
            app_params_reset();
            app_main_apply_params();
            app_ui_toast("FACTORY RESET", now_ms);
            break;
        }

        case APP_ACTION_FLASH_TEST:
        {
            app_main_enter_demo(7u, now_ms);
            break;
        }

        case APP_ACTION_SELFTEST:
        {
            app_selftest_run();
            app_ui_toast("SELFTEST DONE", now_ms);
            break;
        }

        default:
        {
            break;
        }
    }
}

/*--------------------------------------------------- 按键事件分发 -----------------------------------------------*/
/* 板上"到底有几个可用按键"取决于实物装配（原理图是 74HC148 + 五向开关，实物板也可能只焊了 1~2 个轻触键），
 * 所以菜单态同时给出两条通路，保证任意 1~2 个键都能走完三级菜单：
 *   ① 手势：短按 = 上/下翻项（编辑态 = 加/减）；长按 = 确定（进入下一级 / 提交编辑并写 flash）；双击 = 返回
 *   ② 74HC148 原始语义：中键 = 确定、左 = 返回、KEY1 = 上、KEY2 / 摇杆下 = 下、摇杆右 = 列表态确定 / 编辑态加
 * 注意 app_menu.c 里列表态用 up/down、编辑态用 inc/dec（up 与 inc 都会加），所以这里按深度分派 */
static void app_main_menu_input (const app_key_event_t *ev, uint32_t now_ms, uint8_t gesture_back, uint8_t gesture_enter)
{
    app_menu_input_t in;
    uint8_t          editing = (app_menu_depth(&s_menu) >= 3u) ? 1u : 0u;   /* 1=主菜单 2=列表 3=编辑 */

    in.now_ms = now_ms;
    in.up     = 0u;
    in.down   = 0u;
    in.enter  = 0u;
    in.back   = 0u;
    in.inc    = 0u;
    in.dec    = 0u;

    if(0u != gesture_back)
    {
        in.back  = 1u;                                  /* 双击 = 返回上一级 */
    }
    else if(0u != gesture_enter)
    {
        in.enter = 1u;                                  /* 长按 = 确定 */
    }
    else if(APP_KEY_ENTER == ev->key)
    {
        in.enter = 1u;                                  /* 五向中键 = 确定 */
    }
    else if(APP_KEY_BACK == ev->key)
    {
        in.back  = 1u;                                  /* 五向左键 = 返回 */
    }
    else if(0u != editing)
    {
        if((APP_KEY_UP == ev->key) || (APP_KEY_INC == ev->key))
        {
            in.inc = 1u;
        }
        else if((APP_KEY_DOWN == ev->key) || (APP_KEY_DEC == ev->key))
        {
            in.dec = 1u;
        }
    }
    else
    {
        if(APP_KEY_UP == ev->key)
        {
            in.up = 1u;                                 /* KEY1 = 光标上移 */
        }
        else if((APP_KEY_DOWN == ev->key) || (APP_KEY_DEC == ev->key))
        {
            in.down = 1u;                               /* KEY2 / 摇杆下 = 光标下移（KEY2 不再"按了没反应"） */
        }
        else if(APP_KEY_INC == ev->key)
        {
            in.enter = 1u;                              /* 摇杆右 = 在列表态当确定，更好按 */
        }
    }

    app_menu_apply(&s_menu, &app_params, &in);
    s_menu.event_count ++;
}

static void app_main_menu_long_press (const app_key_event_t *ev, uint32_t now_ms)
{
    uint8_t editing = (app_menu_depth(&s_menu) >= 3u) ? 1u : 0u;

    if(APP_KEY_BACK == ev->key)
    {
        if(0u != editing)
        {
            app_main_menu_input(ev, now_ms, 1u, 0u);            /* 编辑态长按左键 = 取消编辑 */
        }
        else
        {
            app_menu_goto_main(&s_menu);                        /* 列表态长按左键 = 直接回主菜单 */
            app_ui_toast("MAIN MENU", now_ms);
        }
        return;
    }

    app_main_menu_input(ev, now_ms, 0u, 1u);                    /* 其余键长按 = 确定 / 提交编辑 */

    if(0u != editing)
    {
        app_params_save();                                      /* 提交后立刻落 flash：断电重启参数还在 */
        app_ui_toast("PARAM SAVED", now_ms);
    }
}

static void app_main_handle_event (const app_key_event_t *ev)
{
    uint32_t now_ms;
    uint8_t  dbl = 0u;

    if(APP_KEY_NONE == ev->key)
    {
        return;
    }

    now_ms = app_time_now();

    /* 双击判定：两次短按间隔 <= APP_GESTURE_DOUBLE_MS 视为"返回"（存 now+1，0 = 没有上一次） */
    if(0u != ev->click)
    {
        uint32_t stamp = now_ms + 1u;

        if((0u != s_last_click_ms) && ((stamp - s_last_click_ms) <= APP_GESTURE_DOUBLE_MS))
        {
            dbl             = 1u;
            s_last_click_ms = 0u;                               /* 消费掉，三击不会连发两次返回 */
        }
        else
        {
            s_last_click_ms = stamp;
        }
    }

    if(APP_MODE_DEMO == s_mode)
    {
        /* 演示态：短按留给演示功能自己消费，长按切下一个演示，双击退出到菜单
         * （这样即使板上只有 1 个键，也能"看演示 -> 换演示 -> 回菜单"） */
        if(0u != dbl)
        {
            app_main_exit_demo();
            return;
        }
        if(0u != ev->long_press)
        {
            s_demo_long = 1u;
            app_buzzer_beep(app_params.buzzer_freq_hz, 40u, now_ms);
            app_main_enter_demo((uint8_t)((app_demo_id() + 1u) % APP_DEMO_NUM), now_ms);
            return;
        }
        if((0u != ev->click) && (APP_KEY_BACK == ev->key))
        {
            app_main_exit_demo();
            return;
        }
        if(0u != ev->click)
        {
            app_buzzer_beep(app_params.buzzer_freq_hz, 30u, now_ms);    /* 每按一次响一下 */
            s_demo_click = 1u;
        }
        return;
    }

    /* 菜单态 */
    if(0u != ev->long_press)
    {
        app_buzzer_beep(app_params.buzzer_freq_hz, 30u, now_ms);
        app_main_menu_long_press(ev, now_ms);
        return;
    }

    if((0u != ev->click) || (0u != ev->repeat))
    {
        app_main_menu_input(ev, now_ms, dbl, 0u);
    }
    else if(0u != ev->press)
    {
        app_buzzer_beep(app_params.buzzer_freq_hz, 15u, now_ms);        /* 按下就响一声：证明按键被读到 */
    }
}

/*--------------------------------------------------- 初始化 -----------------------------------------------------*/
void app_main_init (void)
{
    uint32_t now_ms;

    printf("\r\n=========================================================\r\n");
    printf(" CH32V307VCT6 WCH main board - assessment firmware v%s\r\n", APP_VERSION_STRING);
    printf(" one firmware / no re-flash: every item is a runtime mode\r\n");
    printf("=========================================================\r\n");

    app_port_init();                                    /* LED / 按键 / 三路 PWM / 蜂鸣器，含关 JTAG 保 SWD */
    app_time_init();                                    /* TIM6 1ms 系统节拍（中断里也会翻转 LED） */
    app_params_init();                                  /* 读取 flash 参数 + boot_count+1 + 回写 */
    app_screen_init();                                  /* ST7735 128x160 硬件 SPI2 */
    app_led_init();
    app_pwm_init(app_params.pwm_freq_hz, app_params.pwm_duty_permille);
    app_buzzer_init();
    app_key_init(&s_key);
    app_menu_init(&s_menu);
    app_ui_init();
    app_port_exti_key_init();                           /* PD8 双边沿外部中断 */

    app_main_apply_params();

    /* 上电自检：逐项驱动硬件并把结果打到串口和屏幕 */
    app_selftest_run();
    app_main_apply_params();

    printf("[pin] LED : LED5=PD1 LED6=PA15 LED7=PA12 LED8=PA11 (high = on)\r\n");
    printf("[pin] KEYS: A2=PB14 A1=PB12 A0=PD8 (74HC148, low active)\r\n");
    printf("[pin] PWM : H1-1=PC6(TIM3_CH1) H1-3=PD14(TIM4_CH3) H1-2=PD15(TIM4_CH4)\r\n");
    printf("[pin] BUZZ: PA8(TIM1_CH1) -> AO3400A -> passive buzzer 2.731kHz\r\n");
    printf("[pin] TFT : SCL=PB13 SDA=PB15 RES=PB7 DC=PD7 CS=PD4 BL=PD0\r\n");
    printf("[pin] FLASH: sector 63 page0/page1 = A/B params, page2 = r/w demo\r\n");

    s_status.boot_count = app_params.boot_count;
    s_status.flash_slot = app_params_store_slot();
    s_status.flash_seq  = app_params.seq;
    s_status.led_mask   = app_led_mask();

    now_ms = app_time_now();
    app_ui_toast("READY", now_ms);

    /* 上电行为：boot_demo = 0（默认）停在三级菜单，方便现场用按键交互；
     * 1..APP_DEMO_NUM 表示上电直接进第 (boot_demo-1) 号演示，两者都不需要重新烧录 */
    if((app_params.boot_demo > 0u) && (app_params.boot_demo <= APP_DEMO_NUM))
    {
        app_main_enter_demo((uint8_t)(app_params.boot_demo - 1u), now_ms);
    }
}

/*--------------------------------------------------- 主循环 -----------------------------------------------------*/
void app_main_loop (void)
{
    uint32_t now_ms = app_time_now();

    /* 1. 按键扫描（10ms 一次）：读 74HC148 编码 -> 消抖/短按/长按/连发 */
    if((uint32_t)(now_ms - s_last_key_ms) >= APP_TASK_KEY_MS)
    {
        app_key_event_t ev;

        s_last_key_ms  = now_ms;
        s_demo_key_code = app_port_key_code_read();
        s_status.key_code = s_demo_key_code;
        s_status.key_id   = (uint8_t)app_key_code_to_key(s_demo_key_code);

        app_key_scan(&s_key, s_demo_key_code, now_ms);
        ev = app_key_take_events(&s_key);
        app_main_handle_event(&ev);
    }

    /* 2. 周期性任务：LED / PWM 扫描 / 蜂鸣器超时 */
    app_led_task(now_ms);
    app_pwm_task(now_ms);
    app_buzzer_task(now_ms);

    /* 3. 菜单动作 */
    app_main_run_action(now_ms);

    /* 4. 演示态跑演示功能，菜单态画界面 */
    if(APP_MODE_DEMO == s_mode)
    {
        /* 演示态也要同步状态：每 100ms 刷新按键回显行 + 状态栏（E=EXTI 计数、C=原始编码）
         * 现场按任意键都能立刻在屏幕上看到编码/名字变化，用于区分"按键没读到"还是"演示不响应" */
        if((uint32_t)(now_ms - s_last_echo_ms) >= APP_TASK_DEMO_MS)
        {
            s_last_echo_ms = now_ms;
            app_main_status_sync(now_ms);
            app_ui_key_echo(APP_UI_ROW_ECHO, s_status.key_code, app_key_name((app_key_enum)s_status.key_id));
        }

        if((uint32_t)(now_ms - s_last_demo_ms) >= APP_TASK_DEMO_MS)
        {
            app_demo_ctx_t ctx;

            s_last_demo_ms = now_ms;
            ctx.now_ms     = now_ms;
            ctx.key_code   = s_demo_key_code;
            ctx.key_id     = (app_key_enum)s_status.key_id;
            ctx.click      = s_demo_click;
            ctx.long_press = s_demo_long;
            ctx.status     = &s_status;
            s_demo_click   = 0u;
            s_demo_long    = 0u;

            if(0u == app_demo_task(&ctx))
            {
                app_main_exit_demo();
            }
        }
    }
    else if((uint32_t)(now_ms - s_last_ui_ms) >= APP_TASK_UI_MS)
    {
        s_last_ui_ms = now_ms;
        app_main_status_sync(now_ms);
        app_ui_render(&s_menu, &app_params, &s_status);
    }
}

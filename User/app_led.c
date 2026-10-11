/*********************************************************************************************************************
* 文件名称          app_led
* 说明              LED 驱动层实现
********************************************************************************************************************/
#include "app_led.h"
#include "app_port.h"
#include "app_time.h"

static volatile app_led_mode_enum s_mode = APP_LED_MODE_OFF;
static volatile uint16_t s_period_ms    = 200u;         /* 闪烁半周期 */
static uint32_t          s_last_ms      = 0u;           /* 主循环模式的相位基准 */
static uint8_t           s_phase        = 0u;
static uint8_t           s_flow_index   = 0u;
static uint8_t           s_binary       = 0u;

static volatile uint32_t s_isr_ms       = 0u;           /* 中断模式：累计的毫秒数 */
static volatile uint32_t s_isr_toggles  = 0u;           /* 中断里翻转次数（答辩/自证） */

/*--------------------------------------------------- 模式控制 ---------------------------------------------------*/
void app_led_init (void)
{
    s_mode      = APP_LED_MODE_OFF;
    s_period_ms = 200u;
    s_last_ms   = 0u;
    s_phase     = 0u;
    s_isr_ms    = 0u;
    s_isr_toggles = 0u;
    app_led_all_off();
}

void app_led_all_off (void)
{
    uint8_t i;
    for(i = 0; i < APP_LED_NUM; i ++)
    {
        app_port_led_set((app_led_enum)i, 0u);
    }
}

void app_led_set_mode (app_led_mode_enum mode)
{
    s_mode      = mode;
    s_last_ms   = app_tick_ms;
    s_phase     = 0u;
    s_flow_index = 0u;
    s_isr_ms    = 0u;
    s_isr_toggles = 0u;

    if(APP_LED_MODE_OFF == mode)
    {
        app_led_all_off();
    }
    else if(APP_LED_MODE_ON == mode)
    {
        uint8_t i;
        for(i = 0; i < APP_LED_NUM; i ++)
        {
            app_port_led_set((app_led_enum)i, 1u);
        }
    }
    else if(APP_LED_MODE_BLINK_ISR == mode)
    {
        app_led_all_off();                  /* 先全灭，之后只由中断翻转 APP_LED_ISR_INDEX 那一个 */
    }
}

app_led_mode_enum app_led_mode (void)
{
    return s_mode;
}

const char *app_led_mode_name (app_led_mode_enum mode)
{
    switch(mode)
    {
        case APP_LED_MODE_OFF:      return "OFF";
        case APP_LED_MODE_ON:       return "ON";
        case APP_LED_MODE_BLINK_ISR:return "BLINK ISR";
        case APP_LED_MODE_BLINK_SW: return "BLINK SW";
        case APP_LED_MODE_FLOW:     return "FLOW";
        case APP_LED_MODE_BINARY:   return "BINARY";
        default:                    return "?";
    }
}

void app_led_set_period_ms (uint16_t ms)
{
    if(ms < 10u)
    {
        ms = 10u;
    }
    s_period_ms = ms;
}

void app_led_set_binary (uint8_t value)
{
    s_binary = (uint8_t)(value & 0x0Fu);
}

uint8_t app_led_get (uint8_t index)
{
    if(index >= APP_LED_NUM)
    {
        return 0u;
    }
    return app_port_led_get((app_led_enum)index);
}

uint8_t app_led_mask (void)
{
    uint8_t i;
    uint8_t mask = 0u;

    for(i = 0; i < APP_LED_NUM; i ++)
    {
        if(app_port_led_get((app_led_enum)i))
        {
            mask |= (uint8_t)(1u << i);
        }
    }
    return mask;
}

uint32_t app_led_isr_toggle_count (void)
{
    return s_isr_toggles;
}

/*--------------------------------------------------- 中断服务（细则 6） -----------------------------------------*/
void app_led_isr_tick (void)
{
    if(APP_LED_MODE_BLINK_ISR != s_mode)
    {
        return;
    }

    s_isr_ms ++;
    if(s_isr_ms >= (uint32_t)s_period_ms)
    {
        s_isr_ms = 0u;
        s_isr_toggles ++;
        app_port_led_toggle((app_led_enum)APP_LED_ISR_INDEX);
    }
}

/*--------------------------------------------------- 主循环任务 ------------------------------------------------*/
void app_led_task (uint32_t now_ms)
{
    uint8_t i;

    switch(s_mode)
    {
        case APP_LED_MODE_OFF:
        {
            app_led_all_off();
            break;
        }

        case APP_LED_MODE_ON:
        {
            for(i = 0; i < APP_LED_NUM; i ++)
            {
                app_port_led_set((app_led_enum)i, 1u);
            }
            break;
        }

        case APP_LED_MODE_BLINK_ISR:
        {
            /* 闪烁交给中断，主循环只保证其它 LED 熄灭 */
            for(i = 0; i < APP_LED_NUM; i ++)
            {
                if(i != APP_LED_ISR_INDEX)
                {
                    app_port_led_set((app_led_enum)i, 0u);
                }
            }
            break;
        }

        case APP_LED_MODE_BLINK_SW:
        {
            if((uint32_t)(now_ms - s_last_ms) >= (uint32_t)s_period_ms)
            {
                s_last_ms = now_ms;
                s_phase   = (s_phase) ? 0u : 1u;
                for(i = 0; i < APP_LED_NUM; i ++)
                {
                    app_port_led_set((app_led_enum)i, s_phase);
                }
            }
            break;
        }

        case APP_LED_MODE_FLOW:
        {
            if((uint32_t)(now_ms - s_last_ms) >= (uint32_t)s_period_ms)
            {
                s_last_ms = now_ms;
                s_flow_index = (uint8_t)((s_flow_index + 1u) % APP_LED_NUM);
                for(i = 0; i < APP_LED_NUM; i ++)
                {
                    app_port_led_set((app_led_enum)i, (i == s_flow_index) ? 1u : 0u);
                }
            }
            break;
        }

        case APP_LED_MODE_BINARY:
        {
            for(i = 0; i < APP_LED_NUM; i ++)
            {
                app_port_led_set((app_led_enum)i, (uint8_t)((s_binary >> i) & 0x01u));
            }
            break;
        }

        default:
        {
            app_led_all_off();
            break;
        }
    }
}

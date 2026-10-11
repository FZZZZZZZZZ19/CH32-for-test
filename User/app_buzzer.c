/*********************************************************************************************************************
* 文件名称          app_buzzer
* 说明              无源蜂鸣器驱动层实现
********************************************************************************************************************/
#include "app_buzzer.h"
#include "app_port.h"
#include "app_time.h"

static uint8_t  s_on            = 0u;
static uint32_t s_off_at_ms     = 0u;
static uint32_t s_count         = 0u;
static uint32_t s_last_ms       = 0u;
static uint16_t s_last_freq     = 0u;

void app_buzzer_init (void)
{
    s_on        = 0u;
    s_off_at_ms = 0u;
    s_count     = 0u;
    app_port_buzzer_stop();
}

void app_buzzer_beep (uint32_t freq_hz, uint16_t duration_ms, uint32_t now_ms)
{
    if(0u == freq_hz)
    {
        freq_hz = 2731u;                    /* 无源蜂鸣器谐振频率 */
    }
    if(0u == duration_ms)
    {
        duration_ms = 100u;
    }

    app_port_buzzer_pwm(freq_hz, 500u);     /* 50% 占空比驱动无源蜂鸣器 */
    s_on        = 1u;
    s_off_at_ms = now_ms + (uint32_t)duration_ms;
    s_count ++;
    s_last_ms   = now_ms;
    s_last_freq = (uint16_t)freq_hz;
}

void app_buzzer_stop (void)
{
    app_port_buzzer_stop();
    s_on = 0u;
}

void app_buzzer_task (uint32_t now_ms)
{
    if((s_on) && ((int32_t)(now_ms - s_off_at_ms) >= 0))
    {
        app_buzzer_stop();
    }
}

uint8_t app_buzzer_is_on (void)
{
    return s_on;
}

uint32_t app_buzzer_count (void)
{
    return s_count;
}

uint32_t app_buzzer_last_ms (void)
{
    return s_last_ms;
}

uint16_t app_buzzer_last_freq (void)
{
    return s_last_freq;
}

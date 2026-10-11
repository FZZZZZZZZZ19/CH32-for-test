/*********************************************************************************************************************
* 文件名称          app_pwm
* 说明              三路 PWM 驱动层实现
********************************************************************************************************************/
#include "app_pwm.h"
#include "app_port.h"
#include "app_time.h"

#define APP_PWM_SWEEP_STEP_MS   20u
#define APP_PWM_SWEEP_STEP      20u         /* 占空比步进（千分比） */

static uint16_t s_duty[APP_PWM_CH_NUM];
static uint16_t s_freq_hz      = 1000u;
static uint8_t  s_sweep         = 0u;
static uint8_t  s_sweep_dir     = 0u;
static uint32_t s_sweep_last_ms = 0u;

static uint16_t app_pwm_clamp (uint16_t duty_permille)
{
    if(duty_permille > 1000u)
    {
        return 1000u;
    }
    return duty_permille;
}

void app_pwm_init (uint16_t freq_hz, uint16_t duty_permille)
{
    uint8_t i;

    if(0u == freq_hz)
    {
        freq_hz = 1000u;
    }
    s_freq_hz = freq_hz;

    for(i = 0; i < APP_PWM_CH_NUM; i ++)
    {
        s_duty[i] = app_pwm_clamp(duty_permille);
        app_port_pwm_set(i, s_freq_hz, s_duty[i]);
    }
}

void app_pwm_set_all (uint16_t freq_hz, uint16_t duty_permille)
{
    uint8_t i;

    if(0u == freq_hz)
    {
        freq_hz = s_freq_hz;
    }
    s_freq_hz = freq_hz;

    for(i = 0; i < APP_PWM_CH_NUM; i ++)
    {
        s_duty[i] = app_pwm_clamp(duty_permille);
        app_port_pwm_set(i, s_freq_hz, s_duty[i]);
    }
}

void app_pwm_set_channel (uint8_t ch, uint16_t duty_permille)
{
    if(ch >= APP_PWM_CH_NUM)
    {
        return;
    }
    s_duty[ch] = app_pwm_clamp(duty_permille);
    app_port_pwm_set(ch, s_freq_hz, s_duty[ch]);
}

uint16_t app_pwm_get_duty (uint8_t ch)
{
    return (ch < APP_PWM_CH_NUM) ? s_duty[ch] : 0u;
}

uint16_t app_pwm_get_freq (void)
{
    return s_freq_hz;
}

void app_pwm_sweep_enable (uint8_t enable)
{
    s_sweep         = (enable) ? 1u : 0u;
    s_sweep_dir     = 0u;
    s_sweep_last_ms = app_tick_ms;
}

uint8_t app_pwm_sweep_enabled (void)
{
    return s_sweep;
}

void app_pwm_task (uint32_t now_ms)
{
    uint16_t duty;

    if(0u == s_sweep)
    {
        return;
    }
    if((uint32_t)(now_ms - s_sweep_last_ms) < APP_PWM_SWEEP_STEP_MS)
    {
        return;
    }
    s_sweep_last_ms = now_ms;

    duty = s_duty[0];
    if(0u == s_sweep_dir)
    {
        duty = (uint16_t)(duty + APP_PWM_SWEEP_STEP);
        if(duty >= 1000u)
        {
            duty         = 1000u;
            s_sweep_dir  = 1u;
        }
    }
    else
    {
        duty = (duty > APP_PWM_SWEEP_STEP) ? (uint16_t)(duty - APP_PWM_SWEEP_STEP) : 0u;
        if(0u == duty)
        {
            s_sweep_dir = 0u;
        }
    }

    app_pwm_set_all(s_freq_hz, duty);
}

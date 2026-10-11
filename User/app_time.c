/*********************************************************************************************************************
* 文件名称          app_time
* 说明              系统节拍实现
********************************************************************************************************************/
#include "app_time.h"

volatile uint32_t app_tick_ms = 0u;

void app_time_init (void)
{
#ifndef APP_HOST_TEST
    pit_ms_init(TIM6_PIT, 1);                   /* 1ms 定时中断，IRQ 里调 app_time_isr_tick() */
#endif
}

void app_time_isr_tick (void)
{
    app_tick_ms ++;
}

uint32_t app_time_now (void)
{
    return app_tick_ms;
}

uint8_t app_time_elapsed (uint32_t start_ms, uint32_t span_ms)
{
    return ((uint32_t)(app_tick_ms - start_ms) >= span_ms) ? 1u : 0u;
}

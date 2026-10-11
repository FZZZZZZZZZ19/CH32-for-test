/*********************************************************************************************************************
* 文件名称          stub_time
* 说明              主机端测试替身：app_time 模块依赖 TIM6 硬件中断，主机端只提供可自增的节拍
********************************************************************************************************************/
#include "app_time.h"

volatile uint32_t app_tick_ms = 0u;

void app_time_init (void)
{
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
    return (uint8_t)(((uint32_t)(app_tick_ms - start_ms) >= span_ms) ? 1u : 0u);
}

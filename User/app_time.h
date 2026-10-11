/*********************************************************************************************************************
* 文件名称          app_time
* 说明              系统节拍：TIM6 1ms 中断，中断里只做计数与 LED 翻转（细则 6）
********************************************************************************************************************/
#ifndef APP_TIME_H
#define APP_TIME_H

#include <stdint.h>
#include "app_config.h"

#ifndef APP_HOST_TEST
#include "zf_common_headfile.h"
#endif

extern volatile uint32_t app_tick_ms;            /* 毫秒节拍，中断里自增 */

void     app_time_init      (void);              /* pit_ms_init(TIM6_PIT, 1) */
void     app_time_isr_tick  (void);              /* 由 User/zf_isr.c 的 TIM6_IRQHandler 调用 */
uint32_t app_time_now       (void);
uint8_t  app_time_elapsed   (uint32_t start_ms, uint32_t span_ms);

#endif

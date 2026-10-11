/*********************************************************************************************************************
* 文件名称          app_buzzer
* 说明              无源蜂鸣器：PA8(TIM1_CH1) -> 20Ω -> AO3400A 栅极，高电平导通（细则 5）
*                   目标端专用
********************************************************************************************************************/
#ifndef APP_BUZZER_H
#define APP_BUZZER_H

#include <stdint.h>
#include "app_config.h"

void     app_buzzer_init    (void);
void     app_buzzer_beep    (uint32_t freq_hz, uint16_t duration_ms, uint32_t now_ms);
void     app_buzzer_stop    (void);
void     app_buzzer_task    (uint32_t now_ms);
uint8_t  app_buzzer_is_on   (void);
uint32_t app_buzzer_count   (void);
uint32_t app_buzzer_last_ms (void);
uint16_t app_buzzer_last_freq (void);

#endif

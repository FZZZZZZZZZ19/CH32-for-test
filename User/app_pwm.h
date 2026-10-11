/*********************************************************************************************************************
* 文件名称          app_pwm
* 说明              三路 PWM：H1-1(C6/TIM3_CH1)、H1-3(D14/TIM4_CH3)、H1-2(D15/TIM4_CH4)
*                   D14/D15 同属 TIM4，频率必须一致；C6 属 TIM3，本层统一用同一频率
*                   目标端专用（通过 app_port 访问硬件）
********************************************************************************************************************/
#ifndef APP_PWM_H
#define APP_PWM_H

#include <stdint.h>
#include "app_config.h"

void     app_pwm_init          (uint16_t freq_hz, uint16_t duty_permille);
void     app_pwm_set_all       (uint16_t freq_hz, uint16_t duty_permille);
void     app_pwm_set_channel   (uint8_t ch, uint16_t duty_permille);
uint16_t app_pwm_get_duty      (uint8_t ch);
uint16_t app_pwm_get_freq      (void);
void     app_pwm_sweep_enable  (uint8_t enable);
uint8_t  app_pwm_sweep_enabled (void);
void     app_pwm_task          (uint32_t now_ms);        /* 演示用占空比扫描 */

#endif

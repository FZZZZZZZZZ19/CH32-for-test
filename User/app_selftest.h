/*********************************************************************************************************************
* 文件名称          app_selftest
* 说明              上电自检：逐项驱动硬件并把结果同时输出到调试串口和屏幕（答辩时可现场复现）
*                   目标端专用
********************************************************************************************************************/
#ifndef APP_SELFTEST_H
#define APP_SELFTEST_H

#include <stdint.h>
#include "app_config.h"

#define APP_SELFTEST_LED        0x01u
#define APP_SELFTEST_KEY        0x02u
#define APP_SELFTEST_PWM        0x04u
#define APP_SELFTEST_BUZZER     0x08u
#define APP_SELFTEST_SCREEN     0x10u
#define APP_SELFTEST_FLASH      0x20u
#define APP_SELFTEST_EXTI       0x40u
#define APP_SELFTEST_PARAMS     0x80u

void        app_selftest_run    (void);
uint8_t     app_selftest_result (void);
uint8_t     app_selftest_failed (void);
const char *app_selftest_name   (uint8_t bit_index);

#endif

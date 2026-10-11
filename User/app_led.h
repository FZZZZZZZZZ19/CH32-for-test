/*********************************************************************************************************************
* 文件名称          app_led
* 说明              LED 驱动层：常亮/常灭/定时器中断闪烁（细则 6）/软件闪烁/流水灯（细则 3）/二进制显示
*                   目标端专用：硬件动作全部通过 app_port 完成
********************************************************************************************************************/
#ifndef APP_LED_H
#define APP_LED_H

#include <stdint.h>
#include "app_config.h"

typedef enum
{
    APP_LED_MODE_OFF = 0,                   /* 全灭 */
    APP_LED_MODE_ON,                        /* 全亮 */
    APP_LED_MODE_BLINK_ISR,                 /* 1ms 中断里翻转：细则 6 */
    APP_LED_MODE_BLINK_SW,                  /* 主循环软件定时翻转 */
    APP_LED_MODE_FLOW,                      /* 流水灯：细则 3 */
    APP_LED_MODE_BINARY                     /* 4 个 LED 显示 4bit 值 */
} app_led_mode_enum;

void                app_led_init             (void);
void                app_led_set_mode         (app_led_mode_enum mode);
app_led_mode_enum   app_led_mode             (void);
const char         *app_led_mode_name        (app_led_mode_enum mode);
void                app_led_set_period_ms    (uint16_t ms);
void                app_led_set_binary       (uint8_t value);
void                app_led_task             (uint32_t now_ms);
void                app_led_isr_tick         (void);        /* 1ms 中断调用 */
uint32_t            app_led_isr_toggle_count (void);
uint8_t             app_led_get              (uint8_t index);
uint8_t             app_led_mask             (void);
void                app_led_all_off          (void);

#endif

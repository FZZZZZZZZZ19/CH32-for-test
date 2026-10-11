/*********************************************************************************************************************
* 文件名称          app_ui
* 说明              界面层：把菜单状态 + 运行状态画到 1.8 寸屏上（细则 9/10 的 UI 交互界面）
*                   目标端专用（依赖 app_screen）
********************************************************************************************************************/
#ifndef APP_UI_H
#define APP_UI_H

#include <stdint.h>
#include "app_config.h"
#include "app_menu.h"
#include "app_key.h"

/*--------------------------------------------------- 状态栏数据 -------------------------------------------------*/
typedef struct
{
    uint8_t  task_no;                       /* 当前对应的考核条目号 */
    uint8_t  demo_id;                       /* 当前演示功能编号 */
    uint8_t  demo_running;                  /* 是否处于演示态 */
    uint8_t  key_code;                      /* 最近一次 74HC148 编码 */
    uint8_t  key_id;                        /* 编码翻译后的按键编号（app_key_enum） */
    uint8_t  led_mask;                      /* 4 个 LED 的当前状态 */
    uint16_t pwm_freq_hz;
    uint16_t pwm_duty;
    uint8_t  buzzer_on;
    uint32_t exti_count;                    /* PD8 外部中断计数 */
    uint32_t exti_last_ms;
    uint8_t  exti_level;                    /* PD8 当前电平 */
    uint32_t isr_toggles;                   /* 定时器中断里翻转 LED 的次数 */
    uint8_t  flash_slot;                    /* 参数所在槽位 */
    uint32_t flash_seq;                     /* 参数序号（每次保存 +1） */
    uint16_t boot_count;
    uint32_t uptime_ms;
} app_ui_status_t;

void app_ui_init      (void);
void app_ui_render    (const app_menu_t *m, const app_params_t *p, const app_ui_status_t *st);
void app_ui_demo_frame(const char *title, const app_ui_status_t *st);
void app_ui_line      (uint8_t row, const char *text);
void app_ui_kv        (uint8_t row, const char *key, uint32_t value, uint8_t digits);
void app_ui_toast     (const char *text, uint32_t now_ms);

#endif

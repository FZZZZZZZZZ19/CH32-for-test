/*********************************************************************************************************************
* 文件名称          app_demo
* 说明              11 个现场演示功能（与考核细则条目一一对应），运行时由菜单切换，不需要重新烧录
*                   目标端专用
********************************************************************************************************************/
#ifndef APP_DEMO_H
#define APP_DEMO_H

#include <stdint.h>
#include "app_config.h"
#include "app_key.h"
#include "app_ui.h"

/*--------------------------------------------------- 演示上下文 ------------------------------------------------*/
typedef struct
{
    uint32_t         now_ms;
    uint8_t          key_code;              /* 74HC148 原始编码 */
    app_key_enum     key_id;
    uint8_t          click;                 /* 短按事件 */
    uint8_t          long_press;            /* 长按事件 */
    app_ui_status_t *status;                /* 可写：演示过程更新状态栏 */
} app_demo_ctx_t;

void     app_demo_start  (uint8_t demo_id, uint32_t now_ms);
void     app_demo_stop   (void);
uint8_t  app_demo_active (void);
uint8_t  app_demo_id     (void);
uint8_t  app_demo_task   (const app_demo_ctx_t *ctx);   /* 返回 1 = 继续演示 */

#endif

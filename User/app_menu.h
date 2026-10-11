/*********************************************************************************************************************
* 文件名称          app_menu
* 说明              三级菜单状态机（纯逻辑，可主机端单测，不包含任何硬件头文件）
*                   层级：MAIN(1) -> DEMO/PARAMS/SYSTEM(2) -> EDIT(3)
*                   按键语义：UP/DOWN 移动光标，ENTER 进入/提交，BACK 返回/取消，INC/DEC 编辑时增减
********************************************************************************************************************/
#ifndef APP_MENU_H
#define APP_MENU_H

#include <stdint.h>
#include "app_config.h"
#include "app_params.h"

/*--------------------------------------------------- 页面与动作 -------------------------------------------------*/
typedef enum
{
    APP_MENU_PAGE_MAIN = 0,                 /* 一级：DEMO / PARAMETERS / SYSTEM */
    APP_MENU_PAGE_DEMO,                     /* 二级：11 个演示功能列表 */
    APP_MENU_PAGE_PARAM,                    /* 二级：8 个参数列表 */
    APP_MENU_PAGE_EDIT,                     /* 三级：单个参数编辑 */
    APP_MENU_PAGE_SYSTEM,                   /* 二级：保存/重载/恢复/存储自测/自检 */
    APP_MENU_PAGE_NUMBER
} app_menu_page_enum;

typedef enum
{
    APP_ACTION_NONE = 0,
    APP_ACTION_DEMO_START,                  /* 进入某个演示功能 */
    APP_ACTION_PARAM_COMMIT,                /* 参数已确认写入（可触发保存） */
    APP_ACTION_SAVE,                        /* 保存参数到 flash */
    APP_ACTION_RELOAD,                      /* 从 flash 重新读取参数 */
    APP_ACTION_FACTORY,                     /* 恢复默认参数 */
    APP_ACTION_FLASH_TEST,                  /* 读写演示页自测 */
    APP_ACTION_SELFTEST                     /* 整机自检 */
} app_menu_action_enum;

#define APP_MENU_EDIT_NONE      0xFFu
#define APP_MENU_CURSOR_NONE    0xFFu

/*--------------------------------------------------- 菜单上下文 -------------------------------------------------*/
typedef struct
{
    app_menu_page_enum   page;
    uint8_t              cursor;            /* 当前页光标 */
    uint8_t              edit_id;           /* 正在编辑的参数 id */
    uint16_t             edit_value;        /* 编辑中的临时值（未提交，BACK 直接丢弃即取消） */
    uint8_t              demo_id;           /* 最近一次进入的演示功能编号 */
    app_menu_action_enum action;            /* 待上层消费的动作 */
    uint32_t             event_count;       /* 按键事件累计（自证/答辩用） */
} app_menu_t;

/*--------------------------------------------------- 一次按键输入 ------------------------------------------------*/
typedef struct
{
    uint32_t now_ms;
    uint8_t  up;
    uint8_t  down;
    uint8_t  enter;
    uint8_t  back;
    uint8_t  inc;
    uint8_t  dec;
} app_menu_input_t;

/*--------------------------------------------------- 接口 ------------------------------------------------------*/
void                 app_menu_init        (app_menu_t *m);
void                 app_menu_goto_main   (app_menu_t *m);
void                 app_menu_apply       (app_menu_t *m, app_params_t *p, const app_menu_input_t *in);
app_menu_action_enum app_menu_take_action (app_menu_t *m);

const char          *app_menu_page_title  (const app_menu_t *m);
uint8_t              app_menu_depth       (const app_menu_t *m);
uint8_t              app_menu_item_count  (const app_menu_t *m);
uint8_t              app_menu_cursor      (const app_menu_t *m);
uint8_t              app_menu_is_editing  (const app_menu_t *m);
void                 app_menu_item_text   (const app_menu_t *m, const app_params_t *p, uint8_t index, char *buf, uint8_t len);
const char          *app_demo_name        (uint8_t demo_id);

#endif

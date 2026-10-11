/*********************************************************************************************************************
* 文件名称          test_ui
* 说明              主机端单测：用真实的 User/app_ui.c（配合 stub_screen.c 记录上屏内容）
*                   检查界面层产生的每一行都不超过 16 列，并遍历所有页面/光标/参数编辑态。
*                   这条测试就是本次"屏幕一直显示 Assert error ... zf_device_tft180.c:409"的回归测试：
*                   修复前状态栏 "#1 L15 E999 T999 F1 9999" 有 24 个字符，必然越界触发断言。
********************************************************************************************************************/
#include <string.h>
#include "test_util.h"
#include "app_config.h"
#include "app_ui.h"
#include "app_menu.h"
#include "app_params.h"
#include "app_key.h"

/* stub_screen.c / stub_time.c 提供 */
extern char              g_stub_row_text[APP_SCREEN_ROWS][APP_SCREEN_TEXT_BUF + 1];
extern uint8_t           g_stub_row_col [APP_SCREEN_ROWS];
extern uint8_t           g_stub_row_valid[APP_SCREEN_ROWS];
extern volatile uint32_t app_tick_ms;
void host_screen_reset (void);

static uint8_t s_worst_width;

/* 任何一行：起始列 + 字符数 必须 <= 16（8x16 字体、128 像素宽）=> x + 8*j < 128 */
static void check_rows_fit (void)
{
    uint8_t row;

    for(row = 0u; row < APP_SCREEN_ROWS; row ++)
    {
        if(0u != g_stub_row_valid[row])
        {
            size_t width = (size_t)g_stub_row_col[row] + strlen(g_stub_row_text[row]);
            CHECK(width <= (size_t)APP_SCREEN_COLS);
            if(width > (size_t)s_worst_width)
            {
                s_worst_width = (uint8_t)width;
            }
        }
    }
}

static void press (app_menu_t *m, app_params_t *p, uint8_t up, uint8_t down, uint8_t enter, uint8_t back, uint8_t inc)
{
    app_menu_input_t in;

    memset(&in, 0, sizeof(in));
    in.up    = up;
    in.down  = down;
    in.enter = enter;
    in.back  = back;
    in.inc   = inc;
    app_menu_apply(m, p, &in);
}

static void render_page (app_menu_t *m, app_params_t *p, const app_ui_status_t *st)
{
    host_screen_reset();
    app_ui_render(m, p, st);
    check_rows_fit();
}

int test_ui (void)
{
    app_ui_status_t st;
    app_menu_t      menu;
    app_params_t    params;
    uint8_t         i;

    app_ui_init();

    /* 1) 单行文本：修复后 16 列 */
    host_screen_reset();
    app_ui_line(5u, "5 PWM C6/D14/D15");
    CHECK_EQ(strlen(g_stub_row_text[5]), 16u);
    check_rows_fit();

    /* 2) app_ui_kv 最长键 + 最大数值 */
    host_screen_reset();
    app_ui_kv(6u, "TOGGLE", 99999u, 5u);
    CHECK_EQ(strlen(g_stub_row_text[6]), 12u);
    check_rows_fit();

    /* 3) 状态栏最坏取值（task 11 / mask 15 / exti 2999 / isr 4999） */
    memset(&st, 0, sizeof(st));
    st.task_no     = 11u;
    st.led_mask    = 15u;
    st.exti_count  = 2999u;
    st.isr_toggles = 4999u;
    st.flash_slot  = 1u;
    st.flash_seq   = 9999u;
    st.key_code    = 7u;
    st.key_id      = 0u;

    host_screen_reset();
    app_ui_demo_frame("WCH BOARD", &st);
    CHECK(g_stub_row_valid[9] != 0u);
    CHECK(strlen(g_stub_row_text[9]) <= 16u);
    check_rows_fit();

    /* 4) 遍历菜单：MAIN(3) -> DEMO(11) -> PARAM(8, 每个都进三级编辑态) -> SYSTEM(5)
          每一步都用页标题断言确认真的在同一页上，避免"按键没生效导致覆盖率不足"。 */
    app_params_defaults(&params);
    app_menu_init(&menu);

    s_worst_width = 0u;

    for(i = 0u; i < 3u; i ++)                               /* MAIN 三个条目 */
    {
        CHECK(strcmp(app_menu_page_title(&menu), "WCH MAIN") == 0);
        render_page(&menu, &params, &st);
        press(&menu, &params, 0u, 1u, 0u, 0u, 0u);          /* 三次 DOWN 后回绕到 0 = DEMO */
    }
    CHECK_EQ(app_menu_cursor(&menu), 0u);

    press(&menu, &params, 0u, 0u, 1u, 0u, 0u);              /* 进入 DEMO 列表 */
    CHECK_EQ(app_menu_depth(&menu), 2u);
    CHECK(strcmp(app_menu_page_title(&menu), "DEMO LIST") == 0);
    for(i = 0u; i < APP_DEMO_NUM; i ++)                     /* 11 个演示条目 */
    {
        render_page(&menu, &params, &st);
        press(&menu, &params, 0u, 1u, 0u, 0u, 0u);
    }
    CHECK_EQ(app_menu_cursor(&menu), 0u);                   /* 11 次 DOWN 回绕 */

    press(&menu, &params, 0u, 0u, 0u, 1u, 0u);              /* 回 MAIN（光标复位到 0） */
    CHECK(strcmp(app_menu_page_title(&menu), "WCH MAIN") == 0);
    press(&menu, &params, 0u, 1u, 0u, 0u, 0u);              /* 0 -> 1 = PARAMETERS */
    CHECK_EQ(app_menu_cursor(&menu), 1u);
    press(&menu, &params, 0u, 0u, 1u, 0u, 0u);              /* 进入参数列表 */
    CHECK(strcmp(app_menu_page_title(&menu), "PARAMS") == 0);
    for(i = 0u; i < APP_PARAM_NUM; i ++)                    /* 8 个参数 */
    {
        render_page(&menu, &params, &st);
        press(&menu, &params, 0u, 0u, 1u, 0u, 0u);          /* 进入三级编辑 */
        CHECK_EQ(app_menu_is_editing(&menu), 1u);
        CHECK_EQ(app_menu_depth(&menu), 3u);
        CHECK(strcmp(app_menu_page_title(&menu), "PARAM EDIT") == 0);
        press(&menu, &params, 0u, 0u, 0u, 0u, 1u);          /* INC 改一个步进 */
        render_page(&menu, &params, &st);
        CHECK(g_stub_row_valid[8] != 0u);                   /* 详情行 "min-max/step" */
        CHECK(strlen(g_stub_row_text[8]) <= 16u);
        press(&menu, &params, 0u, 0u, 0u, 1u, 0u);          /* BACK 取消，回参数列表 */
        CHECK(strcmp(app_menu_page_title(&menu), "PARAMS") == 0);
        render_page(&menu, &params, &st);
        press(&menu, &params, 0u, 1u, 0u, 0u, 0u);          /* 下一个参数 */
    }

    press(&menu, &params, 0u, 0u, 0u, 1u, 0u);              /* 回 MAIN（光标复位到 0） */
    CHECK(strcmp(app_menu_page_title(&menu), "WCH MAIN") == 0);
    press(&menu, &params, 0u, 1u, 0u, 0u, 0u);              /* 0 -> 1 */
    press(&menu, &params, 0u, 1u, 0u, 0u, 0u);              /* 1 -> 2 = SYSTEM */
    CHECK_EQ(app_menu_cursor(&menu), 2u);
    press(&menu, &params, 0u, 0u, 1u, 0u, 0u);              /* 进入系统页 */
    CHECK(strcmp(app_menu_page_title(&menu), "SYSTEM") == 0);
    for(i = 0u; i < 5u; i ++)                               /* 5 个系统动作 */
    {
        render_page(&menu, &params, &st);
        press(&menu, &params, 0u, 1u, 0u, 0u, 0u);
    }

    /* 5) 提示行（toast）与状态栏同时出现 */
    app_tick_ms = 100u;
    app_ui_toast("SELFTEST DONE", app_tick_ms);
    render_page(&menu, &params, &st);
    CHECK(strlen(g_stub_row_text[8]) <= 16u);

    /* 记录本次遍历里最长的一行，便于答辩时说明余量 */
    CHECK(s_worst_width > 0u);
    CHECK(s_worst_width <= 16u);

    return 0;
}

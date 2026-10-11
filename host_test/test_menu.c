/*********************************************************************************************************************
* 文件名称          test_menu
* 说明              app_menu 单元测试：三级菜单层级、光标环绕、进入/退出、参数编辑提交与取消、系统动作映射
********************************************************************************************************************/
#include <string.h>
#include "test_util.h"
#include "app_menu.h"

/*--------------------------------------------------- 输入构造 ---------------------------------------------------*/
static void menu_input (app_menu_t *m, app_params_t *p, uint32_t now_ms,
                        int up, int down, int enter, int back, int inc, int dec)
{
    app_menu_input_t in;

    memset(&in, 0, sizeof(in));
    in.now_ms = now_ms;
    in.up     = (uint8_t)(up    ? 1u : 0u);
    in.down   = (uint8_t)(down  ? 1u : 0u);
    in.enter  = (uint8_t)(enter ? 1u : 0u);
    in.back   = (uint8_t)(back  ? 1u : 0u);
    in.inc    = (uint8_t)(inc   ? 1u : 0u);
    in.dec    = (uint8_t)(dec   ? 1u : 0u);

    app_menu_apply(m, p, &in);
}

static void menu_enter_page (app_menu_t *m, app_params_t *p, uint8_t main_index)
{
    uint8_t i;

    app_menu_init(m);
    for(i = 0u; i < main_index; i ++)
    {
        menu_input(m, p, (uint32_t)(10u * i), 0, 1, 0, 0, 0, 0);    /* 主菜单 DOWN 移动光标 */
    }
    menu_input(m, p, 100u, 0, 0, 1, 0, 0, 0);                       /* ENTER 进入 */
}

/*--------------------------------------------------- 层级与导航 -------------------------------------------------*/
static int test_navigation (void)
{
    app_menu_t   m;
    app_params_t p;

    app_params_defaults(&p);
    app_menu_init(&m);

    CHECK_EQ(m.page, APP_MENU_PAGE_MAIN);
    CHECK_EQ(app_menu_depth(&m), 1u);
    CHECK_EQ(app_menu_item_count(&m), 3u);
    CHECK_EQ(app_menu_cursor(&m), 0u);
    CHECK_EQ(app_menu_is_editing(&m), 0u);
    CHECK(app_menu_page_title(&m) != 0);
    CHECK_EQ((int)app_menu_take_action(&m), (int)APP_ACTION_NONE);

    /* 主菜单 3 项循环移动 */
    menu_input(&m, &p, 10u, 0, 1, 0, 0, 0, 0);
    CHECK_EQ(app_menu_cursor(&m), 1u);
    menu_input(&m, &p, 20u, 0, 1, 0, 0, 0, 0);
    CHECK_EQ(app_menu_cursor(&m), 2u);
    menu_input(&m, &p, 30u, 0, 1, 0, 0, 0, 0);
    CHECK_EQ(app_menu_cursor(&m), 0u);
    menu_input(&m, &p, 40u, 1, 0, 0, 0, 0, 0);
    CHECK_EQ(app_menu_cursor(&m), 2u);

    /* 主菜单 0 号 -> 演示列表 */
    menu_enter_page(&m, &p, 0u);
    CHECK_EQ(m.page, APP_MENU_PAGE_DEMO);
    CHECK_EQ(app_menu_depth(&m), 2u);
    CHECK_EQ(app_menu_item_count(&m), APP_DEMO_NUM);
    CHECK_EQ(app_menu_cursor(&m), 0u);
    CHECK(app_menu_page_title(&m) != 0);

    /* 演示列表：DOWN 移动、ENTER 产生"启动演示"动作、BACK 回主菜单 */
    menu_input(&m, &p, 10u, 0, 1, 0, 0, 0, 0);
    CHECK_EQ(app_menu_cursor(&m), 1u);
    menu_input(&m, &p, 20u, 0, 0, 1, 0, 0, 0);
    CHECK_EQ(m.demo_id, 1u);
    CHECK_EQ((int)app_menu_take_action(&m), (int)APP_ACTION_DEMO_START);
    CHECK_EQ((int)app_menu_take_action(&m), (int)APP_ACTION_NONE);      /* 取走后清空 */
    menu_input(&m, &p, 30u, 0, 0, 0, 1, 0, 0);
    CHECK_EQ(m.page, APP_MENU_PAGE_MAIN);

    /* 主菜单 1 号 -> 参数列表 */
    menu_enter_page(&m, &p, 1u);
    CHECK_EQ(m.page, APP_MENU_PAGE_PARAM);
    CHECK_EQ(app_menu_item_count(&m), APP_PARAM_NUM);
    CHECK_EQ(app_menu_depth(&m), 2u);
    menu_input(&m, &p, 10u, 0, 0, 0, 1, 0, 0);
    CHECK_EQ(m.page, APP_MENU_PAGE_MAIN);

    /* 主菜单 2 号 -> 系统页 */
    menu_enter_page(&m, &p, 2u);
    CHECK_EQ(m.page, APP_MENU_PAGE_SYSTEM);
    CHECK_EQ(app_menu_item_count(&m), 5u);
    CHECK_EQ(app_menu_depth(&m), 2u);

    /* 事件计数随按键增加（答辩自证用） */
    CHECK(m.event_count > 0u);

    return 0;
}

/*--------------------------------------------------- 参数编辑 ---------------------------------------------------*/
static int test_param_edit (void)
{
    app_menu_t   m;
    app_params_t p;
    uint8_t      i;

    /* 走到 PARAMETERS 页，再移动到 PWM DUTY（id = APP_PARAM_PWM_DUTY） */
    app_params_defaults(&p);
    menu_enter_page(&m, &p, 1u);
    CHECK_EQ(m.page, APP_MENU_PAGE_PARAM);

    for(i = 0u; i < (uint8_t)APP_PARAM_PWM_DUTY; i ++)
    {
        menu_input(&m, &p, (uint32_t)(10u * i), 0, 1, 0, 0, 0, 0);
    }
    CHECK_EQ(app_menu_cursor(&m), (uint8_t)APP_PARAM_PWM_DUTY);

    /* ENTER 进入第三级编辑页：初值 = 当前参数值 */
    menu_input(&m, &p, 200u, 0, 0, 1, 0, 0, 0);
    CHECK_EQ(m.page, APP_MENU_PAGE_EDIT);
    CHECK_EQ(app_menu_depth(&m), 3u);
    CHECK_EQ(app_menu_is_editing(&m), 1u);
    CHECK_EQ(m.edit_id, (uint8_t)APP_PARAM_PWM_DUTY);
    CHECK_EQ(m.edit_value, 500u);

    /* INC 两次：500 -> 520（步进 10） */
    menu_input(&m, &p, 210u, 0, 0, 0, 0, 1, 0);
    CHECK_EQ(m.edit_value, 510u);
    menu_input(&m, &p, 220u, 0, 0, 0, 0, 1, 0);
    CHECK_EQ(m.edit_value, 520u);

    /* 编辑中不写回参数结构 */
    CHECK_EQ(app_param_get(&p, APP_PARAM_PWM_DUTY), 500u);

    /* ENTER 提交：写回参数、回上一级、产生提交动作 */
    menu_input(&m, &p, 230u, 0, 0, 1, 0, 0, 0);
    CHECK_EQ(m.page, APP_MENU_PAGE_PARAM);
    CHECK_EQ(app_param_get(&p, APP_PARAM_PWM_DUTY), 520u);
    CHECK_EQ((int)app_menu_take_action(&m), (int)APP_ACTION_PARAM_COMMIT);

    /* 重新进入编辑页：初值为刚提交的值 */
    menu_input(&m, &p, 240u, 0, 0, 1, 0, 0, 0);
    CHECK_EQ(m.page, APP_MENU_PAGE_EDIT);
    CHECK_EQ(m.edit_value, 520u);

    /* BACK：取消编辑，参数保持原值 */
    menu_input(&m, &p, 250u, 0, 0, 0, 1, 0, 0);
    CHECK_EQ(m.page, APP_MENU_PAGE_PARAM);
    CHECK_EQ(app_param_get(&p, APP_PARAM_PWM_DUTY), 520u);

    /* 下界夹取：0 - 10 -> 0 */
    app_param_set(&p, APP_PARAM_PWM_DUTY, 5u);
    menu_input(&m, &p, 260u, 0, 0, 1, 0, 0, 0);
    CHECK_EQ(m.edit_value, 5u);
    menu_input(&m, &p, 270u, 0, 0, 0, 0, 0, 1);
    CHECK_EQ(m.edit_value, 0u);

    /* 上界夹取：9990 + 10 -> 1000 */
    app_param_set(&p, APP_PARAM_PWM_DUTY, 9990u);
    m.edit_value = 9990u;
    menu_input(&m, &p, 280u, 0, 0, 0, 0, 1, 0);
    CHECK_EQ(m.edit_value, 1000u);

    /* 开关型参数（BACKLIGHT）：任意方向键取反 */
    app_params_defaults(&p);
    app_menu_init(&m);
    menu_enter_page(&m, &p, 1u);
    for(i = 0u; i < (uint8_t)APP_PARAM_BACKLIGHT; i ++)
    {
        menu_input(&m, &p, (uint32_t)(10u * i), 0, 1, 0, 0, 0, 0);
    }
    menu_input(&m, &p, 300u, 0, 0, 1, 0, 0, 0);
    CHECK_EQ(m.page, APP_MENU_PAGE_EDIT);
    CHECK_EQ(m.edit_value, 1u);
    menu_input(&m, &p, 310u, 0, 0, 0, 0, 1, 0);
    CHECK_EQ(m.edit_value, 0u);
    menu_input(&m, &p, 320u, 0, 0, 0, 0, 1, 0);
    CHECK_EQ(m.edit_value, 1u);
    menu_input(&m, &p, 330u, 0, 0, 0, 1, 0, 0);                     /* BACK 取消 */
    CHECK_EQ(app_param_get(&p, APP_PARAM_BACKLIGHT), 1u);

    return 0;
}

/*--------------------------------------------------- 系统页动作 -------------------------------------------------*/
static int test_system_actions (void)
{
    static const app_menu_action_enum expect[5] =
    {
        APP_ACTION_SAVE,
        APP_ACTION_RELOAD,
        APP_ACTION_FACTORY,
        APP_ACTION_FLASH_TEST,
        APP_ACTION_SELFTEST,
    };
    app_menu_t   m;
    app_params_t p;
    uint8_t      i;

    app_params_defaults(&p);

    for(i = 0u; i < 5u; i ++)
    {
        uint8_t j;

        menu_enter_page(&m, &p, 2u);
        CHECK_EQ(m.page, APP_MENU_PAGE_SYSTEM);

        for(j = 0u; j < i; j ++)
        {
            menu_input(&m, &p, (uint32_t)(10u * j), 0, 1, 0, 0, 0, 0);
        }
        CHECK_EQ(app_menu_cursor(&m), i);

        menu_input(&m, &p, 400u, 0, 0, 1, 0, 0, 0);
        CHECK_EQ((int)app_menu_take_action(&m), (int)expect[i]);
        CHECK_EQ((int)app_menu_take_action(&m), (int)APP_ACTION_NONE);
    }

    return 0;
}

/*--------------------------------------------------- 文本接口 ---------------------------------------------------*/
static int test_item_text (void)
{
    app_menu_t   m;
    app_params_t p;
    char         buf[APP_SCREEN_TEXT_BUF];

    app_params_defaults(&p);
    app_menu_init(&m);

    memset(buf, 0, sizeof(buf));
    app_menu_item_text(&m, &p, 0u, buf, (uint8_t)sizeof(buf));
    CHECK(buf[0] != '\0');
    CHECK_EQ(strncmp(buf, "DEMO", 4), 0);

    memset(buf, 0, sizeof(buf));
    app_menu_item_text(&m, &p, 1u, buf, (uint8_t)sizeof(buf));
    CHECK_EQ(strncmp(buf, "PARAMETERS", 10), 0);

    memset(buf, 0, sizeof(buf));
    app_menu_item_text(&m, &p, 2u, buf, (uint8_t)sizeof(buf));
    CHECK_EQ(strncmp(buf, "SYSTEM", 6), 0);

    /* 参数页显示 名称 + 当前值 */
    menu_enter_page(&m, &p, 1u);
    memset(buf, 0, sizeof(buf));
    app_menu_item_text(&m, &p, 0u, buf, (uint8_t)sizeof(buf));
    CHECK_EQ(strncmp(buf, "BOOT DEMO", 9), 0);

    menu_input(&m, &p, 500u, 0, 1, 0, 0, 0, 0);
    memset(buf, 0, sizeof(buf));
    app_menu_item_text(&m, &p, 1u, buf, (uint8_t)sizeof(buf));
    CHECK_EQ(strncmp(buf, "BLINK MS", 8), 0);

    /* 编辑页显示正在编辑的参数名 */
    menu_input(&m, &p, 510u, 0, 0, 1, 0, 0, 0);
    CHECK_EQ(m.page, APP_MENU_PAGE_EDIT);
    memset(buf, 0, sizeof(buf));
    app_menu_item_text(&m, &p, 0u, buf, (uint8_t)sizeof(buf));
    CHECK_EQ(strncmp(buf, "BLINK MS", 8), 0);

    /* 演示名称表 */
    CHECK_EQ(strncmp(app_demo_name(0u), "OVERVIEW", 8), 0);
    CHECK_EQ(strncmp(app_demo_name(1u), "LED", 3), 0);
    CHECK_EQ(strncmp(app_demo_name((uint8_t)(APP_DEMO_NUM - 1u)), "MENU", 4), 0);
    CHECK_EQ(strncmp(app_demo_name(200u), "----", 4), 0);

    /* 缓冲区很小时不越界（只检查不崩溃且以 0 结尾） */
    memset(buf, 'X', sizeof(buf));
    app_menu_item_text(&m, &p, 0u, buf, 4u);
    CHECK_EQ(buf[3], '\0');

    return 0;
}

int test_menu (void)
{
    test_navigation();
    test_param_edit();
    test_system_actions();
    test_item_text();
    return 0;
}

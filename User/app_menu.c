/*********************************************************************************************************************
* 文件名称          app_menu
* 说明              三级菜单状态机（纯逻辑，可主机端单测）
********************************************************************************************************************/
#include "app_menu.h"

/*--------------------------------------------------- 菜单项定义 -------------------------------------------------*/
#define MENU_MAIN_ITEM_DEMO     0u
#define MENU_MAIN_ITEM_PARAM    1u
#define MENU_MAIN_ITEM_SYSTEM   2u
#define MENU_MAIN_ITEM_NUM      3u

#define MENU_SYS_ITEM_SAVE      0u
#define MENU_SYS_ITEM_RELOAD    1u
#define MENU_SYS_ITEM_FACTORY   2u
#define MENU_SYS_ITEM_FLASH     3u
#define MENU_SYS_ITEM_SELFTEST  4u
#define MENU_SYS_ITEM_NUM       5u

static const char *const s_main_name[MENU_MAIN_ITEM_NUM] =
{
    "DEMO", "PARAMETERS", "SYSTEM"
};

static const char *const s_sys_name[MENU_SYS_ITEM_NUM] =
{
    "SAVE PARAMS", "RELOAD", "FACTORY", "FLASH R/W", "SELF TEST"
};

static const char *const s_demo_name[APP_DEMO_NUM] =
{
    "OVERVIEW",     /* 0  总览 */
    "LED",          /* 1  细则 3 */
    "KEYS",         /* 2  细则 4 */
    "PWM OUT",      /* 3  细则 5 */
    "BUZZER",       /* 4  细则 5 */
    "TIMER LED",    /* 5  细则 6 */
    "SCREEN",       /* 6  细则 7 */
    "FLASH R/W",    /* 7  细则 8 */
    "EXTI KEY",     /* 8  细则 9 */
    "UI PANEL",     /* 9  细则 9 */
    "MENU"          /* 10 细则 10 */
};

/*--------------------------------------------------- 字符串小工具（不依赖 string.h） -----------------------------*/
static uint8_t menu_str_len (const char *s)
{
    uint8_t n = 0u;
    if(0 == s)
    {
        return 0u;
    }
    while(('\0' != s[n]) && (n < 250u))
    {
        n ++;
    }
    return n;
}

static void menu_str_copy (char *buf, uint8_t len, const char *s)
{
    uint8_t i = 0u;

    if((0 == buf) || (0u == len))
    {
        return;
    }
    if(0 == s)
    {
        s = "";
    }
    while((i < (uint8_t)(len - 1u)) && ('\0' != s[i]))
    {
        buf[i] = s[i];
        i ++;
    }
    buf[i] = '\0';
}

static void menu_str_append (char *buf, uint8_t len, const char *s)
{
    uint8_t i = menu_str_len(buf);

    if((0 == buf) || (0 == s))
    {
        return;
    }
    if(i >= (uint8_t)(len - 1u))
    {
        return;
    }
    while((i < (uint8_t)(len - 1u)) && ('\0' != *s))
    {
        buf[i] = *s;
        i ++;
        s ++;
    }
    buf[i] = '\0';
}

static void menu_u32_append (char *buf, uint8_t len, uint32_t value, uint8_t width)
{
    char    tmp[11];
    uint8_t n = 0u;
    uint8_t i;
    uint8_t pad;

    if((0 == buf) || (0u == len))
    {
        return;
    }

    do
    {
        tmp[n] = (char)('0' + (value % 10u));
        value /= 10u;
        n ++;
    } while((value > 0u) && (n < 10u));

    pad = (width > n) ? (uint8_t)(width - n) : 0u;
    for(i = 0u; i < pad; i ++)
    {
        menu_str_append(buf, len, " ");
    }
    while(n > 0u)
    {
        char one[2];
        n --;
        one[0] = tmp[n];
        one[1] = '\0';
        menu_str_append(buf, len, one);
    }
}

/*--------------------------------------------------- 初始化 / 跳转 ----------------------------------------------*/
void app_menu_init (app_menu_t *m)
{
    if(0 == m)
    {
        return;
    }
    m->page        = APP_MENU_PAGE_MAIN;
    m->cursor      = 0u;
    m->edit_id     = APP_MENU_EDIT_NONE;
    m->edit_value  = 0u;
    m->demo_id     = 0xFFu;
    m->action      = APP_ACTION_NONE;
    m->event_count = 0u;
}

void app_menu_goto_main (app_menu_t *m)
{
    if(0 == m)
    {
        return;
    }
    m->page    = APP_MENU_PAGE_MAIN;
    m->cursor  = 0u;
    m->edit_id = APP_MENU_EDIT_NONE;
}

/*--------------------------------------------------- 按键处理 ---------------------------------------------------*/
void app_menu_apply (app_menu_t *m, app_params_t *p, const app_menu_input_t *in)
{
    uint8_t  sel;
    uint16_t value;
    uint16_t step;

    if((0 == m) || (0 == p) || (0 == in))
    {
        return;
    }

    if(in->up || in->down || in->enter || in->back || in->inc || in->dec)
    {
        m->event_count ++;
    }

    switch(m->page)
    {
        case APP_MENU_PAGE_MAIN:
        {
            if(in->up)
            {
                m->cursor = (0u == m->cursor) ? (uint8_t)(MENU_MAIN_ITEM_NUM - 1u) : (uint8_t)(m->cursor - 1u);
            }
            if(in->down)
            {
                m->cursor = (uint8_t)((m->cursor + 1u) % MENU_MAIN_ITEM_NUM);
            }
            if(in->enter)
            {
                sel = m->cursor;
                app_menu_goto_main(m);
                if(MENU_MAIN_ITEM_DEMO == sel)
                {
                    m->page = APP_MENU_PAGE_DEMO;
                }
                else if(MENU_MAIN_ITEM_PARAM == sel)
                {
                    m->page = APP_MENU_PAGE_PARAM;
                }
                else
                {
                    m->page = APP_MENU_PAGE_SYSTEM;
                }
            }
            break;
        }

        case APP_MENU_PAGE_DEMO:
        {
            if(in->up)
            {
                m->cursor = (0u == m->cursor) ? (uint8_t)(APP_DEMO_NUM - 1u) : (uint8_t)(m->cursor - 1u);
            }
            if(in->down)
            {
                m->cursor = (uint8_t)((m->cursor + 1u) % APP_DEMO_NUM);
            }
            if(in->enter)
            {
                m->demo_id = m->cursor;
                m->action  = APP_ACTION_DEMO_START;
            }
            else if(in->back)
            {
                app_menu_goto_main(m);
            }
            break;
        }

        case APP_MENU_PAGE_PARAM:
        {
            if(in->up)
            {
                m->cursor = (0u == m->cursor) ? (uint8_t)(APP_PARAM_NUM - 1u) : (uint8_t)(m->cursor - 1u);
            }
            if(in->down)
            {
                m->cursor = (uint8_t)((m->cursor + 1u) % APP_PARAM_NUM);
            }
            if(in->enter)
            {
                m->edit_id    = m->cursor;
                m->edit_value = app_param_get(p, m->edit_id);
                m->page       = APP_MENU_PAGE_EDIT;
            }
            else if(in->back)
            {
                app_menu_goto_main(m);
            }
            break;
        }

        case APP_MENU_PAGE_EDIT:
        {
            if(m->edit_id >= APP_PARAM_NUM)
            {
                m->edit_id = APP_MENU_EDIT_NONE;
                m->page    = APP_MENU_PAGE_PARAM;
                break;
            }

            value = m->edit_value;
            step  = app_param_meta[m->edit_id].step;

            if(app_param_meta[m->edit_id].is_bool)
            {
                if(in->up || in->down || in->inc || in->dec)
                {
                    value = (0u == value) ? 1u : 0u;
                }
            }
            else
            {
                if(in->up || in->inc)
                {
                    value = (uint16_t)(value + step);
                }
                if(in->down || in->dec)
                {
                    value = (value > step) ? (uint16_t)(value - step) : app_param_meta[m->edit_id].min;
                }
            }
            m->edit_value = app_param_clamp(m->edit_id, value);

            if(in->enter)
            {
                app_param_set(p, m->edit_id, m->edit_value);
                m->edit_id = APP_MENU_EDIT_NONE;
                m->page    = APP_MENU_PAGE_PARAM;
                m->action  = APP_ACTION_PARAM_COMMIT;
            }
            else if(in->back)
            {
                m->edit_id = APP_MENU_EDIT_NONE;
                m->page    = APP_MENU_PAGE_PARAM;
            }
            break;
        }

        case APP_MENU_PAGE_SYSTEM:
        {
            if(in->up)
            {
                m->cursor = (0u == m->cursor) ? (uint8_t)(MENU_SYS_ITEM_NUM - 1u) : (uint8_t)(m->cursor - 1u);
            }
            if(in->down)
            {
                m->cursor = (uint8_t)((m->cursor + 1u) % MENU_SYS_ITEM_NUM);
            }
            if(in->enter)
            {
                switch(m->cursor)
                {
                    case MENU_SYS_ITEM_SAVE:     m->action = APP_ACTION_SAVE;      break;
                    case MENU_SYS_ITEM_RELOAD:   m->action = APP_ACTION_RELOAD;    break;
                    case MENU_SYS_ITEM_FACTORY:  m->action = APP_ACTION_FACTORY;   break;
                    case MENU_SYS_ITEM_FLASH:    m->action = APP_ACTION_FLASH_TEST;break;
                    case MENU_SYS_ITEM_SELFTEST: m->action = APP_ACTION_SELFTEST;  break;
                    default: break;
                }
            }
            else if(in->back)
            {
                app_menu_goto_main(m);
            }
            break;
        }

        default:
        {
            app_menu_goto_main(m);
            break;
        }
    }
}

app_menu_action_enum app_menu_take_action (app_menu_t *m)
{
    app_menu_action_enum action;

    if(0 == m)
    {
        return APP_ACTION_NONE;
    }
    action    = m->action;
    m->action = APP_ACTION_NONE;
    return action;
}

/*--------------------------------------------------- 显示查询接口 -----------------------------------------------*/
const char *app_menu_page_title (const app_menu_t *m)
{
    if(0 == m)
    {
        return "?";
    }
    switch(m->page)
    {
        case APP_MENU_PAGE_MAIN:    return "WCH MAIN";
        case APP_MENU_PAGE_DEMO:    return "DEMO LIST";
        case APP_MENU_PAGE_PARAM:   return "PARAMS";
        case APP_MENU_PAGE_EDIT:    return "PARAM EDIT";
        case APP_MENU_PAGE_SYSTEM:  return "SYSTEM";
        default:                    return "?";
    }
}

uint8_t app_menu_depth (const app_menu_t *m)
{
    if(0 == m)
    {
        return 0u;
    }
    switch(m->page)
    {
        case APP_MENU_PAGE_MAIN:    return 1u;
        case APP_MENU_PAGE_EDIT:    return 3u;
        case APP_MENU_PAGE_NUMBER:  return 0u;
        default:                    return 2u;
    }
}

uint8_t app_menu_item_count (const app_menu_t *m)
{
    if(0 == m)
    {
        return 0u;
    }
    switch(m->page)
    {
        case APP_MENU_PAGE_MAIN:    return MENU_MAIN_ITEM_NUM;
        case APP_MENU_PAGE_DEMO:    return APP_DEMO_NUM;
        case APP_MENU_PAGE_PARAM:   return APP_PARAM_NUM;
        case APP_MENU_PAGE_EDIT:    return 1u;
        case APP_MENU_PAGE_SYSTEM:  return MENU_SYS_ITEM_NUM;
        default:                    return 0u;
    }
}

uint8_t app_menu_cursor (const app_menu_t *m)
{
    return (0 == m) ? APP_MENU_CURSOR_NONE : m->cursor;
}

uint8_t app_menu_is_editing (const app_menu_t *m)
{
    return ((0 != m) && (APP_MENU_PAGE_EDIT == m->page) && (m->edit_id < APP_PARAM_NUM)) ? 1u : 0u;
}

void app_menu_item_text (const app_menu_t *m, const app_params_t *p, uint8_t index, char *buf, uint8_t len)
{
    menu_str_copy(buf, len, "");

    if((0 == m) || (0 == p) || (0 == buf))
    {
        return;
    }
    if(index >= app_menu_item_count(m))
    {
        return;
    }

    switch(m->page)
    {
        case APP_MENU_PAGE_MAIN:
        {
            menu_str_copy(buf, len, s_main_name[index]);
            break;
        }

        case APP_MENU_PAGE_DEMO:
        {
            menu_str_copy(buf, len, app_demo_name(index));
            break;
        }

        case APP_MENU_PAGE_PARAM:
        {
            menu_str_copy(buf, len, app_param_meta[index].name);
            menu_str_append(buf, len, " ");
            menu_u32_append(buf, len, app_param_get(p, index), 0u);
            break;
        }

        case APP_MENU_PAGE_EDIT:
        {
            if(m->edit_id >= APP_PARAM_NUM)
            {
                break;
            }
            menu_str_copy(buf, len, app_param_meta[m->edit_id].name);
            menu_str_append(buf, len, " ");
            menu_u32_append(buf, len, m->edit_value, 0u);
            break;
        }

        case APP_MENU_PAGE_SYSTEM:
        {
            menu_str_copy(buf, len, s_sys_name[index]);
            break;
        }

        default:
        {
            break;
        }
    }
}

const char *app_demo_name (uint8_t demo_id)
{
    if(demo_id >= APP_DEMO_NUM)
    {
        return "----";
    }
    return s_demo_name[demo_id];
}

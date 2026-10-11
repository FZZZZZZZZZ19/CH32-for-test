/*********************************************************************************************************************
* 文件名称          app_ui
* 说明              界面层实现（128x160 = 16 列 x 10 行，8x16 字体全 ASCII）
*                   行布局：0 标题 / 1 分隔 / 2..7 列表(6 行) / 8 详情&提示 / 9 状态
********************************************************************************************************************/
#include "app_ui.h"
#include "app_screen.h"
#include "app_time.h"

#define APP_UI_LIST_ROWS        6u
#define APP_UI_ROW_TITLE        0u
#define APP_UI_ROW_LIST_FIRST   2u
#define APP_UI_ROW_DETAIL       8u
#define APP_UI_ROW_STATUS       9u
#define APP_UI_TOAST_LEN        22u

static char     s_toast[APP_UI_TOAST_LEN];
static uint32_t s_toast_ms = 0u;

/*--------------------------------------------------- 字符串小工具 -----------------------------------------------*/
static uint8_t ui_str_len (const char *s)
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

static void ui_str_copy (char *buf, uint8_t len, const char *s)
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

static void ui_str_append (char *buf, uint8_t len, const char *s)
{
    uint8_t i = ui_str_len(buf);

    if((0 == buf) || (0 == s))
    {
        return;
    }
    while((i < (uint8_t)(len - 1u)) && ('\0' != *s))
    {
        buf[i] = *s;
        s ++;
        i ++;
    }
    buf[i] = '\0';
}

static void ui_u32_append (char *buf, uint8_t len, uint32_t value, uint8_t width)
{
    char    tmp[11];
    uint8_t n = 0u;
    uint8_t i;
    uint8_t pad;

    do
    {
        tmp[n] = (char)('0' + (value % 10u));
        value /= 10u;
        n ++;
    } while((value > 0u) && (n < 10u));

    pad = (width > n) ? (uint8_t)(width - n) : 0u;
    for(i = 0u; i < pad; i ++)
    {
        ui_str_append(buf, len, " ");
    }
    while(n > 0u)
    {
        char one[2];
        n --;
        one[0] = tmp[n];
        one[1] = '\0';
        ui_str_append(buf, len, one);
    }
}

/*--------------------------------------------------- 基础绘制 ---------------------------------------------------*/
void app_ui_init (void)
{
    ui_str_copy(s_toast, APP_UI_TOAST_LEN, "");
    s_toast_ms = 0u;
}

void app_ui_line (uint8_t row, const char *text)
{
    char buf[APP_SCREEN_TEXT_BUF];

    ui_str_copy(buf, (uint8_t)sizeof(buf), text);
    app_screen_text(0u, row, buf);
}

void app_ui_kv (uint8_t row, const char *key, uint32_t value, uint8_t digits)
{
    char buf[APP_SCREEN_TEXT_BUF];

    ui_str_copy(buf, (uint8_t)sizeof(buf), key);
    ui_str_append(buf, (uint8_t)sizeof(buf), " ");
    ui_u32_append(buf, (uint8_t)sizeof(buf), value, digits);
    app_screen_text(0u, row, buf);
}

void app_ui_toast (const char *text, uint32_t now_ms)
{
    ui_str_copy(s_toast, APP_UI_TOAST_LEN, text);
    s_toast_ms = now_ms;
}

static void app_ui_draw_toast (uint32_t now_ms)
{
    if('\0' == s_toast[0])
    {
        return;
    }
    if((uint32_t)(now_ms - s_toast_ms) >= APP_TOAST_MS)
    {
        s_toast[0] = '\0';
        app_screen_clear_row(APP_UI_ROW_DETAIL);
        return;
    }
    app_screen_row_text(APP_UI_ROW_DETAIL, APP_COLOR_BLUE, s_toast);
}

static void app_ui_draw_status (const app_ui_status_t *st)
{
    char buf[APP_SCREEN_TEXT_BUF];

    if(0 == st)
    {
        return;
    }

    /* 状态栏必须 <= 16 列（128 像素 / 8 像素）："#1 L15 E999 T999" 刚好 16 列 */
    ui_str_copy(buf, (uint8_t)sizeof(buf), "#");
    ui_u32_append(buf, (uint8_t)sizeof(buf), st->task_no % 10u, 1u);
    ui_str_append(buf, (uint8_t)sizeof(buf), " L");
    ui_u32_append(buf, (uint8_t)sizeof(buf), st->led_mask, 2u);
    ui_str_append(buf, (uint8_t)sizeof(buf), " E");
    ui_u32_append(buf, (uint8_t)sizeof(buf), st->exti_count % 1000u, 3u);
    ui_str_append(buf, (uint8_t)sizeof(buf), " T");
    ui_u32_append(buf, (uint8_t)sizeof(buf), st->isr_toggles % 1000u, 3u);

    app_screen_row_text(APP_UI_ROW_STATUS, APP_COLOR_RED, buf);
}

void app_ui_demo_frame (const char *title, const app_ui_status_t *st)
{
    app_screen_clear_row(APP_UI_ROW_TITLE);
    if(0 != title)
    {
        app_screen_row_text(APP_UI_ROW_TITLE, APP_COLOR_BLUE, title);
    }
    app_ui_draw_status(st);
}

/*--------------------------------------------------- 菜单绘制 ---------------------------------------------------*/
void app_ui_render (const app_menu_t *m, const app_params_t *p, const app_ui_status_t *st)
{
    char     item[APP_SCREEN_TEXT_BUF];
    char     detail[APP_SCREEN_TEXT_BUF];
    uint8_t  count;
    uint8_t  cursor;
    uint8_t  first;
    uint8_t  i;
    uint8_t  row;

    if((0 == m) || (0 == p))
    {
        return;
    }

    count  = app_menu_item_count(m);
    cursor = app_menu_cursor(m);

    /* 标题行：页面名 + 层级 */
    ui_str_copy(detail, (uint8_t)sizeof(detail), app_menu_page_title(m));
    ui_str_append(detail, (uint8_t)sizeof(detail), "  L");
    ui_u32_append(detail, (uint8_t)sizeof(detail), app_menu_depth(m), 1u);
    ui_str_append(detail, (uint8_t)sizeof(detail), "/3");
    app_screen_row_text(APP_UI_ROW_TITLE, APP_COLOR_BLUE, detail);

    /* 列表窗口 */
    first = 0u;
    if(cursor >= APP_UI_LIST_ROWS)
    {
        first = (uint8_t)(cursor - APP_UI_LIST_ROWS + 1u);
    }
    if((uint8_t)(first + APP_UI_LIST_ROWS) > count)
    {
        first = (count > APP_UI_LIST_ROWS) ? (uint8_t)(count - APP_UI_LIST_ROWS) : 0u;
    }

    for(i = 0u; i < APP_UI_LIST_ROWS; i ++)
    {
        uint8_t index = (uint8_t)(first + i);
        row = (uint8_t)(APP_UI_ROW_LIST_FIRST + i);

        if(index >= count)
        {
            app_screen_clear_row(row);
            continue;
        }
        app_menu_item_text(m, p, index, item, (uint8_t)sizeof(item));
        ui_str_copy(detail, (uint8_t)sizeof(detail), (index == cursor) ? ">" : " ");
        ui_str_append(detail, (uint8_t)sizeof(detail), item);
        app_screen_row_text(row, (index == cursor) ? APP_COLOR_RED : APP_COLOR_BLACK, detail);
    }

    /* 详情行：编辑参数时显示范围与步进，否则显示按键编码 */
    if(app_menu_is_editing(m))
    {
        uint8_t id = (m->edit_id < APP_PARAM_NUM) ? m->edit_id : 0u;
        /* 编辑态提示改用紧凑的 "min-max/step"，保证不超过 16 列 */
        ui_str_copy(detail, (uint8_t)sizeof(detail), "");
        ui_u32_append(detail, (uint8_t)sizeof(detail), app_param_meta[id].min, 1u);
        ui_str_append(detail, (uint8_t)sizeof(detail), "-");
        ui_u32_append(detail, (uint8_t)sizeof(detail), app_param_meta[id].max, 1u);
        ui_str_append(detail, (uint8_t)sizeof(detail), "/");
        ui_u32_append(detail, (uint8_t)sizeof(detail), app_param_meta[id].step, 1u);
        app_screen_row_text(APP_UI_ROW_DETAIL, APP_COLOR_GREEN, detail);
    }
    else if(0 != st)
    {
        ui_str_copy(detail, (uint8_t)sizeof(detail), "C");
        ui_u32_append(detail, (uint8_t)sizeof(detail), st->key_code, 1u);
        ui_str_append(detail, (uint8_t)sizeof(detail), " ");
        ui_str_append(detail, (uint8_t)sizeof(detail), app_key_name((app_key_enum)st->key_id));
        app_screen_row_text(APP_UI_ROW_DETAIL, APP_COLOR_GREEN, detail);
    }

    app_ui_draw_status(st);
    app_ui_draw_toast(app_time_now());
}

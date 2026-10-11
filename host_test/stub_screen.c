/*********************************************************************************************************************
* 文件名称          stub_screen
* 说明              主机端测试替身：把 app_screen_* 调用记录下来，供 test_ui.c 检查"每一行是否超过 16 列"
*                   真实实现 User/app_screen.c 依赖 zf_common_headfile.h（无法在 PC 上编译），
*                   所以主机端用本文件顶替，只关心"上屏内容宽度"这一个回归点。
********************************************************************************************************************/
#include <string.h>
#include "app_screen.h"

char     g_stub_row_text[APP_SCREEN_ROWS][APP_SCREEN_TEXT_BUF + 1];
uint8_t  g_stub_row_col [APP_SCREEN_ROWS];
uint8_t  g_stub_row_valid[APP_SCREEN_ROWS];
uint16_t g_stub_row_color[APP_SCREEN_ROWS];

uint16_t g_stub_text_calls;
uint16_t g_stub_clear_calls;
uint16_t g_stub_clear_row_calls;
uint16_t g_stub_band_calls;
uint8_t  g_stub_backlight;

void host_screen_reset (void)
{
    memset(g_stub_row_text,  0, sizeof(g_stub_row_text));
    memset(g_stub_row_col,   0, sizeof(g_stub_row_col));
    memset(g_stub_row_valid, 0, sizeof(g_stub_row_valid));
    memset(g_stub_row_color, 0, sizeof(g_stub_row_color));
    g_stub_text_calls      = 0u;
    g_stub_clear_calls     = 0u;
    g_stub_clear_row_calls = 0u;
    g_stub_band_calls      = 0u;
    g_stub_backlight       = 0u;
}

static void stub_record (uint8_t col, uint8_t row, const char *s)
{
    if(row >= APP_SCREEN_ROWS)
    {
        return;
    }

    g_stub_text_calls ++;
    g_stub_row_col[row]   = col;
    g_stub_row_valid[row] = 1u;

    if(0 == s)
    {
        g_stub_row_text[row][0] = '\0';
        return;
    }

    strncpy(g_stub_row_text[row], s, APP_SCREEN_TEXT_BUF);
    g_stub_row_text[row][APP_SCREEN_TEXT_BUF] = '\0';
}

void app_screen_init (void)                                  { }
uint8_t app_screen_ready (void)                              { return 1u; }
void app_screen_clear (void)                                 { g_stub_clear_calls ++; }
void app_screen_fill (uint16_t color)                        { (void)color; }

void app_screen_text (uint8_t col, uint8_t row, const char *s)
{
    stub_record(col, row, s);
}

void app_screen_clear_row (uint8_t row)
{
    if(row >= APP_SCREEN_ROWS)
    {
        return;
    }
    g_stub_clear_row_calls ++;
    g_stub_row_text[row][0] = '\0';
    g_stub_row_col[row]     = 0u;
    g_stub_row_valid[row]   = 1u;
}

void app_screen_row_text (uint8_t row, uint16_t color, const char *s)
{
    if(row >= APP_SCREEN_ROWS)
    {
        return;
    }
    g_stub_row_color[row] = color;
    stub_record(0u, row, s);
}

void app_screen_bar (uint8_t row, uint16_t permille)          { (void)row; (void)permille; }
void app_screen_backlight (uint8_t on)                       { g_stub_backlight = on; }
void app_screen_band_test (void)                             { g_stub_band_calls ++; }

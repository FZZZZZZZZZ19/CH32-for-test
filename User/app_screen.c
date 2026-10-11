/*********************************************************************************************************************
* 文件名称          app_screen
* 说明              屏幕驱动层实现（逐飞 tft180 设备驱动，默认硬件 SPI + 本板引脚）
********************************************************************************************************************/
#include "app_screen.h"
#include "zf_common_headfile.h"

static uint8_t s_ready = 0u;

void app_screen_init (void)
{
    tft180_init();                              /* SCL=B13 SDA=B15 RST=B7 DC=D7 CS=D4 BL=D0 */
    tft180_set_font(TFT180_8X16_FONT);          /* 8x16：16 列 x 10 行 */
    tft180_set_color(APP_SCREEN_FG_COLOR, APP_SCREEN_BG_COLOR);
    tft180_clear();
    s_ready = 1u;
}

uint8_t app_screen_ready (void)
{
    return s_ready;
}

void app_screen_clear (void)
{
    tft180_clear();
}

void app_screen_fill (uint16_t color)
{
    tft180_full(color);
}

void app_screen_text (uint8_t col, uint8_t row, const char *s)
{
    tft180_set_color(APP_SCREEN_FG_COLOR, APP_SCREEN_BG_COLOR);
    tft180_show_string((uint16)((uint16)col * 8u), (uint16)((uint16)row * APP_SCREEN_ROW_H), s);
}

void app_screen_clear_row (uint8_t row)
{
    uint16_t y;
    uint16_t y0 = (uint16_t)((uint16)row * APP_SCREEN_ROW_H);
    uint16_t y1 = (uint16_t)(y0 + APP_SCREEN_ROW_H);
    uint16_t x1 = (uint16_t)(APP_SCREEN_W - 1u);

    for(y = y0; (y < y1) && (y < APP_SCREEN_H); y ++)
    {
        tft180_draw_line(0u, y, x1, y, APP_SCREEN_BG_COLOR);
    }
}

void app_screen_row_text (uint8_t row, uint16_t color, const char *s)
{
    app_screen_clear_row(row);
    tft180_set_color(color, APP_SCREEN_BG_COLOR);
    tft180_show_string(0u, (uint16)((uint16)row * APP_SCREEN_ROW_H), s);
    tft180_set_color(APP_SCREEN_FG_COLOR, APP_SCREEN_BG_COLOR);
}

void app_screen_bar (uint8_t row, uint16_t permille)
{
    uint16_t y;
    uint16_t y0 = (uint16_t)((uint16)row * APP_SCREEN_ROW_H);
    uint16_t y1 = (uint16_t)(y0 + APP_SCREEN_ROW_H);
    uint16_t width;
    uint16_t x1 = (uint16_t)(APP_SCREEN_W - 1u);

    if(permille > 1000u)
    {
        permille = 1000u;
    }
    width = (uint16_t)(((uint32_t)APP_SCREEN_W * permille) / 1000u);

    for(y = y0; (y < y1) && (y < APP_SCREEN_H); y ++)
    {
        tft180_draw_line(0u, y, x1, y, APP_COLOR_LGRAY);
    }
    for(y = (uint16_t)(y0 + 2u); (y + 2u < y1) && (y < APP_SCREEN_H); y ++)
    {
        if(width > 0u)
        {
            tft180_draw_line(0u, y, (uint16)(width - 1u), y, APP_COLOR_GREEN);
        }
    }
}

void app_screen_backlight (uint8_t on)
{
    gpio_init(TFT180_BL_PIN, GPO, (on) ? GPIO_HIGH : GPIO_LOW, GPO_PUSH_PULL);
}

void app_screen_band_test (void)
{
    uint16_t y;

    for(y = 0u; y < APP_SCREEN_H; y ++)
    {
        uint16_t color;
        if(y < (APP_SCREEN_H / 3u))
        {
            color = APP_COLOR_RED;
        }
        else if(y < ((APP_SCREEN_H * 2u) / 3u))
        {
            color = APP_COLOR_GREEN;
        }
        else
        {
            color = APP_COLOR_BLUE;
        }
        tft180_draw_line(0u, y, (uint16)(APP_SCREEN_W - 1u), y, color);
    }
}

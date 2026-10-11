/*********************************************************************************************************************
* 文件名称          app_screen
* 说明              1.8 寸 TFT（ST7735 128x160，硬件 SPI2）驱动层：文本/状态栏/进度条（细则 7）
*                   目标端专用
********************************************************************************************************************/
#ifndef APP_SCREEN_DRIVER_H
#define APP_SCREEN_DRIVER_H

#include <stdint.h>
#include "app_config.h"

/* RGB565 常用颜色 */
#define APP_COLOR_BLACK     0x0000u
#define APP_COLOR_WHITE     0xFFFFu
#define APP_COLOR_RED       0xF800u
#define APP_COLOR_GREEN     0x07E0u
#define APP_COLOR_BLUE      0x001Fu
#define APP_COLOR_YELLOW    0xFFE0u
#define APP_COLOR_CYAN      0x07FFu
#define APP_COLOR_MAGENTA   0xF81Fu
#define APP_COLOR_GRAY      0x8410u
#define APP_COLOR_LGRAY     0xC618u

#define APP_SCREEN_FG_COLOR APP_COLOR_BLACK
#define APP_SCREEN_BG_COLOR APP_COLOR_WHITE

void     app_screen_init        (void);
uint8_t  app_screen_ready       (void);
void     app_screen_clear       (void);
void     app_screen_fill        (uint16_t color);
void     app_screen_text        (uint8_t col, uint8_t row, const char *s);
void     app_screen_clear_row   (uint8_t row);
void     app_screen_row_text    (uint8_t row, uint16_t color, const char *s);
void     app_screen_bar         (uint8_t row, uint16_t permille);
void     app_screen_backlight   (uint8_t on);
void     app_screen_band_test   (void);         /* 三色横条：自检/接线确认 */

#endif

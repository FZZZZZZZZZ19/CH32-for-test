/*********************************************************************************************************************
* 文件名称          test_text
* 说明              主机端单测：app_text_clip / app_text_len
*                   回归点：屏幕一行只有 16 列（128 像素 / 8 像素），
*                   超过 16 列的字符串会让 tft180_show_string 把第 17 个字符画到 x=128，
*                   触发 zf_device_tft180.c:409 的 zf_assert(x < tft180_x_max) 死循环刷屏。
********************************************************************************************************************/
#include <string.h>
#include "test_util.h"
#include "app_config.h"
#include "app_text.h"

int test_text (void)
{
    char    buf[APP_SCREEN_COLS + 1u];
    char    small[5];
    uint8_t n;

    /* 长度 */
    CHECK_EQ(app_text_len(""), 0u);
    CHECK_EQ(app_text_len(0), 0u);
    CHECK_EQ(app_text_len("abc"), 3u);

    /* 修复前 app_demo.c 里的这一行是 17 个字符 => 上屏必然断言 */
    CHECK_EQ(app_text_len("5 PWM  C6/D14/D15"), 17u);
    n = app_text_clip(buf, (uint8_t)sizeof(buf), "5 PWM  C6/D14/D15", APP_SCREEN_COLS);
    CHECK_EQ(n, 16u);
    CHECK_EQ(strlen(buf), 16u);
    CHECK(strcmp(buf, "5 PWM  C6/D14/D1") == 0);

    /* 恰好 16 列不受影响 */
    n = app_text_clip(buf, (uint8_t)sizeof(buf), "5 PWM C6/D14/D15", APP_SCREEN_COLS);
    CHECK_EQ(n, 16u);
    CHECK(strcmp(buf, "5 PWM C6/D14/D15") == 0);

    /* 短字符串原样复制 */
    n = app_text_clip(buf, (uint8_t)sizeof(buf), "READY", APP_SCREEN_COLS);
    CHECK_EQ(n, 5u);
    CHECK(strcmp(buf, "READY") == 0);

    /* 目标缓冲更小时按缓冲上限（dst_size - 1）截断 */
    n = app_text_clip(small, (uint8_t)sizeof(small), "ABCDEFGH", APP_SCREEN_COLS);
    CHECK_EQ(n, 4u);
    CHECK(strcmp(small, "ABCD") == 0);

    /* 列数上限优先于缓冲上限 */
    n = app_text_clip(buf, (uint8_t)sizeof(buf), "ABCDEFGH", 3u);
    CHECK_EQ(n, 3u);
    CHECK(strcmp(buf, "ABC") == 0);

    /* 边界：max_chars = 0 */
    n = app_text_clip(buf, (uint8_t)sizeof(buf), "ABC", 0u);
    CHECK_EQ(n, 0u);
    CHECK_EQ(buf[0], '\0');

    /* 边界：空指针 / 零长度缓冲 */
    CHECK_EQ(app_text_clip(0, 8u, "ABC", 8u), 0u);
    CHECK_EQ(app_text_clip(buf, 0u, "ABC", 8u), 0u);
    n = app_text_clip(buf, (uint8_t)sizeof(buf), 0, APP_SCREEN_COLS);
    CHECK_EQ(n, 0u);
    CHECK_EQ(buf[0], '\0');

    return 0;
}

/*********************************************************************************************************************
* 文件名称          app_text
* 说明              纯逻辑文本工具（不依赖任何硬件头文件，可在主机端单测）
*
*   为什么需要它：
*   本板屏幕是 1.8 寸 ST7735 128x160，8x16 字体 => 一行只有 16 列（16 * 8 = 128 像素）。
*   逐飞 zf_device_tft180.c 的 tft180_show_string() 只把第 j 个字符画在 x + 8*j，
*   既不做换行也不做裁剪；一旦字符串超过 16 个字符，第 17 个字符就落在 x = 128，
*   立刻触发 zf_device_tft180.c:409 的 zf_assert(x < tft180_x_max)，
*   而 debug_assert_handler() 是 while(1) 死循环刷屏，看起来就是"屏幕卡在 Assert error"。
*   所以所有上屏文本都必须先经过 app_text_clip() 裁剪到列数以内。
********************************************************************************************************************/
#ifndef APP_TEXT_H
#define APP_TEXT_H

#include <stdint.h>

/* 字符串长度（s 为 0 时返回 0，最长按 250 截断避免死循环） */
uint8_t app_text_len (const char *s);

/* 把 src 裁剪到最多 max_chars 个字符后写入 dst（dst_size 含结尾 '\0'），返回实际复制长度
 * 约束：max_chars 与 dst_size-1 取较小值；dst 为 0 或 dst_size 为 0 时不做任何事并返回 0
 * 用法：char line[17]; app_text_clip(line, sizeof(line), text, 16u);   // 保证 'x + 8*j < 128' */
uint8_t app_text_clip (char *dst, uint8_t dst_size, const char *src, uint8_t max_chars);

#endif

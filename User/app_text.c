/*********************************************************************************************************************
* 文件名称          app_text
* 说明              纯逻辑文本工具实现（细则 3-11 的界面文本都经这里裁剪，见 app_text.h 注释）
********************************************************************************************************************/
#include "app_text.h"

uint8_t app_text_len (const char *s)
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

uint8_t app_text_clip (char *dst, uint8_t dst_size, const char *src, uint8_t max_chars)
{
    uint8_t i = 0u;
    uint8_t limit;

    if((0 == dst) || (0u == dst_size))
    {
        return 0u;
    }
    if(0 == src)
    {
        src = "";
    }

    limit = (uint8_t)(dst_size - 1u);                           /* 先按缓冲区上限 */
    if(max_chars < limit)
    {
        limit = max_chars;                                      /* 再按调用方给的列数上限 */
    }

    while((i < limit) && ('\0' != src[i]))
    {
        dst[i] = src[i];
        i ++;
    }
    dst[i] = '\0';

    return i;
}

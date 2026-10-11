/*********************************************************************************************************************
* 文件名称          test_util
* 说明              主机端单元测试的最小断言工具（不依赖任何第三方框架）
*                   纯逻辑模块（app_crc32 / app_params / app_menu / app_key）在 PC 上用 gcc 编译运行
********************************************************************************************************************/
#ifndef HOST_TEST_UTIL_H
#define HOST_TEST_UTIL_H

#include <stdio.h>
#include <stdint.h>

extern int g_checks;                            /* 断言总数 */
extern int g_failures;                          /* 失败数 */

#define CHECK(cond)                                                         \
    do                                                                      \
    {                                                                       \
        g_checks ++;                                                        \
        if(!(cond))                                                         \
        {                                                                   \
            g_failures ++;                                                  \
            printf("  FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);        \
        }                                                                   \
    } while(0)

#define CHECK_EQ(actual, expected)                                          \
    do                                                                      \
    {                                                                       \
        unsigned long long va_ = (unsigned long long)(actual);              \
        unsigned long long ve_ = (unsigned long long)(expected);            \
        g_checks ++;                                                        \
        if(va_ != ve_)                                                      \
        {                                                                   \
            g_failures ++;                                                  \
            printf("  FAIL %s:%d  %s = %llu, expect %s = %llu\n",           \
                   __FILE__, __LINE__, #actual, va_, #expected, ve_);       \
        }                                                                   \
    } while(0)

int test_crc32  (void);
int test_params (void);
int test_menu   (void);
int test_key    (void);
int test_text   (void);                         /* app_text_clip：屏幕文本裁剪 */
int test_ui     (void);                         /* app_ui：每一行必须 <= 16 列（128/8） */

#endif

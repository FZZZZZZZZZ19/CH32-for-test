/*********************************************************************************************************************
* 文件名称          test_main
* 说明              主机端单元测试入口：依次运行 4 组纯逻辑测试并汇总结果
*                   返回 0 = 全部通过，返回 1 = 有失败
********************************************************************************************************************/
#include "test_util.h"

int g_checks   = 0;
int g_failures = 0;

typedef int (*test_fn_t)(void);

int main (void)
{
    static const struct
    {
        const char *name;
        test_fn_t   fn;
    } tests[] =
    {
        { "app_crc32",  test_crc32  },
        { "app_params", test_params },
        { "app_menu",   test_menu   },
        { "app_key",    test_key    },
    };
    unsigned int i;
    int          result = 0;

    printf("== host unit test : CH32V307VCT6 firmware pure logic ==\n");

    for(i = 0u; i < (unsigned int)(sizeof(tests) / sizeof(tests[0])); i ++)
    {
        int before = g_failures;

        printf("[RUN ] %s\n", tests[i].name);
        tests[i].fn();
        if(g_failures == before)
        {
            printf("[ OK ] %s\n", tests[i].name);
        }
        else
        {
            printf("[FAIL] %s  (+%d)\n", tests[i].name, g_failures - before);
            result = 1;
        }
    }

    printf("---- checks: %d, failures: %d ----\n", g_checks, g_failures);
    printf("RESULT: %s\n", (0 == result) ? "PASS" : "FAIL");

    return result;
}

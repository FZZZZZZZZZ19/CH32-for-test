/*********************************************************************************************************************
* 文件名称          test_crc32
* 说明              app_crc32 单元测试：标准向量、空数据、分段更新与一次性结果一致
********************************************************************************************************************/
#include "test_util.h"
#include "app_crc32.h"

#define CRC32_INIT      0xFFFFFFFFu
#define CRC32_FINAL     0xFFFFFFFFu

int test_crc32 (void)
{
    static const uint8_t check[] = "123456789";
    uint32_t crc;

    /* 标准测试向量：CRC-32/ISO-HDLC("123456789") = 0xCBF43926 */
    CHECK_EQ(app_crc32(check, 9u), 0xCBF43926u);

    /* 长度为 0 时结果固定为 0（init 与 final 相同） */
    CHECK_EQ(app_crc32(check, 0u), 0u);
    CHECK_EQ(app_crc32(0, 16u), 0u);

    /* 分段更新 == 一次性计算 */
    crc = app_crc32_update(CRC32_INIT, check, 5u);
    crc = app_crc32_update(crc, &check[5], 4u);
    CHECK_EQ(crc ^ CRC32_FINAL, 0xCBF43926u);

    /* 单字节推进 9 次同样得到标准向量 */
    crc = CRC32_INIT;
    for(uint16_t i = 0u; i < 9u; i ++)
    {
        crc = app_crc32_update(crc, &check[i], 1u);
    }
    CHECK_EQ(crc ^ CRC32_FINAL, 0xCBF43926u);

    /* 空指针不改变已有值 */
    crc = app_crc32_update(0x12345678u, 0, 4u);
    CHECK_EQ(crc, 0x12345678u);

    /* 数据不同则校验值不同（防止实现退化为常量） */
    CHECK(app_crc32(check, 8u) != app_crc32(check, 9u));

    return 0;
}

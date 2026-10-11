/*********************************************************************************************************************
* 文件名称          app_crc32
* 说明              CRC-32/ISO-HDLC 实现（纯逻辑，可主机端单测）
********************************************************************************************************************/
#include "app_crc32.h"

#define APP_CRC32_POLY      0xEDB88320u
#define APP_CRC32_INIT      0xFFFFFFFFu
#define APP_CRC32_FINAL     0xFFFFFFFFu

uint32_t app_crc32_update (uint32_t crc, const uint8_t *data, uint16_t len)
{
    uint16_t i;
    uint8_t  bit;

    if(data == 0)
    {
        return crc;
    }

    for(i = 0; i < len; i ++)
    {
        crc ^= (uint32_t)data[i];
        for(bit = 0; bit < 8; bit ++)
        {
            if(crc & 0x00000001u)
            {
                crc = (crc >> 1) ^ APP_CRC32_POLY;
            }
            else
            {
                crc = (crc >> 1);
            }
        }
    }
    return crc;
}

uint32_t app_crc32 (const uint8_t *data, uint16_t len)
{
    return app_crc32_update(APP_CRC32_INIT, data, len) ^ APP_CRC32_FINAL;
}

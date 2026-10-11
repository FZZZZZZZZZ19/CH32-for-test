/*********************************************************************************************************************
* 文件名称          app_params
* 说明              参数管理：默认值/夹取/序列化/CRC32/双槽选择（纯逻辑）
*                   以及目标端的 flash 存取（APP_HOST_TEST 时关闭）
* 存储布局          扇区 63：页 0 = A 槽，页 1 = B 槽，页 2 = 读写演示页
*                   每个槽 44 字节：magic(4) version(2) paylen(2) seq(4) payload(28) crc32(4)
********************************************************************************************************************/
#ifndef APP_PARAMS_H
#define APP_PARAMS_H

#include "app_config.h"

/*--------------------------------------------------- 参数结构 ---------------------------------------------------*/
typedef struct
{
    uint16_t boot_demo;                     /* 上电进入的功能编号 */
    uint16_t blink_ms;                      /* 定时器中断 LED 周期 */
    uint16_t pwm_freq_hz;                   /* PWM 频率 */
    uint16_t pwm_duty_permille;             /* PWM 占空比（千分比） */
    uint16_t buzzer_freq_hz;                /* 蜂鸣器频率 */
    uint16_t buzzer_ms;                     /* 蜂鸣器鸣叫时长 */
    uint8_t  backlight;                     /* 背光常亮 0/1 */
    uint8_t  flow_speed;                    /* 流水灯速度等级 1..10 */
    uint32_t seq;                           /* 保存序号（存储层维护） */
    uint16_t save_count;                    /* flash 保存次数 */
    uint16_t boot_count;                    /* 上电次数 */
} app_params_t;

/*--------------------------------------------------- 参数元数据 -------------------------------------------------*/
typedef struct
{
    const char *name;                       /* 屏幕显示名（ASCII，8x16 字体） */
    uint16_t    min;
    uint16_t    max;
    uint16_t    step;
    uint8_t     is_bool;                    /* 1 = 开关型参数 */
} app_param_meta_t;

extern const app_param_meta_t app_param_meta[APP_PARAM_NUM];

/*--------------------------------------------------- 序列化常量 -------------------------------------------------*/
#define APP_PARAMS_MAGIC            0x31484357u     /* "WCH1" */
#define APP_PARAMS_VERSION          1u
#define APP_PARAMS_HEADER_LEN       12u
#define APP_PARAMS_PAYLOAD_LEN      28u
#define APP_PARAMS_CRC_OFFSET       40u
#define APP_PARAMS_BLOCK_LEN        44u
#define APP_PARAMS_BLOCK_WORDS      (APP_PARAMS_BLOCK_LEN / 4u)

/*--------------------------------------------------- 纯逻辑接口 -------------------------------------------------*/
void     app_params_defaults        (app_params_t *p);
uint8_t  app_param_is_valid_id      (uint8_t id);
uint16_t app_param_clamp            (uint8_t id, uint16_t value);
uint16_t app_param_get              (const app_params_t *p, uint8_t id);
void     app_param_set              (app_params_t *p, uint8_t id, uint16_t value);

uint16_t app_params_serialize       (const app_params_t *p, uint8_t *buf, uint16_t len);
uint8_t  app_params_deserialize     (const uint8_t *buf, uint16_t len, app_params_t *p);
uint8_t  app_params_block_valid     (const uint8_t *buf, uint16_t len);
uint8_t  app_params_pick_slot       (uint8_t valid0, uint32_t seq0, uint8_t valid1, uint32_t seq1);

/*--------------------------------------------------- 目标端接口 -------------------------------------------------*/
#ifndef APP_HOST_TEST
extern app_params_t app_params;             /* 当前生效参数（RAM 副本） */

void    app_params_init             (void);                          /* 读 flash + boot_count+1 + 回写 */
void    app_params_save             (void);                          /* 参数写 flash（A/B 轮换） */
void    app_params_reset            (void);                          /* 恢复默认值并保存 */
uint8_t app_params_store_load       (app_params_t *out);             /* 返回有效槽位号，0xFF = 无有效数据 */
uint8_t app_params_store_save       (const app_params_t *p);         /* 返回 1 = 成功 */
uint8_t app_params_store_slot       (void);                          /* 当前有效槽位号 */
void    app_params_store_erase_all  (void);                          /* 擦除两个槽（恢复出厂） */
#endif

#endif

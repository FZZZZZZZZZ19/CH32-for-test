/*********************************************************************************************************************
* 文件名称          app_port
* 说明              板级绑定层：所有"哪个引脚/哪个外设"的事实只在本层出现
*                   依据：ProDoc_22主板 WCH主板 V1.0 原理图（CH32V307VCT6）
*                   目标端专用（包含逐飞库头文件），主机端单测不编译本文件
********************************************************************************************************************/
#ifndef APP_PORT_H
#define APP_PORT_H

#include "zf_common_headfile.h"
#include "app_config.h"

/*--------------------------------------------------- 可控 LED ---------------------------------------------------
* 原理图：MCU 引脚 -> RN3 限流电阻 -> LED 阳极 -> 阴极 -> GND，高电平点亮
* LED5 = PD1(LIGHT5_D1)、LED6 = PA15(LIGHT3_A15)、LED7 = PA12(LIGHT2_A12)、LED8 = PA11(LIGHT1_A11)
* LED1..LED4 由 RN4 直接接 5V/3.3V/3.3V_MAIN/3.3V_CAM，是电源指示灯，MCU 不可控
* 注意：PA15 是 JTDI，必须在初始化时关闭 JTAG（保留 SWD），否则 PA15 无法当普通 IO
*--------------------------------------------------------------------------------------------------------------*/
typedef enum
{
    APP_LED_1 = 0,                          /* PD1  -> LED5 */
    APP_LED_2,                              /* PA15 -> LED6 */
    APP_LED_3,                              /* PA12 -> LED7 */
    APP_LED_4,                              /* PA11 -> LED8 */
    APP_LED_NUMBER
} app_led_enum;

/*--------------------------------------------------- 按键原始编码 -------------------------------------------------
* 74HC148 编码：EI 接地（常使能），编码输出接 MCU：
*   KEY_IN1_D8(PD8) = A0、KEY_IN2_B12(PB12) = A1、KEY_IN3_B14(PB14) = A2
* 74HC148 输入低有效，GS/EO 未接 MCU，因此"无键"与"U(上)"同为 111，硬件上不可区分（见 docs/PINMAP.md）
* 编码到按键的语义映射在纯逻辑模块 User/app_key.c 的 app_key_code_to_key()，本层只负责读三位编码
*--------------------------------------------------------------------------------------------------------------*/

/*--------------------------------------------------- 外设资源 ---------------------------------------------------*/
#define APP_PORT_PIN_LED1           (D1)    /* LED5 */
#define APP_PORT_PIN_LED2           (A15)   /* LED6  需关 JTAG */
#define APP_PORT_PIN_LED3           (A12)   /* LED7 */
#define APP_PORT_PIN_LED4           (A11)   /* LED8 */

#define APP_PORT_PIN_KEY_IN1        (D8)    /* 74HC148 A0 */
#define APP_PORT_PIN_KEY_IN2        (B12)   /* 74HC148 A1 */
#define APP_PORT_PIN_KEY_IN3        (B14)   /* 74HC148 A2 */

#define APP_PORT_PIN_BUZZER         (A8)    /* FMQ_IO_A8 -> AO3400A 栅极，高电平导通 */
#define APP_PORT_PIN_BACKLIGHT      (D0)    /* IPS_BLK_D0，高电平开背光 */

#define APP_PORT_PWM_CH0_PIN        (TIM3_PWM_MAP3_CH1_C6)      /* H1-1 DUOJI_PWM_C6 */
#define APP_PORT_PWM_CH1_PIN        (TIM4_PWM_MAP1_CH3_D14)     /* H1-3 TIM4_CH3_PWM1 */
#define APP_PORT_PWM_CH2_PIN        (TIM4_PWM_MAP1_CH4_D15)     /* H1-2 TIM4_CH3_PWM2 */
#define APP_PORT_PWM_CH_NUM         (APP_PWM_CH_NUM)

/*--------------------------------------------------- 接口 ------------------------------------------------------*/
void        app_port_init            (void);
void        app_port_led_init        (void);
void        app_port_led_set         (app_led_enum led, uint8_t on);
void        app_port_led_toggle      (app_led_enum led);
uint8_t     app_port_led_get         (app_led_enum led);
const char *app_port_led_pin_name    (app_led_enum led);

void        app_port_key_init        (void);
uint8_t     app_port_key_code_read   (void);

void        app_port_exti_key_init   (void);
void        app_port_exti_key_isr    (void);        /* 由 User/zf_isr.c 的 EXTI9_5 line8 分支调用 */
uint32_t    app_port_exti_key_count  (void);
uint32_t    app_port_exti_key_last_ms(void);
uint8_t     app_port_exti_level      (void);        /* 直接读 PD8 电平（GPIO 轮询证据） */

void        app_port_buzzer_init     (void);
void        app_port_buzzer_pwm      (uint32_t freq_hz, uint16_t duty_permille);
void        app_port_buzzer_stop     (void);

void        app_port_backlight_set   (uint8_t on);

void        app_port_pwm_init        (void);
void        app_port_pwm_set         (uint8_t ch, uint32_t freq_hz, uint16_t duty_permille);

#endif

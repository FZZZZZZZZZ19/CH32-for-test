/*********************************************************************************************************************
* 文件名称          app_port
* 说明              板级绑定层实现（目标端专用）
********************************************************************************************************************/
#include "app_port.h"
#include "app_time.h"

/*--------------------------------------------------- 引脚表 -----------------------------------------------------*/
static const gpio_pin_enum s_led_pin[APP_LED_NUMBER] =
{
    APP_PORT_PIN_LED1,      /* PD1  -> LED5 */
    APP_PORT_PIN_LED2,      /* PA15 -> LED6 */
    APP_PORT_PIN_LED3,      /* PA12 -> LED7 */
    APP_PORT_PIN_LED4,      /* PA11 -> LED8 */
};

static const char *const s_led_pin_name[APP_LED_NUMBER] =
{
    "PD1/LED5", "PA15/LED6", "PA12/LED7", "PA11/LED8"
};

static const pwm_channel_enum s_pwm_pin[APP_PORT_PWM_CH_NUM] =
{
    APP_PORT_PWM_CH0_PIN,   /* C6  -> H1-1 */
    APP_PORT_PWM_CH1_PIN,   /* D14 -> H1-3 */
    APP_PORT_PWM_CH2_PIN,   /* D15 -> H1-2 */
};

static uint8_t s_led_state[APP_LED_NUMBER];

static volatile uint32_t s_exti_key_count   = 0;
static volatile uint32_t s_exti_key_last_ms = 0;

/*--------------------------------------------------- LED -------------------------------------------------------*/
void app_port_led_init (void)
{
    uint8_t i;

    /* PA15 是 JTDI：只关闭 JTAG、保留 SWD，否则 PA15 无法作为普通 IO
     * ch32v30x_gpio.h 里的 GPIO_Remap_SWJ_Disable(0x00300400) 会把 SWD 一起关掉，
     * 这里直接写 AFIO->PCFR1 的 SWJ_CFG = 001(JTAG-DP Disabled and SW-DP Enabled) */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    AFIO->PCFR1 = (AFIO->PCFR1 & ~(uint32_t)AFIO_PCFR1_SWJ_CFG) | (uint32_t)AFIO_PCFR1_SWJ_CFG_JTAGDISABLE;

    for(i = 0; i < APP_LED_NUMBER; i ++)
    {
        s_led_state[i] = 0u;
        gpio_init(s_led_pin[i], GPO, GPIO_LOW, GPO_PUSH_PULL);
    }
}

void app_port_led_set (app_led_enum led, uint8_t on)
{
    if(led >= APP_LED_NUMBER)
    {
        return;
    }
    s_led_state[led] = (on) ? 1u : 0u;
    gpio_set_level(s_led_pin[led], (on) ? GPIO_HIGH : GPIO_LOW);
}

void app_port_led_toggle (app_led_enum led)
{
    if(led >= APP_LED_NUMBER)
    {
        return;
    }
    s_led_state[led] = (s_led_state[led]) ? 0u : 1u;
    gpio_toggle_level(s_led_pin[led]);
}

uint8_t app_port_led_get (app_led_enum led)
{
    if(led >= APP_LED_NUMBER)
    {
        return 0u;
    }
    return s_led_state[led];
}

const char *app_port_led_pin_name (app_led_enum led)
{
    if(led >= APP_LED_NUMBER)
    {
        return "?";
    }
    return s_led_pin_name[led];
}

/*--------------------------------------------------- 按键 -------------------------------------------------------*/
void app_port_key_init (void)
{
    gpio_init(APP_PORT_PIN_KEY_IN1, GPI, GPIO_HIGH, GPI_PULL_UP);
    gpio_init(APP_PORT_PIN_KEY_IN2, GPI, GPIO_HIGH, GPI_PULL_UP);
    gpio_init(APP_PORT_PIN_KEY_IN3, GPI, GPIO_HIGH, GPI_PULL_UP);
}

uint8_t app_port_key_code_read (void)
{
    uint8_t code = 0u;

    if(gpio_get_level(APP_PORT_PIN_KEY_IN1))
    {
        code |= 0x01u;                      /* A0 -> bit0 */
    }
    if(gpio_get_level(APP_PORT_PIN_KEY_IN2))
    {
        code |= 0x02u;                      /* A1 -> bit1 */
    }
    if(gpio_get_level(APP_PORT_PIN_KEY_IN3))
    {
        code |= 0x04u;                      /* A2 -> bit2 */
    }
    return code;
}

void app_port_exti_key_init (void)
{
    s_exti_key_count   = 0u;
    s_exti_key_last_ms = 0u;
    exti_init(APP_PORT_PIN_KEY_IN1, EXTI_TRIGGER_BOTH);     /* PD8 双边沿 */
}

void app_port_exti_key_isr (void)
{
    s_exti_key_count ++;
    s_exti_key_last_ms = app_tick_ms;
}

uint32_t app_port_exti_key_count (void)
{
    return s_exti_key_count;
}

uint32_t app_port_exti_key_last_ms (void)
{
    return s_exti_key_last_ms;
}

uint8_t app_port_exti_level (void)
{
    return gpio_get_level(APP_PORT_PIN_KEY_IN1);
}

/*--------------------------------------------------- 蜂鸣器 -----------------------------------------------------*/
void app_port_buzzer_init (void)
{
    pwm_init(APP_PORT_PIN_BUZZER, 2731u, PWM_DUTY_MAX / 2u);    /* 无源蜂鸣器谐振 2.731kHz，50% 占空比 */
    pwm_set_duty(APP_PORT_PIN_BUZZER, 0u);                      /* 上电先静音 */
}

void app_port_buzzer_pwm (uint32_t freq_hz, uint16_t duty_permille)
{
    uint32_t duty = (uint32_t)duty_permille * (PWM_DUTY_MAX / 1000u);

    if(duty > PWM_DUTY_MAX)
    {
        duty = PWM_DUTY_MAX;
    }
    if(0u == freq_hz)
    {
        freq_hz = 2731u;
    }
    pwm_set_freq(APP_PORT_PIN_BUZZER, freq_hz, duty);
}

void app_port_buzzer_stop (void)
{
    pwm_set_duty(APP_PORT_PIN_BUZZER, 0u);
}

/*--------------------------------------------------- 背光 -------------------------------------------------------*/
void app_port_backlight_set (uint8_t on)
{
    gpio_init(APP_PORT_PIN_BACKLIGHT, GPO, (on) ? GPIO_HIGH : GPIO_LOW, GPO_PUSH_PULL);
}

/*--------------------------------------------------- PWM --------------------------------------------------------*/
void app_port_pwm_init (void)
{
    uint8_t i;

    for(i = 0; i < APP_PORT_PWM_CH_NUM; i ++)
    {
        pwm_init(s_pwm_pin[i], 1000u, 0u);
    }
}

void app_port_pwm_set (uint8_t ch, uint32_t freq_hz, uint16_t duty_permille)
{
    uint32_t duty;

    if(ch >= APP_PORT_PWM_CH_NUM)
    {
        return;
    }
    if(0u == freq_hz)
    {
        freq_hz = 1000u;
    }
    duty = (uint32_t)duty_permille * (PWM_DUTY_MAX / 1000u);
    if(duty > PWM_DUTY_MAX)
    {
        duty = PWM_DUTY_MAX;
    }
    pwm_set_freq(s_pwm_pin[ch], freq_hz, duty);
}

/*--------------------------------------------------- 统一初始化 -------------------------------------------------*/
void app_port_init (void)
{
    app_port_led_init();
    app_port_key_init();
    app_port_pwm_init();
    app_port_buzzer_init();
}

/********************************************************************************
* File Name          : main.c
* Description        : CH32V307VCT6 + SeekFree CH32V307 opensource library
*                      软件一轮考核：一份固件完成 GPIO / 按键 / PWM+蜂鸣器 / 定时器中断 /
*                      SPI 屏幕 / flash 存储 / 外部中断 + 屏幕 UI / 三级菜单参数 / 免重烧录切换 / git 管理
*
* Clock / debug UART / interrupt vector are owned by the SeekFree library:
*   clock : clock_init()  (zf_common_clock.c) -> also sets system_clock
*   debug : debug_init()  (zf_common_debug.c) -> UART3, PB10 TX / PB11 RX, 115200
*   IRQ   : User/zf_isr.c (SeekFree hooks + app_* hooks) + User/ch32v30x_it.c (NMI/HardFault)
*
* NOTE: the debug UART is UART3 on PB10/PB11, NOT USART1 on PA9.
* NOTE: all application logic lives in User/app_*.c, so main() stays readable.
********************************************************************************/
#include "zf_common_headfile.h"
#include "app_main.h"

int main(void)
{
    clock_init(SYSTEM_CLOCK_144M);      /* 144 MHz - see zf_common_clock.h for other values */
    debug_init();                       /* must be kept: sets up the debug UART            */

    app_main_init();                    /* 板级初始化 + flash 参数 + 自检 + 进入上电功能 */

    while (1)
    {
        app_main_loop();                /* 按键 / 菜单 / 演示 / 界面 调度 */
    }
}

# WCH 主板（ProDoc_22 / WCH主板_4，V1.0）引脚与资源对照表

> 依据：嘉立创 EDA 原理图 3 页（页1 最小系统板 / 页2 电源管理 / 页3 外设接口，V1.0，2026-09-24）。
> 本表是**固件里唯一的引脚事实来源**：所有引脚号只在 `User/app_port.c` 出现一次，其它模块只调用 `app_port_*`。
> 芯片：**CH32V307VCT6**（LQFP100，RV32IMAFC，标称 256KB Flash / 64KB SRAM）。

---

## 1. 受控资源总表（固件实际使用）

| 资源 | 数量 | 引脚 | 网络名 | 固件符号 | 备注 |
|------|------|------|--------|----------|------|
| 可控 LED | 4 | PD1 / PA15 / PA12 / PA11 | LIGHT5_D1 / LIGHT3_A15 / LIGHT2_A12 / LIGHT1_A11 | `APP_LED_1..4` | **高电平点亮**，经 RN3 4×串阻接 LED5..LED8 阳极，阴极公共轨接 GND |
| 按键（74HC148 编码） | 3 位 | PD8 / PB12 / PB14 | KEY_IN1_D8 / KEY_IN2_B12 / KEY_IN3_B14 | `APP_PORT_PIN_KEY_IN*` | 依次为 A0 / A1 / A2，内部上拉，低有效 |
| 外部中断按键 | 1 路 | PD8 | KEY_IN1_D8 | `app_port_exti_key_*` | 细则 9：双边沿中断计数（也可用 GPIO 轮询） |
| PWM 输出 | 3 路 | PC6 / PD14 / PD15 | DUOJI_PWM_C6 / TIM4_CH3_PWM1 / TIM4_CH3_PWM2 | `APP_PORT_PWM_CH0..2` | 引出到 H1（3 路 PWM 接口） |
| 无源蜂鸣器 | 1 | PA8 | FMQ_IO_A8 | `APP_PORT_PIN_BUZZER` | TIM1_CH1 → 20Ω → AO3400A 栅极；谐振 2.731kHz，高电平导通 |
| TFT 屏 | 1 | PB13 / PB15 / PB7 / PD7 / PD4 / PD0 | IPS_SCL_B13 / IPS_SDA_B15 / IPS_RST_B7 / IPS_DC_D7 / IPS_CS_D4 / IPS_BLK_D0 | `app_screen.c` | 1.8" 128×160，ST7735，硬件 SPI2；BLK 高电平开背光 |
| Flash 参数存储 | — | 内部 Flash 扇区 63 | — | `app_params.c` | 页0=A 槽、页1=B 槽、页2=读写自测页；双槽 + seq + CRC32 |
| 调试串口 | 1 | PB10 / PB11 | USART3_TX_PB10 / USART3_RX_PB11 | `debug.c` | 115200-8-N-1（**不是 PA9/PA10**） |
| 下载/调试 | 2 | PA13 / PA14 | SWDIO / SWCLK | — | 不要当普通 GPIO 用；PA15 是 LED6，需关 JTAG 留 SWD（固件已处理） |

### 1.1 板载非受控 LED（理解现象用）

| LED | 驱动来源 | 说明 |
|-----|----------|------|
| LED1 | 5V | 经 RN4 串阻，**电源指示灯**，MCU 不可控 |
| LED2 | 3.3V | 同上 |
| LED3 | 3.3V_MAIN | 同上 |
| LED4 | 3.3V_CAM | 同上 |
| LED5 / LED6 / LED7 / LED8 | PD1 / PA15 / PA12 / PA11 | **可控**，高电平点亮 |

> 注意：网络 `LIGHT4_C10`（PC10）在原理图上未接入 RN3，因此本板可控 LED 是 4 个（不是 5 个）。
> 注意：`LIGHT1_A11` / `LIGHT2_A12` 与 Type-C 的 `USB-N` / `USB-P` 是**同一对网络**（见 §4.6），
> 所以 PA11/PA12 既驱动 LED7/LED8 又连着 Type-C 数据脚 —— 本固件把它们当 LED 输出用，不使用 USB 外设。

---

## 2. MCU 完整引脚表（U1，LQFP100）

### 2.1 左侧 1–50 脚

| 脚 | 引脚名 | 网络 / 用途 |
|----|--------|-------------|
| 1–5 | PE2–PE6 | 未接（×） |
| 6 | VBAT | VBAT（3.3V_MAIN 经 R5 0Ω 供 VBAT） |
| 7 | PC13/TAMPER-RTC | 未接（×） |
| 8 | PC14/OSC32IN | OSC32IN（X4 32.768kHz） |
| 9 | PC15/OSC32OUT | OSC32OUT |
| 10 | VSS_5 | 数字地 |
| 11 | VDD_5 | 3.3V_MAIN + C11 100nF |
| 12 | OSC_IN | OSC_IN（X3 8MHz + C68/C69 10pF） |
| 13 | OSC_OUT | OSC_OUT |
| 14 | NRST | RSET（R91 10k 上拉 + C59 100nF + SW17 复位键） |
| 15–18 | PC0–PC3/ADC10–13 | 未接（×） |
| 19 | VSSA | 模拟地 |
| 20 | VREF- | VREF_L |
| 21 | VREF+ | VREF_H |
| 22 | VDDA | VDDA（L1 0Ω 从 3.3V_MAIN 经 C12 100nF） |
| 23 | PA0/WKUP/ADC0 | L_EN_1_A0（左编码器 A 相） |
| 24 | PA1/ADC1 | L_EN_2_A1（左编码器 B 相） |
| 25–26 | PA2–PA3/ADC2–3 | 未接（×） |
| 27 | VSS_4 | 数字地 |
| 28 | VDD_4 | 3.3V_MAIN + C10 100nF |
| 29 | PA4/ADC4/DAC0 | CM_HSYNC_A4（摄像头行同步） |
| 30 | PA5/ADC5/DAC1 | CM_VSYNC_A5（场同步） |
| 31 | PA6/ADC6 | CM_PCLK_A6（像素时钟） |
| 32 | PA7/ADC7 | 未接（×） |
| 33 | PC4/ADC14 | UART4_CTS |
| 34 | PC5/ADC15 | UART4_RTS |
| 35 | PB0/ADC8 | UART4_1_TX |
| 36 | PB1/ADC9 | UART4_1_RX |
| 37 | PB2/BOOT1 | BOOT1（R89 10k 下拉） |
| 38 | PE7 | BL_RTS_E7 |
| 39 | PE8 | 未接（×） |
| 40 | PE9 | LPWM_1_E9（电机驱动隔离 U4 输入） |
| 41 | PE10 | BL_CTS_E10 |
| 42 | PE11 | LPWM_2_E11 |
| 43 | PE12 | BL_CMD_E12 |
| 44 | PE13 | RPWM_1_E13 |
| 45 | PE14 | RPWM_2_E14 |
| 46 | PE15 | 未接（×） |
| 47 | PB10 | **USART3_TX_PB10（调试串口 TX）** |
| 48 | PB11 | **USART3_RX_PB11（调试串口 RX）** |
| 49 | VSS_1 | 数字地 |
| 50 | VIO_1/VDD_1 | 3.3V_MAIN + C6 100nF |

### 2.2 右侧 100–51 脚

| 脚 | 引脚名 | 网络 / 用途 |
|----|--------|-------------|
| 100 | VIO_3 | 3.3V_MAIN + C9 100nF |
| 99 | VSS_3 | 数字地 |
| 98–97 | PE1 / PE0 | 未接（×） |
| 96 | PB9 | CM_D7_B9（摄像头 D7） |
| 95 | PB8 | CM_D6_B8 |
| 94 | BOOT0 | BOOT0（R90 10k 下拉 + SW16 20Ω 到 3.3V_MAIN） |
| 93 | PB7/USB2DP | IPS_RST_B7（屏复位；同时是 USB2 D+，本工程不用 USB2） |
| 92 | PB6/USB2DM | CM_D5_B6（摄像头 D5） |
| 91 | PB5 | IMU_SDA（陀螺仪 IIC 数据） |
| 90 | PB4 | 未接（×） |
| 89 | PB3 | IMU_SCL（陀螺仪 IIC 时钟） |
| 88 | PD7 | **IPS_DC_D7（屏数据/命令）** |
| 87 | PD6 | UART2_RX |
| 86 | PD5 | UART2_TX |
| 85 | PD4 | **IPS_CS_D4（屏片选）** |
| 84 | PD3 | 未接（×） |
| 83 | PD2 | CM+_D2（摄像头电源正标注） |
| 82 | PD1 | **LIGHT5_D1 → LED5** |
| 81 | PD0 | **IPS_BLK_D0（屏背光，高电平开）** |
| 80 | PC12 | CM-_C12 |
| 79 | PC11 | CM_D4_C11 |
| 78 | PC10 | LIGHT4_C10（**未接 RN3，本板不用**） |
| 77 | PA15 | **LIGHT3_A15 → LED6**（JTDI，需关 JTAG） |
| 76 | PA14/SWCLK | SWCLK（下载/调试） |
| 75 | VDD_2 | 3.3V_MAIN |
| 74 | VSS_2 | 数字地 |
| 73 | NC | — |
| 72 | PA13/SWDIO | SWDIO（下载/调试） |
| 71 | PA12/USB1DP | **LIGHT2_A12 → LED7**（同时接 Type-C USB-P） |
| 70 | PA11/USB1DM | **LIGHT1_A11 → LED8**（同时接 Type-C USB-N） |
| 69 | PA10 | CM_D1_A10 |
| 68 | PA9 | CM_D0_A9 |
| 67 | PA8 | **FMQ_IO_A8（蜂鸣器，TIM1_CH1）** |
| 66 | PC9/TXN | CM_D3_C9 |
| 65 | PC8/TXP | CM_D2_C8 |
| 64 | PC7/RXN | 未接（×） |
| 63 | PC6/RXP | **DUOJI_PWM_C6（PWM 通道 0，TIM3 全重映射 CH1）** |
| 62 | PD15 | **TIM4_CH3_PWM2（PWM 通道 2，实际 TIM4_CH4）** |
| 61 | PD14 | **TIM4_CH3_PWM1（PWM 通道 1，TIM4_CH3）** |
| 60 | PD13 | R_EN_2_D13（右编码器 B 相） |
| 59 | PD12 | R_EN_1_D12（右编码器 A 相） |
| 58–56 | PD11 / PD10 / PD9 | 未接（×） |
| 55 | PD8 | **KEY_IN1_D8（74HC148 A0，同时用作外部中断按键）** |
| 54 | PB15 | **IPS_SDA_B15（SPI2 MOSI）** |
| 53 | PB14 | **KEY_IN3_B14（74HC148 A2）** |
| 52 | PB13 | **IPS_SCL_B13（SPI2 SCK）** |
| 51 | PB12 | **KEY_IN2_B12（74HC148 A1）** |

---

## 3. 按键编码（U15 = SN74HC148DR，8-3 优先编码器）

### 3.1 原理图连接（物理引脚为准）

| 74HC148 物理脚 | 符号 | 芯片功能（TI 数据手册） | 板上网络 | 备注 |
|----------------|------|------------------------|----------|------|
| 5 | EI | 使能输入（低有效） | GND | 常使能 |
| 16 | VCC | 电源 | 3.3V | — |
| 15 | EO | 使能输出 | **悬空** | 未接 MCU |
| 14 | GS | 有键指示 | **悬空** | 未接 MCU（**这就是缺陷根源**） |
| 13 | I0 | 最高优先输入 | **U**（五向开关上） | 与"无键"同码 |
| 12 | I1 | 输入 1 | **D**（下） | — |
| 11 | I2 | 输入 2 | **L**（左） | — |
| 10 | I3 | 输入 3 | **USELESS**（RN2 上拉） | 未使用的第 4 个输入 |
| 1 | I4 | 输入 4 | **KEY1**（SW14） | 独立按键 1 |
| 2 | I5 | 输入 5 | **R**（右） | — |
| 3 | I6 | 输入 6 | **M**（五向开关中键） | — |
| 4 | I7 | 最低优先输入 | **KEY2**（SW15） | 独立按键 2 |
| 6 | A2 | 编码输出 | KEY_IN3_B14 → PB14 | — |
| 7 | A1 | 编码输出 | KEY_IN2_B12 → PB12 | — |
| 9 | A0 | 编码输出 | KEY_IN1_D8 → PD8 | — |

### 3.2 编码真值表（编码 = A2 A1 A0 = PB14 PB12 PD8）

| 编码 | 触发输入 | 板上按键 | 固件语义（`app_key.c`） |
|------|----------|----------|-------------------------|
| `000` | I7 | KEY2 | `APP_KEY_DEC`（长按 = 保存参数） |
| `001` | I6 | 五向中键 M | `APP_KEY_ENTER`（进入 / 确认） |
| `010` | I5 | 五向右 R | `APP_KEY_INC`（+ / 右移） |
| `011` | I4 | KEY1 | `APP_KEY_UP`（上移，**代替不可分辨的"上"**） |
| `100` | I3 | 未接（USELESS） | 忽略 |
| `101` | I2 | 五向左 L | `APP_KEY_BACK`（返回 / 取消） |
| `110` | I1 | 五向下 D | `APP_KEY_DOWN`（下移） |
| `111` | 无键 **且** I0（五向上 U） | — | 判定为**无键** |

### 3.3 硬件缺陷与固件对策（答辩重点）

* **缺陷**：`GS`（有键指示）与 `EO` 都没接到 MCU，因此 MCU 只能读 3 位编码。
  五向开关的"上"接在 **I0（最高优先）**，其编码 `111` 与"无键（全高）"完全相同 ⇒ **"上"无法被识别**。
* 另一个副作用：I0 优先级最高，若"上"被按住，其它方向键的编码也会被屏蔽成 `111`。
* 另外要注意：立创 EDA 的 SN74HC148 符号内部把 pin13/12/11/10 标成了 `3/2/1/0`，
  与 TI 数据手册（pin13 = I0、pin10 = I3）相反。板上实际走线以**物理引脚**为准，固件按物理引脚建模。
* **固件对策**：`111` 一律当"无键"；用独立按键 `KEY1`（编码 `011`）承担"上"的导航职责；
  UI 与三级菜单只用 `000/001/010/011/101/110` 六种可分辨编码。
* 消抖 20ms（2 次 10ms 采样）、长按 1000ms、长按后 250ms 连发，全部在 `app_key.c`（纯逻辑，可主机端单测）。

---

## 4. 其它板上资源（固件不使用，但答辩可能被问）

| 区块 | 器件 / 接口 | MCU 连接 | 说明 |
|------|-------------|----------|------|
| 4.1 电机驱动隔离 | U4 SN74HC125PWR（四路三态缓冲） | PE9 / PE11 / PE13 / PE14（LPWM_1/LPWM_2/RPWM_1/RPWM_2），R95/R96 10k 下拉 | 本固件未输出电机 PWM |
| 4.2 编码器接口 | CN5 / CN6（XH2.54） | 左：PA0 / PA1；右：PD12 / PD13 | 本固件未做编码器测速 |
| 4.3 陀螺仪 | H2 / H6 | PB5（SDA）/ PB3（SCL）/ 3.3V | 本固件未使用 |
| 4.4 摄像头 | FPC1（DVP 并口） | PA4 行同步、PA5 场同步、PA6 PCLK、D0–D7 = PA9/PA10/PC8/PC9/PC11/PB6/PB8/PB9 | 本固件未使用 |
| 4.5 串口扩展 | H5（FG-PZ2.54-02-06P） | UART4：PB0 TX / PB1 RX、PC4 CTS / PC5 RTS | 本固件不使用 UART4（调试用 UART3） |
| 4.6 USB Type-C | USB1（TypeC-16P 母座） | `USB-N` = PA11 同网络、`USB-P` = PA12 同网络；CC1/CC2 各 10k 下拉到地 | PA11/PA12 已用于 LED8/LED7，**本固件不使用 USB 外设**（下载走 SWD） |
| 4.7 复位 | R91 10k + C59 100nF + SW17 | NRST（脚 14） | — |
| 4.8 启动模式 | R89 10k 下拉（BOOT1）、R90 10k 下拉（BOOT0）、SW16 20Ω 到 3.3V_MAIN | PB2 / BOOT0 | 默认从内置 Flash 启动 |
| 4.9 蜂鸣器驱动 | AO3400A N-MOS + R93 10k 栅源下拉 + D12 1N4148W 续流 | PA8（栅极串 20Ω） | 5V 供电，无源，谐振 2.731kHz |
| 4.10 电源 | U23 TPS5430DDAR（12V→5V，3A；页内注释：换 TPS5450 可 5A）；U8/U5/U6 RT9013-33GB（5V→3.3V 外设 / 3.3V_MAIN MCU / 3.3V_CAM 摄像头） | — | 3.3V_MAIN 经 R5 0Ω 到 VBAT，经 L1 0Ω + C12 到 VDDA |
| 4.11 电源开关 | U27 7107SYZ3QE | — | 控制 12V 输入 |
| 4.12 晶振 | X3 8MHz（C68/C69 10pF）；X4 32.768kHz（C71/C72 12pF） | OSC_IN/OSC_OUT；PC14/PC15 | 固件 `clock_init(SYSTEM_CLOCK_144M)` 内部 8MHz 倍频到 144MHz |

---

## 5. 存储与链接脚本

| 项 | 值 | 说明 |
|----|----|------|
| Flash 总容量 | 256KB（Link.ld 配置） | 与 `User/system_ch32v30x.c` 中的分区常量必须一致 |
| 参数存储扇区 | **扇区 63**（0x0803F000，4KB） | `APP_PARAMS_SECTOR`，最后 4KB，不会被程序代码占用 |
| A 槽 / B 槽 / 自测页 | 页 0 / 页 1 / 页 2（每页 1KB） | `APP_PARAMS_SLOT0_PAGE` / `SLOT1_PAGE` / `RAW_PAGE` |
| 单槽数据结构 | 44 字节 | magic(4) + version(2) + paylen(2) + seq(4) + payload(28) + crc32(4) |
| 擦除后读出值 | `0xE339E339` | 逐飞 `flash_erase_sector()` 只擦 1KB 一页，返回 1 = 失败 |

---

## 6. 硬件判读踩坑记录（写代码前必须知道）

1. **调试串口是 UART3（PB10/PB11）**，不是 PA9/PA10；`Debug/debug.c` 已按 UART3 初始化。
2. **PA13/PA14 是 SWDIO/SWCLK**，配成普通 GPIO 会掉线，无法再下载。
3. **PA15 是 LED6**（JTDI）：必须在初始化时写 `AFIO->PCFR1` 的 `SWJ_CFG = 001`（关 JTAG、保 SWD），
   不能用 `GPIO_Remap_SWJ_Disable`（那会把 SWD 一起关掉）。
4. **可控 LED 只有 4 个**（LED5–LED8）；LED1–LED4 是电源指示灯；网络 `LIGHT4_C10` 未接入 LED 网络。
5. **按键只有 6 种可分辨编码**，"上"不可用（见 §3.3）。
6. **编译必须 `-march=rv32imafcxw -mabi=ilp32f`**：`zf_device/libzf_device_config.a` 是单精度硬浮点 ABI，
   用软浮点链接会报 ABI 不兼容（工具链 multilib 已含该组合）。
7. **`zf_driver_flash.c` 的原型用 SDK 的 `uint32`（`unsigned int`）**，与 `stdint.h` 的 `uint32_t`
   （rv32 上是 `unsigned long`）是不同类型 ⇒ 传指针时要显式转换 `(uint32 *)` / `(const uint32 *)`。
8. **新增根级目录必须同步 `.cproject` 根 `sourceEntry` 的 `excluding`**，否则 MounRiver 会递归编译
   （本次已加入 `.work|docs|host_test|tools`）。

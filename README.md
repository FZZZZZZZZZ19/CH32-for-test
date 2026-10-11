# CH32V307VCT6 MounRiver 工程模板（已内含逐飞开源库）

一个**克隆下来就能直接编译**的 CH32V307VCT6 裸机工程模板。

- **IDE**：MounRiver Studio 2（VS Code 内核，自带 RISC-V GCC 8.2.0 与 WCH OpenOCD）
- **芯片**：CH32V307VCT6（青稞 V4F，RV32IMAFC，256K Flash / 64K SRAM 标称）
- **已内含**：逐飞科技 CH32V307 开源库 **V3.7.2**（`zf_common` / `zf_driver` / `zf_device` / `zf_components`）
- **实测构建**：模板原始为 `text 22964 / data 264 / bss 2700`；**加入本文档描述的考核固件后为
  `text 46792 / data 304 / bss 4144`（sources 101），0 error / 0 warning**

> 本仓库现在不只是模板：`User\` 下已经落了一份**软件组一轮考核固件** ——
> 用**一份程序**覆盖细则第 3～11 项（GPIO / 按键 / PWM / 蜂鸣器 / 定时器中断 / SPI 屏 /
> Flash 掉电存储 / 外部中断 / UI / 三级菜单），**全程靠板上按键切换，不需要重新烧录**。
> 说明见 [§ 软件一轮考核固件](#软件一轮考核固件一份程序不重新烧录)，细节见 [`docs/`](docs/)。

---

## 软件一轮考核固件（一份程序，不重新烧录）

### 上电后会发生什么

1. 初始化外设 → 应用 Flash 里的参数 → 跑一遍**自检 8 项**（结果同时打在屏幕上和 UART3 日志里）
2. 停在**三级菜单**主页面：`DEMO` / `PARAMETERS` / `SYSTEM`
   （参数 `BOOT DEMO = 0` 是默认值；改成 `1..11` 就变成"上电直接进第 `值-1` 号演示"，同一份固件不用重烧）

### 按键（板上 74HC148 + 五向开关）

原理图上物理上是 **7 个开关**——两个轻触键 `SW14`(`KEY1`)、`SW15`(`KEY2`) 加一个五向开关 `U/L/D/M/R`——
它们全部接到 `U15`（`SN74HC148DR` 8→3 优先编码器），只占 MCU 的 `PD8/PB12/PB14` 三根线，共 **6 个可分辨编码**。

**但实物装配数不一定一样**（现场实测：只有部分按键有效）。所以固件提供**手势通路**，
**板上任意 1~2 个按键都能走完三级菜单**：

| 手势 | 菜单态 | 演示态 |
|---|---|---|
| **短按** | 下一项 / 上一项（编辑态 = 加 / 减） | 交给该演示功能自己用 |
| **长按**（≥1 s） | **确定**：进入下一级 / 提交编辑并立刻写入 Flash（提示 `PARAM SAVED`） | **切下一个演示** |
| **双击**（两次短按 < 400 ms） | **返回**上一级 / 取消编辑 | 退出演示回到菜单 |

同时保留 74HC148 六个编码的直接语义（两种通路并存，互不冲突）：

| 操作 | 事件 | 菜单里做什么 |
|---|---|---|
| 五向 **中键** | `ENTER` | 进入下一级 / 提交编辑 |
| 五向 **下** | `DOWN` | 光标下移 / 参数减 |
| 五向 **左** | `BACK` | 返回上一级 / 取消编辑（长按直接回主菜单） |
| 五向 **右** | `INC` | 编辑态参数加；列表态等同"确定" |
| **KEY1** | `UP` | 光标上移 / 编辑态参数加 |
| **KEY2** | `DEC` | 光标下移 / 编辑态参数减 |

> ⚠️ 五向开关的"上"在硬件上与"无键"编码相同（`111`，优先编码器 `GS/EO` 未接 MCU），
> 因此固件用 `KEY1` 承担"上"的职责。原因与真值表见 [`docs/PINMAP.md`](docs/PINMAP.md)。

按键链路是**看得见也听得见**的：状态行末尾的 `C` 是 74HC148 原始编码（无键 `7`，KEY1 `3`、中键 `1`、下 `6`、左 `5`、右 `2`、KEY2 `0`），
演示态下第 1 行还有 `KEY UP C3` 回显；按下/松开都有蜂鸣器短鸣；`EXTI` 计数 `E` 记录 `PD8` 的边沿次数。

### 功能与细则对应

| 菜单 | 覆盖细则 | 现场观察 |
|---|---|---|
| `DEMO → LED` | 3 GPIO 控制 LED | 4 个可控 LED（LED5–LED8 = PD1/PA15/PA12/PA11）流水/全亮/全灭 |
| `DEMO → KEYS` | 4 读取按键 | 屏幕显示编码、键名、按键计数 |
| `DEMO → PWM OUT` | 5 PWM | H1 接口（PC6/PD14/PD15）自动扫频，可接示波器 |
| `DEMO → BUZZER` | 5 无源蜂鸣器 | 2.731kHz 方波鸣叫 |
| `DEMO → TIMER LED` | 6 定时器中断控 LED | LED6 周期亮灭，**翻转发生在 TIM6 中断里** |
| `DEMO → SCREEN` | 7 SPI 屏 | ST7735 1.8" 文字 + 进度条 |
| `DEMO → FLASH R/W` | 8 Flash 保存/读取 | 槽位、序号、保存次数、上电次数、CRC 校验结果 |
| `DEMO → EXTI KEY` | 9 外部中断 | PD8 双边沿中断计数、实时电平 |
| `DEMO → UI PANEL` | 9 屏幕 UI 交互 | 4 个 tile 随按键变化 |
| `DEMO → MENU` / `PARAMETERS` | 10 多级菜单记录参数 | `L1/L2/L3` 层级、8 个参数编辑、断电后仍在 |
| 全部 | 11 不重新烧录 | 只用板上按键完成以上所有项 |
| 仓库提交历史 | 12 git | `git log --oneline` |

### 构建与自测

```cmd
:: 目标端（等价于 MounRiver Build，不开 IDE）
powershell -NoProfile -ExecutionPolicy Bypass -File tools\build.ps1
::   → sources 101 / errors 0 / warnings 0，产物 obj\CH32V307VCT.elf|.hex|.map

:: 主机端单元测试（纯逻辑模块：CRC32 / 参数存储 / 菜单 / 按键状态机）
powershell -NoProfile -ExecutionPolicy Bypass -File host_test\run_tests.ps1
::   → ---- checks: 2192, failures: 0 ----  RESULT: PASS
```

烧写（两条路都实测过）：

* **WCH-Link + SWD**：用 WCH-LinkUtility 烧 `obj\CH32V307VCT.hex`，或 MounRiver 里直接 Download（接板上 H4：SWCLK=PA14、SWDIO=PA13、3.3V_MAIN、GND）。
* **只有 Type-C、没有 WCH-Link**：按住板上 BOOT 键 `SW16` → 按一下复位键 `SW17` 让芯片进 ISP，
  然后 `powershell -NoProfile -ExecutionPolicy Bypass -File tools\flash_usb.ps1`（实测 47104 字节写入 + Verify OK）。
  详见 [`docs/BUILD.md` §1.3](docs/BUILD.md)。

串口日志：**UART3 PB10(TX)/PB11(RX) @115200**。

### 文档索引

| 文件 | 内容 |
|---|---|
| [`docs/DESIGN.md`](docs/DESIGN.md) | 分层设计、任务→代码映射、设计决策与已知限制 |
| [`docs/PINMAP.md`](docs/PINMAP.md) | 全 100 脚对照表、LED/按键真值表、硬件缺陷与对策、存储布局 |
| [`docs/BUILD.md`](docs/BUILD.md) | 编译 / 烧录 / 单测步骤与实测数据 |
| [`docs/DEFENSE.md`](docs/DEFENSE.md) | 现场答辩手册：学习路径、代码结构、逐条演示剧本、高频问答、盲盒模块清单 |
| [`SEEKFREE_MOUNRIVER_PLAN.md`](SEEKFREE_MOUNRIVER_PLAN.md) | 逐飞库接入过程与踩坑记录 |

---

## 快速开始

```cmd
git clone https://github.com/FZZZZZZZZZ19/CH32-for-test.git
```

> 原始模板仓库是 <https://github.com/FZZZZZZZZZ19/CH32>（本仓库由它派生并加入考核固件）。

1. 用 **MounRiver Studio 2** 打开 `CH32` 目录（`File → Open Folder`）
2. `Project → Clean`
3. `Project → Build`

**没有第 4 步。** 不需要装库、不需要建链接、不需要改任何配置。

> 若打开后 MounRiver 提示工程配置需要刷新，让它刷新即可 ——
> `.cproject` 里用的是 `${workspace_loc}` 相对路径，不含任何机器相关的绝对路径。

---

## 第一次上手

```c
/* User/main.c —— 现在只有 26 行，所有功能都在 User/app_*.c 里 */
#include "zf_common_headfile.h"
#include "app_main.h"

int main(void)
{
    clock_init(SYSTEM_CLOCK_144M);      /* 144MHz */
    debug_init();                       /* 调试串口，必须保留 */

    app_main_init();                    /* 板级初始化 + flash 参数 + 自检 + 进上电功能 */

    while (1)
    {
        app_main_loop();                /* 按键 / 菜单 / 演示 / 界面 调度 */
    }
}
```

> 想要**模板原始的最小示例**（空的 `while(1)`）见 git 历史里加入考核固件之前的提交。

**接线（WCH-LinkE）**

| WCH-LinkE | CH32V307 |
|---|---|
| 3V3 | 3V3 |
| GND | GND |
| SWDIO | **PA13** |
| SWCLK | **PA14** |

**看串口输出**

> ⚠️ 调试串口是 **UART3 的 PB10(TX) / PB11(RX) @ 115200**，**不是 PA9**。
> 把 USB-TTL 的 RXD 接到板子的 **PB10**。

**PA13 / PA14 是调试口**，程序里不要配成普通 GPIO，否则会连不上调试器。

---

## 目录结构

```
CH32\
├── User\                      应用层 —— 考核固件全部在这里（app_* 共 29 个文件 3247 行）
│   ├── main.c                 入口：clock_init + debug_init + app_main_init + 主循环（26 行）
│   ├── zf_isr.c               逐飞中断服务函数（TIM6 1ms 节拍 / EXTI9_5 按键中断已接好）
│   ├── app_config.h           全部常量：版本、任务周期、8 个参数的范围/步进/存储位置
│   ├── app_main.c/.h          调度器与模式管理（菜单态 / 演示态）
│   ├── app_port.c/.h          板级绑定：LED/按键输入/PWM/蜂鸣器/背光/EXTI（唯一引脚事实源）
│   ├── app_time.c/.h          1ms 软件节拍
│   ├── app_key.c/.h           按键状态机：74HC148 编码 → 消抖 / 短按 / 长按 / 连发
│   ├── app_led.c/.h           4 路 LED：常亮/熄灭/软件闪烁/中断闪烁/流水灯/二进制
│   ├── app_pwm.c/.h           3 路 PWM：统一频率、独立占空比、自动扫频
│   ├── app_buzzer.c/.h        无源蜂鸣器：任意频率 + 定时关闭
│   ├── app_screen.c/.h        1.8" ST7735 屏：文本/清行/进度条/背光/色带自检
│   ├── app_ui.c/.h            屏幕界面：标题/列表/详情/状态行
│   ├── app_menu.c/.h          三级菜单状态机
│   ├── app_params.c/.h        8 个参数 + 44 字节块 + A/B 双槽 + Flash 存取
│   ├── app_crc32.c/.h         CRC-32/ISO-HDLC
│   ├── app_demo.c/.h          11 个演示功能
│   ├── app_selftest.c/.h      上电自检 8 项
│   ├── ch32v30x_it.c          NMI / HardFault
│   ├── system_ch32v30x.c      系统时钟 / FLASH-RAM 分区
│   └── ch32v30x_conf.h        外设库配置头
├── Core\                      core_riscv.c/h —— CSR 访问层
├── Peripheral\                WCH 标准外设库（inc 27 + src 27）
├── Startup\                   startup_ch32v30x_D8C.S（D8 版已在 .cproject 中排除）
├── Debug\                     debug.c —— Delay_* 与 _sbrk
├── Ld\Link.ld                 链接脚本：256K FLASH / 32K RAM
├── zf_common\      (14 文件)  逐飞公共层：clock/debug/fifo/font/function/interrupt/typedef
├── zf_driver\      (32 文件)  逐飞外设驱动层
├── zf_device\      (58 文件)  逐飞器件驱动层（含 libzf_device_config.a）
├── zf_components\  ( 4 文件)  逐飞应用组件（逐飞助手）
├── host_test\                 主机端单元测试（PC 上 gcc 直接跑，2192 项断言）
├── tools\build.ps1            不开 IDE 的等价编译脚本
├── docs\                      DESIGN / PINMAP / BUILD / DEFENSE
├── LICENSES\                  第三方许可说明与 GPL-3.0 全文
├── .cproject .project .launch  MounRiver 工程定义
└── SEEKFREE_MOUNRIVER_PLAN.md 接入过程记录（含踩坑与回滚方式）
```

> `docs\` `host_test\` `tools\` 是给 PC 用的，已在 `.cproject` 根 sourceEntry 的
> `excluding=` 里排除，MounRiver 不会递归编译它们。

**所有驱动都已经参与编译** —— `Peripheral\src` 下 27 个 WCH 驱动、以及
`zf_driver` / `zf_device` 下全部逐飞驱动都在构建里，你**直接调用 API 即可**，
不需要改 `.cproject` 去"添加源文件"。

---

## 关键配置要点

### 编译选项：`-march=rv32imafcxw -mabi=ilp32f`

CH32V307 有单精度硬件浮点，而且逐飞预编译库 `zf_device\libzf_device_config.a`
是**单精度硬浮点 ABI**，所以必须是 `ilp32f`。**改成软浮点 `ilp32` 会链接失败**：

```
failed to merge target specific data of file ... libzf_device_config.a
```

对应 `.cproject` 里的 `target.isa.fp = single` 与 `target.abi.fp = single`。
语言标准是 `gnu11`。

### 时钟

`User\main.c` 里 `clock_init(SYSTEM_CLOCK_144M)`，可选
24 / 48 / 72 / 96 / 120 / 144 MHz（见 `zf_common_clock.h`）。

逐飞的延时、PWM 占空比、串口波特率换算都依赖全局变量 `system_clock`，
用 `clock_init()` 改频率它会自动同步 —— **不要绕过它直接改 RCC 寄存器**。

### 调试串口

由逐飞的 `zf_common_debug.c` 负责（`printf` → `_write`）。默认配置在
`zf_common_debug.h`：

```c
#define DEBUG_UART_INDEX     (UART_3)
#define DEBUG_UART_BAUDRATE  (115200)
#define DEBUG_UART_TX_PIN    (UART3_MAP0_TX_B10)   // PB10
#define DEBUG_UART_RX_PIN    (UART3_MAP0_RX_B11)   // PB11
#define DEBUG_UART_USE_INTERRUPT (1)
```

**要换成别的串口**，改这几个宏之后，还必须把 `User\zf_isr.c` 里对应的
接收中断处理（现在是 `USART3_IRQHandler` 调用 `debug_interrupr_handler()`）
搬到新串口的中断函数里。

### `_write` 与 `_sbrk`

- `_write`（printf 重定向）**由逐飞提供**，`Debug\debug.c` 里原来那份已删除
- `Debug\debug.c` 里的 **`_sbrk` 必须保留** —— 逐飞摄像头驱动
  （`zf_device_mt9v03x_dvp.c` / `scc8660_dvp.c`）会调用 `malloc`，
  而逐飞库自己不提供 `_sbrk`

### FLASH / RAM 分区

`Ld\Link.ld` 当前是 **256K FLASH / 32K RAM**（WCH EVT 的默认值）。

CH32V30x 支持多种切分：`192K+128K` / `224K+96K` / **`256K+64K`** / `288K+32K` / `128K+192K`。

> ⚠️ **改动 `Link.ld` 必须同步改 `User\system_ch32v30x.c` 里的分区设置**，
> 否则会静默踩内存。

> 当前 `bss` 只有 2.7KB，32K RAM 够用。但逐飞的**摄像头驱动**需要大块图像缓冲，
> 真要用摄像头前建议先确认芯片实际的 FLASH/RAM 配置，再决定是否切成 64K RAM。

### `FLASH_FLAG_PGERR`

逐飞 SDK 里有、本工程使用的 WCH EVT SDK 里没有，已在
`Peripheral\inc\ch32v30x_flash.h` 用 `#ifndef` 保护补上（`zf_driver_flash.c` 需要）。
未来 WCH SDK 若补上同名宏，`#ifndef` 保证不会冲突。

---

## 怎么写自己的代码

1. **加功能**：在 `User\app_demo.c` 里加一个演示，或新建 `User\xxx.c`
   （`User` 目录下的 `.c` 会自动纳入构建）；`User\main.c` 只保留初始化和主循环
2. **用逐飞驱动**：`#include "zf_common_headfile.h"` 后直接调用，例如
   ```c
   gpio_init(P20_0, GPO, 0, GPO_PUSH_PULL);
   pit_ms_init(PIT_TIMG0, 10);            // 10ms 定时中断
   uart_init(UART_1, 115200, UART1_TX_A9, UART1_RX_A10);
   ```
   各驱动的用法、可用引脚枚举都在对应头文件的注释里
3. **加中断处理**：中断服务函数已集中在 `User\zf_isr.c`，在对应
   `xxx_IRQHandler` 里填逻辑（逐飞驱动的回调也在这里接）
4. **换主频**：只改 `main()` 里 `clock_init()` 的参数
5. **参考例程**：逐飞库自带 12 个例程（GPIO/UART/ADC/PWM/PIT/EXTI/编码器/
   FLASH/定时器/printf调试/中断优先级/USB-CDC），
   见 <https://gitee.com/seekfree/CHV307_Library> 的 `Example\` 目录

---

## 更新逐飞库到新版本

本仓库内嵌的是一份**副本**（这样克隆即可编译）。要升级：

```cmd
git clone --depth 1 https://gitee.com/seekfree/CHV307_Library.git %TEMP%\CHV307
```

然后把 `%TEMP%\CHV307\Seekfree_CH32V307VCT6_Opensource_Library\libraries\` 下的
`zf_common` / `zf_driver` / `zf_device` / `zf_components` 四个目录，
覆盖本工程的同名目录，并同步更新 `LICENSES\SeekFree-GPL-3.0.txt`。

> 升级后建议先 `Project → Clean` 再 Build，并留意逐飞是否新增了
> 需要补的 SDK 宏（参考 `SEEKFREE_MOUNRIVER_PLAN.md` 里 `FLASH_FLAG_PGERR` 那类问题）。

---

## 已知限制

| 项 | 状态 |
|---|---|
| 编译 | ✅ 已在 MounRiver Studio 2 实测通过；考核固件另有 `tools\build.ps1` 实测 `sources 101 / 0 error / 0 warning` |
| 主机端单元测试 | ✅ CRC32 / 参数存储 / 菜单 / 按键状态机 / 文本裁剪 / 界面行宽 6 组，`checks: 2192, failures: 0` |
| 链接（含逐飞预编译库） | ✅ 无 ABI 冲突、无重复定义 |
| 中断向量表 | ✅ `.vector` 位于 0x4，入口 0x0 |
| **实际烧写 / 上板运行** | ❌ **未验证**（写这份代码时手上无硬件，所有引脚结论来自原理图判读） |
| 五向开关"上"方向 | ⚠️ 硬件上与"无键"同编码，不可用（详见 `docs/PINMAP.md`） |
| 可控 LED | ⚠️ 只有 4 个（LED1–LED4 是电源指示灯） |
| LTO | 未启用（MounRiver 默认构建不含 LTO） |
| 32K RAM | ⚠️ 考核固件 bss 4144B，够用；摄像头图像缓冲需重新评估分区 |

---

## 许可

本仓库包含三个来源的代码，许可各不相同 —— 详见 [`LICENSES/README.md`](LICENSES/README.md)。

| 部分 | 来源 | 许可 |
|---|---|---|
| `User\` 与工程配置 | 本仓库作者 | — |
| `Core\` `Peripheral\` `Startup\` `Debug\` | WCH CH32V30x EVT SDK | **Apache-2.0**（每文件头部有 SPDX 声明） |
| `zf_common\` `zf_driver\` `zf_device\` `zf_components\` | 逐飞科技 CH32V307 开源库 V3.7.2 | **GPL-3.0**（全文见 [`LICENSES/SeekFree-GPL-3.0.txt`](LICENSES/SeekFree-GPL-3.0.txt)） |

> 逐飞部分为**原样复制、未作修改**，每个源文件都保留了逐飞的完整版权与许可声明。
> **分发包含这部分代码的固件或源码时必须遵守 GPL-3.0 条款。**

---

## 附：接入过程记录

`SEEKFREE_MOUNRIVER_PLAN.md` 记录了把逐飞库接入本工程的完整方案与执行清单，
以及实际踩过的四个坑：

1. Eclipse 的 `<locationURI>PROJECT_LOC/...` 链接文件夹**不被 MounRiver 识别**
2. 根 sourceEntry 未排除库目录会导致**递归全扫**（把逐飞自带的 `main.c`、`isr.c`、
   `Example\` 12 个例程全编进去，与工程自身重复定义）
3. 工程默认是**软浮点** `rv32imacxw/ilp32`，与逐飞硬浮点预编译库**ABI 不兼容**
4. SDK 版本差异导致缺 `FLASH_FLAG_PGERR`

同文件也写了回滚方式（`.seekfree-bak\`）。

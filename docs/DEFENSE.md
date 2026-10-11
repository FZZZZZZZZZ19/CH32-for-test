# 现场答辩手册（一份固件 / 不重新烧录）

> 配套文件：`docs/DESIGN.md`（分层与设计决策）、`docs/PINMAP.md`（引脚事实与硬件坑）、`docs/BUILD.md`（编译/烧录/测试）
> 本文用于细则第 1 项（现场答辩：讲学习路径、讲代码结构、回答每部分作用）与第 11 项（一份固件不重烧录）。

---

## 1. 一分钟版本

* **一份固件**：`User/main.c`（26 行）只做 `clock_init → debug_init → app_main_init → while(1) app_main_loop()`。
* **三层结构**：`app_main`（调度器/模式）→ `app_port`（板级绑定，唯一引脚事实源）+ 各功能模块 → 纯逻辑模块（可 PC 单测）。
* **不重烧录**：上电进自检，随后停在三级菜单；11 个演示功能、8 个参数、参数存取全部由板上按键切换；
  `KEY2` 长按保存、`L`（左）长按回主菜单。
* **自证手段**：屏幕状态行（`#任务 L灯掩码 E中断数 T中断翻转 F槽位 序号`）+ UART3 115200 日志 + `SELF TEST` 自检 8 项。
* **代码量**：`User/app_*` 共 29 个文件 3247 行（纯逻辑 4 个模块约 700 行可在 PC 上单测，1671 项断言全通过）。

---

## 2. 学习路径（怎么从零走到这份固件）

| 阶段 | 学到的东西 | 落在代码里的证据 |
|------|-----------|------------------|
| ① 工具链 | MounRiver / xPack GCC、`-march=rv32imafcxw -mabi=ilp32f`（硬浮点 ABI）、链接脚本 FLASH/RAM 分区 | `tools/build.ps1`、`Ld/Link.ld`、`docs/BUILD.md` |
| ② GPIO | 输出推挽/输入上拉、AFIO 重映射、SWJ_CFG 关 JTAG 保 SWD | `app_port.c`（4 个 LED、晶振、PA15 处理） |
| ③ 按键 | 独立按键 + 74HC148 优先编码器、消抖/短按/长按/连发状态机 | `app_key.c` + `app_port.c::app_port_key_code_read` |
| ④ PWM | 定时器通道/重映射、频率与占空比、占空比千分比换算 | `app_pwm.c`、`app_buzzer.c`（2.731kHz 方波驱动无源蜂鸣器） |
| ⑤ 中断 | 系统节拍 `pit_ms_init`、不抢时间的 ISR 设计、外部中断 | `zf_isr.c` 的 TIM6 / EXTI9_5 钩子、`app_time.c`、`app_led.c` |
| ⑥ SPI 屏幕 | 硬件 SPI2 + ST7735 初始化、坐标/方向/字体、刷新策略 | `app_screen.c`（tft180 驱动）、`app_ui.c` |
| ⑦ Flash 存储 | 页/扇区擦写、掉电安全（双槽 + 序号 + CRC32） | `app_params.c`、`app_crc32.c` |
| ⑧ UI 与菜单 | 三级菜单状态机 + 参数编辑 + 屏幕局部刷新 | `app_menu.c`、`app_ui.c` |
| ⑨ 工程化 | 分层解耦、主机端单测、文档与 git 里程碑 | `host_test/`、`docs/`、git 提交历史 |

---

## 3. 代码结构（每部分作用）

```
User/
  main.c            入口：时钟 144MHz + 调试串口 + app_main_init + 主循环（26 行）
  zf_isr.c          中断向量表：TIM6（1ms 节拍 + 中断内翻 LED）、EXTI9_5（PD8 按键计数）
  system_ch32v30x.c 时钟树与 Flash/RAM 分区（与 Link.ld 对应）
  app_main.c/.h     调度器与模式管理：菜单态/演示态、动作分发、参数应用、状态同步（298 行）
  app_config.h      全部常量：版本、周期、板级规模、8 个参数的范围/步进/存储位置
  ── 板级绑定层（只有这里出现硬件） ────────────────────────────────
  app_port.c/.h     引脚初始化与读写：4 LED、3 按键输入、3 PWM、蜂鸣器、背光、EXTI、SWJ_CFG
  app_time.c/.h     1ms 软件节拍（TIM6 中断只做 app_tick_ms++）
  app_screen.c/.h   1.8" ST7735 屏：初始化、定位文本、清行、进度条、背光、色带自检
  ── 功能模块层 ──────────────────────────────────────────────────
  app_led.c/.h      4 个 LED：常亮/熄灭/软件闪烁/中断闪烁/流水灯/二进制显示
  app_pwm.c/.h      三路 PWM：统一频率、独立占空比、自动扫频（示波器可见）
  app_buzzer.c/.h   无源蜂鸣器：任意频率 + 定时关闭（TIM1_CH1）
  app_ui.c/.h       屏上界面：标题/列表/详情/状态行/toast，含状态结构体
  app_demo.c/.h     11 个演示功能（与细则 3–11 一一对应）
  app_selftest.c/.h 上电自检 8 项（LED/按键/PWM/蜂鸣器/屏幕/Flash/EXTI/参数回环）
  ── 纯逻辑层（不 include 任何硬件头文件，可在 PC 上单测） ──────────
  app_crc32.c/.h    CRC-32/ISO-HDLC
  app_params.c/.h   8 个参数 + 44 字节块序列化 + A/B 双槽 + flash 存取
  app_menu.c/.h     三级菜单状态机（MAIN → DEMO/PARAM/SYSTEM → EDIT）
  app_key.c/.h      按键状态机（74HC148 编码 → 消抖/短按/长按/连发）
host_test/          主机端单元测试（4 组、1671 断言、PC 上 gcc 直接跑）
tools/build.ps1     不开 IDE 的等价编译脚本（参数与 .cproject 一致）
docs/               DESIGN / PINMAP / BUILD / DEFENSE
```

**依赖方向是单向的**：`app_main → 功能模块 → app_port → 逐飞库`；
纯逻辑模块只依赖 `app_config.h`，所以能被 PC 测试覆盖，也不会把硬件细节散落到业务逻辑里。

---

## 4. 细则逐条对照（3–11）

| 细则 | 分值 | 演示入口（菜单） | 关键代码 | 现场怎么看 |
|------|------|------------------|----------|------------|
| 3 GPIO 控制 LED | 5 | `DEMO → LED` | `app_led.c`、`app_port.c` | 4 个 LED 流水/常亮/熄灭，`M` 键切换模式 |
| 4 读取按键状态 | 5 | `DEMO → KEYS` | `app_key.c` | 屏上显示编码/键名/点击数，按键时蜂鸣一下 |
| 5 PWM + 蜂鸣器 | 5 | `DEMO → PWM OUT`、`DEMO → BUZZER` | `app_pwm.c`、`app_buzzer.c` | H1 接口（PC6/PD14/PD15）接示波器看扫频；蜂鸣器响 |
| 6 定时器中断控 LED | 5 | `DEMO → TIMER LED` | `zf_isr.c`(TIM6) + `app_led.c::app_led_isr_tick` | 屏上显示中断翻转计数，LED6 按周期亮灭（**翻转在中断里发生**） |
| 7 SPI 屏幕 | 5 | `DEMO → SCREEN` | `app_screen.c`（tft180 / SPI2） | 文字 + 进度条动画 |
| 8 Flash 保存并读取 | 10 | `DEMO → FLASH R/W` | `app_params.c`、`app_crc32.c` | 显示槽位/序号/保存次数/上电次数 + 页 2 读写 CRC 校验结果 |
| 9 外部中断/GPIO 按键 + UI 交互 | 10 | `DEMO → EXTI KEY`、`DEMO → UI PANEL` | `app_port.c`(EXTI) + `app_ui.c` | 中断计数随按键增长；UI 面板 4 个 tile 交互 |
| 10 多级菜单记录参数 | 10 | `DEMO → MENU`、`PARAMETERS`、`SYSTEM → SAVE PARAMS` | `app_menu.c` | 三级层级 `L1/L2/L3`、编辑参数、保存后断电重启仍在 |
| 11 不重新烧录完成全部 | 15 | 全部 | `app_main.c` | 全程只用板上按键与菜单，不接下载器 |
| 12 git 版本管理 | 10 | — | 仓库提交历史 | `git log --oneline` 分阶段提交 |
| 13 盲盒任务 | 40 | — | 现成模块 | 现场抽题（**不能用 AI**），见 §7 的模块清单 |

> 细则 2（阻容焊接正常运行）属于硬件，上电即验证：4 个电源指示灯 + 屏幕点亮 + 串口有日志。
> 细则 1（现场答辩）用本文件回答。

---

## 5. 现场操作剧本（不重新烧录）

1. **上电**：屏幕显示自检过程，串口打印版本/引脚表/参数槽位；
   自检 8 项结果以 `[SELFTEST] xxx OK` 逐行输出，失败位掩码会打印在末尾。
2. **主菜单**：`DOWN`（五向开关下）移动光标，`ENTER`（五向中键）进入，`BACK`（五向左）返回。
   主菜单三项：`DEMO` / `PARAMETERS` / `SYSTEM`。
3. **看细则 3/4**：进 `DEMO → LED`，按 `M` 切换「流水 / 全亮 / 全灭」；
   进 `DEMO → KEYS`，分别按 `U/L/D/R/M/KEY1/KEY2`，观察编码与键名（注意"上"不可分辨，见 §6）。
4. **看细则 5/6**：进 `DEMO → PWM OUT`，H1 的 3 个引脚接示波器/蜂鸣器，观察自动扫频；
   进 `DEMO → BUZZER`，`M` 键重复鸣叫；
   进 `DEMO → TIMER LED`，看 LED6 按参数周期亮灭、计数在涨（中断里翻转）。
5. **看细则 7/9**：进 `DEMO → SCREEN`（进度条动画）；进 `DEMO → UI PANEL`（4 个 tile 交互）。
6. **看细则 8/10**：
   * 进 `PARAMETERS`，选 `BLINK MS` 按 `ENTER` 进三级编辑页，用 `R`/`KEY2` 加减，`ENTER` 提交、`BACK` 取消；
   * 长按 `KEY2`（保存）或进 `SYSTEM → SAVE PARAMS`，屏幕提示保存到哪个槽、序号多少；
   * **断电再上电**，屏幕/串口显示参数与序号仍在（Flash 生效），`boot_count` 每上电 +1；
   * 进 `DEMO → FLASH R/W` 看页 2 的原始读写与 CRC 校验结果。
7. **看细则 9 的外部中断**：进 `DEMO → EXTI KEY`，`M` 键交替「重装 EXTI / 关闭 EXTI」，
   对比 PD8 的计数是否随按键变化、实时电平是否跟随。
8. **看细则 11**：上述全过程只用板上按键，**没有再次下载程序**。
9. **看细则 12**：`git log --oneline` 展示分阶段提交（模板 → 库 → 板级绑定 → 功能 → 菜单/存储 → 文档/测试）。

---

## 6. 高频提问与标准答案

**Q1：为什么可控 LED 只有 4 个，不是 8 个？**
A：LED1–LED4 经 RN4 分别接 5V / 3.3V / 3.3V_MAIN / 3.3V_CAM，是电源指示灯，MCU 不可控；
真正的 MCU 控制通道只有 RN3 的 4 路：PD1/PA15/PA12/PA11 → LED5–LED8，且**高电平点亮**。

**Q2：为什么五向开关的"上"不能用？**
A：74HC148 的 `GS`/`EO` 没接到 MCU，MCU 只能读 3 位编码。"上"接在优先编码器的最高优先输入 I0，
其编码是 `111`，与"无键（输入全高）"完全相同，硬件上不可区分；而且按住"上"会屏蔽其它键。
所以固件把 `111` 当无键，用独立按键 `KEY1`（`011`）承担"上"的导航职责。

**Q3：立创 EDA 符号上 pin13 标的是 I3，为什么你说它是 I0？**
A：以物理引脚和数据手册为准。TI SN74HC148：pin13 = I0、pin12 = I1、pin11 = I2、pin10 = I3
（符号内部标注与数据手册相反）。板上走线连的是物理引脚，固件也按物理引脚建模，并在 `docs/PINMAP.md` 记录了这个坑。

**Q4：为什么用 Flash 扇区 63？掉电会不会丢？**
A：Link.ld 只用前 256KB，最后 4KB（扇区 63）不会被动到。每个槽 44 字节，含 magic/版本/长度/序号/CRC32；
A/B 双槽轮换：每次保存都写"非当前槽"并把序号 +1，上电时取序号大且 CRC 正确的一槽，单槽写坏仍能用另一槽；
写入前先擦 1KB 一页，失败会中止并保留旧数据；参数反序列化后还会再夹取一次范围。

**Q5：为什么按键扫描放在主循环，不放在中断里？**
A：1ms 节拍中断只做两件事（`app_tick_ms++`、中断内翻 LED），保证中断极短；
按键消抖/长按/连发/菜单/UI 都在主循环（10ms 扫描 / 50ms 刷新），没有 ISR 与主循环共享队列的竞争问题。
外部中断（PD8）只累加计数与记录时间戳，也不做业务逻辑。

**Q6：三路 PWM 的频率可以不同吗？**
A：同一定时器内的通道频率必须相同（本次三路分属 TIM3 与 TIM4，仍统一设为同一频率），占空比可独立。

**Q7：屏幕为什么是英文？**
A：逐飞 `tft180` 驱动只内置 ASCII 点阵（`zf_common_font.h` 只有 6×8 / 8×16 与少量中文字模），
要中文需自备字模数组，因此界面统一用 8×16 ASCII，信息密度和可读性都够。

**Q8：怎么证明这些功能真的在工作，而不是"看着像"？**
A：三条自证链：
① 屏幕状态行 `#任务 L灯掩码 E中断数 T中断翻转 F槽位 序号` 实时显示内部状态；
② UART3 115200 日志打印上电信息、参数存取、自检逐项结果；
③ `SELF TEST` 自检把 8 个子系统跑一遍并汇总失败掩码（LED 回读、按键编码、PWM 回读、蜂鸣器开关、屏幕色带、Flash 槽位、EXTI、参数序列化回环）。

**Q9：为什么 144MHz？**
A：`clock_init(SYSTEM_CLOCK_144M)`，8MHz 晶振倍频；PWM/SPI 分频都在 144MHz 下计算。

**Q10：用了逐飞库的哪些部分？**
A：`zf_driver_gpio`（LED/按键/背光）、`zf_driver_pwm`（3 路 PWM + 蜂鸣器）、`zf_driver_pit`（TIM6 1ms 节拍）、
`zf_driver_exti`（PD8 双边沿）、`zf_driver_flash`（参数存储）、`zf_device_tft180`（ST7735 屏）、`zf_common_clock/interrupt`、
`Debug/debug.c`（printf 重定向到 UART3）。

**Q11：如果现场要改成"按键控制 LED 亮灭"（盲盒常见变体），改哪里？**
A：只改 `app_main.c::app_main_handle_event` 里的映射（例如把 `ENTER` 事件映射到 `app_led_toggle`），
或直接加一个演示分支；引脚层与按键层都不用动 —— 这正是分层的目的。

---

## 7. 盲盒任务可用模块清单（现场快速组装，不能用 AI）

| 需求关键词 | 直接可用的接口 |
|------------|----------------|
| 点灯/翻转/流水 | `app_port_led_set(idx, on)`、`app_port_led_toggle(idx)`、`app_led_set_mode()` |
| 读按键/长短按 | `app_key_scan()` + `app_key_take_events()`（返回 press/click/long_press/repeat） |
| 中断计数 | `app_port_exti_key_count()`、`app_port_exti_key_last_ms()` |
| 定时/计时/周期 | `app_time_now()`、`app_time_elapsed()`、`app_led_set_period_ms()` |
| PWM 输出/调光/调速 | `app_port_pwm_set(ch, freq, duty_permille)`、`app_pwm_set_all()` |
| 蜂鸣器 | `app_buzzer_beep(freq, ms, now)`、`app_buzzer_stop()` |
| 屏幕显示 | `app_screen_row_text()`、`app_screen_text()`、`app_screen_bar()`、`tft180_*` |
| 存数据/读数据 | `app_params_save()`、`app_params_store_load()`、`flash_read_page/flash_write_page` |
| 校验/查表 | `app_crc32()`、`app_param_meta[]` |
| 新菜单项 | `app_menu.c` 的 `s_demo_name[]` / `s_sys_name[]` + `app_main_run_action()` 加一个动作 |

**注意**：盲盒要求不能用 AI，所以现场优先复用以上已验证接口，改动集中在 `app_main.c` / `app_demo.c` 两处即可。

---

## 8. 已知限制（主动说明，避免被追问）

1. "上"方向键不可用（硬件未接 GS/EO），已用 `KEY1` 代偿。
2. 可控 LED 4 个（LED1–LED4 是电源灯）。
3. 中文界面需自备字模，当前为 ASCII。
4. 屏幕 128×160、8×16 字体 ⇒ 16 列 × 10 行，列表窗口一次显示 6 行（滚动显示）。
5. 主机端单测只覆盖纯逻辑模块，硬件路径依赖板上 `SELF TEST`。
6. `tools/build.ps1` 内写死本机工具链路径（MounRiver IDE 编译不受影响）。

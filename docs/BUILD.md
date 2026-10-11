# 构建、烧录与测试说明

> 工程：`CH32V307VCT`（CH32V307VCT6 + 逐飞 SeekFree 开源库 V3.7.2 + 一份考核固件）
> 目标端工具链：MounRiver Studio 2（内置 RISC-V GCC）或独立 xPack 工具链
> 主机端单测：系统 gcc（MinGW / Linux 均可）

---

## 1. 目标端编译

### 1.1 方式 A：MounRiver Studio（推荐用于答辩现场）

1. `File → Import → General → Existing Projects into Workspace`，选择本仓库根目录。
2. 选中 `CH32V307VCT` 工程，右键 `Build Project`（配置 `Debug`，优化 `-Os`）。
3. 期望输出：`0 error 0 warning`，`obj/CH32V307VCT.elf` / `.hex` / `.map`。
4. 下载：接 WCH-Link，`Run → Download`（SWD：PA13/PA14，无需按住任何按键）。

> `.cproject` 的根 `sourceEntry` 已排除 `Startup|Peripheral|Ld|Debug|Core|zf_common|zf_driver|zf_device|zf_components|.seekfree-bak|.work|docs|host_test|tools`，
> 因此 `host_test/`（主机端测试）**不会**被 IDE 编译。

### 1.2 方式 B：不开 IDE，脚本直接编译（本仓库已验证）

```powershell
# 在仓库根目录执行
powershell -NoProfile -ExecutionPolicy Bypass -File tools\build.ps1          # 增量编译 + 链接
powershell -NoProfile -ExecutionPolicy Bypass -File tools\build.ps1 -Clean   # 清理后全量编译
```

脚本读取的编译/链接参数与 `.cproject` **完全一致**（见 `tools/build.ps1` 顶部注释）：

* 工具链：`D:\Toolchains\riscv-none-embed-gcc8\bin\riscv-none-embed-gcc.exe`
  （xPack GNU RISC-V Embedded GCC 8.2.0，32-bit；`-print-multi-lib` 含 `rv32imafcxw/ilp32f`）
* 编译：`-march=rv32imafcxw -mabi=ilp32f -msmall-data-limit=8 -msave-restore -Os -std=gnu11
  -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g`
* 链接：`-T Ld/Link.ld -nostartfiles -Xlinker --gc-sections --specs=nano.specs --specs=nosys.specs
  -L zf_device -lm -lzf_device_config`
* 输出：`obj/CH32V307VCT.elf` / `.hex` / `.map`，日志 `obj/build.log`

**实测结果（本机，2026-10-11）**

```
sources   : 100
errors    : 0
warnings  : 0
text      data    bss     dec     hex
46320     304     4136    50760   c648     obj/CH32V307VCT.elf
```

* Flash：46320 + 304 ≈ 45.5KB（Link.ld 给 256KB）
* RAM：data 304 + bss 4136 ≈ 4.3KB（Link.ld 给 32KB，剩余为栈/堆）

> 若换用其它工具链或改 `Ld/Link.ld` 的 FLASH/RAM 分区，**必须同步修改** `User/system_ch32v30x.c`
> 里的分区常量，否则 Flash 读写地址会算错。

### 1.3 烧录（hex）

```powershell
# WCH-LinkUtility（图形界面）或命令行：选择 obj\CH32V307VCT.hex 下载
# MounRiver 自带下载也可；芯片型号选 CH32V307VCT6，接口 SWD
```

### 1.4 串口（观察运行日志）

* 调试串口：**UART3，TX = PB10、RX = PB11**，`115200-8-N-1`。
* 上电即可看到：版本号、关键引脚表、参数槽位/序号/上电次数、自检结果 `[SELFTEST] ...`。

---

## 2. 主机端单元测试（纯逻辑模块）

只有 4 个模块是"可脱离硬件"的纯逻辑：`app_crc32`（CRC32）、`app_params`（参数与序列化）、
`app_menu`（三级菜单状态机）、`app_key`（消抖/长按/连发）。它们不 include 任何硬件头文件，
因此在 PC 上用 `-DAPP_HOST_TEST` 直接编译运行。

```powershell
# 方式 A（Windows 一键）
powershell -NoProfile -ExecutionPolicy Bypass -File host_test\run_tests.ps1

# 方式 B（有 make 的 POSIX/MSYS 环境）
make -C host_test            # 编译并运行
make -C host_test build      # 只编译
make -C host_test clean
```

**实测结果（本机，2026-10-11）**

```
== host unit test : CH32V307VCT6 firmware pure logic ==
[ OK ] app_crc32 / app_params / app_menu / app_key
---- checks: 1671, failures: 0 ----
RESULT: PASS
```

覆盖内容：

| 测试文件 | 覆盖点 |
|----------|--------|
| `host_test/test_crc32.c` | 标准向量 `CRC32("123456789") = 0xCBF43926`、空数据、分段更新一致、空指针 |
| `host_test/test_params.c` | 8 个参数的元数据/默认值合法性、夹取边界、44 字节序列化布局、CRC 校验、5 种损坏场景、A/B 槽选择 |
| `host_test/test_menu.c` | 三级层级与深度、光标环绕、进入/退出、参数编辑提交/取消/上下界、开关型参数取反、系统页 5 个动作映射、文本接口 |
| `host_test/test_key.c` | 74HC148 编码映射、20ms 消抖、短按、长按 1000ms、250ms 连发、无键/忽略码不产生事件、空指针安全 |

---

## 3. 与细则的对应关系（构建相关的验收点）

| 细则 | 验收方式 |
|------|----------|
| 11 不重新烧录完成全部任务 | 只烧一次 `obj/CH32V307VCT.hex`；所有演示/参数/保存都通过板上按键 + 菜单切换（见 `docs/DEFENSE.md` 的操作剧本） |
| 12 使用 git 版本管理 | 本仓库有分阶段提交历史；`tools/`、`host_test/`、`docs/` 与固件源码一起入库 |
| 1 现场答辩 | `docs/DESIGN.md`（结构）、`docs/PINMAP.md`（引脚事实）、`docs/DEFENSE.md`（问答） |
| 8 Flash 保存数据 | 上电日志 + 屏幕 `FLASH R/W` 演示 + 长按 KEY2 保存后断电重启验证 |

---

## 4. 已知限制

* `host_test/` 只测纯逻辑，不含屏幕/GPIO/PWM/中断等硬件路径（这些在板上用 `SELF TEST` 演示自检）。
* 本仓库不附带原理图源文件（PDF/图片），`docs/PINMAP.md` 是人工从原理图核对后的引脚结论。
* 编译脚本 `tools/build.ps1` 里写死了本机工具链路径，换机器只需改脚本顶部的 `$GCC` 变量（IDE 编译不受影响）。

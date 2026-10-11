<#
    tools\flash_usb.ps1
    ------------------------------------------------------------------
    不用 WCH-Link，只用主板自己的 Type-C 口 + 芯片内置 bootloader 烧录固件。

    原理：CH32V307 的 BOOT 区把 BOOT0 通过 20Ω 电阻 SW16 接到 3.3V_MAIN，
          按住 SW16 时 BOOT0 = 1，复位后芯片进入出厂 ISP 模式，
          在 USB 上枚举出 VID_4348&PID_55E0，此时可用 wchisp 通过 Type-C 烧录。

    用法：
        # 1) 按住板上 BOOT/下载 键（SW16，位置见 docs/PINMAP.md）不放
        # 2) 按一下复位键 SW17（没有复位键就按住 SW16 拔插一次 Type-C）
        # 3) 运行：
        powershell -NoProfile -ExecutionPolicy Bypass -File tools\flash_usb.ps1
        # 只探测芯片、不烧录：
        powershell -NoProfile -ExecutionPolicy Bypass -File tools\flash_usb.ps1 -Probe

    注意：脚本用的是 wchisp，需要 USB 上出现 4348:55E0 设备；
          v0.3.0 正式版只支持 libusb/WinUSB 驱动，本机装的是 WCH 官方驱动，
          所以要用 nightly 构建（tools\wchisp-nightly\...，实测可直接用官方驱动打开）。
#>
[CmdletBinding()]
param(
    [string]$Hex        = "",
    [string]$WchispExe  = "",
    [int]   $WaitSeconds = 180,
    [switch]$Probe
)

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
if ([string]::IsNullOrEmpty($Hex))       { $Hex       = Join-Path $root "obj\CH32V307VCT.hex" }
if ([string]::IsNullOrEmpty($WchispExe)) { $WchispExe = Join-Path $root "tools\wchisp-nightly\wchisp-win-x64\wchisp.exe" }

function Get-IspDevice {
    Get-PnpDevice -PresentOnly -ErrorAction SilentlyContinue |
        Where-Object { $_.InstanceId -match 'VID_4348&PID_55E0' }
}

Write-Host "==== CH32V307 USB-ISP 烧录 ====" -ForegroundColor Cyan
Write-Host "固件   : $Hex"
Write-Host "wchisp : $WchispExe"
Write-Host ""

if (-not (Test-Path $Hex)) {
    Write-Host "[错误] 找不到固件，请先编译：powershell -NoProfile -ExecutionPolicy Bypass -File tools\build.ps1" -ForegroundColor Red
    exit 2
}
if (-not (Test-Path $WchispExe)) {
    Write-Host "[错误] 找不到 wchisp.exe。下载 nightly 构建：" -ForegroundColor Red
    Write-Host "  https://github.com/ch32-rs/wchisp/releases/download/nightly/wchisp-win-x64.zip"
    Write-Host "  解压到 tools\wchisp-nightly\wchisp-win-x64\ 后重试。"
    exit 2
}

# 官方 WCH 驱动 + CH375DLL64.dll 组合：把系统目录里的 DLL 放到 exe 旁边
$dll = Join-Path $env:WINDIR "System32\CH375DLL64.dll"
if (Test-Path $dll) {
    Copy-Item $dll (Split-Path $WchispExe) -Force -ErrorAction SilentlyContinue
}

if (Get-IspDevice) {
    Write-Host "[信息] 已经检测到 ISP 设备（4348:55E0），无需再按 BOOT 键。" -ForegroundColor Green
} else {
    Write-Host "[请操作] 现在按住板上 BOOT/下载 键（SW16）不放，再按一下复位键（SW17）。" -ForegroundColor Yellow
    Write-Host "         本脚本会等待最多 $WaitSeconds 秒……" -ForegroundColor Yellow
    $deadline = (Get-Date).AddSeconds($WaitSeconds)
    while (-not (Get-IspDevice)) {
        if ((Get-Date) -gt $deadline) {
            Write-Host "[错误] 等待超时：没有出现 4348:55E0 设备。" -ForegroundColor Red
            Write-Host "  自查：① Type-C 线是否带数据线（不是纯充电线）"
            Write-Host "        ② 是否按住了 BOOT 键再复位（顺序反了会直接跑旧固件）"
            Write-Host "        ③ 设备管理器里是否有带感叹号的 WCH 设备（装驱动：WCH-LinkUtility\Drv_Link\CH372DRV_S.exe）"
            exit 3
        }
        Start-Sleep -Milliseconds 500
    }
    Write-Host "[信息] 检测到 ISP 设备！" -ForegroundColor Green
}

Write-Host ""
Write-Host "---- wchisp info ----"
& $WchispExe info 2>&1 | ForEach-Object { Write-Host "  $_" }
if ($LASTEXITCODE -ne 0) {
    Write-Host "[错误] 打不开 USB 设备（通常是驱动后端问题）。" -ForegroundColor Red
    Write-Host "  本机已装的 WCH 官方驱动需要 nightly 版 wchisp；若仍失败，可用 Zadig 把"
    Write-Host "  4348:55E0 的驱动换成 WinUSB/libusb，或改用 WCH-LinkE 走 SWD 烧录。"
    exit 4
}

if ($Probe) {
    Write-Host ""
    Write-Host "[完成] 只探测（-Probe），未烧录。" -ForegroundColor Cyan
    exit 0
}

Write-Host ""
Write-Host "---- wchisp flash ----"
& $WchispExe flash $Hex 2>&1 | ForEach-Object { Write-Host "  $_" }
if ($LASTEXITCODE -ne 0) {
    Write-Host "[错误] 烧录失败，退出码 $LASTEXITCODE。" -ForegroundColor Red
    exit 5
}

Write-Host ""
Write-Host "[完成] 烧录并校验通过。" -ForegroundColor Green
Write-Host "接下来：① 松开 BOOT 键（SW16）  ② 按一下复位键 SW17（或拔插一次 Type-C）" -ForegroundColor Yellow
Write-Host "        芯片离开 ISP 模式后固件开始运行：屏幕亮起主菜单，UART3(PB10/PB11,115200) 打印自检结果。"
Write-Host "提示：运行时 PA11/PA12 被固件当作 LED7/LED8 输出，烧完建议拔掉 Type-C 线。"
exit 0

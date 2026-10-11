<#
==========================================================================================
 tools/build.ps1 - 无 IDE 本地构建脚本（复现 MounRiver Studio 2.5.0 生成的编译/链接命令）

 用途：在没有 MounRiver IDE 的机器上，用同一套 xPack riscv-none-embed-gcc 8.2.0
       编译/链接本工程，用来验证 "0 error 0 warning" 和最终固件大小。

 工具链要求：multilib 里必须有 rv32imafcxw/ilp32f（逐飞 libzf_device_config.a 是
             单精度硬浮点 ABI，软浮点会链接失败）。

 用法：
   powershell -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1
   powershell -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1 -Clean

 输出：obj\ 下的 .o / CH32V307VCT.elf / .hex / .map / build.log，最后打印 errors / warnings / size。
==========================================================================================
#>
param(
    [string] $Toolchain = "D:\Toolchains\riscv-none-embed-gcc8\bin",
    [string] $OutDir    = "obj",
    [switch] $Clean,
    [switch] $ShowCmd
)

$ErrorActionPreference = "Continue"

$root    = Split-Path -Parent $PSScriptRoot
$outFull = Join-Path $root $OutDir
$gcc     = Join-Path $Toolchain "riscv-none-embed-gcc.exe"
$objcopy = Join-Path $Toolchain "riscv-none-embed-objcopy.exe"
$sizeExe = Join-Path $Toolchain "riscv-none-embed-size.exe"

if(-not (Test-Path $gcc))
{
    throw "找不到工具链: $gcc  (用 -Toolchain 指定目录)"
}

if($Clean -and (Test-Path $outFull))
{
    Remove-Item $outFull -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $outFull | Out-Null

# 运行外部工具：用 cmd.exe 做重定向，把 gcc 的 stdout/stderr 一起收进日志
function Invoke-Tool
{
    param([string] $Exe, [string[]] $ToolArgs)

    $tmpOut = Join-Path $outFull ("_tool_" + [guid]::NewGuid().ToString("N") + ".log")
    $quoted = @()
    foreach($a in $ToolArgs)
    {
        if($a -match '\s') { $quoted += ('"' + $a + '"') } else { $quoted += $a }
    }
    $cmdLine = '"' + $Exe + '" ' + ($quoted -join ' ') + ' > "' + $tmpOut + '" 2>&1'
    & cmd.exe /c $cmdLine | Out-Null
    $code = $LASTEXITCODE
    $text = ""
    if(Test-Path $tmpOut)
    {
        $text = Get-Content -Raw -Path $tmpOut
        Remove-Item $tmpOut -Force -ErrorAction SilentlyContinue
    }
    return @{ exit = $code; text = $text }
}

# ---------------------------------------------------------------- 编译参数（与 .cproject 一致）
$incs = @("Startup", "Core", "Debug", "User", "Peripheral/inc",
          "zf_common", "zf_driver", "zf_device", "zf_components") |
        ForEach-Object { "-I" + (Join-Path $root $_) }

$cflags = @(
    "-march=rv32imafcxw", "-mabi=ilp32f", "-msmall-data-limit=8", "-msave-restore",
    "-fmax-errors=20", "-Os", "-fmessage-length=0", "-fsigned-char",
    "-ffunction-sections", "-fdata-sections", "-fno-common",
    "-Wunused", "-Wuninitialized", "-g", "-std=gnu11"
) + $incs

$srcDirs = @("Core", "Debug", "User", "Peripheral/src",
             "zf_common", "zf_components", "zf_device", "zf_driver")

$sources = @()
foreach($d in $srcDirs)
{
    $dir = Join-Path $root $d
    if(Test-Path $dir)
    {
        $sources += Get-ChildItem -Path $dir -Filter *.c -File | ForEach-Object { $_.FullName }
    }
}
$sources += (Join-Path $root "Startup/startup_ch32v30x_D8C.S")

$log = Join-Path $outFull "build.log"
("build log - " + (Get-Date -Format "yyyy-MM-dd HH:mm:ss")) | Set-Content -Path $log

$objFiles = @()
$errors   = 0
$warnings = 0

foreach($src in $sources)
{
    $rel    = $src.Substring($root.Length + 1)
    $objDir = Join-Path $outFull (Split-Path -Parent $rel)
    New-Item -ItemType Directory -Force -Path $objDir | Out-Null
    $obj = Join-Path $outFull ([System.IO.Path]::ChangeExtension($rel, ".o"))

    $ccArgs = @()
    if($src.EndsWith(".S")) { $ccArgs += "-x"; $ccArgs += "assembler-with-cpp" }
    $ccArgs += $cflags
    $ccArgs += @("-c", "-o", $obj, $src)

    if($ShowCmd) { Write-Host ("CC " + $rel) }
    $r = Invoke-Tool -Exe $gcc -ToolArgs $ccArgs
    Add-Content -Path $log -Value ("=== CC " + $rel + "`r`n" + $r.text)

    if($r.exit -ne 0)
    {
        $errors ++
        Write-Host ("ERROR   " + $rel) -ForegroundColor Red
        Write-Host $r.text
    }
    elseif($r.text -match "warning:")
    {
        $warnings ++
        Write-Host ("WARNING " + $rel) -ForegroundColor Yellow
        Write-Host $r.text
    }
    $objFiles += $obj
}

# ---------------------------------------------------------------- 链接
$elf = Join-Path $outFull "CH32V307VCT.elf"
$map = Join-Path $outFull "CH32V307VCT.map"
$ldArgs = @(
    "-march=rv32imafcxw", "-mabi=ilp32f", "-msmall-data-limit=8", "-msave-restore",
    "-fmax-errors=20", "-Os", "-fmessage-length=0", "-fsigned-char",
    "-ffunction-sections", "-fdata-sections", "-fno-common", "-Wunused", "-Wuninitialized", "-g",
    "-T", (Join-Path $root "Ld/Link.ld"),
    "-nostartfiles", "-Xlinker", "--gc-sections",
    "-L", (Join-Path $root "zf_device"),
    "-Wl,-Map,$map",
    "--specs=nano.specs", "--specs=nosys.specs",
    "-o", $elf
) + $objFiles + @("-lm", "-lzf_device_config")

$r = Invoke-Tool -Exe $gcc -ToolArgs $ldArgs
Add-Content -Path $log -Value ("=== LINK`r`n" + $r.text)
if($r.exit -ne 0)
{
    $errors ++
    Write-Host "LINK FAILED" -ForegroundColor Red
    Write-Host $r.text
}

if($errors -eq 0)
{
    Invoke-Tool -Exe $objcopy -ToolArgs @("-O", "ihex", $elf, (Join-Path $outFull "CH32V307VCT.hex")) | Out-Null
    $sz = Invoke-Tool -Exe $sizeExe -ToolArgs @("--format=berkeley", $elf)
    Add-Content -Path $log -Value ("=== SIZE`r`n" + $sz.text)

    Write-Host ""
    Write-Host ("sources   : " + $sources.Count)
    Write-Host ("errors    : " + $errors)
    Write-Host ("warnings  : " + $warnings)
    Write-Host $sz.text.Trim()
    Write-Host ("elf       : " + $elf)
    Write-Host ("log       : " + $log)
}
else
{
    Write-Host ""
    Write-Host ("errors    : " + $errors) -ForegroundColor Red
    Write-Host ("warnings  : " + $warnings) -ForegroundColor Yellow
    Write-Host ("see log   : " + $log)
    exit 1
}

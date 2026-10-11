#---------------------------------------------------------------------------------------------------------------------
# 文件名称          run_tests.ps1
# 说明              主机端单元测试一键脚本（Windows / PowerShell 5.1+）
#                   用系统 gcc（C:\mingw64\bin）编译 4 个纯逻辑模块 + 5 个测试文件并运行
#                   用法：powershell -NoProfile -ExecutionPolicy Bypass -File host_test\run_tests.ps1
#                   退出码：0 = 全部通过，1 = 有失败，2 = 编译失败
#---------------------------------------------------------------------------------------------------------------------
$ErrorActionPreference = 'Stop'

$here    = Split-Path -Parent $MyInvocation.MyCommand.Path
$userDir = Join-Path (Split-Path -Parent $here) 'User'
$cc      = if ($env:CC) { $env:CC } else { 'gcc' }
$out     = Join-Path $here 'host_test.exe'

$src  = @('test_main.c', 'test_crc32.c', 'test_params.c', 'test_menu.c', 'test_key.c') |
        ForEach-Object { Join-Path $here $_ }
$src += @('app_crc32.c', 'app_params.c', 'app_menu.c', 'app_key.c') |
        ForEach-Object { Join-Path $userDir $_ }

Write-Host "[host_test] compiler : $cc"
Write-Host "[host_test] out      : $out"

& $cc -std=gnu11 -O1 -Wall -Wextra -DAPP_HOST_TEST "-I$userDir" -o $out @src
if ($LASTEXITCODE -ne 0)
{
    Write-Host '[host_test] COMPILE FAILED'
    exit 2
}

& $out
if ($LASTEXITCODE -ne 0)
{
    Write-Host '[host_test] TESTS FAILED'
    exit 1
}

Write-Host '[host_test] ALL TESTS PASSED'
exit 0

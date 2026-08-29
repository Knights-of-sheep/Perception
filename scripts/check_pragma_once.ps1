# ===== #pragma once 门禁（006-constitution-refactor / FR-004，SC-003）=====
# 扫描 src/ 与 tests/cpp/ 全部 .h/.hpp，检测缺失 #pragma once 的文件。
# 退出码：0 = 全部合规；1 = 存在缺失（输出文件清单）。
# 用法：powershell -File scripts/check_pragma_once.ps1 [-Root <repo>]

param(
    [string]$Root = (Split-Path -Parent $PSScriptRoot)
)

$ErrorActionPreference = 'Stop'
$srcDir  = Join-Path $Root 'src'
$testDir = Join-Path $Root 'tests\cpp'
if (-not (Test-Path $srcDir)) { Write-Error "src dir not found: $srcDir"; exit 2 }

$paths = @($srcDir)
if (Test-Path $testDir) { $paths += $testDir }

$missing = [System.Collections.Generic.List[string]]::new()
$total = 0

Get-ChildItem -Path $paths -Recurse -File -Include *.h, *.hpp |
    Where-Object { $_.FullName -notmatch '\\build\\' } |
    ForEach-Object {
        $total++
        $found = Get-Content -LiteralPath $_.FullName -TotalCount 64 |
                     Select-String -Pattern '^\s*#\s*pragma\s+once\s*$' -Quiet
        if (-not $found) {
            $missing.Add($_.FullName)
            Write-Host "[ERROR] missing #pragma once: $($_.FullName)" -ForegroundColor Red
        }
    }

Write-Host "pragma-once check: $total header(s), $($missing.Count) missing"
if ($missing.Count -gt 0) { exit 1 }
exit 0

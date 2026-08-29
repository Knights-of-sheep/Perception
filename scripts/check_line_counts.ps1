# ===== 行数上限门禁（006-constitution-refactor / FR-001/002/003，SC-001）=====
# 规则（宪法 v4.0.0 头文件约束 + spec Assumptions）：
#   .cpp  ≤ 800（红线）
#   .h    红线 500 / 建议 300（tests/cpp 下仅红线约束，建议值对测试从宽）
#   .hpp  ≤ 800（模板放宽）
# 建议值达成率报告供 SC-001「建议 300 行达成率 ≥ 90%」验收。
# 退出码：0 = 通过；1 = 有红线违规（ERROR）；2 = 参数/路径错误。
# 用法：powershell -File scripts/check_line_counts.ps1 [-Root <repo>]

param(
    [string]$Root = (Split-Path -Parent $PSScriptRoot)
)

$ErrorActionPreference = 'Stop'
$srcDir  = Join-Path $Root 'src'
$testDir = Join-Path $Root 'tests\cpp'
if (-not (Test-Path $srcDir)) { Write-Error "src dir not found: $srcDir"; exit 2 }

$limits = @{ '.cpp' = 800; '.hpp' = 800; '.h' = 500 }  # 红线
$suggestedH = 300

$errors = [System.Collections.Generic.List[string]]::new()
$warnings = [System.Collections.Generic.List[string]]::new()
$hCount = 0; $hWithinSuggest = 0

$paths = @($srcDir)
if (Test-Path $testDir) { $paths += $testDir }

Get-ChildItem -Path $paths -Recurse -File -Include *.cpp, *.h, *.hpp |
    Where-Object { $_.FullName -notmatch '\\build\\' } |
    ForEach-Object {
        $lines = (Get-Content -LiteralPath $_.FullName).Count
        $ext = $_.Extension.ToLowerInvariant()
        $isTest = $_.FullName.StartsWith($testDir, [System.StringComparison]::OrdinalIgnoreCase)

        $redline = $limits[$ext]
        if ($lines -gt $redline) {
            $errors.Add("$($_.FullName): $lines lines > REDLINE $redline")
        } elseif ($ext -eq '.h' -and -not $isTest -and $lines -gt $suggestedH) {
            $warnings.Add("$($_.FullName): $lines lines > suggested $suggestedH (REDLINE $redline)")
        }
        if ($ext -eq '.h') {
            $hCount++
            if ($lines -le $suggestedH) { $hWithinSuggest++ }
        }
    }

foreach ($w in $warnings) { Write-Host "[WARN] $w" -ForegroundColor Yellow }
foreach ($e in $errors)   { Write-Host "[ERROR] $e" -ForegroundColor Red }

if ($hCount -gt 0) {
    $pct = [math]::Round(100.0 * $hWithinSuggest / $hCount, 1)
    Write-Host "SC-001: .h within suggested 300 lines: $hWithinSuggest/$hCount ($pct%)" -ForegroundColor Cyan
}

Write-Host "line-count check: $($errors.Count) error(s), $($warnings.Count) warning(s)"
if ($errors.Count -gt 0) { exit 1 }
exit 0

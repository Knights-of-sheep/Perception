# ===== 全库 clang-format 对齐（006-constitution-refactor / FR-014，SC-007）=====
# 对 src/ 与 tests/cpp/ 全部 C/C++ 源文件执行 .clang-format 格式化。
#   -Check 模式：clang-format --dry-run --Werror，仅报告不合规文件（门禁，退出码非 0）。
#   无 -Check 模式：-i 就地改写（须与逻辑变更分次提交，便于审查/回滚）。
# 依赖：clang-format（LLVM）。安装：winget install LLVM.LLVM 或 pip install clang-format。
# 用法：powershell -File scripts/format_all.ps1 [-Check] [-Root <repo>]

param(
    [switch]$Check,
    [string]$Root = (Split-Path -Parent $PSScriptRoot)
)

$ErrorActionPreference = 'Stop'

$clangFormat = Get-Command clang-format -ErrorAction SilentlyContinue
if (-not $clangFormat) {
    Write-Error "clang-format not found. Install LLVM (winget install LLVM.LLVM) or pip install clang-format."
    exit 2
}

$srcDir  = Join-Path $Root 'src'
$testDir = Join-Path $Root 'tests\cpp'
$paths = @($srcDir)
if (Test-Path $testDir) { $paths += $testDir }

$files = Get-ChildItem -Path $paths -Recurse -File -Include *.cpp, *.h, *.hpp, *.cc, *.cxx |
             Where-Object { $_.FullName -notmatch '\\build\\' }

if ($files.Count -eq 0) { Write-Host "no source files found under $paths"; exit 0 }

if ($Check) {
    Write-Host "format check: $($files.Count) file(s) (dry-run --Werror)"
    & clang-format --dry-run --Werror $files.FullName
    if ($LASTEXITCODE -eq 0) { Write-Host "format check: PASS" } else { Write-Host "format check: FAIL" }
    exit $LASTEXITCODE
}

Write-Host "formatting $($files.Count) file(s) in place..."
& clang-format -i $files.FullName
exit $LASTEXITCODE

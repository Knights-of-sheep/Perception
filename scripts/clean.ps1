<#
.SYNOPSIS
    Perception 清理脚本：删除 CMake 中间生成物与构建产物，恢复到「克隆即构建」状态。

.DESCRIPTION
    删除以下内容（源码 / 规格 / 文档 / 脚本一律保留）：
      - build\          CMake 构建目录（中间文件 + 生成物）
      - build-gui\      历史 GUI 构建目录（若存在）
      - bin\            exe/dll 运行产物
      - lib\            .lib/.a/.so 库产物
      - .pytest_cache\  pytest 缓存
      - __pycache__ / *.py[cod]   Python 字节码缓存（递归，排除上述目录）
      - 源码树中的 CMakeFiles\ / CMakeCache.txt（若曾被就地生成）

.PARAMETER Force
    跳过删除前的 y/N 确认，直接删除。

.PARAMETER KeepPythonCache
    保留 Python 缓存（__pycache__ / .pytest_cache），默认清理。

.PARAMETER WhatIf
    预览模式：仅列出将删除的路径，不执行删除。

.EXAMPLE
    # 预览将删除哪些内容（不实际删除）
    .\scripts\clean.ps1 -WhatIf

.EXAMPLE
    # 交互式确认后清理
    .\scripts\clean.ps1

.EXAMPLE
    # 免确认直接清理
    .\scripts\clean.ps1 -Force
#>
[CmdletBinding()]
param(
    [switch]$Force,
    [switch]$KeepPythonCache,
    [switch]$WhatIf
)

$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot

function Write-Step($msg) { Write-Host "==> $msg" -ForegroundColor Cyan }
function Write-Ok($msg)   { Write-Host "OK: $msg" -ForegroundColor Green }
function Write-Warn2($msg){ Write-Host "WARN: $msg" -ForegroundColor Yellow }

# ---------------------------------------------------------------
# 1. 收集待清理路径
# ---------------------------------------------------------------
# 顶层固定目录（CMake 中间生成物 + 产物输出目录）
$topLevel = @("build", "build-gui", "bin", "lib", ".pytest_cache")

$toDelete = @()
foreach ($name in $topLevel) {
    $p = Join-Path $RepoRoot $name
    if (Test-Path $p) { $toDelete += Get-Item -Path $p }
}

# 永不清理的根目录：第三方 vendored 依赖 / git 目录
$protectedRoots = @(
    (Join-Path $RepoRoot "third-party"),
    (Join-Path $RepoRoot ".git")
) | ForEach-Object { $_.TrimEnd('\') }

# 判断路径是否位于任一受保护/待删目录之内
function Test-InsideAny([string]$full, [string[]]$roots) {
    foreach ($r in $roots) {
        if ($full.StartsWith($r + '\', [System.StringComparison]::OrdinalIgnoreCase)) {
            return $true
        }
    }
    return $false
}

if (-not $KeepPythonCache) {
    # 已收集的顶层目录（如 build/bin/lib）将在删除时连带其中的缓存，
    # 因此递归收集 __pycache__ / *.pyc 时排除它们，避免重复与误删。
    $ignored = @($toDelete | ForEach-Object { $_.FullName.TrimEnd('\') }) + $protectedRoots

    $pyCache = Get-ChildItem -Path $RepoRoot -Directory -Filter "__pycache__" -Recurse -ErrorAction SilentlyContinue
    $pyc     = Get-ChildItem -Path $RepoRoot -File -Filter "*.py[cod]" -Recurse -ErrorAction SilentlyContinue

    foreach ($item in @($pyCache) + @($pyc)) {
        if (-not (Test-InsideAny $item.FullName $ignored)) { $toDelete += $item }
    }
}

# 源码树中可能残留的 CMake 就地产物（排除已收集目录与受保护根）
$ignoredAll = @($toDelete | ForEach-Object { $_.FullName.TrimEnd('\') }) + $protectedRoots
$cmakeFiles = Get-ChildItem -Path $RepoRoot -Directory -Filter "CMakeFiles" -Recurse -ErrorAction SilentlyContinue
$cmakeCache = Get-ChildItem -Path $RepoRoot -File -Filter "CMakeCache.txt" -Recurse -ErrorAction SilentlyContinue
foreach ($item in @($cmakeFiles) + @($cmakeCache)) {
    if (-not (Test-InsideAny $item.FullName $ignoredAll)) { $toDelete += $item }
}

# ---------------------------------------------------------------
# 2. 展示 / 确认 / 执行
# ---------------------------------------------------------------
if ($toDelete.Count -eq 0) {
    Write-Ok "没有需要清理的内容，仓库已是干净状态。"
    exit 0
}

Write-Step "将删除以下内容（共 $($toDelete.Count) 项）："
foreach ($item in $toDelete) {
    Write-Host "  - $($item.FullName)"
}

if ($WhatIf) {
    Write-Warn2 "（-WhatIf 预览模式，未执行删除）"
    exit 0
}

if (-not $Force) {
    $reply = Read-Host "确认删除？[y/N]"
    if ($reply -notmatch "^[yY]") {
        Write-Warn2 "已取消，未删除任何内容。"
        exit 0
    }
}

foreach ($item in $toDelete) {
    Remove-Item -Recurse -Force -LiteralPath $item.FullName -ErrorAction SilentlyContinue
}

Write-Ok "清理完成，可重新执行 .\scripts\build.ps1 进行全新构建。"

<#
.SYNOPSIS
    Perception 构建脚本：CMake 配置 + 编译 +（可选）C++ 单测 / pytest。

.DESCRIPTION
    通过入参控制：
      -Version      程序版本号（透传 -DPERCEPTION_VERSION，默认沿用 CMakeLists 缓存值）
      -UnitTests    构建成功后运行 CTest（C++ 单元测试）
      -Pytest       构建成功后运行 pytest（命令层测试）
      -Gui          构建 Qt5/VTK 界面层（PERCEPTION_BUILD_GUI=ON）
      -Qt5Dir       指定 Qt5_DIR（-Gui 时必填，未指定则探测默认路径）
      -VtkDir       指定 VTK_DIR（-Gui 时必填，未指定则探测默认路径）
      -Config       构建配置，默认 Release（多配置生成器生效）
      -Clean        先删除 build 目录再重新配置
      -BinDir       可执行文件（exe/dll）输出目录，默认 <仓库根>/bin
      -LibDir       库文件（.lib/.a/.so）输出目录，默认 <仓库根>/lib

.PARAMETER Version
    程序版本号，例如 "0.2.0"、"1.0.0"。

.EXAMPLE
    # 仅编译（沿用默认版本号 0.1.0）
    .\scripts\build.ps1

.EXAMPLE
    # 设置版本号 0.2.0，编译后依次运行 CTest 与 pytest
    .\scripts\build.ps1 -Version 0.2.0 -UnitTests -Pytest

.EXAMPLE
    # 构建 GUI 层（需先指定 Qt/VTK 路径）
    .\scripts\build.ps1 -Version 0.3.0 -Gui -Qt5Dir "C:\Qt\5.12.12\msvc2019_64\lib\cmake\Qt5" -VtkDir "D:\vtk-941-qt\bak\lib\cmake\vtk-9.4"

.EXAMPLE
    # 清理后重新构建并跑单测
    .\scripts\build.ps1 -Clean -UnitTests -Config Debug
#>
[CmdletBinding()]
param(
    [string]$Version = "",
    [switch]$UnitTests,
    [switch]$Pytest,
    [switch]$Gui,
    [string]$Qt5Dir = "",
    [string]$VtkDir = "",
    [string]$Config = "Release",
    [switch]$Clean,
    [string]$BinDir = "",
    [string]$LibDir = ""
)

$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot
$BuildDir = Join-Path $RepoRoot "build"

# 产物输出目录：exe/dll -> bin，lib/import lib -> lib
if (-not $BinDir) { $BinDir = Join-Path $RepoRoot "bin" }
if (-not $LibDir) { $LibDir = Join-Path $RepoRoot "lib" }

function Write-Step($msg) { Write-Host "==> $msg" -ForegroundColor Cyan }
function Write-Ok($msg)   { Write-Host "OK: $msg" -ForegroundColor Green }
function Write-Warn2($msg){ Write-Host "WARN: $msg" -ForegroundColor Yellow }

# ---------------------------------------------------------------
# 1. 定位工具链（cmake / ctest / python）
# ---------------------------------------------------------------
function Find-Tool([string]$name) {
    $cmd = Get-Command $name -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    return $null
}

$cmakeExe = Find-Tool "cmake"
if (-not $cmakeExe) {
    # 兜底：通过 vswhere 查找 VS 自带的 CMake
    $vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswhere) {
        $vsPath = & $vswhere -latest -products * `
            -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
            -property installationPath
        if ($vsPath) {
            $candidate = Join-Path $vsPath `
                "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
            if (Test-Path $candidate) { $cmakeExe = $candidate }
        }
    }
}
if (-not $cmakeExe) {
    throw "未找到 cmake，请安装 VS2022 C++ 工作负载或将 CMake 加入 PATH。"
}
Write-Ok "cmake : $cmakeExe"

$ctestExe = Find-Tool "ctest"
if (-not $ctestExe) {
    $cmakeBin = Split-Path -Parent $cmakeExe
    $candidate = Join-Path $cmakeBin "ctest.exe"
    if (Test-Path $candidate) { $ctestExe = $candidate }
}
if ($UnitTests -and -not $ctestExe) {
    throw "-UnitTests 需要 ctest，但未找到 ctest。"
}

# ---------------------------------------------------------------
# 2. 清理（可选）
# ---------------------------------------------------------------
if ($Clean -and (Test-Path $BuildDir)) {
    Write-Step "清理 build 目录: $BuildDir"
    Remove-Item -Recurse -Force $BuildDir
}

# ---------------------------------------------------------------
# 3. CMake 配置
# ---------------------------------------------------------------
$configureArgs = @("-S", $RepoRoot, "-B", $BuildDir)

# 单配置生成器（如 Ninja）需要 CMAKE_BUILD_TYPE；多配置生成器会忽略它
$configureArgs += "-DCMAKE_BUILD_TYPE=$Config"

# 产物输出目录：exe/dll -> bin，lib/import lib -> lib。
# 注意：多配置生成器（VS）会自动在 bin/lib 下追加 <Config> 子目录，
# 例如 -Config Release 时实际产物落在 bin\Release 与 lib\Release；
# 单配置生成器（Ninja/Makefile）则直接输出到 bin 与 lib。
# （CMAKE_*_OUTPUT_DIRECTORY_<CONFIG> 由 CMake 内部推导，不可在命令行覆盖，勿设置。）
$configureArgs += "-DCMAKE_RUNTIME_OUTPUT_DIRECTORY=$BinDir"
$configureArgs += "-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=$LibDir"
$configureArgs += "-DCMAKE_ARCHIVE_OUTPUT_DIRECTORY=$LibDir"

if ($Version) {
    Write-Step "设置版本号: $Version"
    $configureArgs += "-DPERCEPTION_VERSION=$Version"
}

if ($Gui) {
    $configureArgs += "-DPERCEPTION_BUILD_GUI=ON"
    if (-not $Qt5Dir) { $Qt5Dir = "C:\Qt\5.12.12\msvc2019_64\lib\cmake\Qt5" }
    if (-not $VtkDir) { $VtkDir = "D:\vtk-941-qt\bak\lib\cmake\vtk-9.4" }
    if (-not (Test-Path $Qt5Dir)) {
        throw "Qt5_DIR 不存在: $Qt5Dir（请用 -Qt5Dir 指定正确路径）"
    }
    if (-not (Test-Path $VtkDir)) {
        throw "VTK_DIR 不存在: $VtkDir（请用 -VtkDir 指定正确路径）"
    }
    $configureArgs += "-DQt5_DIR=$Qt5Dir"
    $configureArgs += "-DVTK_DIR=$VtkDir"
}

Write-Step "CMake 配置: $($configureArgs -join ' ')"
& $cmakeExe @configureArgs
if ($LASTEXITCODE -ne 0) { throw "CMake 配置失败（exit=$LASTEXITCODE）" }

# ---------------------------------------------------------------
# 4. 编译
# ---------------------------------------------------------------
Write-Step "编译 (Config=$Config)"
& $cmakeExe --build $BuildDir --config $Config
if ($LASTEXITCODE -ne 0) { throw "编译失败（exit=$LASTEXITCODE）" }
Write-Ok "编译成功"

# ---------------------------------------------------------------
# 4.5 GUI：用 windeployqt 部署 Qt 运行库（-Gui 时）
#    让 perception.exe 无需手动配置 PATH 即可直接运行/双击启动
# ---------------------------------------------------------------
$qtDeployStatus = "未启用（非 -Gui）"
if ($Gui) {
    # 多配置生成器（VS）产物在 bin\<Config>；单配置生成器直接在 bin
    $deployDir = Join-Path $BinDir $Config
    $guiExe = Join-Path $deployDir "perception.exe"
    if (-not (Test-Path $guiExe)) {
        $deployDir = $BinDir
        $guiExe = Join-Path $BinDir "perception.exe"
    }
    if (-not (Test-Path $guiExe)) {
        Write-Warn2 "未找到 perception.exe（$guiExe），跳过 Qt 运行库部署"
        $qtDeployStatus = "跳过（未找到 exe）"
    } else {
        # 由 Qt5_DIR（<qt>\lib\cmake\Qt5）反推 Qt 根目录，定位 windeployqt.exe
        $qtRoot = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $Qt5Dir))
        $windeployqt = Get-ChildItem -Path (Join-Path $qtRoot "bin") `
            -Filter "windeployqt.exe" -ErrorAction SilentlyContinue | Select-Object -First 1
        if (-not $windeployqt) {
            Write-Warn2 "未找到 windeployqt.exe（$qtRoot\bin），跳过 Qt 运行库部署"
            $qtDeployStatus = "跳过（未找到 windeployqt）"
        } else {
            Write-Step "部署 Qt 运行库: $deployDir"
            & $windeployqt.FullName --release --no-translations `
                --no-system-d3d-compiler --no-opengl-sw $guiExe
            if ($LASTEXITCODE -ne 0) {
                Write-Warn2 "windeployqt 部署异常（exit=$LASTEXITCODE），可手动运行: $($windeployqt.FullName) $guiExe"
                $qtDeployStatus = "异常（exit=$LASTEXITCODE）"
            } else {
                Write-Ok "Qt 运行库部署完成"
                $qtDeployStatus = "已完成"
            }
        }
    }
}

# ---------------------------------------------------------------
# 5. 测试
# ---------------------------------------------------------------
if ($UnitTests) {
    Write-Step "运行 CTest（C++ 单元测试）"
    & $ctestExe --test-dir $BuildDir -C $Config --output-on-failure
    if ($LASTEXITCODE -ne 0) { throw "CTest 失败（exit=$LASTEXITCODE）" }
    Write-Ok "CTest 全部通过"
}

if ($Pytest) {
    Write-Step "运行 pytest（命令层测试）"

    $pythonArgs = @()
    $pythonExe = Find-Tool "python"
    if (-not $pythonExe) {
        $pyExe = Find-Tool "py"
        if ($pyExe) { $pythonExe = $pyExe; $pythonArgs = @("-3") }
    }
    if (-not $pythonExe) {
        throw "-Pytest 需要 python，但未找到 python / py 启动器。"
    }

    # 先确认 pytest 已安装
    & $pythonExe @pythonArgs -m pytest --version *> $null
    if ($LASTEXITCODE -ne 0) {
        Write-Warn2 "未安装 pytest，跳过 pytest 测试（可执行: python -m pip install pytest）"
    } else {
        & $pythonExe @pythonArgs -m pytest (Join-Path $RepoRoot "tests\python") -v
        if ($LASTEXITCODE -ne 0) { throw "pytest 失败（exit=$LASTEXITCODE）" }
        Write-Ok "pytest 全部通过"
    }
}

# ---------------------------------------------------------------
# 6. 汇总
# ---------------------------------------------------------------
Write-Host ""
Write-Host "========== 构建完成 ==========" -ForegroundColor Green
Write-Host "  版本号 : $(if ($Version) { $Version } else { '（沿用 CMake 缓存）' })"
Write-Host "  配置   : $Config"
Write-Host "  CTest  : $(if ($UnitTests) { '已运行' } else { '跳过' })"
Write-Host "  pytest : $(if ($Pytest) { '已运行' } else { '跳过' })"
Write-Host "  Qt 部署: $qtDeployStatus"
Write-Host "  bin    : $BinDir（多配置生成器自动加 <Config> 子目录）"
Write-Host "  lib    : $LibDir（多配置生成器自动加 <Config> 子目录）"
Write-Host "  产物   : $BuildDir"

cmake_minimum_required(VERSION 3.16)

# 程序版本号：默认 0.1.0，构建时可用 -DPERCEPTION_VERSION=<版本> 覆盖
#（构建脚本 scripts/build.ps1 的 -Version 参数即透传此变量）。
set(PERCEPTION_VERSION "0.1.0" CACHE STRING "Perception program version")

project(Perception VERSION ${PERCEPTION_VERSION} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 说明：Qt 的 AUTOMOC/AUTOUIC/AUTORCC 在 GUI target（src/app、M3 后 render/ui）
# 上按需开启，避免 core/tests 无 Qt 时产生 AUTOGEN 警告。

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# MSVC：源码含 UTF-8 中文注释，统一用 /utf-8 编译，避免 C4819 警告
if(MSVC)
  add_compile_options(/utf-8)
endif()

# ===== 骨架开关 =====
# M0-M1：GUI 依赖（Qt5 / VTK）尚未接入，默认只构建 core 层 + 单测，
# 保证「克隆即构建、构建即可测」。
# M3 起打开界面：-DPERCEPTION_BUILD_GUI=ON，并配置 Qt5_DIR / VTK_DIR。
option(PERCEPTION_BUILD_GUI "构建 Qt5/VTK 界面层（src/render + src/ui + src/app，M3+ 打开）" OFF)

# ===== 子目录 =====
enable_testing()

# 数据层：无 UI 依赖，M1 起独立可测
add_subdirectory(src/core)
# 单测：CTest
add_subdirectory(tests)

if(PERCEPTION_BUILD_GUI)
  find_package(Qt5 5.12 REQUIRED COMPONENTS Widgets)
  find_package(VTK 9.4 REQUIRED
    COMPONENTS ChartsCore GUISupportQt ViewsQt RenderingQt)
  # 内嵌 Python 控制台（M5 命令层同源）：需 Python 开发库（python3X.lib / Python.h）。
  # Windows 下 Anaconda / python.org 官方安装器的 libs 目录均自带；若未找到，
  # 请安装含开发库的 Python 或指定 -DPython3_ROOT_DIR=<前缀>。
  find_package(Python3 3.8 REQUIRED COMPONENTS Interpreter Development)
  add_subdirectory(src/render)  # M3：VTK 曲线渲染
  add_subdirectory(src/ui)      # M4：Qt Widgets 界面层
  add_subdirectory(src/app)     # 可执行程序入口
endif()

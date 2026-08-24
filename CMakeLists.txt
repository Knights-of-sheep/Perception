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
# /MP12：单次 cl 调用内多核并行编译（与 MSBuild 的 /m:<N> 互不冲突，叠加生效）。
#   12 为本机物理/逻辑核数，按需调整；缺省 1 时回退到单核。
if(MSVC)
  add_compile_options(/utf-8 /MP12)
endif()

# ===== 骨架开关 =====
# M0-M1：GUI 依赖（Qt5 / VTK）尚未接入，默认只构建 core 层 + 单测，
# 保证「克隆即构建、构建即可测」。
# M3 起打开界面：-DPERCEPTION_BUILD_GUI=ON，并配置 Qt5_DIR / VTK_DIR。
option(PERCEPTION_BUILD_GUI "构建 Qt5/VTK 界面层（src/render + src/ui + src/app，M3+ 打开）" OFF)

# ===== 第三方依赖路径 =====
# 以下为各依赖库在本机的安装位置（find_package 的 *_DIR 提示变量）。
# 均声明为 CACHE 变量：命令行 -DQt5_DIR=... / -DVTK_DIR=... / -DPython3_ROOT_DIR=...
# 传入的值优先生效；未传时使用下方默认路径。
# 换机器/换路径时只需修改此处默认值即可，无需改动 find_package 逻辑。
set(Qt5_DIR "D:/Qt/5.15.2/msvc2019_64/lib/cmake/Qt5" CACHE PATH "Qt5 CMake 配置目录（含 Qt5Config.cmake，Qt5Widgets/Qt5Gui/Qt5Core）")
set(VTK_DIR "D:/vtk-941-qt/bak/lib/cmake/vtk-9.4" CACHE PATH "VTK CMake 配置目录（含 VTKConfig.cmake，vtk 9.4 + Qt 支持）")
# 内嵌 Python 控制台依赖 Python 开发库（python3X.lib / Python.h）：
# Windows 下 Anaconda / python.org 官方安装器均自带；指向 Python 安装根目录即可。
set(Python3_ROOT_DIR "D:/anaconda3" CACHE PATH "Python 安装根目录（含 include/ 与 libs/ 开发库）")

# ===== clangd 配置生成（供 IDE 静态分析使用） =====
# clangd 需知道 C++ 标准与 Qt/Python 的 include 路径才能正确解析源码；
# 缺失时会报 "QApplication file not found" 等连锁误报（非编译错误）。
# 此处从上方依赖变量推导并生成根目录 .clangd（由 .clangd.in 模板渲染），
# 保证依赖路径唯一来源：换路径只需改 Qt5_DIR / Python3_ROOT_DIR 后重新 configure。
get_filename_component(_qt_dir "${Qt5_DIR}" DIRECTORY)    # <qt>/lib/cmake
get_filename_component(_qt_dir "${_qt_dir}" DIRECTORY)    # <qt>/lib
get_filename_component(_qt_dir "${_qt_dir}" DIRECTORY)    # <qt>
set(PERCEPTION_CLANGD_QT_INCLUDE "${_qt_dir}/include")
set(PERCEPTION_CLANGD_PYTHON_INCLUDE "${Python3_ROOT_DIR}/include")
set(PERCEPTION_CLANGD_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/.clangd.in"
  "${CMAKE_CURRENT_SOURCE_DIR}/.clangd"
  @ONLY)

# ===== 子目录 =====
enable_testing()

# 数据层：无 UI 依赖，M1 起独立可测
add_subdirectory(src/core)

# ===== 编译严格性（仅 perception_core 项目代码）=====
# 仅对核心静态库启用 warning-as-error（/WX），不污染第三方（Qt/VTK 头会产生大量
# 与项目无关的 warning）。非 MSVC 用 -Wall -Wextra -Wpedantic -Werror。
# 暂不应用到 perception_ui：Qt 头在 /W4 下可能产生警告，先把 core 拉严为基线，
# UI 头回归通过后再推广。
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  target_compile_options(perception_core PRIVATE /W4 /WX)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR
       CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR
       CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
  target_compile_options(perception_core PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

# ===== 编译加速：Unity Build（perception_core）=====
# 纯 C++（无 Qt AUTOMOC / 匿名 namespace / 单例变量冲突风险），合并编译单元可显著减少
# 头解析开销，对增量构建尤其有效。CMake 3.16+ 内置支持。
set_target_properties(perception_core PROPERTIES UNITY_BUILD ON)

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
  # Anaconda / python.org 官方安装器均不提供 pythonXX_d.lib（Debug 导入库），
  # FindPython3 的 imported target（Python3::Python）在 Debug 配置会指向不存在的
  # python313_d.lib，MSVC Debug 构建报 LNK1104。
  # 方案：改用其解析到的 Release 导入库（发布版 python313.dll，路径已验证有效），
  # 存为变量供 src/ui 显式链接，Debug/Release 构建一致。
  set(PERCEPTION_PYTHON_IMPORT_LIB "${Python3_LIBRARY_RELEASE}")
  message(STATUS "Python3 import library : ${PERCEPTION_PYTHON_IMPORT_LIB}")
  # Python 的 pyconfig.h 在 _DEBUG 下会 #pragma comment(lib, "pythonXX_d.lib")，
  # 而 Anaconda 不提供该调试库 → MSVC Debug 链接报 LNK1104。
  # 据此生成库名，供可执行目标用 /NODEFAULTLIB 忽略该 pragma 引入的库。
  string(REGEX REPLACE "^([0-9]+)\\.([0-9]+).*" "\\1\\2"
    PERCEPTION_PYTHON_VER_NODOT "${Python3_VERSION}")
  set(PERCEPTION_PYTHON_DEBUG_LIB "python${PERCEPTION_PYTHON_VER_NODOT}_d.lib")
  message(STATUS "Python debug lib to ignore : ${PERCEPTION_PYTHON_DEBUG_LIB}")
  add_subdirectory(src/render)  # M3：VTK 曲线渲染
  add_subdirectory(src/ui)      # M4：Qt Widgets 界面层
  add_subdirectory(src/app)     # 可执行程序入口
endif()

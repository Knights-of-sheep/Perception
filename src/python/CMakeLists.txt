# ===== src/python：pybind11 桥接模块（006 US4，双库拆分）=====
# 产出两个独立 .pyd（一接口类 = 一动态库，Python 侧按模块 import）：
#   - perception_console：REPL 桥模块（ConsoleOut 流 / _cpp_log / 引导脚本），见 console/
#   - perception_py：命令层模块（ICommandService 骨架 + create_window 真实命令），见 command/
# 宪法技术栈：脚本层 = CPython 3.13 + pybind11（vendored：third-party/pybind11-2.13.6）。
# 本文件仅在 PERCEPTION_BUILD_GUI 时由顶层 add_subdirectory（依赖 Python 开发库与
# src/ui/console/python_debug_shim.cpp 的 MSVC Debug 兼容方案）。
#
# pybind11 为 header-only：vendored 源码树（third-party/pybind11-2.13.6）未预生成
# pybind11Config.cmake，直接以 include 路径接入，无需 find_package。
# pyd 目标也不链接 pybind11::module——其 Debug 配置会把 Python3::Module 指向
# 不存在的 python313_d.lib（Anaconda 无调试库，LNK1104，同 src/ui 注释所述），
# 改为显式链接发布版导入库 ${PERCEPTION_PYTHON_IMPORT_LIB}，Debug/Release 一致。

set(PERCEPTION_PYBIND11_INCLUDE_DIR
  "${CMAKE_SOURCE_DIR}/third-party/pybind11-2.13.6/include")

# .pyd 输出至 exe 同级目录：REPL 运行时经 QCoreApplication::applicationDirPath()
# 注入 sys.path（PythonConsole::initPython），pyd 必须与 perception.exe 同目录。
# exe 目录由 build.ps1 的 -BinDir 决定（CMAKE_RUNTIME_OUTPUT_DIRECTORY=bin，
# 多配置生成器自动追加 <Config> 子目录）；直接 cmake 构建时回退到构建根。
if(CMAKE_RUNTIME_OUTPUT_DIRECTORY)
  set(_PERCEPTION_PYD_OUT_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
else()
  set(_PERCEPTION_PYD_OUT_DIR "${CMAKE_BINARY_DIR}")
endif()

# 通用 pyd 目标配置：include 路径、Python 链接、Windows 输出为 .pyd、MSVC Debug 兼容
function(perception_configure_pyd target)
  target_include_directories(${target} PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${Python3_INCLUDE_DIRS}"
    "${PERCEPTION_PYBIND11_INCLUDE_DIR}"
  )
  target_link_libraries(${target} PRIVATE "${PERCEPTION_PYTHON_IMPORT_LIB}")
  set_target_properties(${target} PROPERTIES
    PREFIX ""
    SUFFIX ".pyd"
    LIBRARY_OUTPUT_DIRECTORY "${_PERCEPTION_PYD_OUT_DIR}"
    RUNTIME_OUTPUT_DIRECTORY "${_PERCEPTION_PYD_OUT_DIR}"
  )
  if(MSVC)
    target_compile_options(${target} PRIVATE /bigobj)
    # MSVC Debug + 发布版 Python：python_debug_shim 提供 Py_REF_DEBUG 下缺失的
    # 调试符号（pybind11 内部同样使用 Py_INCREF/Py_DECREF）；/NODEFAULTLIB 忽略
    # pyconfig.h 的 #pragma comment(lib, "pythonXX_d.lib")。方案同 src/app。
    target_sources(${target} PRIVATE
      "${CMAKE_SOURCE_DIR}/src/ui/console/python_debug_shim.cpp")
    if(DEFINED PERCEPTION_PYTHON_DEBUG_LIB)
      target_link_options(${target} PRIVATE
        "/NODEFAULTLIB:${PERCEPTION_PYTHON_DEBUG_LIB}"
        "/alternatename:__imp__Py_NegativeRefcount=_Py_NegativeRefcount"
        "/alternatename:__imp__Py_INCREF_IncRefTotal=_Py_INCREF_IncRefTotal"
        "/alternatename:__imp__Py_DECREF_DecRefTotal=_Py_DECREF_DecRefTotal"
      )
    endif()
  endif()
endfunction()

# ---- perception_console：REPL 桥模块（ConsoleOut / _cpp_log / 引导脚本）----
add_library(perception_console MODULE
  console/console_module.cpp
  console/console_out.cpp
  console/log_bridge.cpp
  console/bootstrap.cpp
)
perception_configure_pyd(perception_console)
# 回调经虚接口（IOutputSink/ILogBridge）转发，pyd 不直接依赖 core/UI

# ---- perception_py：命令层模块（ICommandService 骨架 + create_window 真实命令）----
add_library(perception_py MODULE
  command/command_module.cpp
  command/command_service.cpp
  command/registry.cpp
)
perception_configure_pyd(perception_py)

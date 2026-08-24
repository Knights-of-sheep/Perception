# ===== src/app：可执行程序入口 =====
# WIN32：标记为 GUI 应用，链接器使用 /SUBSYSTEM:WINDOWS 替代默认 CONSOLE。
# 行为差异：
#   1) PowerShell/Cmd 启动时不弹/继承控制台窗口（避免遮挡 GUI 主窗口）
#   2) stdout/stderr 无控制台载体 → 调 AllocConsole 或 OutputDebugString 才能看
#   3) 调试/回归仍可由 Allocation=console (Debug) 或 windbg/cdb attach 看 qInfo
# 副作用：app.log 文件 sink 仍记录所有日志，与控制台可见性无关。
add_executable(perception WIN32 main.cpp)

set_target_properties(perception PROPERTIES
  AUTOMOC ON
  AUTOUIC ON
  AUTORCC ON
)

target_include_directories(perception PRIVATE
  ${CMAKE_SOURCE_DIR}/src
)

# 版本号：随顶层 PROJECT_VERSION（build.ps1 -Version 可覆盖）
target_compile_definitions(perception PRIVATE
  PERCEPTION_APP_VERSION="${PROJECT_VERSION}"
)

target_link_libraries(perception PRIVATE
  Qt5::Widgets
  perception_core
  perception_ui
  # M3: perception_render（VTK 曲线渲染）
)

# Python 的 pyconfig.h 在 _DEBUG 下通过 #pragma comment(lib, "pythonXX_d.lib")
# 将调试库名嵌入 obj，而 Anaconda 不提供该库 → MSVC Debug 链接报 LNK1104。
# 用 /NODEFAULTLIB 忽略之（显式链接的发布版 python313.lib 不受影响）。
# 另外，Py_DEBUG 使 Py_INCREF/Py_DECREF 以 dllimport 引用仅存在于调试版 Python 的
# 符号（__imp__Py_INCREF_IncRefTotal 等），发布版 python313.lib 不导出 → LNK2019。
# 用 /alternatename 将这些 __imp_ 引用映射到 python_debug_shim.cpp 提供的普通实现。
if(MSVC AND WIN32 AND DEFINED PERCEPTION_PYTHON_DEBUG_LIB)
  target_link_options(perception PRIVATE
    "/NODEFAULTLIB:${PERCEPTION_PYTHON_DEBUG_LIB}"
    "/alternatename:__imp__Py_NegativeRefcount=_Py_NegativeRefcount"
    "/alternatename:__imp__Py_INCREF_IncRefTotal=_Py_INCREF_IncRefTotal"
    "/alternatename:__imp__Py_DECREF_DecRefTotal=_Py_DECREF_DecRefTotal"
  )
endif()

# ===== tests/cpp：C++ 单元测试（CTest）=====
# core 层测试（core_smoke / event_bus_test / logger_test）不依赖 UI，
# 保证无 -Gui（PERCEPTION_BUILD_GUI=OFF）时无头环境可构建可测。
# GUI 相关测试（icon_action_map / layout_manager / subwindow_view /
# window_geometry）链接 perception_ui，仅随 GUI 一起启用；否则 ui/* 头、
# theme.qrc 资源与 perception_ui 目标均不存在 → C1083 / 链接失败。
# M1 起随 PltReader 实现补充解析用例（plt_reader_test.cpp）。

# 断言保护：Release 构建默认定义 NDEBUG，会使 assert 全部失效（假绿）。
# 测试目标必须保持断言启用，确保 TDD 红/绿判断真实有效。
# 做法：在本目录（仅测试目标）作用域内，从 CMAKE_CXX_FLAGS_RELEASE 中移除
# 编译器默认的 NDEBUG 定义（MSVC: /DNDEBUG；GCC/Clang: -DNDEBUG）。
# 这样命令行中不再同时出现 /DNDEBUG 与 /UNDEBUG，避免 MSVC D9025 警告。
if(MSVC)
  string(REPLACE "/DNDEBUG" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
else()
  string(REPLACE "-DNDEBUG" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
endif()

# Python 的 pyconfig.h 在 _DEBUG 下通过 #pragma comment(lib, "pythonXX_d.lib")
# 将调试库名嵌入 obj（经 perception_ui 静态库透传），而 Anaconda 不提供该库
# → MSVC Debug 链接报 LNK1104。与 src/app 的 perception 目标一致，用 /NODEFAULTLIB
# 忽略。注：tests/ 在根 CMakeLists 的 if(PERCEPTION_BUILD_GUI) 块外配置，看不到
# PERCEPTION_PYTHON_DEBUG_LIB，这里自行 find_package Python3 生成库名。
find_package(Python3 QUIET COMPONENTS Development)
if(MSVC AND WIN32 AND Python3_FOUND)
  string(REGEX REPLACE "^([0-9]+)\\.([0-9]+).*" "\\1\\2"
    _perception_py_ver_nodot "${Python3_VERSION}")
  set(_perception_py_debug_lib "python${_perception_py_ver_nodot}_d.lib")
endif()

add_executable(core_smoke core_smoke_test.cpp)
target_link_libraries(core_smoke PRIVATE perception_core)
add_test(NAME core_smoke COMMAND core_smoke)

add_executable(event_bus_test event_bus_test.cpp)
target_link_libraries(event_bus_test PRIVATE perception_core)
add_test(NAME event_bus_test COMMAND event_bus_test)

# M4：统一日志子系统单测（先红后绿）
add_executable(logger_test logger_test.cpp)
target_link_libraries(logger_test PRIVATE perception_core)
add_test(NAME logger_test COMMAND logger_test)

# ===== GUI 相关测试（PERCEPTION_BUILD_GUI=ON 时启用）=====
# 004/005 引入的 UI 测试均链接 perception_ui（含 QSS 资源与 ui/* 头）。
# build.ps1 不带 -Gui 时 PERCEPTION_BUILD_GUI=OFF，perception_ui 目标不存在，
# 必须整体跳过，否则 C1083「无法打开包括文件 ui/xxx.h」/ 链接失败。
if(PERCEPTION_BUILD_GUI)
# 003-install-icon-bars：动作-图标映射契约单测（contracts/icon-action-map.md，V-1~V-8）。
# 例外：链接 perception_ui（含 theme.qrc 资源），用于 V-5 资源存在性断言。
add_executable(icon_action_map_test icon_action_map_test.cpp)
target_link_libraries(icon_action_map_test PRIVATE perception_ui)
add_test(NAME icon_action_map_test COMMAND icon_action_map_test)

# 004-dock-layout-manager：子窗口布局计算单测（FR-004~009 / FR-014 / FR-015；
# 仅依赖 QtCore 类型，无 GUI 平台依赖，红-绿 TDD）
add_executable(layout_manager_test layout_manager_test.cpp)
target_link_libraries(layout_manager_test PRIVATE perception_ui)
add_test(NAME layout_manager_test COMMAND layout_manager_test)

# 004-dock-layout-manager：子窗口视图交互单测（FR-022 关闭按钮、按钮等高等宽、
# 点击任意区域选中机制）。需要 GUI 平台（QApplication + QtTest），桌面会话下运行。
find_package(Qt5 QUIET COMPONENTS Test)
if(Qt5Test_FOUND)
  add_executable(subwindow_view_test subwindow_view_test.cpp)
  # 链接 perception_core：perception_ui 的 MainWindow/PythonConsole 引用 core::log 符号
  target_link_libraries(subwindow_view_test PRIVATE perception_ui perception_core Qt5::Test)
  add_test(NAME subwindow_view_test COMMAND subwindow_view_test)
  # 与 src/app 的 perception 目标一致：Py_DEBUG 下 Py_INCREF/Py_DECREF 以 dllimport
  # 引用仅存在于调试版 Python 的符号（__imp__Py_*_IncRefTotal 等），发布版
  # python313.lib 不导出 → LNK2019；用 /alternatename 映射到 python_debug_shim.cpp
  # 提供的普通实现（该 shim 已编译进 perception_ui）。
  if(DEFINED _perception_py_debug_lib)
    target_link_options(subwindow_view_test PRIVATE
      "/NODEFAULTLIB:${_perception_py_debug_lib}"
      "/alternatename:__imp__Py_NegativeRefcount=_Py_NegativeRefcount"
      "/alternatename:__imp__Py_INCREF_IncRefTotal=_Py_INCREF_IncRefTotal"
      "/alternatename:__imp__Py_DECREF_DecRefTotal=_Py_DECREF_DecRefTotal"
    )
  endif()
endif()

# 005-multi-screen-maximize：窗口最大化几何纯函数单测（FR-001~006 / SC-001~003；
# 仅依赖 QtCore 类型，无 GUI 平台依赖，红-绿 TDD）。
add_executable(window_geometry_test window_geometry_test.cpp)
target_link_libraries(window_geometry_test PRIVATE perception_ui)
add_test(NAME window_geometry_test COMMAND window_geometry_test)
if(DEFINED _perception_py_debug_lib)
  target_link_options(window_geometry_test PRIVATE "/NODEFAULTLIB:${_perception_py_debug_lib}")
endif()

# 008-unify-dialog-styling：弹窗背景层派生纯函数单测（SC-001 / FR-003；
# 仅依赖 QtGui 类型，无 GUI 平台依赖，红-绿 TDD）。
add_executable(theme_dialog_layer_test theme_dialog_layer_test.cpp)
target_link_libraries(theme_dialog_layer_test PRIVATE perception_ui)
add_test(NAME theme_dialog_layer_test COMMAND theme_dialog_layer_test)
endif()  # PERCEPTION_BUILD_GUI

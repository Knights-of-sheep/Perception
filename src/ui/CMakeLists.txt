# ===== src/ui：界面层（Qt5 Widgets + Dock + QSS 深色主题）=====
# 规范：docs/design/ui-guidelines.md；视觉稿：docs/design/mockups/。
add_library(perception_ui STATIC
  MainWindow.cpp
  action_icon_map.cpp
  console/PythonConsole.cpp
  log/qt_message_bridge.cpp
  theme/icon_factory.cpp
  theme/theme_manager.cpp
  theme/theme.qrc
)
# MSVC Debug + 发布版 Python（Anaconda）链接兼容层：提供 Py_REF_DEBUG 下引用的
# 调试版专用符号（_Py_INCREF_IncRefTotal / _Py_DECREF_DecRefTotal /
# _Py_NegativeRefcount），详见 python_debug_shim.cpp 注释。Release/非 MSVC 下为空。
if(MSVC)
  target_sources(perception_ui PRIVATE console/python_debug_shim.cpp)
endif()

target_include_directories(perception_ui PUBLIC
  ${CMAKE_SOURCE_DIR}/src
)
# Python.h 只在 console/PythonConsole.cpp 内使用（pimpl 隔离），不污染公共头
target_include_directories(perception_ui PRIVATE
  "${Python3_ROOT_DIR}/include"
)

target_link_libraries(perception_ui PUBLIC
  Qt5::Widgets
  # 内嵌 Python：静态库以 LINK_ONLY 透传至最终可执行文件。
  # 不用 Python3::Python（其 Debug 配置指向不存在的 pythonXX_d.lib，MSVC Debug
  # 链接报 LNK1104）；改显式链接 FindPython3 解析到的发布版导入库，Debug/Release 一致。
  PRIVATE "${PERCEPTION_PYTHON_IMPORT_LIB}"
)

# Python.h（pyconfig.h）在 _DEBUG 下会 #define Py_DEBUG，使 Py_INCREF/Py_DECREF 引用仅
# 存在于调试版 Python 库的符号（_Py_INCREF_IncRefTotal 等），Anaconda 无 debug 库 → MSVC
# Debug 链接报 LNK2019。已通过 python_debug_shim.cpp 提供兼容符号解决。
# 注意不可用 /U_DEBUG 或源码 #undef _DEBUG：/MDd 会自动重定义，且会触发 MSVC STL 的
# RuntimeLibrary/_ITERATOR_DEBUG_LEVEL 一致性检查（LNK2038）。

set_target_properties(perception_ui PROPERTIES
  AUTOMOC ON
  AUTOUIC ON
  AUTORCC ON
)

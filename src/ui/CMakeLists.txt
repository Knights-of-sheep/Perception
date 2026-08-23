# ===== src/ui：界面层（Qt5 Widgets + Dock + QSS 深色主题）=====
# 规范：docs/design/ui-guidelines.md；视觉稿：docs/design/mockups/。
add_library(perception_ui STATIC
  MainWindow.cpp
  console/PythonConsole.cpp
  theme/theme_manager.cpp
  theme/theme.qrc
)

target_include_directories(perception_ui PUBLIC
  ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(perception_ui PUBLIC
  Qt5::Widgets
  # 内嵌 Python：静态库以 LINK_ONLY 透传至最终可执行文件；
  # Python.h 只在 console/PythonConsole.cpp 内使用（pimpl 隔离），不污染公共头。
  PRIVATE Python3::Python
)

set_target_properties(perception_ui PROPERTIES
  AUTOMOC ON
  AUTOUIC ON
  AUTORCC ON
)

# ===== src/app：可执行程序入口 =====
# 当前编译 M0 占位 main.cpp；M4 端到端 MVP 时改为加载真实主窗口。
add_executable(perception main.cpp)

set_target_properties(perception PROPERTIES
  AUTOMOC ON
  AUTOUIC ON
  AUTORCC ON
)

target_include_directories(perception PRIVATE
  ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(perception PRIVATE
  Qt5::Widgets
  perception_core
  # M4: perception_ui perception_render
)

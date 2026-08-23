# ===== src/app：可执行程序入口 =====
add_executable(perception main.cpp)

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

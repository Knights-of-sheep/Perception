# ===== src/core：数据层 =====
# 无 UI/渲染依赖，M1 起可独立构建与单测。
# 职责：.plt 解析（PltParser）、数据模型（IDataSet / Curve）。
add_library(perception_core STATIC
  dataset.cpp
  plt_parser.cpp
)

target_include_directories(perception_core PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

# ===== src/core：数据核心层（无 UI/渲染依赖，可独立单测）=====
# 四层，职责分离：
#   model/   格式无关数据模型（曲线 + 结构）
#   io/      格式读取器 + 注册表（新格式 = 新 reader，不修改核心）
#   process/ 数据变换（管道，可组合）
#   event/   事件总线（发布-订阅，驱动渲染 / UI 更新）
add_library(perception_core STATIC
  model/dataset.cpp
  model/structure.cpp
  io/reader.cpp
  io/plt_reader.cpp
  io/csv_reader.cpp
  process/transform.cpp
  event/event_bus.cpp
  log/log_level.cpp
  log/log_level_matrix.cpp
  log/logger.cpp
  log/file_sink.cpp
  log/terminal_sink.cpp
)

# 统一 include 前缀：core/model/... core/io/... 等
target_include_directories(perception_core PUBLIC
  ${CMAKE_SOURCE_DIR}/src
)

# ===== tests/cpp：C++ 单元测试（CTest）=====
# 全部测试不依赖 UI（core 层），保证无头环境可运行。
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

# 003-install-icon-bars：动作-图标映射契约单测（contracts/icon-action-map.md，V-1~V-8）。
# 例外：链接 perception_ui（含 theme.qrc 资源），用于 V-5 资源存在性断言。
add_executable(icon_action_map_test icon_action_map_test.cpp)
target_link_libraries(icon_action_map_test PRIVATE perception_ui)
add_test(NAME icon_action_map_test COMMAND icon_action_map_test)

# ===== tests/cpp：C++ 单元测试（CTest）=====
# 全部测试不依赖 UI（core 层），保证无头环境可运行。
# M1 起随 PltReader 实现补充解析用例（plt_reader_test.cpp）。

add_executable(core_smoke core_smoke_test.cpp)
target_link_libraries(core_smoke PRIVATE perception_core)
add_test(NAME core_smoke COMMAND core_smoke)

add_executable(event_bus_test event_bus_test.cpp)
target_link_libraries(event_bus_test PRIVATE perception_core)
add_test(NAME event_bus_test COMMAND event_bus_test)

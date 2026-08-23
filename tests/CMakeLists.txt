# ===== tests：CTest 单测 =====
# 全部测试不依赖 UI（core 层），保证无头环境可运行。
# M1 起随 PltParser 实现补充解析用例。
add_executable(core_smoke core_smoke_test.cpp)
target_link_libraries(core_smoke PRIVATE perception_core)
add_test(NAME core_smoke COMMAND core_smoke)

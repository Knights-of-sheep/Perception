# ===== tests =====
# cpp/    C++ 单元测试（CTest，core 层，无 UI 依赖，无头可跑）
# python/ pytest（命令层，M5 起依赖 perception_py，见 tests/python/README.md）
add_subdirectory(cpp)

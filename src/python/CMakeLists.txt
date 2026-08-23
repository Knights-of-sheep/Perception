# ===== src/python：pybind 模块（M5 启用）=====
# 由顶层 CMakeLists 在对应里程碑时 add_subdirectory。
# 职责：导出 perception_py 模块：load_plt() / plot() / export()。
# 依赖（vendored，见 third-party/）：
#   - pybind11-2.13.6/   （-Dpybind11_DIR=... 指向源码树）
#   - python3.13.3/      （嵌入式 Python）
# 预期 target：perception_py（MODULE，链接 pybind11::module + perception_core）。

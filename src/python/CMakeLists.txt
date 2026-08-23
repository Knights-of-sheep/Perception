# ===== src/python：命令驱动层（M5 启用）=====
# 由顶层 CMakeLists 在对应里程碑时 add_subdirectory。
# 职责：数据 CRUD 的唯一入口（constitution v1.1）：
#   load_plt(path) / transform(ds, name, **kw) / query(ds, ...) / export(ds, path)
# 数据变更后发布事件，render/ui 订阅刷新；纯 UI 变化不走本层。
# 依赖（vendored，见 third-party/）：
#   - pybind11-2.13.6/   （-Dpybind11_DIR=... 指向源码树）
#   - python3.13.3/      （嵌入式 Python）
# 预期 target：perception_py（MODULE，链接 pybind11::module + perception_core）。
# 测试布局：tests/python/（pytest）。

# tests/python — pytest（命令层测试）

目的：测试 **Python 命令驱动层**（`perception_py`），覆盖 加载 → 变换 → 查询 → 导出 主链路。

前置：M5 构建 `perception_py`（pybind11 + 嵌入式 Python，见 `src/python/`）。

运行：

```powershell
python -m pytest tests/python -v
```

接入 CTest（M5 后，可选）：

```cmake
add_test(NAME pytest COMMAND python -m pytest ${CMAKE_SOURCE_DIR}/tests/python -v)
```

## 布局约定

- `conftest.py` — fixture（`perception_py` 模块加载、`sample_plt` 临时数据文件）
- `test_command_layer.py` — 命令主链路（load → transform → query → export）
- M5 后按功能拆分：`test_transform.py` / `test_query.py` / `test_export.py`

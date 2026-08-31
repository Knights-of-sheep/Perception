# Contract: supported_formats（Python 命令层 · 获取能力）

> 契约对象：`perception_py` 模块的 `CommandService.supported_formats()`。
> 依据 spec：FR-005（运行时可查询）、FR-003/FR-004（条目字段与状态语义）、FR-010（大小写不敏感）。

## 签名

```python
CommandService.supported_formats() -> list[dict]
```

## 返回结构

每个条目 dict 字段：

| 字段 | 类型 | 值域 / 说明 |
|---|---|---|
| `extensions` | `list[str]` | 小写、含点（如 `[".plt"]`；多扩展名 `[".h5", ".hdf5"]`） |
| `format_name` | `str` | 格式名称（如 `"SVisual 曲线数据"`） |
| `family` | `str` | `vtk-legacy` / `vtk-xml` / `vtk-composite` / `vtk-parallel` / `svisual` / `hdf5` / `curve` |
| `kind` | `str` | `curve` / `structure` / `both` |
| `status` | `str` | `supported` / `planned` |

## 语义

- 返回**完整清单**（含 `supported` + `planned`），与权威目录逐条一致（FR-005）。
- 顺序稳定：与目录数据表顺序一致（不要求排序语义，但同一进程内多次调用结果一致）。
- 只读查询，无副作用；不依赖宿主窗口 / IWindowFactory 是否注册。
- `extensions` 全部小写；调用方不应假设存在大写形式（FR-010）。

## 测试断言（pytest，`tests/python/test_supported_formats.py`）

1. 返回 `list`，非空，条目数 ≥ 12（覆盖 4 格式族）。
2. 每条目含且仅含 5 个字段，字段值非空且符合值域。
3. 扩展名全局无重复（大小写归一后）。
4. 至少存在 1 条 `status == "supported"` 且 `extensions` 与当前可打开格式对应（.plt/.csv）；`planned` 条目存在且不混入 supported 语义。
5. 连续两次调用返回一致（稳定性）。

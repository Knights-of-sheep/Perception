# 契约: `create_window` Python 命令

> 对应 spec FR-001/FR-002/FR-027、US1、SC-018；绑定位于 pybind11 命令层模块 `perception_py`，由 `PythonConsole::initPython` import 该模块后取 `create_window` 注入 REPL 全局命名空间（006 US4 桥接迁移后机制，原 `PyMethodDef` + `PyCFunction_New` 手写 C API 已废弃）。

## 签名

```python
create_window(title: str = "") -> str
```

返回新建子窗口的 **id**（字符串）。id 为 `"Plot_"` + 全局递增序号（`Plot_1`、`Plot_2`、…），序号全局递增不回退（删除子窗口不影响后续序号）。

## 行为

- 创建新渲染子窗口，加入中央区域并立即按当前布局排列；返回生成的窗口 id（FR-001）。
- `title` 缺省（`create_window()`）或为空白时，窗口标题 = id；传入 `title` 时窗口标题显示 `title`。
- 参数类型错误（`title` 非 str）抛 `TypeError`，由 REPL 展示 traceback，不使 REPL 崩溃。
- 创建失败（宿主未连接/回调异常）返回 `None`，不使 REPL 崩溃。
- 不提供 Python 版本的布局管理接口（spec Clarifications 第 3 条：布局管理仅 UI 操作）。
- 命令文本回显：无论经 PyShell 手工输入还是菜单栏触发，所执行的 `create_window` 命令文本 MUST 回显到 PyShell 输出区（FR-027）。
- 返回值打印：`create_window` 的返回值（窗口 id 字符串）MUST 打印到 PyShell 输出区（FR-027）。

## 参数校验

| 条件 | 行为 |
|------|------|
| 缺省参数（`create_window()`） | title = id（`Plot_N`），正常创建并返回 id |
| `title` 非 str | 抛 `TypeError`（REPL 展示 traceback） |
| `title` 为空串 / 纯空白 | title = id（同缺省处理），正常创建并返回 id |
| `title` 与既有窗口重复 | 允许创建（id 始终唯一递增，标题可重复） |
| 宿主未连接 / 任意执行异常 | 返回 `None`，不使 REPL 崩溃 |

## 注入与桥接

- 绑定归属：`create_window` 为 `perception_py` 模块的 pybind11 模块函数（经 `py::module_::def` 导出）；`PythonConsole::initPython()` 中 `py::module_::import("perception_py")` 后取该函数注入 `globals`（与 `_cpp_log` 同批注入）。
- 派发链路：`create_window` 为 `perception_py` 模块函数，经 `IWindowFactory` 纯虚接口派发——`PythonConsole` 的 `WindowFactoryAdapter` 实现该接口并在 `initPython()` 时经 `py::capsule` 注册至模块注册表（pyd 不链接 UI 静态库）；`createWindow` 调 `createWindowCallback`（`MainWindow` 注册）→ `MainWindow::createSubwindow(title)`（UI 线程直接调用），返回生成的窗口 id；`create_window` 将其作为返回值回传 REPL。
- 菜单栏"视图 → 新建子窗口"MUST 触发同一命令行执行（构造并执行无参 `create_window()` 命令，禁止绕过命令层直接调用 C++ 入口），命令文本在 PyShell 回显、返回值（窗口 id）打印到 PyShell（FR-002、FR-027、SC-008、SC-018）。

## 测试要点

- 单元层面：桥接回调返回 id 逻辑（id 格式 `Plot_N`、序号递增、title 缺省/空白默认 = id）。
- GUI 验证（quickstart 3.1）：`create_window("曲线图")` 返回 `'Plot_N'` 并创建标题为"曲线图"的子窗口；`create_window()` 无参创建标题 = id；菜单创建行为一致。
- 回显与返回值验证（FR-027 / SC-018）：菜单栏创建子窗口后，PyShell 输出区回显 `create_window` 命令文本并打印返回值（窗口 id 字符串）。

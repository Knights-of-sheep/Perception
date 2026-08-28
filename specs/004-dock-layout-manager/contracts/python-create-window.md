# 契约: `create_window` Python 命令

> 对应 spec FR-001/FR-002/FR-027、US1、SC-018；经 `PythonConsole` 注入 REPL 全局命名空间（复用 `_cpp_log` 的 `PyMethodDef` + `PyCFunction_New` 模式，`PythonConsole.cpp:115-134`）。

## 签名

```python
create_window(name: str) -> bool
```

## 行为

- 创建标题为 `name` 的新渲染子窗口，加入中央区域并立即按当前布局排列。
- 创建成功返回 `True`；失败返回 `False` 并经日志通道输出错误信息。
- 不提供 Python 版本的布局管理接口（spec Clarifications 第 3 条：布局管理仅 UI 操作）。
- 命令文本回显：无论经 PyShell 手工输入还是菜单栏触发，所执行的 `create_window` 命令文本 MUST 回显到 PyShell 输出区（FR-027）。
- 返回值打印：`create_window` 的返回值（True/False）MUST 打印到 PyShell 输出区（FR-027）。

## 参数校验

| 条件 | 行为 |
|------|------|
| `name` 非 str | 返回 `False`，输出类型错误提示 |
| `name` 为空串 / 纯空白 | 返回 `False`，提示"窗口名不能为空" |
| `name` 与既有窗口重复 | 允许创建（以内部唯一 id 区分），spec 未设唯一约束 |
| 任意执行异常 | 捕获并返回 `False`，不使 REPL 崩溃（Edge Cases） |

## 注入与桥接

- `PythonConsole` 初始化时注册全局函数 `create_window`（与 `_cpp_log` 同批注入 `d_->globals`）。
- 回调经 `std::function` 桥接至 `MainWindow::createSubwindow(title)`，UI 线程安全调度（经信号/队列连接）。
- 菜单栏"视图 → 新建子窗口"MUST 触发同一命令行执行（构造并执行 `create_window(...)` 命令，禁止绕过命令层直接调用 C++ 入口），命令文本在 PyShell 回显、返回值打印到 PyShell（FR-002、FR-027、SC-008、SC-018）。

## 测试要点

- 单元层面：桥接回调参数校验逻辑（空名/类型错误）。
- GUI 验证（quickstart 3.1）：`create_window("曲线图")` 与菜单创建行为一致。
- 回显与返回值验证（FR-027 / SC-018）：菜单栏创建子窗口后，PyShell 输出区回显 `create_window` 命令文本并打印返回值（True/False）。

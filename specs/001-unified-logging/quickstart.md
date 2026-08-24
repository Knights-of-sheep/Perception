# Quickstart: 日志统一管理模块 — 验证指南

**Branch**: `001-unified-logging` | **Date**: 2026-08-24 | **Spec**: [spec.md](spec.md)

本指南提供**可运行的端到端验证场景**，证明日志功能按 spec 工作。实现代码与细节见 [tasks.md](tasks.md)（Phase 2）与 [contracts/](contracts/)。本文件不包含实现代码。

## 前置条件

- Windows + MSVC；构建脚本 `scripts/build.ps1`（Qt5/VTK/Python 已按现有流程配置，`-Gui` 参数启用界面层）
- 无新增第三方依赖（core 日志子系统为 C++17 标准库）

## 构建

```powershell
# 启用 GUI 层（含 PythonConsole / 日志接入）
.\scripts\build.ps1 -Gui
```

## 验证场景

### V1: 统一 API → 终端 + 文件（FR-001/003/006，SC-001）

**步骤**：
1. 在终端（PowerShell/Cmd）启动应用（`.\bin\Release\perception.exe`），在 PythonConsole 输入并执行：
   ```python
   import logging
   logging.info("hello from python")
   logging.warning("warn: divisor zero")
   ```
2. 观察启动终端（即"控制台"，GUI 内不内置日志面板）。

**预期**：
- 启动终端（控制台）出现两条记录（完整格式 `YYYY-MM-DD HH:MM:SS.mmm INFO  [__main__:N] hello from python`，WARN 黄色、ERROR 红色着色）
- 文件 `%APPDATA%/Perception/logs/app.log` 出现同两条记录（完整格式，UTF-8）

**判断**：终端与文件内容一致、字段齐全 → 通过。

### V2: C++ 侧日志调用（FR-001/003）

**步骤**：在 `src/app/main.cpp` 启动路径临时调用一次 `PERCEPTION_LOG_I("app started")`（或从控制台命令间接触发任意 C++ 路径），重启应用。

**预期**：日志文件首行附近出现 `INFO  [main.cpp:NN] app started`。

**判断**：来源 `file:line` 正确 → 通过。

### V3: 级别开关矩阵（FR-002）

**步骤**：
```python
import logging
logging.debug("should be hidden")
logging.warning("visible warning")
```

**预期**（默认全局矩阵：DEBUG 关、INFO/WARN/ERROR/FATAL 开）：
- 终端**不**显示 DEBUG 行、显示 WARN 行
- 文件**不**写入 DEBUG 行、写入 WARN 行

**判断**：DEBUG 被全局过滤、其余级别三方正常 → 通过。

### V4: 文件轮转 5MB×3（FR-005，SC-002）

**步骤**：
1. 临时将 `Logger::configure` 的 `maxFileSize` 调小（如 1KB），`maxBackups` 保持 3
2. 循环写日志直到触发多次轮转（可从控制台执行 Python `for i in range(5000): logging.warning("x"*80)`）
3. 检查 `%APPDATA%/Perception/logs/`

**预期**：
- 存在 `app.log`、`app.log.1`、`app.log.2`、`app.log.3`
- 继续写入后 `app.log.3` 被删除，归档恒为 3 份；文件总数 ≤ 4
- 归档为整文件轮转（无截断半行）

**判断**：归档数量稳定、无内容丢失 → 通过。

### V5: 并发写入线程安全（FR-007，SC-003）

**步骤**：运行 CTest 用例 `logger_test`（含 8 线程 × 1000 条写入断言）。

```powershell
ctest -R logger_test -V
```

**预期**：
- 测试输出 `passed`；文件记录总条数 = 8000
- 抽查无交错行（每行前缀格式完整匹配）

**判断**：无交错、无丢失 → 通过。

### V6: Python 日志汇入统一流（FR-008，SC-004）

**步骤**：
```python
import logging
logging.error("中文消息测试 你好")
```

**预期**：
- 终端红色显示该行
- 文件出现 `ERROR [__main__:N] 中文消息测试 你好`，UTF-8 无乱码
- 与相邻 C++ 日志按时间顺序排列

**判断**：同文件同格式、时间有序、中文正确 → 通过。

### V7: 写入失败降级（FR-009，SC-005）

**步骤**：
1. 将日志文件所在目录改为只读（或删除目录后设为不可创建）
2. 启动应用并写日志

**预期**：
- 应用正常运行、不崩溃
- 终端出现**一次**文件写入失败告警，不重复刷屏
- 其余日志仍正常输出到终端

**判断**：不崩溃、单次告警、终端可用 → 通过。

### V8: 菜单栏设置日志级别并持久化（FR-002/012/013，SC-006）

**步骤**：
1. 启动应用，打开 `设置 → 日志级别`，勾选 `DEBUG`
2. 在 PythonConsole 执行：
   ```python
   import logging
   logging.debug("console debug visible")
   ```
3. 观察终端与文件；重启应用后再看 DEBUG 勾选状态

**预期**：
- 勾选后终端、文件**同时**开始输出 DEBUG 行（无需重启，全局矩阵一致）
- 取消勾选后三方同时停止输出 DEBUG
- 重启应用后 `DEBUG` 保持勾选（QSettings 持久化）

**判断**：立即生效、全局一致、重启保持 → 通过。

### V9: Qt 输出重定向（FR-010）

**步骤**：启动应用后在任意 C++ 路径触发一次 `qWarning() << "qt-side warning"`（或通过 UI 操作间接触发 Qt 警告）。

**预期**：
- 终端出现 `WARN  [qt:<file>:<line>] qt-side warning`（格式与统一 API 一致）
- 文件同样写入该行；全局矩阵 WARN 关时终端/文件均不出现

**判断**：Qt 输出进入统一流、遵守格式与全局级别开关 → 通过。

### V10: VTK 日志开关配置（FR-011；VTK 未引入，拦截桥接随后续落地）

**步骤**：
1. 验证 `Logger::Config.vtkLoggingEnabled` 默认值为 `true`
2. 菜单栏 `设置 → 日志级别 → VTK 日志拦截` 复选项存在且默认勾选；切换后立即生效并写入 QSettings，重启保持
3. （后续 VTK 引入后，随 `src/render/vtk_log_bridge` 落地）默认勾选时 VTK 警告/错误出现在终端与文件（`source = "vtk:..."`）；取消勾选后不再进入统一流

**预期**：开关默认启用、持久化；拦截行为在 VTK 引入后补充验证

**判断**：配置开关与持久化生效 → 通过（拦截行为留待 VTK 引入后验证）。

### V11: 日志路径对用户可见并可一键直达（FR-014，SC-007）

**步骤**：
1. 启动应用，打开 `设置(S)` 菜单
2. 观察菜单中的"日志文件：…"条目与"打开日志目录(&O)"动作
3. 点击"打开日志目录"

**预期**：
- 菜单展示当前日志文件完整路径：`%APPDATA%\Perception\logs\app.log`（只读文本，可选中复制）
- 点击"打开日志目录"后系统文件管理器打开 `logs` 目录，可见 `app.log` 及归档文件
- 路径展示与 Logger 实际写入路径一致（同一来源）

**判断**：路径可见、一键直达、与实际写入一致 → 通过。

### V12: 日志与 Python REPL 分离（FR-006 修订）

**步骤**：
1. 启动应用（GUI 内不内置日志面板），在 PythonConsole 执行 `print("hello")` 与 `import logging; logging.warning("warn x")`
2. 观察 PythonConsole（REPL）输出、启动终端与 `app.log`

**预期**：
- PythonConsole（REPL）**不**出现统一日志流内容（C++ 启动日志 `INFO [main.cpp:NN] app started`、Python logging 的 `WARN [__main__:N] ...`），仅显示 Python 交互输出（`hello`、异常 traceback 等）
- 启动终端与 `app.log` 实时出现上述统一日志流记录（按级别着色，WARN 黄色、ERROR 红色）
- 统一日志流持续写入文件，不影响 REPL 交互

**判断**：py shell 与统一日志流职责分离 → 通过。

### V13: 文件对话框默认目录跟随启动路径（FR-015，SC-008）

**步骤**：
1. 在数据目录（如 `E:\data`）打开 PowerShell，执行 `& 'E:\spec-work\Perception\bin\Release\perception.exe' --snapshot _shot.png`（相对路径）
2. 检查 `_shot.png` 生成位置
3. 启动应用后打开"打开数据文件"、"导出窗口图片"、"导出 Python 命令"，观察弹窗初始目录

**预期**：
- `--snapshot` 的相对路径文件名解析到 **启动时所在目录**（`E:\data\_shot.png`），而非 exe 目录或家目录 → 证明进程 cwd 保持启动路径未被强制改写
- 三个文件/导出弹窗的初始目录均为启动时所在目录（`E:\data`）

**判断**：弹窗与相对路径均跟随启动路径 → 通过。

### V14: 设置日志路径并自动迁移（FR-016，SC-009）

**步骤**：
1. 正常启动应用（默认日志位于 `%APPDATA%\Perception\logs\app.log`），确认已产生日志文件
2. `设置 → 设置日志路径...`，选择新目录（如 `E:\data\mylogs`）并确认
3. 检查新目录与旧目录的文件
4. 重启应用，观察日志写入位置

**预期**：
- 新目录自动创建，`app.log` 及轮转归档（如有）完整迁移到新目录；旧目录不再残留日志文件
- 状态栏提示"日志路径已切换并迁移历史日志"
- `设置` 菜单路径展示更新为新路径
- 重启后日志仍写入新路径（持久化生效）

**判断**：路径切换 + 迁移 + 持久化均生效 → 通过。

### V15: 清除历史日志（FR-017，SC-010）

**步骤**：
1. 确认日志目录存在 `app.log`（及归档）
2. `设置 → 清除历史日志`，在确认对话框中点击"确定"
3. 打开日志目录检查文件

**预期**：
- 确认对话框出现；点击"取消"时不做任何删除
- 确定后日志目录中不再存在任何日志文件与归档
- 应用继续运行，新日志正常写入（`app.log` 重建）
- 状态栏提示"已清除历史日志"

**判断**：确认流程 + 全部清除 + 后续写入正常 → 通过。

## 参考

- C++ 调用方式与 API 语义：[contracts/cpp-logger-api.md](contracts/cpp-logger-api.md)
- 文件行格式与轮转规则：[contracts/log-file-format.md](contracts/log-file-format.md)
- Python 桥接约定：[contracts/python-log-bridge.md](contracts/python-log-bridge.md)
- 数据契约：[data-model.md](data-model.md)

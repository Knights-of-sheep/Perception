# Quickstart: 日志统一管理模块 — 验证指南

**Branch**: `001-unified-logging` | **Date**: 2026-08-23 | **Spec**: [spec.md](spec.md)

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

### V1: 统一 API → 控制台 + 文件（FR-001/003/006，SC-001）

**步骤**：
1. 启动应用，在 PythonConsole 输入并执行：
   ```python
   import logging
   logging.info("hello from python")
   logging.warning("warn: divisor zero")
   ```
2. 观察开发控制台输出。

**预期**：
- 控制台出现两行，格式为 `HH:MM:SS.mmm INFO  [__main__:N] hello from python` 与对应 WARN 行（含主题色：WARN 用警示色、ERROR 用红色，与现有错误样式一致）
- 文件 `%APPDATA%/Perception/logs/app.log` 出现同两条记录（完整格式 `YYYY-MM-DD HH:MM:SS.mmm LEVEL [source] message`，UTF-8）

**判断**：控制台与文件内容一致、字段齐全 → 通过。

### V2: C++ 侧日志调用（FR-001/003）

**步骤**：在 `src/app/main.cpp` 启动路径临时调用一次 `PERCEPTION_LOG_I("app started")`（或从控制台命令间接触发任意 C++ 路径），重启应用。

**预期**：日志文件首行附近出现 `INFO  [main.cpp:NN] app started`。

**判断**：来源 `file:line` 正确 → 通过。

### V3: 级别阈值（FR-002）

**步骤**：
```python
import logging
logging.debug("should be hidden")
```

**预期**（默认 INFO 阈值）：
- 控制台**不**显示该行
- 文件**不**写入该行

**判断**：DEBUG 被过滤、其余级别正常 → 通过。

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
- 控制台红色显示该行
- 文件出现 `ERROR [__main__:N] 中文消息测试 你好`，UTF-8 无乱码
- 与相邻 C++ 日志按时间顺序排列

**判断**：同文件同格式、时间有序、中文正确 → 通过。

### V7: 写入失败降级（FR-009，SC-005）

**步骤**：
1. 将日志文件所在目录改为只读（或删除目录后设为不可创建）
2. 启动应用并写日志

**预期**：
- 应用正常运行、不崩溃
- 控制台出现**一次**文件写入失败告警，不重复刷屏
- 其余日志仍正常显示在控制台

**判断**：不崩溃、单次告警、控制台可用 → 通过。

## 参考

- C++ 调用方式与 API 语义：[contracts/cpp-logger-api.md](contracts/cpp-logger-api.md)
- 文件行格式与轮转规则：[contracts/log-file-format.md](contracts/log-file-format.md)
- Python 桥接约定：[contracts/python-log-bridge.md](contracts/python-log-bridge.md)
- 数据契约：[data-model.md](data-model.md)

# Contract: Python 日志桥接

**Branch**: `001-unified-logging` | **Date**: 2026-08-23 | **Spec**: [spec.md](../spec.md) | **Research**: [research.md](../research.md) R4

## 目标

Python 侧 `logging` 模块输出与 C++ 日志汇入同一日志流（FR-008）：同一文件、同一级别与格式策略。桥接内嵌于 `PythonConsole` 的引导脚本（当前进程内 Python 运行时所在处）。

## 桥接架构

```text
Python logging (root logger)
        │  PerceptionLogHandler.emit()
        ▼
_cpp_log(level, source, message)   ← C++ 注入的 PyCFunction
        ▼
perception::core::log::Logger::instance().log(level, "<py:name:lineno>", message)
        ▼
FileSink + ConsoleSink（统一格式、统一级别阈值）
```

## C++ 侧：注入 `_cpp_log`

- 位置：`PythonConsole::initPython()`（`PyRun_String(kBootstrap, ...)` 之前注入到 globals）
- 签名：`_cpp_log(level: int, source: str, message: str) -> None`
  - `level`：C++ `LogLevel` 的整数值（0=Debug, 1=Info, 2=Warn, 3=Error, 4=Fatal）
  - `source`：来源标识字符串（handler 传入，格式 `name:lineno`）
  - `message`：消息正文（UTF-8，Python `str`）
- 行为：调用 `Logger::instance().log(static_cast<LogLevel>(level), source, msg)`；无异常逃逸（错误在内部捕获并 `PyErr_Clear`）

## Python 侧：`PerceptionLogHandler`

引导脚本（`kBootstrap`）追加：

```python
import logging

_LEVEL_MAP = {10: 0, 20: 1, 30: 2, 40: 3, 50: 4}   # Python → C++ LogLevel

class PerceptionLogHandler(logging.Handler):
    def emit(self, record):
        try:
            level = _LEVEL_MAP.get(record.levelno, 1)
            source = "%s:%d" % (record.name, record.lineno)
            _cpp_log(level, source, record.getMessage())
        except Exception:
            # 桥接失败不影响 Python 执行
            self.handleError(record)

# 挂到 root logger：级别放最低，真实过滤由 C++ Logger 统一执行
logging.getLogger().addHandler(PerceptionLogHandler())
logging.getLogger().setLevel(logging.DEBUG)
```

## 语义约定

1. **来源标识**：`record.name:lineno`（如 `__main__:12`）。不携带路径前缀，避免噪音；需要定位时结合消息上下文。
2. **级别策略**：root logger 设为 `DEBUG`，Python 侧不做阈值过滤；C++ `Logger` 阈值统一生效（"单一阈值"，spec FR-002）。因此 Python `logging.DEBUG` 在 C++ 默认 INFO 阈值下会被过滤——行为与 C++ `debug()` 一致。
3. **消息转义**：`record.getMessage()` 后的字符串按 C++ 侧转义规则（`\n`→字面量）进入日志行，保证单条=单行。
4. **异常安全**：handler 失败仅 `handleError`，不影响 Python 调用方与 REPL。
5. **不拦截** `print()`/`sys.stderr`：二者仍走既有 `ConsoleOutObject` 重定向（仅控制台，不进文件）——与 `logging` 的职责分离保持不变。
6. **一次性安装**：`initPython()` 每次启动执行一次；`clearConsole()` 不清 Python 命名空间，handler 保持注册。

## 验证要点

- Python `logging.warning("...")` → 文件与控制台均出现 `WARN [__main__:N] ...`（阈值允许时）
- Python `logging.debug(...)` 在默认 INFO 阈值下：控制台不显示，文件不写入（与 C++ 行为一致）
- Python 中文消息：UTF-8 正确落盘无乱码（SC-004）

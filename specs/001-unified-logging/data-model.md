# Data Model: 日志统一管理模块

**Branch**: `001-unified-logging` | **Date**: 2026-08-23 | **Spec**: [spec.md](spec.md)

## 概述

日志子系统的数据面由四条实体构成：`LogRecord`（单条日志记录）、`LogLevel`（级别值域）、`LogLevelMatrix`（级别开关集合）、`LogSink`（输出目标抽象）。前四者定义于 `src/core/log/`，不持有任何 UI/Python 类型；UI 与 Python 桥接通过 `LogSink` 与回调函数实现跨层传递。

```text
                        ┌────────────────────┐
                        │      Logger        │  单例：广播（按各 sink 矩阵过滤）
                        └─────────┬──────────┘
                                  │ emits LogRecord
              ┌───────────────────┼───────────────────┐
              ▼  isEnabled?       ▼  isEnabled?       ▼
        ┌──────────┐      ┌──────────────┐      ┌──────────────┐
        │ FileSink │      │ ConsoleSink  │      │ (future)     │
        │ (core)   │      │ (ui/log)     │      │ other sinks  │
        │ matrix_  │      │ matrix_      │      └──────────────┘
        └────┬─────┘      └──────┬───────┘
             ▼                   ▼
      app.log.1..3 轮转    PythonConsole 控件
```

## Entities

### LogLevel

日志级别值域，枚举 + 字符串映射。

| 字段 | 类型 | 约束/说明 |
|------|------|----------|
| 枚举值 | `enum class LogLevel : int` | `Debug=0, Info, Warn, Error, Fatal` |
| 字符串形式 | `const char*` | `"DEBUG"`, `"INFO"`, `"WARN"`, `"ERROR"`, `"FATAL"`（文件与控制台统一大写） |

**验证规则**（FR-002）：
- 每级别独立开关（无全局阈值）；过滤按各 sink 的 `LogLevelMatrix` 执行，控制台与文件可分别配置
- 默认矩阵：DEBUG 关，INFO/WARN/ERROR/FATAL 开
- 字符串→枚举映射对大小写不敏感（配置解析友好）；未知字符串回退 `Info` 并告警

**Python 级别映射**（FR-008，详见 contracts/python-log-bridge.md）：

| Python logging | LogLevel |
|---|---|
| DEBUG (10) | Debug |
| INFO (20) | Info |
| WARNING (30) | Warn |
| ERROR (40) | Error |
| CRITICAL (50) | Fatal |

### LogLevelMatrix

某输出目标的级别开关集合（spec Key Entity：每级别一个布尔位）。

| 字段 | 类型 | 约束/说明 |
|------|------|----------|
| 开关位 | `bool[5]`（内部实现可为位掩码） | 索引 = `static_cast<int>(LogLevel)`；默认 DEBUG 关、INFO/WARN/ERROR/FATAL 开 |
| 查询/设置 | `isEnabled(LogLevel) / setEnabled(LogLevel, bool)` | 线程安全由持有方（sink）保证 |

**验证规则**（FR-002）：
- 每个 sink 持有一个独立矩阵：`FileSink`（core）与 `ConsoleSink`（ui）互不影响
- `Logger` 广播记录时，仅向 `isEnabled(level)` 为真的 sink 投递 `emit`（配置归属 sink、执行位于 Logger 广播路径）
- 菜单栏切换即调对应 sink 的 `setLevelEnabled`，立即生效（FR-012，SC-006）

### LogRecord

单条日志记录的数据契约（spec Key Entity）。**只读**；由 Logger 构造并广播。

| 字段 | 类型 | 约束/说明 |
|------|------|----------|
| `timestamp` | `std::chrono::system_clock::time_point` | 本地时间，毫秒精度（FR-003） |
| `level` | `LogLevel` | 已通过 Logger 阈值过滤 |
| `source` | `std::string` | 来源标识：C++ 为 `file:line`（如 `logger.cpp:123`）；Python 为 `name:lineno`（如 `__main__:12`） |
| `message` | `std::string` | 消息正文，UTF-8；不要求含换行（Formatter 负责加 `\n`） |

**验证规则**：
- `message` 含内嵌换行时，Formatter 需保证单条记录在文件/控制台中仍为一行完整块（多行消息可保留原样，但记录边界由格式前缀行确定，见 contracts/log-file-format.md）
- 超长消息（> 4KB 视为异常）：完整写入，不截断（Edge Case 要求"完整写入或明确截断"——本期选完整写入，格式契约保证行边界）

**状态转换**：无状态实体（不可变记录）。由 Logger 的"格式化为单行 → 广播到全部 sink"为唯一转换步骤。

### LogSink

日志输出目标抽象（spec Key Entity）。

| 成员 | 签名 | 说明 |
|------|------|------|
| `emit` | `virtual void emit(const LogRecord&) = 0` | 接收一条已过滤、已格式化前的记录；实现负责最终渲染（文件行 / 控件文本） |
| `name` | `virtual const char* name() const noexcept = 0` | sink 标识（调试/告警使用） |

**验证规则**：
- `emit` 实现不得抛异常（实现内部捕获；Logger 广播层也包裹 try/catch 兜底，FR-009 精神）
- sink 注册与移除由 `Logger::addSink/removeSink` 管理；Logger 广播时持锁遍历副本（防回调中注册/注销导致迭代器失效）

### Logger 配置（Config）

Logger 单例的装配参数（`Logger::configure`，线程安全，可在启动时一次调用）。

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `filePath` | `std::string` | `""`（空 = 不写文件） | `app.log` 完整路径（app 层传入） |
| `levelMatrix` | `LogLevelMatrix` | 默认矩阵（DEBUG 关、其余开） | 全局默认矩阵，作为 FileSink 初始矩阵与菜单 UI 初始态（FR-002） |
| `vtkLoggingEnabled` | `bool` | `true` | VTK 日志纳入开关（FR-011） |
| `maxFileSize` | `std::uint64_t` | `5 * 1024 * 1024` | 轮转阈值字节（FR-005） |
| `maxBackups` | `int` | `3` | 保留归档份数（FR-005） |

**验证规则**：
- `filePath` 非空时：目录不存在自动创建（`std::filesystem::create_directories`）；创建/打开失败 → FileSink 按 R7 降级
- 重复 `configure` 视为更新（重建 FileSink 或热更新阈值；本期仅启动一次配置）
- `levelMatrix` 仅作初始值与 UI 初始态；运行时切换直接调 sink 矩阵，不回写 Config

## 关系

- **Logger → LogSink**: 1:N 广播（默认注册 FileSink[配置非空时]；UI 注册 ConsoleSink）；广播前按各 sink 的矩阵过滤
- **LogSink → LogRecord**: 输入为不可变记录，无回写
- **LogLevelMatrix → LogSink**: 每个 sink 恰持一个矩阵；控制台与文件独立（FR-002）
- **LogLevel → LogRecord**: LogRecord 引用级别值域；过滤执行于 Logger 广播路径、配置归属 sink 矩阵
- **Python 桥接 → Logger**: `_cpp_log(level, source, message)` 是 LogRecord 的构造入口之一（来源 `name:lineno`），与 C++ 直接调用 `Logger::instance().info(...)` 等价
- **菜单栏 ↔ sink**: 菜单勾选状态是 sink 矩阵的视图；切换调 `setLevelEnabled` 并持久化 QSettings（FR-012/013）

## 存储

- 持久化形式：UTF-8 文本文件，追加写，大小轮转（5MB × 3）
- 单行格式契约见 [contracts/log-file-format.md](contracts/log-file-format.md)
- 无数据库、无索引；日志文件即唯一持久存储

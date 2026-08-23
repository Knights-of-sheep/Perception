# Implementation Plan: 日志统一管理模块（统一日志 + 菜单栏级别设置）

**Branch**: `001-unified-logging` | **Date**: 2026-08-23 | **Spec**: [spec.md](spec.md)

**Input**: Feature specification from `specs/001-unified-logging/spec.md`

**Note**: This template is filled in by the `/speckit.plan` command; its definition describes the execution workflow.

## Summary

为 Perception 桌面应用建立统一日志体系：core 层新增 Qt-free 的 `log/` 子系统（`Logger` 单例 + `LogSink` 抽象 + 轮转 `FileSink`），UI 层注册 `ConsoleLogSink` 将日志转发到 `PythonConsole`，Python 侧通过注入的 `_cpp_log` 回调 + `logging.Handler` 桥接将 `logging` 输出汇入同一日志流。Qt 自身输出经 `qInstallMessageHandler` 重定向捕获（FR-010），VTK 日志纳入提供独立开关、默认启用（FR-011）。文件侧按大小轮转（5MB × 3 份归档），控制台与文件各持**独立级别开关矩阵**（每级别可开/关，默认 DEBUG 关、其余开，FR-002），菜单栏 `设置 → 日志级别 → {控制台 / 文件}` 提供可视化切换并持久化到 QSettings（FR-012/013）。UTF-8 编码，多线程安全，写入失败降级不崩溃。

## Technical Context

**Language/Version**: C++17（`CMAKE_CXX_STANDARD 17`，MSVC `/utf-8`）；内嵌 Python 3.13（`find_package(Python3 3.8 REQUIRED)`）；pybind11-2.13.6（vendored，M5 起用于 `perception_py`，本期不新增依赖）

**Primary Dependencies**: Qt5 Widgets（`perception_ui`，仅 UI 层）；`std::filesystem`/`std::chrono`/`std::thread`（core `log/` 子系统，保持 Qt-free）；Python3 开发库（`Python3::Python`，已链接于 `perception_ui`）

**Storage**: 文本日志文件（UTF-8，追加写），默认路径 `%APPDATA%/Perception/logs/app.log`（由 app 层用 `QStandardPaths::AppDataLocation` 计算后传入 Logger；core 层不依赖 Qt）

**Testing**: C++ 单测 `tests/cpp/`（CTest，纯 `assert` 风格，沿用 `core_smoke_test.cpp` 模式）；Python 测试 `tests/python/`（pytest，M5 依赖 `perception_py`，本期不新增 Python 测试目标）

**Target Platform**: Windows 桌面（开发机），Qt5 深色主题桌面应用

**Project Type**: desktop-app（Qt5 Widgets + VTK；core 为无 UI 依赖的静态库 `perception_core`）

**Performance Goals**: 单条日志写入 O(1)；并发写入下 GUI 线程不阻塞（ConsoleSink 经 Qt 队列投递）；满足 SC-003（8 线程 × 1000 条无交错无丢失）

**Constraints**:
- core 层（`perception_core`）不得引入 Qt/Python 依赖（宪法 III + 现有构建隔离）；日志方案采用标准机制（C++17 标准库），**不使用 Qt 日志工具类**（QDebug/QLoggingCategory）作日志实现（Clarifications 2026-08-23）
- 单条记录原子落盘：格式化后再一次性写入，禁止半行交错（FR-007）
- 所有文本输出 UTF-8（宪法 Design Constraints）
- 日志模块不属数据流四层（model/io/process/event），为横切基础设施 → 新增 `src/core/log/` 目录（见 Constitution Check）
- 失败降级：文件不可写时仅控制台输出 + 单次告警，不崩溃（FR-009）
- 级别过滤按各 sink 自身的开关矩阵进行：控制台与文件可分别配置（FR-002）；矩阵由 `LogSink` 持有，`Logger` 广播时查 `isLevelEnabled` 决定是否投递
- Qt 消息重定向（FR-010）与 VTK 拦截（FR-011）属 ui/render 层职责，core 层不接触 Qt/VTK
- 菜单栏级别设置持久化到 QSettings（FR-013）；入口挂在 `设置(&S)` 菜单下（现有菜单栏无"设置"菜单，需新增）

**Scale/Scope**: 单进程桌面应用；日志量级小（KB~MB 级会话）；日志文件 5MB × 3 份上限，磁盘占用可预期

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| 宪法条款 | 评估 | 状态 |
|---|---|---|
| III. Layered Core | core 四层为数据流架构；日志是横切基础设施，新增独立 `src/core/log/` 子目录，**不触碰** model/io/process/event 既有代码；`Logger` 纯 std 实现维持 core 无 UI 依赖、可独立单测 | ✅ PASS |
| V. Test-First | `tests/cpp/logger_test.cpp` 先于实现提交（Red-Green-Refactor）；矩阵过滤、轮转、线程安全、格式、降级均有断言 | ✅ PASS |
| Front-end/back-end separation | `ConsoleLogSink` 与 Qt 消息重定向桥（`qt_message_bridge`）放 `src/ui/log/`；VTK 拦截桥放 `src/render/`；二者实现 core 定义的纯接口，core 不知 UI/render 存在 | ✅ PASS |
| IV. Python Command-Driven | 日志非数据 CRUD，不受影响；Python 日志汇入为桥接（`logging.Handler` → C++ Logger），不绕行数据层 | ✅ PASS |
| 设计约束（UTF-8 / Qt5 / 深色主题） | 文件 UTF-8；ConsoleSink 复用 `PythonConsole::appendOutput` 与主题色；菜单栏新增"设置"菜单沿用现有 QSS 深色样式与 QAction checkable 模式（同主题菜单） | ✅ PASS |
| Quality Gates（ctest + pytest 绿） | 新增 `logger_test` CTest；`pytest` 本期不新增目标（`perception_py` M5 未启用） | ✅ PASS |

**Gate 结论**: 无违反项。复杂性说明见下表（仅为结构记录，不构成违规）。

## Complexity Tracking

> 仅为结构说明：`src/core/log/` 是对 core 层"四层数据流"之外的基础设施扩展，非违规。

| 项 | 为什么需要 | 更简方案被否原因 |
|---|---|---|
| `src/core/log/` 独立目录（而非并入某层） | 日志横切所有层，无单一归属；独立目录保持四层数据流架构不变 | 并入 `event/`（日志也是事件）→ 混淆"数据变更通知"与"诊断输出"职责 |
| `LogSink` 抽象（而非 Logger 直接写死文件+控制台） | ConsoleSink 在 UI 层、FileSink 在 core 层，需接口解耦才能维持 core 无 Qt 依赖 | Logger 内嵌 Qt 控制台写死 → 违反 core 无 UI 依赖宪法 |
| per-sink `LogLevelMatrix`（而非单一全局阈值） | FR-002 要求每级别独立开关、控制台与文件**分别配置**；矩阵归 sink 持有使过滤位置（Logger 广播）与配置归属（sink）解耦 | 单一全局阈值 → 被 Clarifications 2026-08-23 明确否决 |
| `qt_message_bridge`（qInstallMessageHandler 捕获 Qt 输出） | FR-010 要求 Qt 自身输出纳入统一流 | 不捕获 → 违反"全局日志纳入此模块"；在 core 捕获 → 违反 core 无 Qt 依赖 |
| `vtk_log_bridge` + 配置开关 | FR-011 要求 VTK 日志纳入且可开关；render 层当前仅骨架，桥接代码随 render 落地，开关/配置本期生效 | 不拦截 → 违反 FR-011；在 core 依赖 VTK → 违反分层 |

## Project Structure

### Documentation (this feature)

```text
specs/001-unified-logging/
├── plan.md              # This file (/speckit.plan command output)
├── research.md          # Phase 0 output (/speckit.plan command)
├── data-model.md        # Phase 1 output (/speckit.plan command)
├── quickstart.md        # Phase 1 output (/speckit.plan command)
├── contracts/           # Phase 1 output (/speckit.plan command)
│   ├── cpp-logger-api.md
│   ├── log-file-format.md
│   └── python-log-bridge.md
├── checklists/
│   └── requirements.md
└── tasks.md             # Phase 2 output (/speckit.tasks command - NOT created by /speckit.plan)
```

### Source Code (repository root)

```text
src/core/log/                          # 新增：Qt-free 日志子系统（core 层）
├── log_level.h / log_level.cpp        # LogLevel 枚举 + 字符串映射
├── log_level_matrix.h / .cpp          # LogLevelMatrix：每级别布尔位（新增，FR-002）
├── log_record.h                       # LogRecord 数据契约（时间戳/级别/来源/消息）
├── log_sink.h                         # LogSink 抽象基类 + 矩阵接口
├── logger.h / logger.cpp              # Logger 单例：配置、按 sink 矩阵过滤、广播、线程安全
└── file_sink.h / file_sink.cpp        # FileSink：追加写 + 5MB×3 大小轮转

src/ui/log/                            # 新增：UI 侧日志接入
├── console_log_sink.h / .cpp          # ConsoleLogSink：队列投递到 PythonConsole
└── qt_message_bridge.h / .cpp         # Qt 消息重定向：qInstallMessageHandler → Logger（FR-010）

src/render/                            # 修改：render 层（当前仅骨架）
└── vtk_log_bridge.h / .cpp            # VTK 输出拦截 + 配置开关（FR-011，随 render 落地启用）

src/ui/MainWindow.h / .cpp             # 修改：新增"设置(&S)"菜单 + 日志级别子菜单（FR-012/013）
src/ui/console/PythonConsole.h / .cpp  # 修改：注入 _cpp_log 回调 + logging 桥接脚本
src/app/main.cpp                       # 修改：Logger::configure + 注册 sink + Qt bridge + QSettings 读写

src/core/CMakeLists.txt                # 修改：加入 log/logger.cpp、log/file_sink.cpp、log/log_level_matrix.cpp
src/ui/CMakeLists.txt                  # 修改：加入 log/console_log_sink.cpp、log/qt_message_bridge.cpp
src/render/CMakeLists.txt              # 修改：加入 vtk_log_bridge.cpp（GUI 构建）
tests/cpp/CMakeLists.txt               # 修改：注册 logger_test
tests/cpp/logger_test.cpp              # 新增：矩阵过滤/格式/线程安全/轮转/降级断言
```

**Structure Decision**: 采用单项目分层结构（沿用现有 `src/core` → `src/ui` → `src/app` 依赖方向）。日志抽象与文件 sink 置于 core（Qt-free、可单测），GUI 投递 sink 置于 ui，app 层完成配置装配与启动注册。Python 桥接内嵌于 PythonConsole 的引导脚本（当前进程内 Python 运行时所在处），与既有 `sys.stdout/stderr` 重定向机制同构。

## Phase 0: Research

研究任务与结论记录于 [research.md](research.md)。本期为成熟 C++17/标准库技术，研究项 R1~R10 覆盖：core 纯 std、轮转、并发、Python 桥接、时间戳、目录、降级、Qt 消息重定向、VTK 拦截、菜单栏设置与持久化；所有 NEEDS CLARIFICATION 已按代码库既有约束与 Clarifications 2026-08-23 解决（见 research.md）。

## Phase 1: Design

设计产物：
- 数据契约: [data-model.md](data-model.md)
- 接口契约: [contracts/cpp-logger-api.md](contracts/cpp-logger-api.md)、[contracts/log-file-format.md](contracts/log-file-format.md)、[contracts/python-log-bridge.md](contracts/python-log-bridge.md)
- 验证指南: [quickstart.md](quickstart.md)

# Research: 日志统一管理模块

**Branch**: `001-unified-logging` | **Date**: 2026-08-23 | **Spec**: [spec.md](spec.md)

## Research Questions

来自 plan.md Technical Context 中的未知项与依赖，逐项给出决策。

---

### R1: core 日志子系统用纯 std 还是 Qt？

**Decision**: 纯 C++17 标准库（`std::chrono`、`std::filesystem`、`std::ofstream`、`std::mutex`、`std::thread`），不引入 Qt/Python 依赖。

**Rationale**:
- 宪法 III：core 层（`perception_core`）构建隔离，`src/core/CMakeLists.txt` 无任何 Qt 链接；`tests/cpp/` 全部无 UI 无头可跑（宪法 V）
- Logger 作为横切基础设施，须可被任何层（含未来 headless 工具、`perception_py`）复用，Qt-free 是最小依赖面
- C++17 标准库在 MSVC 上完整支持 `std::filesystem`（文件轮转）与 `std::chrono`（毫秒时间戳）

**Alternatives considered**:
- `qInstallMessageHandler` + `QFile`：作为 core 日志实现被否决（绑定 Qt、格式自定义受限、轮转需手写、core 层禁 Qt）；但**仅作为 UI 层捕获 Qt 自身输出的重定向桥**保留（FR-010，见 R8）
- 引入 spdlog 等第三方库：功能全但新增 vendored 依赖，与项目"最小依赖"取向不符 → 放弃

---

### R2: 文件轮转如何实现，Windows 上的关键点？

**Decision**: `FileSink` 内部维护 `std::ofstream` 句柄；写入前检查当前文件大小 ≥ 阈值（5MB）时执行滚动：
`app.log.3` 删除 → `app.log.2`→`app.log.3` → `app.log.1`→`app.log.2` → `app.log`→`app.log.1` → 新建 `app.log`。滚动全程持锁。

**Rationale**:
- `std::filesystem::rename` 同盘为原子操作（Windows 上为 `MoveFileEx` 语义），滚动链在锁内完成，无并发撕裂
- 写前检查大小 + 写后校验，避免单条超长记录跨轮转边界被截断
- 轮转前先 `flush()+close()` 释放句柄，Windows 不允许在句柄未关时 rename 文件（共享冲突）——这是 MSVC 平台的硬性约束

**Alternatives considered**:
- 单文件无轮转：磁盘无限增长，违反 spec 的 SC-002 → 放弃
- 跨进程锁文件（`LockFileEx`）：单进程应用不需要（spec Assumptions：同进程）→ 放弃

---

### R3: 多线程写入模型？

**Decision**: `Logger` 内部一个 `std::mutex` 串行化所有 `sink->emit(record)`；`FileSink` 自身再持锁保证滚动与写入互斥；`ConsoleLogSink` 的跨线程投递用 Qt `QMetaObject::invokeMethod(..., Qt::QueuedConnection)` 切到 GUI 线程后调用 `appendOutput`。

**Rationale**:
- 单进程单文件写入：Logger 级锁 + FileSink 级锁双层，简单且满足 SC-003（8 线程 × 1000 条无交错无丢失）
- 每条记录先 `format()` 成完整单行（含 `\n`），再一次性 `write`，天然原子；无需 `LogFile` 级别更细的锁
- ConsoleSink 不直接操作 QTextEdit（非 GUI 线程写控件是 UB），统一队列投递到 GUI 线程，与 Qt 线程模型一致

**Alternatives considered**:
- 无锁环形缓冲 + 后台写线程：高性能高复杂度，本应用日志量小（KB~MB 级）不需要 → 放弃
- 每条记录独立开文件写：无并发问题但性能差、轮转复杂 → 放弃

---

### R4: Python 侧日志如何汇入统一流？

**Decision**: 在 `PythonConsole::initPython()` 引导脚本（`kBootstrap`）中：
1. C++ 向 Python globals 注入一个 `_cpp_log(level, message)` 函数（`PyCFunction`，回调 C++ `Logger`，并在内部持有 GIL 释放/再获取以不阻塞 Python 调用方——实际 `PyRun_String` 期间 GIL 已在手，直接调用即可）
2. 引导脚本定义 `PerceptionLogHandler(logging.Handler)`，`emit()` 中映射 Python 级别 → `LogLevel`，调用 `_cpp_log(level, record.name + ":" + str(record.lineno), record.getMessage())`
3. `logging.getLogger().addHandler(handler)`，并把 root logger 级别设为 DEBUG（真实过滤由 C++ Logger 按各 sink 的级别开关矩阵统一做，控制台与文件可分别配置）

**Rationale**:
- 与既有 `sys.stdout/stderr` 重定向同构：Python 侧薄桥接 → C++ 侧统一处理，符合 spec FR-008"同一文件、同一级别与格式策略"
- 来源标识用 `record.name:lineno`（如 `__main__:12`），格式契约见 contracts/python-log-bridge.md
- 当前进程内 Python 运行时就在 PythonConsole（M5 `perception_py` 未启用）；未来启用后该 handler 机制同样适用于 `perception_py` 命名空间（同一解释器实例）

**Alternatives considered**:
- `perception_py` 模块导出 C++ Logger：pybind11 模块 M5 才启用，本期无法验证 → 推迟
- Python 直接写同一日志文件：格式/轮转/级别重复实现，违反"单一入口" → 放弃
- 仅重定向 `logging` 到 `sys.stderr`（复用控制台通道）：绕开文件轮转，P2 的"同一文件"不满足 → 放弃

---

### R5: 时间戳本地化与线程安全？

**Decision**: `std::chrono::system_clock::now()` → `std::chrono::milliseconds` 精度；`std::localtime` 在 MSVC 用 `localtime_s`（或 `gmtime_s` 同步版本）避免 POSIX 非线程安全 API。

**Rationale**:
- spec FR-003 要求"本地时间、毫秒精度"
- MSVC 提供 `localtime_s`（C11 Annex K），无共享静态缓冲，线程安全；`std::localtime` 在 MSVC 实现即调用 `localtime_s` 的内部线程安全版本，但显式用 `localtime_s` 更明确
- 格式：`YYYY-MM-DD HH:MM:SS.mmm`（本地）

**Alternatives considered**: 使用 `GetLocalTime`（Windows API）：可行但引入平台 API，`localtime_s` 已跨 MSVC 提供 → 采用标准路线

---

### R6: 应用数据目录如何确定？

**Decision**: app 层（`src/app/main.cpp`）用 `QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)`（已设 `setOrganizationName("Perception")` + `setApplicationName("Perception")`，解析为 `%APPDATA%/Perception`）拼接 `logs/app.log`，启动时调用 `Logger::instance().configure(...)` 传入。

**Rationale**:
- spec FR-004："默认应用数据目录，可配置覆盖"
- Qt 已提供跨平台正确目录；core 层不依赖 Qt，故目录计算放 app 层（ui 层亦可），Logger 只接收路径
- 目录不存在时 `std::filesystem::create_directories`（FR-004 + Edge Case"目录被外部删除后重建"）

**Alternatives considered**: 硬编码 `%APPDATA%/Perception`：可行但绕过 Qt 标准路径，且 QSettings 已用同一体系 → 采用 QStandardPaths

---

### R7: 写入失败如何降级不崩溃？

**Decision**: `FileSink::emit` 捕获所有异常（`std::filesystem` 错误、流错误），首次失败时通过一个"告警回调"（默认为 `std::fprintf(stderr, ...)`；UI 场景由 ConsoleLogSink 承担）输出**一次**告警，之后静默降级为纯控制台模式（`FileSink` 标记 disabled，不再尝试打开文件）。

**Rationale**:
- spec FR-009 + SC-005："不崩溃、单次告警、不重复刷屏"
- "一次告警"状态在 FileSink 内持久：后续失败不再刷屏；若文件路径恢复（目录重建），由配置热更新或下次 `configure` 重新启用（本期仅启动时配置，故维持 disabled 即可）
- 告警本身不发回 Logger 广播（避免递归），直接写 stderr

**Alternatives considered**: 每次失败都告警：日志风暴，违反"不重复刷屏" → 放弃；尝试恢复重开文件：过度设计，本期配置固定 → 放弃

---

### R8: Qt 自身输出如何纳入统一日志流？

**Decision**: UI 层（`src/ui/log/qt_message_bridge`）安装 `qInstallMessageHandler`：回调把 `QtMsgType`（QtDebugMsg/QtInfoMsg/QtWarningMsg/QtCriticalMsg/QtFatalMsg）映射为 `LogLevel`（Debug/Info/Warn/Error/Fatal），构造 `LogRecord`（`source = "qt:<file>:<line>"`，`file` 取 basename）后调用 `Logger::instance().log(...)`。安装与卸载配对（`RAII` 或显式 restore），仅 GUI 构建启用。

**Rationale**:
- FR-010 要求 Qt 输出纳入统一流且遵守统一格式与级别开关；qInstallMessageHandler 是 Qt 官方捕获钩子，不改任何 Qt 源码
- 回调中若 Qt 内部再触发统一 API（嵌套），用一层"重入标志"防递归：标志置位时直通 `fprintf(stderr)`（Edge Case：最多一层跳转）
- `qt_message_bridge` 属 UI 层：`qInstallMessageHandler` 是 Qt 全局 API，core 层禁 Qt → 桥接放 `src/ui/log/`，与 ConsoleLogSink 同目录

**Alternatives considered**:
- 在 core 层提供 `QtLogBridge` 但仅链接时启用：core 仍会引入 Qt 头文件 → 违反宪法 III 构建隔离 → 放弃
- 不捕获 Qt 输出：违反"全局日志纳入此模块"（Clarifications 2026-08-23 Q1=A）→ 放弃

---

### R9: VTK 日志如何拦截与开关？

**Decision**: `src/render/vtk_log_bridge` 提供两条能力：1) 关闭 VTK 自身弹窗/控制台输出（`vtkOutputWindow::SetInstance` 自定义窗口 or `SetGlobalWarningDisplay`），2) 将 VTK 警告/错误（`vtkOutputWindow::DisplayWarningText/DisplayErrorText` 覆写）转发为 `LogRecord`（`source = "vtk:..."`）。开关 `vtkLoggingEnabled`（默认 `true`）存于 `Logger::Config`，关闭时桥接不注册/不转发。

**Rationale**:
- FR-011：VTK 日志纳入统一流且有独立开关；默认启用、可关闭
- render 层当前仅为 CMake 骨架（M3 才实现渲染代码），本期落地：1) 桥接头文件 + 开关配置接入 Logger（可单测开关默认值）；2) 桥接实现随 render 落地时随行启用（拦截代码需 VTK 头文件，编译期即验证）
- 开关配置同时作为菜单栏"VTK 日志拦截"复选项的读写目标（`设置 → 日志级别` 菜单尾部分隔线下的独立项，用户 Clarifications 2026-08-23 补充）

**Alternatives considered**:
- 在 app 层拦截 VTK：app 层链接 VTK 但非渲染域，职责错位 → 放弃
- 无条件拦截不做开关：违反用户"增加开关控制"要求 → 放弃

---

### R10: 菜单栏设置入口与持久化如何实现？

**Decision**: `MainWindow` 新增 `设置(&S)` 菜单 → `日志级别(&L)` 子菜单：
- 子菜单含 `控制台(&C)` 与 `文件(&F)` 两组，每组 5 个 `QAction`（checkable、非互斥、可多选），勾选状态与 `ConsoleSink`/`FileSink` 的 `LogLevelMatrix` 双向同步（`setLevelEnabled` / `isLevelEnabled`）
- 菜单尾部分隔线后加 `VTK 日志拦截` 独立复选项（FR-011 开关）
- 任何切换立即生效（调 sink 矩阵），并写 QSettings：`logging/console/debug` 等 10 个布尔 key（`logging/vtk` 第 11 个），启动时 `Logger::configure` 前读取并作为各 sink 初始矩阵
- 勾选展示沿用主题菜单模式（QAction checkable + 深色 QSS）

**Rationale**:
- FR-012/013 + SC-006：无需重启立即生效、持久化、重启保持
- 菜单栏结构调研：现有菜单为 文件/视图/主题/帮助，无"设置"菜单 → 顶层新增 `设置(&S)`（置于"帮助"之前）
- 矩阵数据源唯一：sink 内部 `LogLevelMatrix` 为 truth；菜单只是视图/控制器（与主题菜单同构：`triggered(bool)` 转发 lambda）

**Alternatives considered**:
- 设置对话框：范围大、引入对话框，与用户确认的"菜单栏直接呈现"（Q3=A）不符 → 放弃
- 不持久化（每次启动默认矩阵）：桌面应用设置惯例要求保持 → 放弃（FR-013）

---

## 结论汇总

| # | 决策 | 关联需求 |
|---|------|---------|
| R1 | core `log/` 纯 std（C++17），无 Qt/Python | 宪法 III/V、FR-001、Clarifications: 标准日志 |
| R2 | 写前检查 + close→rename 链→reopen，锁内滚动 | FR-005、Edge: 轮转瞬间写入 |
| R3 | Logger 锁 + FileSink 锁；ConsoleSink 队列投递 GUI | FR-007、SC-003 |
| R4 | 注入 `_cpp_log` + `logging.Handler` 桥接 | FR-008、SC-004 |
| R5 | `chrono` 毫秒 + `localtime_s` | FR-003 |
| R6 | `QStandardPaths::AppDataLocation` + `create_directories` | FR-004、Edge: 目录被删 |
| R7 | FileSink 首次失败告警一次后降级静默 | FR-009、SC-005 |
| R8 | `qInstallMessageHandler` 桥接于 ui/log，映射 QtMsgType→LogLevel，防递归 | FR-010、Clarifications Q1=A |
| R9 | `vtk_log_bridge` 于 render 层，开关 `vtkLoggingEnabled` 默认启用、可菜单关闭 | FR-011、Clarifications Q1 补充 |
| R10 | 设置菜单 + 控制台/文件 5 级别复选 + VTK 开关，QSettings 持久化、立即生效 | FR-012/013、SC-006、Clarifications Q2/Q3 |

无未解决的 NEEDS CLARIFICATION。可以进入 Phase 1 设计。

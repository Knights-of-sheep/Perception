---

description: "Task list for 日志统一管理模块 implementation"
---

# Tasks: 日志统一管理模块（统一日志 + 菜单栏级别设置）

**Input**: Design documents from `/specs/001-unified-logging/`

**Prerequisites**: plan.md (required), spec.md (required for user stories), research.md, data-model.md, contracts/

**Tests**: C++ 单测为宪法 V 强制要求（`tests/cpp/logger_test.cpp`，Red-Green-Refactor）；端到端验证按 quickstart.md V1~V13 手工执行。Python 测试目标本期不新增（`perception_py` 未启用）。

**Organization**: Tasks are grouped by user story to enable independent implementation and testing of each story.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (e.g., US1, US2, US3)
- Include exact file paths in descriptions

## Path Conventions

- **Single project**: `src/`, `tests/` at repository root
- 本 feature 关键路径：core 日志子系统 `src/core/log/`（Qt-free）；UI 接入 `src/ui/log/`；装配 `src/app/main.cpp`；单测 `tests/cpp/logger_test.cpp`

---

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: 确认现有项目基线，保证后续改动的回归对照

- [X] T001 验证现有项目基线：运行 `scripts/build.ps1` 完整构建 + `ctest` 确认现有测试全绿，记录结果作为回归基线（无新增依赖，core 日志子系统为 C++17 标准库）

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: core 日志子系统 —— 所有用户故事的公共地基（Logger 单例、级别矩阵、格式化、sink 抽象、FileSink 追加写与降级）

**⚠️ CRITICAL**: No user story work can begin until this phase is complete

- [X] T002 按 [contracts/cpp-logger-api.md](contracts/cpp-logger-api.md) 创建 core log 公共头文件：`src/core/log/log_level.h`（`enum class LogLevel` + `toString` + `parseLevel`）、`src/core/log/log_level_matrix.h`（`LogLevelMatrix`，默认 DEBUG 关、其余开）、`src/core/log/log_record.h`（不可变记录契约）、`src/core/log/log_sink.h`（`LogSink` 抽象 + 每 sink 独立矩阵接口）、`src/core/log/logger.h`（`Logger` 单例：`Config`/`configure`/`log` 族/来源宏/`addSink`/`removeSink`）；命名空间 `perception::core::log`，全部 Qt-free
- [X] T003 编写 `tests/cpp/logger_test.cpp`（**先红**，宪法 V）：断言 `LogLevelMatrix` 默认矩阵与开关翻转（FR-002）、`parseLevel` 大小写不敏感与未知回退（契约）、行格式 `YYYY-MM-DD HH:MM:SS.mmm LEVEL [source] message`（FR-003，含毫秒与 `WARN ` 右对齐）、消息换行转义为字面量（log-file-format.md）、8 线程 × 1000 条并发无交错无丢失（FR-007/SC-003）、FileSink 追加写与写入失败降级单次告警（FR-004/009）
- [X] T004 [P] 实现 `src/core/log/log_level.cpp` + `src/core/log/log_level_matrix.cpp`：`toString` 稳定大写字符串、`parseLevel` 大小写不敏感 + 未知回退 `Info`、`LogLevelMatrix` 默认值（DEBUG 关、其余开）与 `setEnabled/isEnabled/setAll`
- [X] T005 [P] 实现 `src/core/log/logger.cpp`：`Logger::instance()` 单例、`configure`（线程安全）、`log` 族与 `debugAt/infoAt/...` 来源宏（`source = "<basename>:<line>"`）、按各 sink `LogLevelMatrix` 过滤后广播（锁内遍历副本，防回调中注册注销迭代器失效）、行格式化（`std::chrono::system_clock` 毫秒 + `localtime_s` 本地时间、UTF-8、`\n`/`\r` 转义、级别 5 字符右对齐）、`log(...)` 不抛异常（内部捕获 sink 异常，FR-009）
- [X] T006 [P] 实现 `src/core/log/file_sink.cpp`：`FileSink` 追加写（UTF-8 无 BOM，`app.log`）、目录不存在自动创建（`std::filesystem::create_directories`，FR-004）、首写失败经 `std::fprintf(stderr, ...)` 输出**一次**告警后标记 disabled 静默降级（FR-009/R7，告警不回流 Logger 防递归）；本任务不含轮转（US2）
- [X] T007 注册构建：修改 `src/core/CMakeLists.txt` 加入 `log/log_level.cpp`、`log/log_level_matrix.cpp`、`log/logger.cpp`、`log/file_sink.cpp`；修改 `tests/cpp/CMakeLists.txt` 注册 `logger_test`；运行 `ctest -R logger_test -V` 确认全绿（Red→Green 完成）

---

## Phase 3: User Story 1 (P1) — 统一 API 记录日志并即时可见

**Story Goal**: 任意模块通过统一日志 API 写入 → 控制台与文件即时可见（SC-001，最小可用闭环）

**Independent Test**: 运行应用后调用 `Logger::instance().info("hello")`（或 Python `logging.info`），开发控制台与 `app.log` 同时出现格式完整的同一条记录（US1 场景 1~4）

- [X] T008 [US1] 创建 `src/ui/log/console_log_sink.h` + `src/ui/log/console_log_sink.cpp`：`ConsoleLogSink : LogSink`（持有独立 `LogLevelMatrix`）；`emit` 内 `QMetaObject::invokeMethod(..., Qt::QueuedConnection)` 投递 GUI 线程后调 `LogConsoleWidget::appendOutput`（注：早期投递目标为 `PythonConsole`，FR-006 修订后改为独立日志输出面板，见 T025），按级别套用主题色（WARN 警示色、ERROR/FATAL 红色，复用现有错误样式）；**禁止在非 GUI 线程直接操作控件**（R3）
- [X] T009 [P] [US1] 修改 `src/app/main.cpp` 完成装配：用 `QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)`（已设 org/app 名）拼接 `logs/app.log` → `Logger::instance().configure({filePath, 默认矩阵, vtkLoggingEnabled=true, ...})` → `addSink(ConsoleSink)` → 启动路径加 `PERCEPTION_LOG_I("app started")`（V2）
- [X] T010 [P] [US1] 修改 `src/ui/CMakeLists.txt` 加入 `log/console_log_sink.cpp`；确保 ui 链接 `perception_core`（Logger 头文件可见）
- [X] T011 [US1] 端到端验证（quickstart V1/V2/V3）：启动应用执行 `logging.info("hello from python")` + `logging.warning(...)` → 控制台与 `%APPDATA%/Perception/logs/app.log` 均出现完整格式行（`HH:MM:SS.mmm INFO [__main__:N] ...`）；C++ 启动日志 `INFO [main.cpp:NN] app started`；默认矩阵下 `logging.debug(...)` 两侧均不出现

---

## Phase 4: User Story 2 (P2) — 日志文件按大小轮转

**Story Goal**: 单文件满 5MB 自动轮转，保留最近 3 份归档，磁盘占用可预期（SC-002）

**Independent Test**: 将阈值调小（1KB）写入超过阈值的日志 → 产生 `app.log` + `app.log.1..3`，继续写入后最旧归档被删除，文件总数 ≤ 4（US2 场景 1~3）

- [X] T012 [US2] 在 `tests/cpp/logger_test.cpp` 新增轮转用例（**先红**）：构造 `FileSink` 于临时目录，`maxFileSize = 1KB`、`maxBackups = 3`，写入超阈值 → 断言 `app.log`/`app.log.1`/`app.log.2`/`app.log.3` 存在、继续写后 `app.log.3` 被删除总数 ≤ 4、归档为整行无截断（FR-005，log-file-format.md 滚动顺序）
- [X] T013 [US2] 在 `src/core/log/file_sink.cpp` + `src/core/log/file_sink.h` 实现轮转（R2）：写前检查文件大小 ≥ `maxFileSize` → 锁内执行滚动链（删 `app.log.3` → `app.log.2`→`3` → `app.log.1`→`2` → `app.log`→`1` → 新建 `app.log`），滚动前先 `flush()`+`close()` 再 `std::filesystem::rename`（Windows 句柄未关时 rename 冲突）、滚动后重新打开追加；轮转触发瞬间不丢缓冲
- [X] T014 [US2] 端到端验证（quickstart V4）：临时将 `Logger::configure` 的 `maxFileSize` 调小（1KB），从控制台执行 Python 循环写日志触发多次轮转 → 检查 `%APPDATA%/Perception/logs/` 归档数量稳定为 3 份、无半行截断

---

## Phase 5: User Story 3 (P2) — Python 命令层日志汇入统一流

**Story Goal**: Python `logging` 模块输出与 C++ 日志同文件、同级别同格式（SC-004）

**Independent Test**: 在 Python 命令层执行 `logging.warning(...)` → 记录出现在统一日志文件与控制台，格式一致、时间顺序正确（US3 场景 1~3）

- [X] T015 [US3] 修改 `src/ui/console/PythonConsole.cpp`：在 `initPython()` 中 `PyRun_String(kBootstrap, ...)` 之前向 globals 注入 `_cpp_log(level: int, source: str, message: str)`（`PyCFunction` 包装，调 `Logger::instance().log(static_cast<LogLevel>(level), source, msg)`；内部捕获异常 + `PyErr_Clear`，无异常逃逸；`level` 取值 0=Debug/1=Info/2=Warn/3=Error/4=Fatal，契约见 python-log-bridge.md）
- [X] T016 [US3] 修改 `PythonConsole.cpp` 引导脚本 `kBootstrap`：追加 `PerceptionLogHandler(logging.Handler)`（`_LEVEL_MAP = {10:0, 20:1, 30:2, 40:3, 50:4}`，`source = "%s:%d" % (record.name, record.lineno)`，`emit` 内 try/except 失败仅 `handleError`），挂到 root logger 并 `setLevel(logging.DEBUG)`（真实过滤由 C++ 侧各 sink 矩阵统一执行，FR-002/008）；`clearConsole()` 不清 handler（一次性安装）
- [X] T017 [US3] 端到端验证（quickstart V1/V3/V6）：`logging.warning("warn: divisor zero")` → 控制台（警示色）+ 文件（`WARN [__main__:N] ...`）；`logging.error("中文消息测试 你好")` → 红色 + UTF-8 无乱码、与相邻 C++ 日志时间有序；默认矩阵下 `logging.debug(...)` 两侧均不出现

---

## Phase 6: User Story 4 (P2) — 菜单栏设置日志级别并持久化

**Story Goal**: `设置 → 日志级别 → {控制台 / 文件}` 复选切换立即生效、重启保持；VTK 日志拦截开关（SC-006）

**Independent Test**: 菜单勾选控制台 DEBUG → 控制台立即显示 DEBUG、文件不显示；重启后勾选保持；VTK 开关默认勾选、可切换并持久化（US4 场景 1~3）

- [X] T018 [US4] 修改 `src/ui/MainWindow.h` + `src/ui/MainWindow.cpp`：新增顶层 `设置(&S)` 菜单（置于"帮助"之前）+ `日志级别(&L)` 子菜单：`控制台(&C)`、`文件(&F)` 两组各 5 个 `QAction`（checkable、非互斥、可多选），尾部分隔线 + `VTK 日志拦截` 独立复选项（FR-011）；沿用深色 QSS 与 QAction checkable 模式（R10）
- [X] T019 [US4] 在 `src/ui/MainWindow.cpp` 实现菜单 ↔ sink 矩阵双向同步：持有 `ConsoleSink`/`FileSink` 指针（通过 `Logger::instance()` 的 sink 注册获得），`triggered(bool)` → 对应 sink `setLevelEnabled(level, checked)` 立即生效；初始化时按 sink 当前矩阵设置勾选态；任一切换写 QSettings：`logging/console/debug|info|warn|error|fatal`、`logging/file/...` 共 10 个 key + `logging/vtk`（FR-013）
- [X] T020 [US4] 修改 `src/app/main.cpp` 启动装配：`Logger::configure` 前读取 QSettings（`logging/console/*`、`logging/file/*`、`logging/vtk`）恢复各 sink 初始矩阵与 VTK 开关（无 key 时用默认矩阵）；确保读取早于 `ConsoleSink` 注册（R10/R6）
- [X] T021 [US4] 端到端验证（quickstart V8/V10）：勾选控制台 DEBUG → 控制台立即显示 `logging.debug` 行、文件不写入；取消后立即停止；重启应用勾选保持；`VTK 日志拦截` 默认勾选、切换后 QSettings 持久化（VTK 拦截行为留待 VTK 引入后验证）

---

## Phase 7: Polish & Cross-Cutting Concerns

**Purpose**: 横切收尾 —— Qt 输出重定向纳入统一流、全量回归

- [X] T022 创建 `src/ui/log/qt_message_bridge.h` + `src/ui/log/qt_message_bridge.cpp` 并装配（FR-010）：`qInstallMessageHandler` 回调把 `QtMsgType`（QtDebugMsg/QtInfoMsg/QtWarningMsg/QtCriticalMsg/QtFatalMsg）映射为 `LogLevel`，构造记录（`source = "qt:<basename>:<line>"`）调 `Logger::instance().log(...)`；回调内重入标志防递归（置位时直通 `std::fprintf(stderr, ...)`，Edge Case：Qt 内部嵌套最多一层跳转）；`src/app/main.cpp` 启动时安装（RAII 或显式 restore）；`src/ui/CMakeLists.txt` 加入 `log/qt_message_bridge.cpp`；按 quickstart V9 验证（`qWarning() << "qt-side warning"` → 控制台与文件出现 `WARN [qt:<file>:<line>] ...`，WARN 关时两处均不出现）
- [X] T023 全量回归与收尾：`scripts/build.ps1 -Gui` 完整构建 + `ctest`（含 `logger_test`）全绿；按 quickstart V1~V10 逐项对照验证并记录结果；检查无遗留 TODO/死代码（render 层本期不动，VTK 桥接随后续引入）

---

## Phase 8: User Story 5 (P3) — 日志路径对用户可见并可一键直达

**Story Goal**: `设置` 菜单展示日志文件完整路径并一键打开日志目录，消除排障可达性缺口（SC-007）

**Independent Test**: 启动应用 → `设置` 菜单可见 `日志文件：%APPDATA%\Perception\logs\app.log`（只读）→ 点击 `打开日志目录` 在文件管理器中定位到 `logs` 目录

- [X] T024 [US5] 修改 `src/ui/MainWindow.h` + `src/ui/MainWindow.cpp`：新增 `setLogFilePath(const QString&)` 与 `openLogDir()`；`设置(&S)` 菜单 VTK 开关后加分隔线 + 只读路径条目（`日志文件：<路径>`，disabled 可复制）+ `打开日志目录(&O)` 动作（`QDesktopServices::openUrl` 打开日志文件所在目录，失败弹提示）；修改 `src/app/main.cpp` 在 `Logger::configure` 后调用 `window.setLogFilePath(logPath)` 注入实际路径；快速验证：菜单展示完整路径、点击直达目录、未装配时占位并禁用（落地验证：`%APPDATA%\Perception\logs\app.log` 实际落盘；过程中修正日志默认路径——`AppDataLocation` 多一层 `Perception`、`GenericDataLocation` 映射 `C:\ProgramData` 不可写、`GenericConfigLocation` 实测返回 `%LOCALAPPDATA%`，最终 Windows 显式取 `%APPDATA%` 环境变量）

---

## Phase 9: 日志面板与 REPL 分离 + 启动工作目录（FR-006 修订 / US6）

**Purpose**: 两个用户体验修复——C++ 日志不再混入 Python REPL 交互输出（FR-006 修订）；文件对话框与相对路径解析跟随启动路径（US6/FR-015）

**Independent Test**: 启动应用后底部 PythonConsole 仅显示 Python 交互内容，统一日志流出现在"日志输出"面板；在数据目录（如 `E:\data`）启动应用后打开文件/导出弹窗初始目录为该目录、相对路径落点同基准（quickstart V12/V13）

- [X] T025 [FR-006 修订] 日志输出与 Python REPL 分离：新建 `src/ui/log/log_console_widget.h` + `log_console_widget.cpp`（只读 QPlainTextEdit 子类：`appendOutput(text, color)`、等宽字体、最大 1 万行、NoWrap）；修改 `src/ui/log/console_log_sink.h/.cpp` 投递目标由 `PythonConsole` 改为 `LogConsoleWidget`；修改 `src/ui/MainWindow.h/.cpp` 新增底部"日志输出"Dock（objectName `logDock`，与 Python 控制台同 tab 组、默认隐藏）+ `视图 → 日志输出(&L)`（Ctrl+3）开关 + `logConsole()` 访问器，`resetLayout` 纳入该 dock；修改 `src/app/main.cpp` 注册 `ConsoleLogSink` 传入 `window.logConsole()`；`src/ui/CMakeLists.txt` 加入新源文件；验证：py shell 不再出现 C++/Python logging 日志，日志输出面板实时显示并按级别着色（quickstart V12）
- [X] T026 [US6] 文件对话框默认目录跟随启动路径（FR-015）：**不修改** `src/app/main.cpp` 的工作目录（保持进程启动时 cwd 不变，不做 `setCurrent` 强制切换）；修改 `src/ui/MainWindow.cpp` 三处 QFileDialog 默认目录由 `QDir::homePath()` 改为 `QDir::current()`（打开数据文件、导出窗口图片、导出 Python 命令；`QDir::current()` 即启动程序时所在路径）；验证：在数据目录启动后弹窗初始目录为启动路径、相对路径（如 `--snapshot _shot.png`）解析到启动路径（quickstart V13）

## Phase 10: 日志路径可配置与清除（FR-016/017，US7/US8）

**Purpose**: 日志路径运行期可配置——`设置 → 设置日志路径...` 切换目录并自动迁移旧日志（US7/FR-016）；`设置 → 清除历史日志` 一键清理（US8/FR-017）

**Independent Test**: 通过菜单切换日志目录后，旧目录 `app.log` 与归档迁移到新目录、新日志写入新路径、重启保持；执行清除后目录中无任何日志文件且后续写入正常（quickstart V14/V15）

- [X] T027 [US7] core 层迁移与清除能力：`src/core/log/file_sink.h/.cpp` 新增 `migrateTo(newPath)`（关闭旧句柄 → 自动创建新目录 → 迁移 `app.log` 与归档链 `app.N.log`，rename 优先、失败退化 copy+remove → 按新路径重开追加）与 `clearHistory()`（关闭句柄 → 删除主文件与全部归档 → trunc 重建空文件）；`src/core/log/logger.h/.cpp` 新增 `setFilePath`/`clearLogFiles`/`filePath`（转发 FileSink，保留级别矩阵与 sink 指针）
- [X] T028 [US7/US8] UI 层入口：修改 `src/ui/MainWindow.h/.cpp`：`设置` 菜单在"打开日志目录"后新增 `设置日志路径...(&P)` 与 `清除历史日志(&C)`；`setLogPath()` 用 `QFileDialog::getExistingDirectory` 选目录 → `Logger::setFilePath`（自动迁移）→ `setLogFilePath` 更新展示 → QSettings 写 `log/path` → 状态栏提示；`clearLogHistory()` 弹 `QMessageBox::question` 二次确认 → `Logger::clearLogFiles` → 成功/失败提示；两动作在路径注入前禁用
- [X] T029 [US7] 启动装配持久化：修改 `src/app/main.cpp` 在默认路径计算后读取 QSettings `log/path`（存在则覆盖默认），`cfg.filePath` 与 `window.setLogFilePath` 均使用生效路径
- [X] T030 [US7/US8] 单测与端到端验证：`tests/cpp/logger_test.cpp` 新增 `testFileSinkMigrate`（写日志触发轮转 → migrateTo → 旧目录不残留、新目录含主文件+归档、迁移后继续追加）与 `testFileSinkClear`（clearHistory → 归档删除、主文件重建为空、旧内容不残留、后续写入正常）；冒烟：QSettings 注入自定义路径后启动，日志落到该路径（quickstart V14/V15）

---

## Dependencies

**Foundational（Phase 2）完成前，任何 User Story 不得开始**。

```text
Phase 1 (T001)
  └── Phase 2 (T002→T003→{T004,T005,T006}→T007)   [阻塞所有故事]
        ├── US1 (T008→{T009,T010}→T011)            [P1, MVP]
        ├── US2 (T012→T013→T014)                    [P2, 依赖 FileSink/Foundational]
        ├── US3 (T015→T016→T017)                    [P2, 依赖 Logger/Foundational + T008 的 ui/log 模式]
        └── US4 (T018→T019→T020→T021)              [P2, 依赖 T008(T009) 注册的 sink 指针]
Phase 7 (T022 依赖 T008/T009 的 ui/log 与 main.cpp 装配；T023 最后)
```

- US1 与 US2/US3/US4 互不阻塞：US1 完成即 MVP；US2/US3/US4 可在 US1 之后（或并行）独立交付
- US3 依赖 T008 仅因 ConsoleSink 为验证载体（Python 桥接本身只依赖 core Logger）；实现上可并行，验证上建议 US1 后
- US4 依赖 T009 注册的 sink 指针用于双向同步；菜单 UI 骨架（T018）可与 US1 并行

---

## Parallel Example: User Story 1

```bash
# 依赖 T008（ConsoleLogSink）完成后，T009 与 T010 可并行：
Task: "修改 src/app/main.cpp 装配 Logger + ConsoleSink（T009）"
Task: "修改 src/ui/CMakeLists.txt 注册 console_log_sink.cpp（T010）"
```

## Parallel Example: Foundational

```bash
# T002→T003 完成后，三个实现文件互不依赖，可并行：
Task: "实现 src/core/log/log_level.cpp + log_level_matrix.cpp（T004）"
Task: "实现 src/core/log/logger.cpp（T005）"
Task: "实现 src/core/log/file_sink.cpp（T006）"
```

## Parallel Example: User Story 3

```bash
# T015（注入 _cpp_log）与 T016（引导脚本 handler）同文件顺序完成；无并行分支。
# US3 整体与 US2、US4 在不同文件，可由不同成员并行推进：
Task: "US2 轮转（T012~T014）"   # file_sink.cpp + logger_test.cpp
Task: "US3 Python 桥接（T015~T017）"  # PythonConsole.cpp
Task: "US4 菜单栏（T018~T021）"   # MainWindow + main.cpp
```

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. 完成 Phase 1: Setup（T001 基线）
2. 完成 Phase 2: Foundational（T002~T007，CRITICAL - blocks all stories）
3. 完成 Phase 3: User Story 1（T008~T011）
4. **STOP and VALIDATE**: 按 quickstart V1/V2/V3 验证 US1（统一 API → 控制台 + 文件，最小闭环）
5. 交付 MVP（P1 完成即可演示：任意模块一行日志调用即双端可见）

### Incremental Delivery

1. 完成 Setup + Foundational → 核心日志地基就绪（可跑 `ctest -R logger_test`）
2. 添加 User Story 1 → 独立验证（MVP 交付）
3. 添加 User Story 2 → 轮转独立验证（V4）
4. 添加 User Story 3 → Python 汇入独立验证（V6）
5. 添加 User Story 4 → 菜单栏独立验证（V8/V10）
6. 收尾 Phase 7 → Qt 重定向（V9）+ 全量回归

### Parallel Team Strategy

With multiple developers:

1. 团队共同完成 Setup + Foundational（T001~T007）
2. Foundational 完成后：
   - Developer A: User Story 1（T008~T011，MVP）
   - Developer B: User Story 2（T012~T014）
   - Developer C: User Story 3（T015~T017）或 User Story 4（T018~T021）
3. 各故事独立完成与验证，最后统一 Phase 7 回归

---

## Notes

- [P] tasks = different files, no dependencies
- [Story] label maps task to specific user story for traceability
- Each user story should be independently completable and testable
- 宪法 V Test-First：T003、T012 为"先红"测试任务，须在对应实现前提交并通过评审
- core 层保持 Qt-free（宪法 III）：`src/core/log/` 不得 include Qt/Python 头文件；`ConsoleSink`/`qt_message_bridge` 在 `src/ui/log/` 承接触 Qt
- VTK 未引入：本期不创建 `src/render/vtk_log_bridge`，仅落配置开关（FR-011）；render 层本期零改动
- Commit after each task or logical group
- 端到端验证以 quickstart.md V1~V15 为准，验证结果记录于提交信息或任务备注

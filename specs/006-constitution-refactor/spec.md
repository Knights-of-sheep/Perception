# Feature Specification: Constitution Compliance Refactor

**Feature Branch**: `006-constitution-refactor`

**Created**: 2026-08-29

**Status**: In Progress（US1/US2/US3 代码完成，全量构建 + 测试 exe 回归通过，行数红线 0 error；US4 pybind11 桥接迁移与双库拆分进行中）

**Input**: User description: "按照新的constitution进行重构"

## User Scenarios & Testing

### User Story 1 - 拆分超标巨型源文件 (Priority: P1)

作为项目维护者，我希望将超过行数上限的源文件（当前最大 `MainWindow.cpp` 1770 行）按单一职责拆分为多个小文件，使每个文件都符合宪法行数约束（`.cpp` ≤ 800、普通 `.h` 建议 300/红线 500、`.hpp` ≤ 800），从而提升代码可读性与可审查性。

**Why this priority**: `MainWindow.cpp` 超过 `.cpp` 上限（800 行）两倍以上，是当前最严重的违规；行数约束是可自动验证的硬性指标，拆分是合规重构的核心。

**Independent Test**: 可独立验证——运行文件行数统计脚本，确认无 `.cpp` 超 800 行；同时现有 `ctest`/`pytest` 全部通过（行为未变）。

**Acceptance Scenarios**:

1. **Given** 现有 `MainWindow.cpp` 1770 行，**When** 完成拆分重构，**Then** 不存在超过 800 行的 `.cpp` 文件，且每个类/功能单元职责单一。
2. **Given** 拆分后的代码库，**When** 运行全部自动化测试（`ctest` + `pytest`），**Then** 100% 通过且测试断言与拆分前一致。

---

### User Story 2 - 头文件合规化 (Priority: P2)

作为项目维护者，我希望所有头文件带有 `#pragma once` 头保护、普通 `.h` 仅含声明、行数控制在建议/红线内，从而降低编译耦合并符合宪法「头文件约束」。

**Why this priority**: 头文件约束影响编译时间与依赖结构，覆盖所有文件（如 `theme_catalog.h` 410 行超过建议值 300 行）；此部分改动集中、风险低、收益直接。

**Independent Test**: 可独立验证——脚本扫描全部 `.h`/`.hpp` 检查 `#pragma once` 缺失与行数超限；构建验证无新增编译耦合。

**Acceptance Scenarios**:

1. **Given** 所有 `.h`/`.hpp` 文件，**When** 执行头文件合规扫描，**Then** 全部文件均带 `#pragma once` 头保护。
2. **Given** 任一普通 `.h` 文件超过建议上限 300 行，**When** 完成拆分，**Then** 文件行数回落至建议范围内（或记录豁免理由）。

---

### User Story 3 - 类/函数/命名规范对齐 (Priority: P3)

作为项目维护者，我希望现有代码的类、函数、命名符合宪法「类编码约束」「函数约束」「命名规范」，消除 God Class、超长函数、public 裸成员、不合规命名，从而提升代码一致性与可维护性。

**Why this priority**: 此类约束对可维护性有长期价值，但改动面广、风险相对较低（不改变行为），优先级次于结构性拆分。

**Independent Test**: 可独立验证——静态检查 + 代码评审对照宪法逐项核对；行为等价由自动化测试保障。

**Acceptance Scenarios**:

1. **Given** 重构后的代码，**When** 对照宪法「类编码约束」评审，**Then** 不存在成员函数超过 30 个的类、无 public 裸暴露成员变量、无遗漏 `override`/`const`。
2. **Given** 重构后的代码，**When** 对照宪法「命名规范」评审，**Then** 无拼音命名、无下划线开头变量、类名大驼峰、成员变量 `name_` 后缀。

---

### User Story 4 - 桥接层 pybind11 迁移与模块化 (Priority: P2)

作为项目维护者，我希望将 REPL 桥接层（`python_bridge.cpp` + `PythonConsole.cpp` 中的手写 CPython C API，共 51 处 `Py*` 调用）全部迁移到 pybind11（宪法锁定的脚本层技术栈），并以「C++ 对外虚接口类 + 一接口类一动态库」的形态重构为 `perception_console`（REPL 桥）与 `perception_py`（命令层）两个独立 `.pyd`，使桥接/命令能力按模块 `import`，从而提升可扩展性与可维护性。

**Why this priority**: 手写 C API 样板重、易错（引用计数/异常状态管理）、扩展成本高（用户反馈"灵活度不够，可扩展性不足"）；pybind11 为宪法已锁定 vendored 依赖，迁移既合规又是架构演进的基础，优先级高于普通重构收尾。

**Independent Test**: 可独立验证——仓库内 `Py*` 调用残留扫描为 0；pytest 新增桥接/命令层测试（ConsoleOut 写回调 mock、`_cpp_log` 转发 mock、`create_window` 链路 mock、占位命令抛错）与既有测试 100% 通过；REPL 冒烟：输入 `create_window("曲线图")` 返回 `'Plot_N'` 并真实创建子窗口。

**Acceptance Scenarios**:

1. **Given** 现有手写 C API 桥接层，**When** 完成 pybind11 迁移，**Then** 仓库内不残留手写 `Py*` 调用（字符串字面量除外），且 REPL 行为等价（续行判定/异常 traceback/输出重定向/日志桥契约不变）。
2. **Given** 双库拆分，**When** Python 侧 `import perception_console` / `import perception_py`，**Then** 两模块独立加载、接口可调用；`create_window` 经 `IWindowFactory` 虚接口派发至 C++ 实现真实创建子窗口并返回 `"Plot_" + 序号` id。
3. **Given** 命令层骨架，**When** 调用占位命令（`load`/`transform`/`query`/`export`），**Then** 抛 `NotImplementedError`；调用真实命令 `create_window(title)`，**Then** 契约逐条成立（title 缺省=id、TypeError、宿主未连接返回 None）。

---

### Edge Cases

- 拆分 `MainWindow.cpp` 时产生循环依赖（拆分单元互相引用）——应通过重新划分职责边界或前置声明消除，禁止为规避依赖而合并回巨文件。
- 模板/`inline` 逻辑必须留在头文件的情况（宪法允许例外）——此类内容放入 `.hpp`，不适用普通 `.h` 红线。
- 第三方库与生成代码不在本仓库范围，不强制重构。
- 重构中发现既有 bug——按「行为不变」原则记录问题，不在本特性内修复（防止范围蔓延），单独走 spec 流程。

## Requirements

### Functional Requirements

- **FR-001**: 重构后每个 `.cpp` 文件 MUST NOT 超过 800 行。
- **FR-002**: 重构后每个普通 `.h` 文件 MUST NOT 超过红线 500 行，SHOULD 不超过建议值 300 行。
- **FR-003**: 重构后每个模板 `.hpp` 文件 MUST NOT 超过 800 行。
- **FR-004**: 所有 `.h`/`.hpp` 文件 MUST 带有 `#pragma once` 头保护。
- **FR-005**: 普通 `.h` 文件 MUST 仅包含声明；业务实现 MUST 位于 `.cpp`；模板/`inline` 逻辑允许留在头文件。
- **FR-006**: 不存在成员函数超过 30 个的类（God Class）；巨类 MUST 按单一职责拆分。
- **FR-007**: 成员变量默认 `private`，外部访问通过 get/set 接口；禁止 public 裸暴露成员。
- **FR-008**: 不修改对象状态的成员函数 MUST 声明 `const`；重写虚函数 MUST 显式写 `override`。
- **FR-009**: 单个函数 MUST NOT 超过 120 行；参数超过 5 个 MUST 封装为结构体传入。
- **FR-010**: 命名 MUST 符合规范：类名大驼峰、成员变量 `name_` 后缀、无下划线开头、无拼音/模糊命名。
- **FR-011**: 重构 MUST 保持行为等价（约束 REPL 桥与既有功能路径）：不新增功能、不修改既有行为、不修复既有 bug（发现的 bug 另行立项）；US4 命令层骨架作为新增能力除外。
- **FR-012**: 重构 MUST NOT 引入新的第三方依赖；pybind11 为宪法已锁定 vendored 依赖（`third-party/pybind11-2.13.6/`，技术栈约束「脚本层：CPython 3.13 + pybind11」），桥接迁移启用该依赖属合规；双 `.pyd` 拆分为构建结构调整，均不构成新增第三方依赖。
- **FR-013**: 重构完成后 `ctest` 与 `pytest` MUST 100% 通过（`pytest` 含 US4 新增桥接/命令层测试）。
- **FR-015**: C++ 侧对 Python 命令层 MUST 提供对外纯虚接口类（`ICommandService` / `IWindowFactory`），一接口类对应一个动态库，Python 侧按模块 `import`。
- **FR-016**: REPL 桥与命令层 MUST 各自编译为独立 `.pyd`（`perception_console` / `perception_py`），输出至 exe 同级目录。
- **FR-017**: 手写 CPython C API（`Py*` 调用）MUST 全部迁移至 pybind11 API，迁移完成后仓库内不残留手写 `Py*` 调用（引导脚本等字符串字面量除外）。
- **FR-014**: 全库 MUST 一次性按 Google C++ Style Guide 执行格式对齐（缩进 2 空格、行宽 80 列等），与文件拆分同步完成；重构后不存在偏离 Google 格式的源文件。

### Key Entities

- **源代码文件（.cpp/.h/.hpp）**: 重构的原子单元；行数、`#pragma once`、声明/实现分离等约束的直接载体。
- **类与函数**: 拆分与规范对齐的对象；类职责边界、函数长度、成员可见性、`const`/`override` 约束的承担者。
- **宪法条款（v4.0.0）**: 本次重构的唯一验收标准来源。

## Success Criteria

### Measurable Outcomes

- **SC-001**: 100% 的 `.cpp` 文件不超过 800 行；100% 的普通 `.h` 文件不超过红线 500 行（建议 300 行达成率 ≥ 90%）。
- **SC-002**: 重构前后自动化测试 100% 通过且测试断言零变更（行为等价）。
- **SC-003**: 100% 头文件带 `#pragma once`；0 个普通 `.h` 含业务实现。
- **SC-004**: 重构后无 God Class（成员函数 ≤30）、无 public 裸暴露成员、无超长函数（>120 行）。
- **SC-005**: 代码评审对照宪法零违规项；重构引入的第三方依赖数为 0。
- **SC-006**: 重构完成后单个源文件评审耗时较重构前下降（可读性提升的代理指标）。
- **SC-007**: 100% 的源文件通过 Google C++ Style Guide 格式检查（缩进、行宽等）。
- **SC-008**: 仓库内手写 CPython C API 调用（`Py*`）残留为 0 处（字符串字面量除外）。
- **SC-009**: `perception_console` / `perception_py` 双 `.pyd` 可独立 `import`；`create_window` 打通链路（Python import → `IWindowFactory` 虚接口 → C++ 实现 → 真实子窗口）验证通过，REPL 契约行为不变。

## Assumptions

- 重构范围限定为本仓库 `src/` 与 `tests/cpp/` 下的自有代码；第三方库与生成代码不在范围内。
- 测试文件同样适用行数约束（红线），建议值（300 行）对测试文件从宽。
- 重构严格行为等价：禁止顺手修复 bug、添加功能或调整交互行为；发现的既有问题单独走 spec 流程。
- 行数约束以宪法 v4.0.0 为准：`.cpp` ≤ 800、普通 `.h` 建议 300/红线 500、`.hpp` ≤ 800。
- 拆分策略（按 UI 区域/按职责层等）属于实现细节，由 plan 阶段决定，spec 仅约束最终状态。
- 格式对齐采用全库一次性 reformat（用户已确认，选项 B）：与拆分同步完成；全库 reformat 产生的大规模 diff 属预期，但须与逻辑变更分次提交，便于审查与回滚。
- 依赖 `docs/architecture/` 现有流程图作为拆分与依赖分析的参考。
- US4 桥接迁移：`create_window` 绑定归属迁移至 `perception_py` 命令层模块，REPL 内经 `import perception_py` 注入 globals；契约 `contracts/python-create-window.md` 的签名与行为表不变，仅注入机制更新。
- US4 命令层骨架本期仅立结构：`ICommandService` 的 `load/transform/query/export` 为占位实现（抛 `NotImplementedError`）；`create_window` 作为真实命令打通验证；命令真实逻辑留待 M5。
- US4 接口实例跨模块传递采用 `py::capsule`（持有 `void*`），pyd 仅依赖接口头、不链接 `perception_ui` 静态库。
- `python_debug_shim.cpp` 与 `/alternatename` 链接方案保留（pybind11 内部同样使用 `Py_INCREF/Py_DECREF`，MSVC Debug 下仍需 shim）。

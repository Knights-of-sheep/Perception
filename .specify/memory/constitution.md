<!--
# 同步影响报告（Sync Impact Report）
- 版本变更：4.1.0 → 4.2.0（MINOR）
- 变更原则：
  - 「工作流规则」新增：每次新增功能或逻辑重构后，必须同步更新 `docs/` 下相关文档与根目录 `README.md`，随功能变更一并提交（2026-08-29 用户要求）
  - 「质量门禁」新增：合并前必须核对 `docs/` 与 `README.md` 已同步最新功能/重构内容；文档未同步的 PR 拒绝合并
- 新增章节：无（条款并入既有章节）
- 移除章节：无
- 治理依据：4.0.0「修改 constitution 属架构变更，必须人工评审」——本修订经用户人工评审批准（2026-08-29）
- 待办 TODO：无
-->

# Perception 宪法

## 核心原则 Core Principles

### I. 单一职责优先
模块、类、头文件、函数都坚持单一职责；出现巨类、大文件、超长函数必须重构拆分，拒绝持续堆积代码。

### II. 声明与实现分离
`.h`/`.hpp` 只放声明；普通业务实现全部放在 `.cpp`；模板、`inline` 逻辑允许留在头文件。

### III. 可维护 > 过度优化
优先可读性、可调试；性能优化必须有数据依据，禁止无依据的提前优化。

### IV. 文档与代码共生
接口、类、复杂逻辑必须写注释；注释解释“为什么”，不要复述代码行为。

### V. 安全与资源管理强制
优先 RAII、智能指针；尽量减少裸指针 `new`/`delete`；资源所有权必须明确。

### VI. Spec-Kit 流程约束
必须遵循 constitution → spec → plan → tasks → implement；新增功能先写 spec 再编码；不允许直接写代码跳过规格阶段。

### VII. GitHub 治理
所有变更走 issue 与 PR；提交信息语义清晰；环境密钥、token 禁止硬编码，全部读取 OS 环境变量。

## Perception 架构契约 Architecture Contract（NON-NEGOTIABLE）

> 本项目的特化架构原则，与「核心原则」同等强度，任何 spec/plan/代码不得违反。

- **分层核心 Layered Core**：数据必须流经 `src/core` 四个解耦层（`model/` 数据模型、`io/` 格式读取器、`process/` 数据变换、`event/` 事件总线），每层独立可测；渲染与 UI 禁止轮询或直接操作数据，必须订阅 `event/` 事件（如 `DataSetChanged`、`StructureChanged`、`SelectionChanged`）驱动重绘。
- **命令驱动与 Python 包 Command-Driven & Python Packages**：所有数据处理操作（加载、增删曲线、变换、查询、导出）必须经命令驱动层执行，UI 禁止绕过命令层直接修改核心数据；逻辑按职责拆分为可 import 的 Python 包（`perception` 整体功能、`extract` 数据计算，对标 SVisual）。纯 UI 变更（布局、主题、面板可见性、选中高亮）除外。
- **本地设计源 Local Design Source**：UI 唯一设计源是 `docs/design/mockups/`（纯本地，禁止 figma 扩展、Figma REST/MCP）；specify 前必须阅读 mockups 与 notes，plan 必须对照 mockups 安排 UI 工作，实现后必须截图对比自查。
- **数据可视化定位 Data-Visualization Scope**：对标 ParaView + SVisual，必须支持打开 1D/2D/3D 数据集并读取 ParaView、SVisual 与 VTK 生态文件格式（范围见「技术栈约束 · 文件格式范围」）；范围之外格式默认超范围，除非规范明确批准。
- **测试先行 Test-First**：必须先写测试、批准、观察失败、再实现（红-绿-重构循环）；`src/core` 必须通过 CTest（`tests/cpp/`），命令层与 Python 逻辑必须通过 pytest（`tests/python/`）。每个新增需求/功能点必须配套对应的单元测试用例——"红"必须来自该需求自身的测试，无对应测试的需求禁止进入实现。

## 技术栈约束 Technology Stack Constraints

> 锁定版本与边界，防止随时间漂移；任何变更必须先按「治理」修订宪法。

- **语言/编译**：C++17 + MSVC（VS2022）；禁止 C++20+ 语法。理由：Qt 5.15.2 预建二进制按 C++17 交付。
- **GUI**：Qt 5.15.2 Widgets + QSS + Fusion 风格；禁止外部 UI 框架、WebView。所有应用弹窗（对话框/消息框/文件对话框）MUST 使用与主界面一致的无边框自定义标题栏（图标+标题+关闭按钮，可拖拽移动），禁止系统原生标题栏。理由：深色主题 QSS 100% 生效；Dock 布局为产品根基；弹窗统一风格（004 FR-011）。
- **渲染**：VTK 9.4.1（Qt5 预建版）；VTK 仅允许出现在 `src/render/`。理由：前后端分离，UI 只持有显示抽象（如 `ICurveChart`），绝不接触原始 VTK/数据内部。
- **脚本层**：CPython 3.13 + pybind11；内嵌 REPL 与命令层共用同一 Python。新增 Python 命令必须通过 pybind11 绑定对应的 C++ 接口（`src/python/`）实现，禁止纯 Python 绕过 `src/core` 实现数据处理逻辑。理由：单一解释器保证命令行为一致；Python 仅做薄绑定与命令编排，数据处理复用 C++ 核心已测实现。
- **构建**：CMake ≥ 3.16 + Ninja + VS2022；禁止引入新构建系统。理由：配置稳定可复现（scripts/build.ps1）。
- **文件格式范围**：VTK 全部类型（.vtk/.vti/.vtp/.vtu/.vts/.vtr 等）、SVisual（.plt/.tdr）、HDF5（.h5/.hdf5）、曲线（.csv/.dat）。新格式 = 在 `io/readers/` 注册新 reader，`model/` 与消费者不变。
- **物理路径**：代码必须落入对应分层——`src/core`（model/io/process/event，禁止依赖 UI/VTK）、`src/render`（VTK 图表）、`src/ui`、`src/python`、`tests/cpp`（CTest）、`tests/python`（pytest）。
- **文本编码**：所有文本 I/O（文件解析、日志）必须使用 UTF-8。

## 架构与工程约束 Architecture & Engineering Constraints

### 头文件约束
- 全部头文件必须带 `#pragma once` 头保护。
- 普通 `.h` 文件建议上限 300 行，红线 500 行；模板 `.hpp` 放宽至 800 行；超过阈值必须拆分。
- 优先使用前置声明 `class X;`，减少不必要 `#include`，降低编译耦合。

### 类编码约束
- 类遵循单一职责；一个类成员函数不宜超过 30 个；禁止 God Class 巨类。
- 成员变量默认 `private`；外部访问提供 get/set 接口，禁止 public 裸暴露成员。
- 不修改对象状态的成员函数强制加 `const`；派生重写虚函数显式写 `override`。
- 基类如需要多态，析构函数必须 `virtual`；不需要多态不要声明 virtual 析构。
- 五特殊函数（构造、拷贝构造、拷贝赋值、移动构造、移动赋值）按需显式定义或 `delete` 禁用，不依赖编译器隐式生成。

### 函数约束
- 单函数建议 80–120 行以内；超长拆分子函数。
- 参数大于 5 个，封装结构体传入；输入参数优先使用 `const Type&`。

### 命名规范
- 类名：大驼峰；成员变量：`name_` 下划线后缀；不要使用下划线开头变量名。
- 禁止模糊命名、拼音命名；命名表达业务语义。

### 依赖约束
- 不随意引入第三方库；新增依赖必须在 spec 中说明理由。
- 密钥、令牌、配置全部从系统环境变量读取，禁止硬编码到源码。

## 安全约束 Security Constraints

- 所有文件输入使用前必须校验：路径存在性、扩展名白名单、大小限制；解析错误以类型化错误报告，绝不导致应用或 Python 解释器崩溃。
- 没有伴随总线事件不得变更数据；消费者必须容忍部分/流式更新。
- Python 命令层是外部输入的唯一信任边界；C++ 核心将所有命令参数视为不可信。

## 测试质量标准 Quality Rules

- 核心业务模块必须配套单元测试；关键路径必须覆盖。
- 新增 Python 命令行接口必须同步补充 pytest 测试用例（`tests/python/`）；无 pytest 覆盖的 CLI 接口禁止合入。
- 内存相关逻辑，必须考虑泄漏、双重释放、空指针场景。
- 多线程模块，必须明确标注线程安全属性；共享状态必须做同步保护。

## 质量门禁 Quality Gates

- 合并前必须 `ctest` 与 `pytest` 全部通过。
- 新增需求的单元测试（`tests/cpp/`）与 pytest（`tests/python/`）必须随 PR 一并提交并全部通过；PR 描述须列出覆盖的测试用例。
- 合并前必须核对 `docs/` 与 `README.md` 已同步最新功能/重构内容；文档未同步的 PR 拒绝合并。
- 在 M2/M4 里程碑必须将 UI 截图与 mockups 对比。
- 每功能一个分支；合并到 `main` 必须走 PR。
- PR 必须通过「架构与工程约束」检查：无新增超标文件（`.cpp` > 800 行、普通 `.h` > 500 行红线、`.hpp` > 800 行）。

## 工作流规则 Workflow

- 所有新功能：先产出 `spec.md`，再 `plan.md`，再 `tasks.md`，最后编码实现。
- 每次新增功能或逻辑重构后，必须同步更新 `docs/` 下相关文档（spec、设计、架构说明）与根目录 `README.md`，随功能变更一并提交。
- 修改 constitution 属于架构变更，需要人工评审，不能由 AI 自行修改。
- 当 spec/plan 与 constitution 冲突：constitution 优先级最高，需要修正 spec/plan。
- 代码评审对照本宪法执行，不符合宪法规则的实现拒绝合并。

## 禁止行为 Non-Negotiable（绝对不允许）

- 在普通 `.h` 头文件大量写业务实现代码。
- 硬编码密钥、token、密码到源代码。
- `public` 直接暴露类成员变量。
- 生成几百上千行超大类、超大函数不做拆分。
- 跳过 spec 流程直接实现功能。
- 头文件无保护宏、大量无效 include。

## 治理 Governance

- 宪法优先于其他实践；修订必须记录并获批准；修改 constitution 属架构变更，必须人工评审，禁止 AI 自行修改。
- 修订程序：提案 → SemVer 版本分析 → 检查并同步受影响模板（plan-template 的 Constitution Check 等）→ 更新 Sync Impact Report → 批准写回。
- 版本规则：MAJOR = 原则/约束增删或语义变更；MINOR = 措辞 MUST 化、新增约束章节；PATCH = 路径/日期/引用修正。
- 软规范（命名、commit 格式、分支策略、PR 流程）不在此处，见根目录 `CONTRIBUTING.md`。
- Mockup 约定见 `docs/design/mockups/README.md`。

**版本**: 4.2.0 | **批准日期**: 2026-08-23 | **最后修订**: 2026-08-29

---
name: Perception-spec-kit-宪法重写
overview: 依据 spec-kit 官方模板与社区最佳实践，重写 Perception 项目的 .specify/memory/constitution.md（新增技术栈硬约束章节、原则 MUST 化、升级 v1.2.2→v1.3.0），同步固化 plan-template.md 的 Constitution Check 条款清单，并新建 CONTRIBUTING.md 承载命名/commit 等软规范。
todos:
  - id: rewrite-constitution
    content: 重写 .specify/memory/constitution.md 至 v1.3.0：Sync Impact Report、6 条 MUST 化原则、Technology Stack Constraints（带 Rationale）、安全/质量门禁、Governance 与版本行
    status: completed
  - id: update-plan-template
    content: 更新 .specify/templates/plan-template.md 的 Constitution Check 段为可勾选 GATE 清单，引用 v1.3.0 各条款
    status: completed
    dependencies:
      - rewrite-constitution
  - id: create-contributing
    content: 新建 CONTRIBUTING.md：分支/PR、commit 格式、C++/Qt 与 Python 代码风格、构建/测试/截图命令速查
    status: completed
  - id: verify-docs
    content: 校验三份文档：版本号/日期一致、无残留占位符、引用路径有效、原则无模糊措辞
    status: completed
    dependencies:
      - rewrite-constitution
      - update-plan-template
      - create-contributing
---

## 需求概述

用户要求：全网调研 spec-kit 流程中 constitution（宪法）的规范写法与内容构成，并针对当前 Perception 项目制定 constitution 的更新方案。已确认三个决策：

1. **语言**：保留中文正文，原则标题保留英文（如 "I. Spec-First 规范先行"）
2. **plan-template 同步**：更新 `.specify/templates/plan-template.md` 的 Constitution Check 段，将需强制检查的宪法条款固化为 GATE 清单
3. **软规范分流**：新建 `CONTRIBUTING.md` 承载编码风格 / commit 格式 / branch 策略等软规范，保持宪法精简

## 交付物（3 个文件）

- **重写** `.specify/memory/constitution.md`（v1.2.2 → v1.3.0）
- **更新** `.specify/templates/plan-template.md` 的 Constitution Check 段
- **新建** `CONTRIBUTING.md`

## 调研依据（全网搜索结论）

**官方模板结构**（`constitution-template.md`）：Core Principles（5~7 条硬规则）→ Section 2（技术栈/安全约束）→ Section 3（工作流/质量门禁）→ Governance（修订程序/版本策略）→ 版本行（**Version** | **Ratified** | **Last Amended**，不可省）。

**社区 6 条最佳实践**：原则可 yes/no 验收（MUST 化）；原则 ≤7 条；技术栈放约束章节且每条带 Rationale（锁版本防时间幻觉）；改宪法必带 Sync Impact Report（HTML 注释置顶）；SemVer 多数用 MINOR；全文 ≤2 页，软规范放 CONTRIBUTING.md。

## 技术方案

本任务为治理类文档重写，不涉及代码实现。方案核心是依据 spec-kit 官方模板与社区最佳实践，重构 Perception 宪法体系，并打通「constitution → plan-template GATE → CONTRIBUTING 软规范」三层治理链路。

### 实施方案

**1. 重写 `.specify/memory/constitution.md`（v1.3.0，MINOR 升级）**

- **顶部 Sync Impact Report**（HTML 注释）：记录 1.2.2 → 1.3.0、原则 MUST 化、新增 Technology Stack Constraints 章节、关联 plan-template 已同步。
- **Core Principles（6 条，全部 MUST 化、可验收）**：
- I. Spec-First（规范先行）NON-NEGOTIABLE：无已批准 `specs/<feature>/spec.md` 不得实现
- II. Test-First（测试先行）NON-NEGOTIABLE：红-绿-重构；core 走 CTest、命令层走 pytest；合并前全绿
- III. Layered Core（分层核心）NON-NEGOTIABLE：model/io/process/event 四层解耦；事件驱动更新，UI/渲染不轮询不直改数据
- IV. Command-Driven & Python Packages（命令驱动与 Python 包）NON-NEGOTIABLE：perception/extract 包拆分；UI 不得绕过命令层
- V. Local Design Source（本地设计源）：docs/design/mockups/ 为唯一 UI 设计源；specify 前先读 preview.png + notes.md
- VI. Data-Visualization Scope（数据可视化定位）：对标 ParaView + SVisual，支持 1D/2D/3D；生态外格式默认超范围
- **新增 Technology Stack Constraints 章节**（每条带 Rationale，锁死版本防 AI 幻觉）：
- 语言/编译：C++17 + MSVC（VS2022）；禁止 C++20+ 语法（Qt5.15 预建二进制兼容）
- GUI：Qt 5.15.2 Widgets + QSS + Fusion；禁止外部 UI 框架/WebView（深色主题 QSS 100% 生效）
- 渲染：VTK 9.4.1（Qt5 预建版）；VTK 仅允许出现在 `src/render/`（前后端分离）
- 脚本层：CPython 3.13 + pybind11（内嵌 REPL 与命令层统一）
- 构建：CMake ≥ 3.16 + Ninja + VS2022；禁止引入新构建系统
- 文件格式范围：VTK 全部类型（.vtk/.vti/.vtp/.vtu/.vts/.vtr...）、SVisual（.plt/.tdr）、HDF5（.h5/.hdf5）、曲线（.csv/.dat）；新格式 = 新 reader 注册，核心不改
- 物理路径：src/core（无 UI 依赖）/ src/ui / src/render / src/python / tests/cpp / tests/python
- **精简保留**：Security Constraints（文件输入校验、事件驱动变更、Python 命令层为唯一信任边界）、Quality Gates（ctest+pytest 全绿、UI 截图对比 mockups、每功能一分支走 PR）。
- **Governance**：宪法优先；修订程序（提案 → SemVer 版本分析 → 模板对齐检查 → Sync Impact Report → 批准写回）；软规范指引到 CONTRIBUTING.md。
- **版本行**：Version 1.3.0 | Ratified 2026-08-23 | Last Amended 2026-08-26。

**2. 更新 `.specify/templates/plan-template.md` 的 Constitution Check 段**

将第 39-43 行占位 `[Gates determined based on constitution file]` 替换为可勾选 GATE 清单（引用 v1.3.0）：Spec-First 已批准 spec / Test-First 顺序与全绿 / 分层核心落点 / 命令驱动合规 / 本地设计源 / 生态范围 / 技术栈版本合规。任一不通过须修订方案或记入 Complexity Tracking。

**3. 新建 `CONTRIBUTING.md`（软规范，宪法不承载）**

- 工作流：specify → plan → tasks 规约闸门；宪法引用
- 分支与 PR：每功能一分支、PR 走 main；commit 格式 `type(scope): subject`（feat/fix/docs/refactor/test）
- C++/Qt 代码风格：PascalCase 类名、camelCase 变量、成员尾下划线、常量 kXxx、pragma once、UTF-8、信号槽惯例
- Python 风格：PEP 8、perception/extract 包结构
- 命令速查：构建（scripts/build.ps1 -Gui）、测试（ctest / pytest）、截图重生成（scripts/update_screenshots.ps1）

### 执行要点

- 只改治理类文档，不触碰任何源码/测试；不执行 speckit 命令、不做 commit/push
- 语言版本行、日期格式（YYYY-MM-DD）、Sync Impact Report 版本号三处必须一致
- 原则措辞杜绝「应该/尽量」，统一为「必须/禁止」（MUST/SHOULD 语义化）
- 全文控制在 ~150-180 行以内（≤2 页原则）
- 修改后自查：无残留占位符 `[ALL_CAPS]`、模板引用路径有效（docs/design/ui-guidelines.md 等）

# Agent Extensions

本任务为治理文档重写，探索已完成、目标文件明确，无需使用 Agent 扩展。
<!--
# 同步影响报告（Sync Impact Report）
- 版本变更：1.3.0 → 2.0.0（MAJOR）
- 变更原则：
  - 「技术栈约束 · GUI」新增强制条款：所有应用弹窗（对话框/消息框/文件对话框）MUST 使用与主界面一致的无边框自定义标题栏（图标+标题+关闭按钮，可拖拽移动），禁止系统原生标题栏（弹窗统一风格，对应 004 FR-011，2026-08-29 用户反馈）
- 新增章节：无（条款并入既有「技术栈约束 · GUI」）
- 移除章节：无
- 关联更新：specs/004-dock-layout-manager（spec.md FR-011 / data-model.md / quickstart.md）；共享工厂 `src/ui/dialog_title_bar.h`；LayoutSettingsDialog 已落地无边框风格
- 待办 TODO：无
-->

# Perception 宪法

## 核心原则 Core Principles

### I. Spec-First 规范先行（NON-NEGOTIABLE）
每个功能必须从 spec-kit 工作流 `specify` → `plan` → `tasks` 开始。`specs/<feature>/spec.md` 未获批准之前，禁止任何实现。

### II. Test-First 测试先行（NON-NEGOTIABLE）
- 必须先写测试、批准、观察失败、再实现（红-绿-重构循环），禁止先实现后补测。
- `src/core` 必须通过 CTest 运行 C++ 单元测试（`tests/cpp/`）；命令层与 Python 逻辑必须通过 pytest 驱动（`tests/python/`）。
- 合并前必须 `ctest` 与 `pytest` 全部通过，禁止跳过或忽略失败测试。

### III. Layered Core 分层核心（NON-NEGOTIABLE）
数据必须流经 `src/core` 中四个解耦的层，每层独立可测：
1. `model/` — 格式无关的数据模型（曲线数据 + 结构数据）。
2. `io/` — 注册表背后的格式读取器（新格式 = 新 reader 注册，核心不变）。
3. `process/` — 数据变换（重采样、单位换算、投影）。
4. `event/` — 事件总线（发布-订阅）。

事件驱动更新：渲染与 UI 禁止轮询或直接操作数据，必须订阅 `event/` 并在事件（如 `DataSetChanged`、`StructureChanged`、`SelectionChanged`）发生时重绘。

### IV. Command-Driven & Python Packages 命令驱动与 Python 包（NON-NEGOTIABLE）
所有数据处理操作（加载、增删曲线、变换、查询、导出）必须经命令驱动层执行；UI 禁止绕过命令层直接修改核心数据。数据处理逻辑必须按职责拆分为可 import 的 Python 包：整体功能位于 `perception` 包（对标 SVisual 的 `svisual` 库），数据计算位于 `extract` 包（对标 SVisual 的 `extract` 库）。纯 UI 变更（布局、主题、面板可见性、选中高亮）除外。
理由：单一脚本化 API 同时服务 GUI 与无头自动化；包拆分对标经实战验证的 SVisual 结构，计算与展示解耦。

### V. Local Design Source 本地设计源
UI 的唯一设计源是 `docs/design/mockups/`（纯本地方案；禁止使用 figma 扩展、Figma REST/MCP，无 `FIGMA_PAT`）。
- `specify` 之前必须阅读 `docs/design/mockups/**/preview.png` + `notes.md`，并将视觉需求写入 `specs/<feature>/spec.md`。
- `plan` 阶段必须对照 mockups 安排 UI 工作；`tasks` 阶段每个 UI 任务必须引用对应 `NNN-<界面名>/` 作为完成标准。
- 实现之后必须将截图与 mockups 对比自查，修复差异。

### VI. Data-Visualization Scope 数据可视化定位
本工具是对标 ParaView + SVisual 的数据可视化工具，必须支持打开 1D/2D/3D 数据集，必须能读取 ParaView、SVisual 与 VTK 生态的文件格式（范围见「技术栈约束 · 文件格式范围」）。不属于这些生态的格式默认超出范围，除非在规范中明确批准。
理由：定位契约驱动 `io/` 读取器与 `extract` 计算包；范围按每个 reader 保持声明式且可测试。

## 技术栈约束 Technology Stack Constraints

> 锁定版本与边界，防止随时间漂移；任何变更必须先按「治理」修订宪法。

- **语言/编译**：C++17 + MSVC（VS2022）；禁止使用 C++20+ 语法。理由：Qt 5.15.2 预建二进制按 C++17 交付。
- **GUI**：Qt 5.15.2 Widgets + QSS + Fusion 风格；禁止引入外部 UI 框架，禁止 WebView。所有应用弹窗（对话框/消息框/文件对话框）MUST 使用与主界面一致的无边框自定义标题栏（图标+标题+关闭按钮，可拖拽移动），禁止系统原生标题栏。理由：深色主题 QSS 须 100% 生效；Dock 布局为产品根基；弹窗与主界面视觉统一（004 FR-011，弹窗统一风格）。
- **渲染**：VTK 9.4.1（Qt5 预建版）；VTK 仅允许出现在 `src/render/`。理由：前后端分离，UI 只持有显示抽象（如 `ICurveChart`），绝不接触原始 VTK/数据内部。
- **脚本层**：CPython 3.13 + pybind11；内嵌 REPL 与命令层共用同一 Python。理由：单一解释器保证命令行为一致。
- **构建**：CMake ≥ 3.16 + Ninja + VS2022；禁止引入新构建系统。理由：配置稳定可复现（scripts/build.ps1）。
- **文件格式范围**：VTK 全部类型（.vtk/.vti/.vtp/.vtu/.vts/.vtr 等）、SVisual（.plt/.tdr）、HDF5（.h5/.hdf5）、曲线（.csv/.dat）。新格式 = 在 `io/readers/` 注册新 reader，`model/` 与消费者不变。
- **物理路径**：代码必须落入对应分层——`src/core`（model/io/process/event，禁止依赖 UI/VTK）、`src/render`（VTK 图表）、`src/ui`、`src/python`、`tests/cpp`（CTest）、`tests/python`（pytest）。
- **文本编码**：所有文本 I/O（文件解析、日志）必须使用 UTF-8。

## 安全约束 Security Constraints

- 所有文件输入使用前必须校验：路径存在性、扩展名白名单、大小限制；解析错误以类型化错误报告，绝不导致应用或 Python 解释器崩溃。
- 没有伴随总线事件不得变更数据；消费者必须容忍部分/流式更新。
- Python 命令层是外部输入的唯一信任边界；C++ 核心将所有命令参数视为不可信。

## 质量门禁 Quality Gates

- 合并前必须 `ctest` 与 `pytest` 全部通过。
- 在 M2/M4 里程碑必须将 UI 截图与 mockups 对比。
- 每功能一个分支；合并到 `main` 必须走 PR。

## 治理 Governance

- 宪法优先于其他实践；修订必须记录并获批准。
- 修订程序：提案 → SemVer 版本分析 → 检查并同步受影响模板（plan-template 的 Constitution Check 等）→ 更新 Sync Impact Report → 批准写回。
- 版本规则：MAJOR = 原则/约束增删或语义变更；MINOR = 措辞 MUST 化、新增约束章节；PATCH = 路径/日期/引用修正。
- 软规范（命名、commit 格式、分支策略、PR 流程）不在此处，见根目录 `CONTRIBUTING.md`。
- Mockup 约定见 `docs/design/mockups/README.md`。

**版本**: 2.0.0 | **批准日期**: 2026-08-23 | **最后修订**: 2026-08-29

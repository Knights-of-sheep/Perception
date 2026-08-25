<!--
# 同步影响报告（Sync Impact Report）
- 版本变更：1.2.1 → 1.2.2（PATCH）
- 变更原则：修正「设计约束」调色板引用路径（docs/design/design.md → docs/design/ui-guidelines.md §3.1），语义不变
- 新增章节：无
- 移除章节：无
- 待办 TODO：无
-->

# Perception 宪法

## 核心原则

### I. 规范先行（Spec-First）（NON-NEGOTIABLE）
每个功能都必须从 spec-kit 工作流的 `specify` → `plan` → `tasks` 开始。在 `specs/<feature>/` 中存在已批准的规范之前，不得进行任何实现。

### II. 本地设计源
UI 的唯一设计源是 `docs/design/mockups/`（纯本地方案；不使用 figma 扩展、不使用 Figma REST/MCP、无 `FIGMA_PAT`）。
- `specify` 之前：阅读 `docs/design/mockups/**/preview.png` + `notes.md`，并将视觉需求写入 `specs/<feature>/spec.md`。
- `plan` 阶段：对照 mockups 安排 UI 工作。
- `tasks` 阶段：每个 UI 任务都引用对应的 `NNN-<界面名>/` 作为完成标准。
- 实现之后：将截图与 mockups 对比自查，修复差异。

### III. 分层核心（NON-NEGOTIABLE）
数据流经 `src/core` 中四个解耦的层，每层可独立测试：
1. `model/` — 格式无关的数据模型（曲线数据 + 结构数据）。
2. `io/` — 注册表背后的格式读取器（可扩展；新格式 = 新 reader，核心代码不变）。
3. `process/` — 数据变换（重采样、单位换算、投影）。
4. `event/` — 事件总线（发布-订阅）。

**事件驱动更新**：渲染与 UI 绝不轮询或直接操作数据；它们订阅 `event/` 并在事件（如 `DataSetChanged`、`StructureChanged`、`SelectionChanged`）发生时重绘。

### IV. 命令驱动的数据处理与 Python 包（NON-NEGOTIABLE）
所有数据处理操作（加载、增删曲线、变换、查询、导出）都必须通过命令驱动层执行；UI 绝不能在绕过命令层的情况下直接修改核心数据。所有数据处理逻辑必须按职责拆分为可 import 的 Python 包：整体功能位于 `perception` 包中（对标 SVisual 中的 `svisual` 库），数据计算位于 `extract` 包中（对标 SVisual 中的 `extract`）。纯 UI 变更（布局、主题、面板可见性、选中高亮）除外。
理由：单一的脚本化 API 同时服务 GUI 与无头自动化；包拆分对标经实战验证的 SVisual 结构，并让计算与展示解耦。

### V. 测试先行、双重布局
- `src/core` 必须通过 CTest 运行 C++ 单元测试（`tests/cpp/`）：先写测试 → 批准 → 失败 → 再实现。严格遵循红-绿-重构循环。
- Python 测试（`tests/python/`，pytest）通过 `perception` 驱动命令层，模拟真实用法（加载 → 变换 → 查询 → 导出）。
- 合并前必须 `ctest` 与 `pytest` 全部通过。

### VI. 数据可视化工具定位（ParaView + SVisual）
本工具是对标 ParaView + SVisual 的数据可视化工具。它必须支持打开一维、二维、三维数据集，并且必须能读取 ParaView、SVisual 与 VTK 生态的文件格式（如 .vtk/.vtu/.vti/.vtr、曲线/结构格式、SVisual 原生格式）。不属于这些生态的格式默认超出范围，除非在规范中明确批准。
理由：定位契约驱动 `io/` 读取器与 `extract` 计算包；范围必须按每个 reader 保持声明式且可测试。

## 设计约束

- 深色主题，固定调色板定义在 `docs/design/ui-guidelines.md` §3.1（主窗口 / Dock / 曲线视图 / 结构视图）。
- Qt5 Widgets + Dock 布局 + QSS；不引入外部 UI 框架。
- 所有文本 I/O（文件解析、日志）均使用 UTF-8。
- 前后端分离：UI 只持有显示抽象（如 `ICurveChart`），绝不接触原始 VTK/数据内部；VTK 只存在于 `render/`。
- 数据集涵盖 1D/2D/3D；格式面必须覆盖 ParaView + SVisual + VTK 生态（见原则 VI）。

## 安全约束

- 所有文件输入在使用前都必须校验：路径存在性、扩展名白名单、大小限制、解析错误以类型化错误报告——绝不导致应用或 Python 解释器崩溃。
- 没有伴随总线事件不得变更数据；消费者必须容忍部分/流式更新。
- Python 命令层是外部输入的唯一信任边界；C++ 核心将所有命令参数视为不可信。

## 可扩展性

- 新曲线格式（.plt/.csv/.dat/...）→ 在 `io/readers/` 添加 `ICurveReader` 并注册；`model/` 与消费者不变。
- 新结构格式（.tdr/...）→ 同样添加 `IStructureReader`。
- 新 ParaView/SVisual/VTK 格式（1D/2D/3D）→ 在 `io/readers/` 添加对应 reader；`model/` 与消费者不变。
- 新变换 → 添加到 `process/`；管线可组合。
- 新数据计算 → 添加到 `extract` 包；它必须保持可 import 且命令驱动（见原则 IV）。

## 质量门禁

- 合并前必须 `ctest` 与 `pytest` 全部通过。
- 在 M2/M4 里程碑将 UI 截图与 mockups 对比。
- 每功能一个分支；合并到 `main` 走 PR。

## 治理

宪法优先于其他实践。修订必须记录并获批准。
Mockup 约定见 `docs/design/mockups/README.md`（本文件链接到那里）。

**版本**: 1.2.2 | **批准日期**: 2026-08-23 | **最后修订**: 2026-08-25

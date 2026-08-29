# Contributing to Perception

本文件承载开发流程的**软规范**（命名、commit、分支、PR、代码风格）。硬性治理规则见 [宪法](.specify/memory/constitution.md)（版本见文件头部），本文件与宪法冲突时以宪法为准。

## 工作流总览

每个功能必须通过 spec-kit 规约闸门：

```text
specify（写 spec，对照 docs/design/mockups/）
  → plan（产出 plan.md，通过 Constitution Check 的 GATE 清单）
  → tasks（拆解任务）
  → 实现（红-绿-重构，合并前 ctest + pytest 全绿）
```

- 宪法 GATE 清单：见 `.specify/templates/plan-template.md` 的 `## Constitution Check`。
- 无已批准 `specs/<feature>/spec.md` 之前，禁止实现（宪法 I）。

## 分支与 PR

- 每功能一个分支，命名 `NNN-<feature-name>`（NNN 为 spec 编号，如 `001-vtk-reader`）。
- 只从 `main` 切出；合并到 `main` 必须走 PR，PR 标题 = 功能名，描述引用对应 spec。
- 合并前自查：ctest + pytest 全绿、UI 截图与 mockups 对比无差异（涉及 UI 时）。
- 禁止直接把提交推到 `main`。

## Commit 格式

统一 `type(scope): subject`，subject 用祈使句、中文优先：

| type     | 用途                               | 示例                                   |
|----------|------------------------------------|----------------------------------------|
| `feat`   | 新功能                             | `feat(io): 注册 VTK .vtu 曲线读取器`  |
| `fix`    | 缺陷修复                           | `fix(ui): 修正深色主题下图标对比度`    |
| `docs`   | 文档（README、宪法、mockups 等）   | `docs: 更新技术栈约束至 v1.3.0`        |
| `refactor` | 重构，无行为变化                 | `refactor(core): 拆分 process 变换管线`|
| `test`   | 测试新增/调整                      | `test(python): 补充 extract 查询用例`  |
| `chore`  | 构建/脚本/依赖等杂项               | `chore: 更新 build.ps1 默认 Qt 路径`   |

- 一个 commit 只做一件事；不混合文档与代码改动。
- 涉及截图/视觉变更时在 body 说明「截图已用 scripts/update_screenshots.ps1 重生成」。

## C++/Qt 代码风格

- 语言：C++17；禁止 C++20+ 语法（Qt 5.15.2 预建二进制按 C++17 交付，宪法「技术栈约束」）。
- 头文件：一律 `#pragma once`。
- 命名：
  - 类/结构体：`PascalCase`；接口加 `I` 前缀（`IDataSet`、`ICurveReader`）。
  - 方法：`camelCase`（`addCurve`、`findByPath`）。
  - 成员变量：`snake_case_` 尾下划线（`name_`、`curves_`）。
  - 文件：`snake_case`（`core/event/event_bus.h`）；UI 层沿用现有 `MainWindow.h`/`PythonConsole.h` 惯例。
- 命名空间：`perception::core::{model,io,process,event}`、`perception::ui::theme` 等，与物理目录一致。
- 注释：中文；说明「为什么」而非「是什么」。
- Qt 惯例：
  - 信号/槽：`QObject::connect` 或 lambda；槽用 `onXxx`/lambda，禁止裸 `Q_OBJECT` 宏滥用。
  - 生命周期：`QObject` 父子树管理；跨线程用信号队列，禁止直接改 UI。
  - 字符串：界面文本用 `QString`，文件路径传给 core 时转 `std::string`（UTF-8）。
- 分层边界：`src/core` 禁止 `#include` 任何 Qt Widgets/VTK 头；VTK 仅限 `src/render/`（宪法「技术栈约束」）。
- 辅助类组织：只服务单个大部件的私有辅助类可留在其 `.cpp` 匿名命名空间（如 MainWindow.cpp 内的 Dock 交互类）；但当辅助类超过约 200 行、或出现第二个消费者（如多个对话框复用同一标题栏）时，必须拆到独立文件（`src/ui/frameless/`、`src/ui/dock/` 等），不得继续堆进大文件。

## Python 代码风格

- PEP 8；`perception` 包（整体功能，对标 `svisual`）与 `extract` 包（数据计算，对标 `extract`）职责分离（宪法 IV）。
- 数据处理必须命令驱动，禁止在 UI 侧直接改核心数据。
- 新增计算逻辑进 `extract`，保持可 import、纯函数优先。
- 注释中文；类型标注（type hints）在新代码中必须。

## 构建 / 测试 / 截图命令速查

```powershell
# 完整构建（GUI）+ 全部测试（需已装 VS2022/Qt 5.15.2/VTK 9.4.1，默认路径固化）
.\scripts\build.ps1 -Gui -UnitTests -Pytest

# 仅编译（无 GUI，core/python 层）
.\scripts\build.ps1

# 只跑测试（构建后）
ctest --test-dir build -C Release --output-on-failure
python -m pytest tests\python -v

# 重生成 docs/screenshots 下全部截图（需先构建 GUI，产物 bin\Release\perception.exe）
powershell -ExecutionPolicy Bypass -File scripts\update_screenshots.ps1
```

- 涉及 UI/主题改动后，合并前必须重生成截图并与 `docs/design/mockups/` 对比。
- 图标/主题相关改动可跑 `scripts/check_icons.py`、`scripts/check_theme_contrast.py` 自检。

## 文档

- README 与 `docs/design/ui-guidelines.md` 为长期文档，功能/约束变化时同步更新。
- Mockup 约定见 `docs/design/mockups/README.md`。

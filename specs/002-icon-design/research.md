# Research: 图标体系设计决策

**Feature**: 002-icon-design | **Branch**: `002-icon-design` | **Date**: 2026-08-25

> 本文档解决 Technical Context 中的全部未知项，产出可执行的图标设计决策。信息来源：
> 项目唯一设计系统规范 `docs/design/ui-guidelines.md`（Token 色板、几何、交互规范）+ 通用深色主题图标设计惯例 + 对标软件 ParaView / SVisual 的公开界面风格。全部决策以「可检查」为第一标准，服务于 `contracts/` 中的验收契约。

---

## 1. 图标整体风格（决策 1）

- **Decision**: 采用**线性描边风格（line/outline style）**：2px 描边、圆头端点（round cap）与圆角连接（round join）、无填充（透明底）。
- **Rationale**: ① 深色主题界面（VS Code Dark+ 体系）的行业主流选择，与 `ui-guidelines.md` §1「深色主题、扁平化、克制」一致；② 16px 小尺寸下线性图标比实心/彩色图标更清晰、不糊；③ 与 SVisual 中性克制的工具风格对齐；④ 单色线性风格最利于状态变换（透明度/亮度调节即可表达五态）与色盲可辨识（FR-005）。
- **Alternatives considered**: 实心填充风格（对比强但 16px 下易糊、状态区分难）、彩色多色风格（ParaView 浅色工具栏风格，与深色主题和「语义色只用于状态表达」原则冲突，被否）。

## 2. 图标色板（决策 2）

- **Decision**: 图标**默认单色**，主色 = `FG_TEXT`（`#D4D4D4`）；仅「语义性图标」（成功/警告/错误类，若有）允许使用语义色 `SUCCESS`/`WARNING`/`DANGER`；高亮/选中态使用 `ACCENT`（`#0A84FF`）。**不新增任何色值**，全部复用 `ui-guidelines.md` §3.1 Token。
- **Rationale**: 宪法「固定调色板唯一来源」+ ui-guidelines §3.5「语义色只用于状态表达，不用于装饰」；单一事实源避免花斑。
- **Alternatives considered**: 为图标单独定义图标专用色板（引入新 Token，破坏单一事实源，被否）。

## 3. 尺寸档位与绘制网格（决策 3）

- **Decision**: 三档尺寸：**16px / 24px / 32px**（spec Assumptions 已确认）。绘制网格：**基础网格 16×16**，安全区（safe zone）居中 **12×12**，关键图形占位 ≤ 14×14；24px/32px 按同一网格整数倍放大（24 = 16×1.5、32 = 16×2），保证视觉比例一致。描边宽度按档位微调：16px=2px、24px=2.5px、32px=3px（半像素取整，避免渲染模糊）。
- **Rationale**: 工具栏图标基准 16px（ui-guidelines §3.4）+ 侧边栏/菜单可放大档；整数倍网格保证「同一语义图标在不同尺寸档位视觉比例一致」（spec Edge Cases）。
- **Alternatives considered**: 只做 16px 单档（不满足菜单/侧边栏/应用场景）；响应式多网格（过度设计，三档 + 整数倍放大足够）。

## 4. 五态表现规则（决策 4）

- **Decision**: 五态通过**颜色 + 透明度 + 背景**组合表达，不依赖单一通道：
  | 状态 | 图标色 | 透明度 | 底/容器 |
  |---|---|---|---|
  | 正常 | FG_TEXT | 100% | 透明 |
  | 悬停 | FG_TEXT | 100% | BG_CONTROL 圆角 4px 底 |
  | 按下 | FG_TEXT_WEAK | 100% | BG_CONTROL 加深 + 内缩 1px |
  | 禁用 | FG_TEXT_DISABLED | 40% | 透明 |
  | 选中 | ACCENT | 100% | SELECTION_BG 圆角 4px 底 |
- **Rationale**: FR-004（五态深色主题清晰可辨）+ FR-005（不依赖单一通道：禁用态是颜色+透明度双重信号；选中态是颜色+背景双重信号）。
- **Alternatives considered**: 仅改透明度（禁用/选中难区分）、仅改颜色（色盲不友好），被否。

## 5. 应用主图标（决策 5）

- **Decision**: 应用主图标为**品牌图标**，与功能按钮图标共享同一线性视觉语言，但允许使用 `ACCENT`（`#0A84FF`）作为品牌主色 + `FG_TEXT` 辅助色；图形语义建议：**「感知/观测」主题**（如：视窗/十字准星/曲线波形组合——呼应"数据可视化工具"定位与产品名 Perception）。
- **Rationale**: FR-011（与功能按钮图标共享视觉语言与品牌色；适配深色主题与窗口/任务栏）。ACCENT 是 ui-guidelines 定义的品牌主色（唯一主色），应用图标是品牌载体，可例外使用品牌色（同属现有 Token，不违反单一事实源）。
- **Alternatives considered**: 与按钮图标完全同色系单色（品牌辨识度弱）；引入专属品牌新色（违反 Token 单一来源），被否。
- **交付格式**: SVG 源 + 多尺寸 PNG（16/24/32/48/64/128/256）+ Windows `.ico`（含 16/24/32/48/64/128/256 多分辨率），满足 spec SC-006（16px 极小尺寸可辨）。

## 6. Qt 图标资源集成（决策 6）

- **Decision**: 图标以 **SVG 为源 + PNG 位图为运行格式**，经 **`.qrc` 资源系统**打包。目录：`src/ui/theme/icons/`（`actions/` 功能按钮、`app/` 应用图标），资源前缀 `/perception/icons/`。窗口图标：`main.cpp` 中 `mainWindow.setWindowIcon(QIcon(":/perception/icons/app/app-icon.ico"))`。
- **Rationale**: ① Qt 5.15 内置 `svg` 模块可加载 SVG，但 `QIcon` 对 `.ico` 多尺寸支持更稳、`.ico` 为 Windows 窗口图标标准；② `.qrc` 编译进可执行文件，免安装路径依赖，与既有 `theme.qrc` 同机制；③ 小尺寸抗锯齿：PNG 由 SVG 渲染导出，避免 Qt SVG 运行时缩放模糊。
- **Alternatives considered**: 运行时加载文件系统 SVG（依赖安装目录，部署脆弱，被否）；仅 SVG 不导出位图（16px 渲染质量不可控，被否）。**注意**：SVG 渲染器是否随 Qt 发布需在 tasks 阶段确认（`Qt5Svg` 依赖），否则全 PNG 方案兜底。
- **风险登记**: ~~Qt 安装可能不含 `Qt5Svg.dll`；若缺，功能图标全部改用预渲染 PNG（16/24/32 三档 × 五态），应用图标用 `.ico`。该取舍由 tasks 阶段按构建环境验证后定。~~ **已验证（T002，2026-08-25）**：`D:/Qt/5.15.2/msvc2019_64` 下 `bin/Qt5Svg.dll` 与 `lib/cmake/Qt5Svg` 均存在 → **SVG 源 + PNG 运行格式**方案成立，无需全 PNG 兜底。

## 7. 命名与映射规则（决策 7）

- **Decision**: 图标命名采用 **kebab-case 语义命名**，格式 `<功能区>-<功能>[-<变体>]`，如 `file-open`、`view-zoom-in`、`analysis-extract`、`animation-play`。图标-功能映射以 **YAML 清单**为唯一来源（`contracts/icon-function-map.md` 内嵌或独立 `icon-map.yaml`），字段：`icon_id / semantic / states / sizes / category`。
- **Rationale**: FR-006（命名与功能语义一一对应、无重复与歧义）；YAML 机器可校验（tasks 阶段可加覆盖检查脚本），人工可读。
- **Alternatives considered**: 中文命名（Qt 资源路径/文件系统兼容性差，被否）；UUID 命名（无语义，评审不可读，被否）。

## 8. 评审验收方法（决策 8）

- **Decision**: 验收采用**双评审制**：① 规范符合性评审——评审者对照 `contracts/conformance-checklist.md` 逐项勾选（风格/网格/尺寸/色板/状态/命名/映射完整）；② 语义识别抽样——抽取 ≥10% 图标（最少 10 枚）请 3 名评审者盲测功能语义，正确率 ≥90%（SC-003）。mockups 中展示全部图标五态。
- **Rationale**: SC-002（不符合项 0）、SC-003（≥90% 语义识别）、SC-004/SC-005/SC-006 全部需要可重复的评审程序；无自动化视觉手段，人工清单是唯一可靠验证。
- **Alternatives considered**: 纯自动像素对比（无黄金样本，mockups 尚未建立基线，被否）；仅口头评审（不可复现，被否）。

---

## 未决项（Deferred）

| 项 | 说明 | 移交 |
|---|---|---|
| ~~SVG 渲染依赖（Qt5Svg）可用性~~ | **已决（T002，2026-08-25）**：Qt5Svg.dll 与 CMake 包存在，SVG→PNG 渲染方案确认，无兜底路径 | —— |
| 应用图标具体图形语义 | 品牌图形方向（视窗/曲线/十字准星）在规范契约中给出 1 个推荐方案，允许设计评审调整 | 设计执行阶段 |
| ActionRegistry 落地 | 图标挂接依赖 §7.2 动作集中管理（后续需求），本功能仅交付资源 + 挂接接口位 | 后续「按钮布局」需求 |

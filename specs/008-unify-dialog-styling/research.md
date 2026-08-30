# Research: 弹窗样式层次化统一

**Date**: 2026-08-30 | **Feature**: [spec.md](spec.md) | **Plan**: [plan.md](plan.md)

## 现状调研（代码事实）

1. **弹窗背景与主界面同色**：`theme_template.qss` 中 `QMainWindow, QDialog, QMessageBox { background-color: @windowBg@; }`——所有弹窗背景 = 主窗口背景 = 用户反馈的"隐身"问题根因。
2. **主题体系**：25 套主题（15 深色 / 7 浅色 / 3 高对比），色板为 `ThemeColors` 结构（4 层背景 token：windowBg / panelBg / controlBg / viewBg），`ThemeManager::renderQss` 硬编码 token 数组逐项替换模板占位符。**色板中无弹窗层背景 token**。（校正：早期版本写"10 浅色"，实为 7，`theme_catalog.h` 头注释同误，已一并修正）
3. **弹窗实现现状**：
   - `FramelessDialog`（帮助/关于）：QDialog + `buildDialogTitleBar` + 富文本 + OK 按钮
   - `ThemedFileDialog`（打开/导出/日志路径）：QDialog + `buildDialogTitleBar` + 内嵌 QFileDialog
   - `LayoutSettingsDialog`（布局设置）：QDialog + `buildDialogTitleBar` + 表单，objectName `layoutSettingsDialog`
   - `QMessageBox`（警告/确认，共 5 处：`MainWindow.cpp` 2 处、`log/log_settings_controller.cpp` 3 处）：**系统原生标题栏，未改造**
4. **标题栏**：`buildDialogTitleBar` 共享工厂（30px 高、`titleBarRow` 对象名、`@panelBg@` 背景 + 底部 1px 分隔线），三个弹窗组件已复用，视觉一致。
5. **子窗口基类确认**：`SubwindowView : QFrame`、`SubwindowContainer : QWidget`、`DockTitleBar : QWidget`——**均非 QDialog**。故 QSS 对 `QDialog` 的全局规则（背景/边框）只影响 4 类弹窗，不会误伤子窗口/浮动面板。
6. **对比度校验**：`theme_catalog.h` 注释提及 `build/_theme_check.py` WCAG 校验，但该脚本在工作区不存在（未实现/已移除）；25 套色板的对比度目前无自动化校验。本 feature 以 C++ 单测补上 dialogBg 相关校验。

---

## 设计决策

### R1: 弹窗背景层（dialogBg）的取值策略

- **Decision**: 在 `ThemeColors` 末尾新增 `QColor dialogBg` 语义 token（默认无效色），由新函数 `deriveDialogBg(windowBg, text, family)` 在渲染时派生：
  - **Dark family**：`dialogBg = windowBg` 的 HSL lightness **+8 个百分点**（上限 70）
  - **Light family**：`dialogBg = windowBg` 的 HSL lightness **−8 个百分点**（下限 30）
  - **High Contrast family**：不派生，`dialogBg == windowBg`，层次靠 1px 高对比边框（`@border@` 为纯白/纯黑）达成
  - **对比度保护（实现期修订）**：提亮/压暗后若 `contrast(dialogBg, text) < 4.5:1`（WCAG AA，FR-003 可读性硬约束），自动逐级回调派生量至达标；保护降级后亮度差可能 < 8pt，但仍须 ≥ 2pt 保有可感知层次。触发案例：solarized-dark 的 `windowBg` 与 `text` 亮度接近（base CR≈5.6），提亮 8pt 将对比度压至 3.53——保护回调后以可读性优先（同 spec Edge Cases 精神）
  - `ThemeManager::renderQss` 增加 `@dialogBg@` token 替换；`dialogBg.isValid()` 为假时使用派生值
- **Rationale**:
  - 单一实现点自动覆盖 25 套主题，保证层次差异**恒定达标**（SC-001 亮度差 ≥ 8pt 天然满足深/浅族），避免 25 处人工取值造成的"有的主题明显、有的不明显"的不一致
  - `QColor` 在 HSL 上做明度偏移**保持色相与饱和度**，天然满足"同一配色家族、有搭配感"（FR-003）
  - 新增主题零成本（不用记得补第 6 个背景色）
  - 高对比主题遵循其"弱化装饰、纯黑白"设计精神，以边框区分而非灰色底
- **Alternatives considered**:
  - *25 套色板显式逐套取值*：token 最直白，但 25 处人工挑选必然引入亮度差不一致（违反 SC-001 一致性），且每新增主题都要记补；放弃
  - *复用 `panelBg` 作为弹窗背景*：零新 token，但深色 `panelBg #252526` 与 `windowBg #1E1E1E` 亮度差仅约 3pt，达不到"明显不突出"的层次要求；放弃
  - *复用 `controlBg`（VS Code 对话框背景 = 控件层）*：深色 `#3C3C3C` 与 windowBg 差 11pt 达标，但浅色主题 `controlBg #EDEDED` 与 `windowBg #F5F5F5` 差约 4pt 不达标，且语义错位（控件层不应充当容器层）；放弃

### R2: QMessageBox 统一方案

- **Decision**: 新建 `themed_message_box.h/.cpp`，类 `ThemedMessageBox : QDialog`（`Qt::Dialog | Qt::FramelessWindowHint`），复用 `buildDialogTitleBar` 共享标题栏；布局为「状态图标 + 文本 + 按钮行」（图标用 `QStyle::standardIcon` 随主题 palette）；工厂函数签名：
  ```cpp
  QMessageBox::StandardButton showThemedMessageBox(
      QWidget* parent, QMessageBox::Icon icon, const QString& title,
      const QString& text,
      QMessageBox::StandardButtons buttons = QMessageBox::Ok,
      QMessageBox::StandardButton defaultButton = QMessageBox::Ok);
  ```
  替换 `MainWindow.cpp` 2 处与 `log_settings_controller.cpp` 3 处 `QMessageBox::warning/question` 调用。
- **Rationale**: QMessageBox 无 `setTitleBarWidget` 公开 API（Qt 5.15），无法注入共享标题栏，故无法在原生 QMessageBox 上实现宪法要求的统一标题栏；自建组件与 `FramelessDialog` 同构（无边框 + 共享标题栏工厂），可完全复用主题样式与拖拽行为，且返回 `QMessageBox::StandardButton` 保持调用方语义不变（`Yes/No` 判定逻辑零改动）。
- **Alternatives considered**:
  - *`QMessageBox` + `Qt::FramelessWindowHint` + QSS 定制*：可去系统标题栏，但 QMessageBox 内部布局由私有实现管理，无法插入自定义标题栏 widget 到顶部，标题栏仍不统一；放弃
  - *扩展现有 `FramelessDialog` 支持图标与多按钮*：可行但会把"信息弹窗"与"交互消息框"两种语义揉进一个类，且帮助/关于的富文本正文与消息框的单行文本形态不同；保持两组件（同背景/同标题栏/同按钮样式）更清晰，改动面更小；放弃合并
  - *继续用 QMessageBox 但忽略标题栏*：违反宪法「技术栈约束 · GUI」与 FR-005；放弃

### R3: QSS 弹窗层规则的施加方式

- **Decision**: 在 `theme_template.qss` 中拆分原合并规则：
  ```css
  QMainWindow { background-color: @windowBg@; }
  QDialog { background-color: @dialogBg@; border: 1px solid @border@; }
  QMessageBox { background-color: @dialogBg@; }   /* 兼容未替换的残留调用 */
  ```
  顶层无边框弹窗的 1px 边框绘制在窗口客户区边缘，形成明确边界（FR-006）。
- **Rationale**: 已确认应用内 `QDialog` 子类仅 4 类弹窗（FramelessDialog / ThemedFileDialog / LayoutSettingsDialog / 新增 ThemedMessageBox），全局 `QDialog` 规则安全无副作用；三个既有弹窗组件**零代码改动**即自动获得 dialogBg 背景与边框，改动面最小。
- **Alternatives considered**:
  - *按对象名逐个选择器（`QDialog#layoutSettingsDialog` 等）*：需给每个弹窗加对象名且未来新增弹窗易漏；放弃
  - *自定义属性 `QDialog[perceptionRole="dialog"]`*：需修改全部弹窗构造函数 setProperty，且对象名方案已足够安全；放弃

### R4: 测试策略

- **Decision**: 新增 `tests/cpp/theme_dialog_layer_test.cpp`（CTest），校验：
  - 25 套主题：dialogBg 与 text 的 WCAG 对比度 ≥ 4.5:1（FR-003 可读性硬约束，保护保证）
  - 25 套主题：Dark/Light family 的 `|L(dialogBg) − L(windowBg)| ≥ 8pt` 为默认目标（SC-001）；对比度保护触发时允许 < 8pt，但仍须 ≥ 2pt（可感知层次）
  - HC family：dialogBg == windowBg 且 border 与 windowBg 对比度 ≥ 7:1（SC-001 HC 分支）
  - 派生函数边界：clamp 上限/下限、无效色/nullptr/未知 family、色相/饱和度保持
- **Rationale**: 项目无 UI 自动化视觉测试框架（现有 CTest 为 core 层）；派生函数是纯函数，可直接以单测验证"层次恒定达标"，把 SC-001 的可测量部分自动化；视觉呈现仍由 quickstart 手动矩阵验收（宪法 M2/M4 截图对比精神）。
- **Alternatives considered**: 引入 QtTest 视觉回归框架——超出本 feature 范围、无现有基础设施；放弃，仅以派生校验 + 手动视觉矩阵覆盖。

### R5: 文档同步（宪法「工作流规则」）

- **Decision**: 更新 `docs/design/ui-guidelines.md` §3.1 分层色板：新增第 5 层 `BG_DIALOG`（弹窗背景）及派生规则说明（深色亮一档 / 浅色暗一档 / 高对比靠边框）；§4.1 控件矩阵「窗口」行补充弹窗层说明。
- **Rationale**: 宪法要求"每次新增功能或逻辑重构后，必须同步更新 docs/ 下相关文档与根目录 README.md"；ui-guidelines 是设计系统唯一事实源，dialog 层属于色板体系扩展，必须落档。README 不涉及主题/弹窗细节（README 为项目总览），核对后无需改动。
- **Alternatives considered**: 仅改代码不落档——违反宪法质量门禁；放弃。

### R6: 排列模式语义修订（computeGrid）

- **Decision**: `LayoutManager::computeGrid` 按 2026-08-30 澄清修订（FR-008/009/010）：
  - `mode == Row`：恒返回 `{rows=n, cols=1}`（一列多行 N×1，忽略 maxRows/maxCols/gridDirection）
  - `mode == Column`：恒返回 `{rows=1, cols=n}`（一行多列 1×N，同上）
  - `mode == Grid`：按 `gridDirection` 决定**约束轴**与比例网格方向：
    - `gridDirection == Row`：`maxRows > 0` → `rows = min(n, maxRows)`、`cols = ceil(n/rows)`；否则比例（`cols = round(√n)`、`rows = ceil(n/cols)`）
    - `gridDirection == Column`：`maxCols > 0` → `cols = min(n, maxCols)`、`rows = ceil(n/cols)`；否则比例（`rows = round(√n)`、`cols = ceil(n/rows)`）
  - `windowCount <= 0` 返回空网格（不变）
- **Rationale**: 用户明确"按行排=一列多行、按列排=一行多列"；约束轴 = 优先级轴消除"Grid 同时显示行+列"的歧义（FR-010）；比例网格公式与既有已测逻辑零变化，仅约束分支按轴收敛。
- **Alternatives considered**:
  - *保留"约束优先于模式"（Row/Column 也响应约束）*：与 FR-008（一列多行/一行多列）直接冲突；放弃
  - *Row 模式允许 maxRows>0 约束行数*：一列多行语义下（行数=窗口数）自相矛盾；放弃
  - *Grid 同时响应双轴约束*：与"互斥显隐、隐藏值保留不参与计算"的 Edge Case 冲突；放弃

### R7: 约束轴辅助函数 `constraintAxis`

- **Decision**: `layout_manager.h` 新增枚举 `enum class ConstraintAxis { None, Row, Column }` 与 `ConstraintAxis LayoutManager::constraintAxis(const LayoutConfig&) const`：
  - `mode == Row || mode == Column` → `None`（无约束控件）
  - `mode == Grid && gridDirection == Row` → `Row`（行数受限）
  - `mode == Grid && gridDirection == Column` → `Column`（列数受限）
  弹窗显隐规则与 `computeGrid` 内部约束分支**共用此判定**（单一事实源），单测覆盖全组合。
- **Rationale**: 将 UI 显隐规则从弹窗提取为纯函数——Test-First 可测（宪法「测试先行」），弹窗与计算逻辑不会因各自维护判定而漂移。
- **Alternatives considered**: 弹窗内联写 if/else 判断——重复逻辑、不可单测；放弃。

### R8: 弹窗界面重构（分段按钮组 + 显隐 + 恢复默认）

- **Decision**: `LayoutSettingsDialog` 重构：
  - 排列模式：下拉框 → **分段按钮组**（`QButtonGroup` + 3 个 checkable 按钮 Grid / By Row / By Column，互斥单选、点击即切换），落实 004 FR-011 原要求（FR-012）
  - 控件显隐矩阵（以 `constraintAxis` 驱动）：
    | 模式 | 优先级 radio | 最大行数 | 最大列数 | 间隙宽度 | 保持相同宽高 | 恢复默认 |
    |---|---|---|---|---|---|---|
    | By Row | 隐藏 | 隐藏 | 隐藏 | 显示 | 显示 | 显示 |
    | By Column | 隐藏 | 隐藏 | 隐藏 | 显示 | 显示 | 显示 |
    | Grid | 显示（互斥） | 按轴：Row 显示 | 按轴：Column 显示 | 显示 | 显示 | 显示 |
  - 隐藏侧的 spinbox 值**保留**（`current()` 仍带出，但 computeGrid 不参与计算；Edge Case「隐藏一侧的当前值保留但不参与布局计算」）
  - 「恢复默认」按钮：一键将全部控件重置为 `LayoutConfig` 默认值（Grid / Row 优先 / 无约束 / 不保持相同宽高 / 间隙 6）并发射 `configChanged`（FR-014）
  - 提示文案随控件调整（"Constraints 0–10; Gap 0–50"）
- **Rationale**: 分段按钮组是 004 已写明的未落地要求；显隐规则经 `constraintAxis` 收敛避免弹窗堆叠"行+列+优先级"（用户反馈的混乱点）；恢复默认解决"改乱难回退"。
- **Alternatives considered**: 保留下拉框仅做显隐修正——用户明确要求分段按钮组（Q2-A）；放弃。恢复默认做成关闭时"放弃修改"——与"修改即生效"（FR-011 精神）矛盾；放弃。

### R9: 实时排列预览

- **Decision**: 新增 `LayoutPreviewWidget : QWidget`（`src/ui/subwindow/layout_preview_widget.{h,cpp}`）：
  - 输入：`setPreviewCount(int n)`（可见子窗口数）与当前 `LayoutConfig`
  - `paintEvent` 自绘：以固定预览可用区调用 `LayoutManager::computeGrid` + `cellRect` 计算各 cell 矩形，绘制为色块（背景 `@viewBg@`、cell `@panelBg@` + 1px `@border@` 边框、间隙按 `spacing`），**几何逻辑零新实现**（全复用已测 LayoutManager）
  - 任一配置变更（模式/优先级/约束/间隙）即 `update()` 重绘；`n=0` 画空态提示、`n=1` 画单格铺满（Edge Cases）
  - 计数来源：`SubwindowContainer` 新增 `int visibleSubwindowCount() const`；`MainWindow::openLayoutSettings` 打开时传入，并订阅容器 `subwindowCountChanged` 保持同步
- **Rationale**: 所见即所得（FR-013）；预览不复制布局算法，避免"预览与真实排布不一致"的漂移风险；绘制轻量（N≤20 矩形）。
- **Alternatives considered**: 用 `QWidget::render` 真实子窗口缩略图——需访问子窗口渲染内容、重量级且与视图状态耦合；放弃。用现成 QListView/表格示意——几何与真实排布不符；放弃。

### R10: 间隙宽度与容器

- **Decision**: 弹窗新增「Gap width」spinbox（0–50，默认 6，单位 px）绑定 `LayoutConfig::spacing`：
  - 容器 `SubwindowContainer::relayout` 已 `grid_->setSpacing(cfg_.spacing)` 消费该字段，**无需改动**；`cellSize`/`cellRect` 公式已扣除 `(cols-1)*spacing`，几何与 QGridLayout 一致
  - 004 data-model 原「spacing = 常量 4、不对用户暴露」修订为**可配置**（默认以代码现状 6 为准，004 文档记载 4 与实现不符，已在 008 spec Assumption 注明）
- **Rationale**: spacing 全链路已贯通（computeGrid 计数不依赖 spacing，几何依赖），仅缺 UI 控件；0–50 覆盖常见 0/4/6/8/10 取值且与 `cellSize` 整数除法兼容。
- **Alternatives considered**: 维持常量——用户明确要求可配置；放弃。

### R11: 跨 feature 落盘（004 同步修订）

- **Decision**: 随 008 一并提交对 004 的语义同步：`specs/004-dock-layout-manager/spec.md`（US2 排布示例、FR-004~006 语义、FR-011 约束显隐、FR-015 间隙可配置）、`data-model.md`（LayoutConfiguration 字段语义、spacing 行）、`tests/cpp/layout_manager_test.cpp`（Row/Column 期望值改单行/单列、Grid 约束轴用例、constraintAxis、间隙 cellSize 用例）。
- **Rationale**: 008 spec Assumption 明示"跨 feature 落盘在实施阶段完成"；不修订则 004 文档与 008 目标行为冲突、旧单测对行/列模式全红。
- **Alternatives considered**: 仅改 008 不动 004——文档冲突 + 测试失败，违反宪法质量门禁；放弃。

---

## 影响面汇总

| 层 | 改动 | 风险 |
|---|---|---|
| `src/ui/theme/theme_types.h` | +1 字段（dialogBg） | 无（聚合初始化末尾追加，缺省 QColor()） |
| `src/ui/theme/theme_manager.{h,cpp}` | renderQss 签名与 token 数组 | 中（需同步调用方） |
| `src/ui/theme/theme_dialog_layer.{h,cpp}` | 新派生函数 | 低（纯函数，单测覆盖） |
| `src/ui/theme/theme_template.qss` | QDialog/QMessageBox 规则拆分 | 低（子窗口非 QDialog 已确认） |
| `src/ui/themed_message_box.{h,cpp}` | 新组件 + 工厂 | 中（新增 UI 组件，手动验证） |
| `src/ui/theme/file_dialog_qss.{h,cpp}` | 内嵌 QFileDialog 主题注入 | 低（随弹窗层同源） |
| `MainWindow.cpp` / `log_settings_controller.cpp` | 5 处 QMessageBox 替换 + 预览计数传递 | 低（按钮语义不变；计数同步简单） |
| `src/ui/subwindow/layout_manager.{h,cpp}` | computeGrid 语义修订 + constraintAxis | **中**（核心语义变更，单测先红后绿覆盖） |
| `src/ui/subwindow/layout_settings_dialog.{h,cpp}` | 重构（分段按钮/显隐/间隙/恢复默认/预览联动） | 中（UI 交互多，手动验证矩阵） |
| `src/ui/subwindow/layout_preview_widget.{h,cpp}` | 新增预览组件 | 低（纯绘制，复用已测几何） |
| `src/ui/subwindow/subwindow_container.{h,cpp}` | +visibleSubwindowCount() | 无 |
| `tests/cpp/layout_manager_test.cpp` | 语义修订 | 中（期望值全面核对） |
| `specs/004-dock-layout-manager/{spec,data-model}.md` | 语义同步 | 无（文档） |
| `src/ui/CMakeLists.txt` / `tests/cpp/CMakeLists.txt` | 源文件注册 | 低 |
| `docs/design/ui-guidelines.md` | §3.1/§4.1 同步 | 无 |

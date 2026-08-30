# Contract: 弹窗样式契约（Dialog Style Contract）

**Date**: 2026-08-30 | **Feature**: [../spec.md](../spec.md) | **Data Model**: [../data-model.md](../data-model.md)

> 本契约定义弹窗样式体系对外承诺的**不变语义**，实现必须满足；验收按「Success Criteria 映射」逐项核对。

## 1. 弹窗背景层 token 契约（`dialogBg`）

- **语义**：`dialogBg` 是主题色板中弹窗背景的语义色，与主窗口背景（`windowBg`）为**同一配色家族、不同亮度层次**。
- **值来源**：默认由 `deriveDialogBg(windowBg, text, family)` 派生；`ThemeColors::dialogBg` 显式赋值时优先使用显式值（未来定制钩子）。
- **不变式**：
  - Dark/Light family：`|L(dialogBg) − L(windowBg)| ≥ 8`（HSL lightness 百分点）为**默认目标**；对比度保护触发（`windowBg` 与 `text` 亮度接近的低对比色板，如 solarized-dark）时可降级，但须 ≥ 2pt 保有可感知层次
  - 全部 family：`contrast(dialogBg, text) ≥ 4.5:1`（WCAG AA，FR-003 可读性硬约束，优先于亮度差）
  - 全部 family：`dialogBg` 与 `windowBg` 色相一致（同配色家族）
- **生效范围**：所有 QDialog 类弹窗（FramelessDialog / ThemedFileDialog / LayoutSettingsDialog / ThemedMessageBox）；**不得**作用于主窗口、子窗口（`SubwindowView`/`SubwindowContainer` 等 QFrame/QWidget）、Dock 面板。
- **ThemedFileDialog 内嵌 QFileDialog 主题化**：Qt 5.15 的 `QFileDialog`（`DontUseNativeDialog`）内部 `QFileWidget` 在构造中 `setStyleSheet` 设置自身硬编码样式表，会**屏蔽应用级 QSS**——导致侧边栏 / Look-in 行 / 文件视图 / 底部按钮 / 文件名下拉等区域停留在浅色原生观感，与弹窗层视觉割裂。修复：ThemedFileDialog 构造时按当前主题色板由 `buildFileDialogQss(colors, dialogBg)` 生成完整样式表并 `fileDialog_->setStyleSheet(...)` 显式注入，dialogBg 与全局 QSS 同源（`deriveDialogBg` 派生），控件色全部取自 `ThemeColors` 同一语义 token（ui-guidelines §3/§4.1）。视觉验证：深色主题下侧边栏与文件视图呈 BG_PANEL，Open 按钮呈 ACCENT 主按钮态，文件名/类型下拉与底部 Cancel 按钮呈 BG_CONTROL。

## 2. 弹窗外观契约

每个弹窗 MUST 具备以下三层结构，且层间语义固定：

| 区域 | 样式 | 来源 |
|---|---|---|
| 背景 | `@dialogBg@`（L4 弹窗层） | QSS `QDialog { background-color: @dialogBg@; }` |
| 边界 | 1px `@border@` 边框（HC 族以此为主要层次手段） | QSS `QDialog { border: 1px solid @border@; }` |
| 标题栏 | `@panelBg@` 背景 + 底部 1px `@border@` 分隔线 + 图标 + 标题 + 关闭按钮（可拖拽） | `buildDialogTitleBar` 共享工厂 |

**约束**：
- 标题栏高度固定 30px，图标/标题字号/关闭按钮位置在所有弹窗间一致（SC-003）
- 弹窗内控件（按钮/输入框/下拉/列表）继续使用主界面同款主题 QSS（`@controlBg@` 等），不引入第二套控件样式（FR-007）
- 主题热切换时弹窗全部区域即时跟随（FR-002）

## 3. 消息框 API 契约（`showThemedMessageBox`）

```cpp
// ui/themed_message_box.h
namespace perception { namespace ui {
QMessageBox::StandardButton showThemedMessageBox(
    QWidget* parent,
    QMessageBox::Icon icon,
    const QString& title,
    const QString& text,
    QMessageBox::StandardButtons buttons = QMessageBox::Ok,
    QMessageBox::StandardButton defaultButton = QMessageBox::Ok);
}}
```

- **前置**：无（`parent` 可空；空父窗口时按 QDialog 默认行为）
- **行为**：模态显示无边框消息框（自定义标题栏 + 图标 + 文本 + 按钮行），阻塞至用户操作
- **返回**：用户点击按钮对应的 `QMessageBox::StandardButton`；用户以关闭按钮/X/Esc 关闭时返回 `defaultButton`
- **兼容**：调用方 `ret == QMessageBox::Yes` 判定逻辑不变；按钮集合与默认按钮语义与 QMessageBox 静态接口一致
- **替换范围**：`MainWindow.cpp`（导出失败 ×2）与 `log_settings_controller.cpp`（日志目录打不开 / 清除日志确认 / 清除失败）共 5 处调用点全部替换，**禁止残留 QMessageBox 静态调用**（FR-005）

## 4. 弹窗类型全覆盖契约

`data-model.md §3` 的 10 类弹窗 MUST 全部满足「第 2 节外观契约」。验收时逐类核对，任一遗漏即违反 FR-001/FR-004/FR-005。

## 5. Success Criteria 映射

| SC | 验收方式 | 对应契约项 |
|---|---|---|
| SC-001 亮度差 ≥ 8%（深/浅）或 HC 边框 | 自动：`ctest -R theme_dialog_layer`；手动：视觉 | §1 不变式 / §2 边界 |
| SC-002 主题切换 100% 随动、无配色冲突 | 手动：25 套抽样全家族切换逐类弹窗 | §1 / §2 |
| SC-003 标题栏逐项一致（≥5 类弹窗） | 手动：并排比对 | §2 标题栏约束 |
| SC-004 1 秒内指出弹窗边界（5 人 4 通过） | 手动：体验者测试 | §2 背景+边界 |
| SC-005 功能行为不回归 | 手动 + 回归测试 | §3 API 契约 / data-model §5 状态机 |

## 6. 范围外（Out of Scope）

- OS 级 / 第三方系统对话框
- 非对话框浮层（ToolTip / 下拉菜单 / Dock 浮动窗口）——已有既有样式，不在本契约范围
- 弹窗阴影/投影等装饰增强（当前以边框满足层次，不承诺投影）

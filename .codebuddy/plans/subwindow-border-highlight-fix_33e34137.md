---
name: subwindow-border-highlight-fix
overview: 修复子窗口边框选中高亮渲染缺陷：QStyleSheetStyle 对 QFrame 边框宽度被 lineWidth 钳制为 1px、内容区子控件覆盖导致 @panelBg@ 背景不显示。改为 paintEvent 自绘边框（未选中 1px / 选中 2px，颜色 @border@）+ 透明化内容区以显示 panelBg。
todos:
  - id: rewrite-paint
    content: SubwindowView 改为 QPainter 自绘背景与边框，content_ 透明，监听主题切换刷新
    status: completed
  - id: fix-qss
    content: theme_template.qss 选中规则改回 2px solid @border@ 并注明自绘接管
    status: completed
  - id: verify-snapshot
    content: 构建并走快照验证：选中 2px、未选中 1px、颜色均 @border@、panelBg 可见
    status: completed
    dependencies:
      - rewrite-paint
      - fix-qss
---

## 用户需求

修复子窗口（SubwindowView）两个渲染缺陷，并回答根因：

**根因分析（已确认，与 spacing 间隙无关）**

- 边框宽度被压成 1px：SubwindowView 继承 QFrame（默认 lineWidth=1），QStyleSheetStyle 渲染 QFrame 边框的几何宽度受 `QFrame::frameWidth()` 钳制，QSS border 只贡献颜色。选中 4px 红色只画 1px 即为此因；未选中 1px 恰好等于 lineWidth 所以显示正常。
- @panelBg@ 不显示：内容区 `content_`（QWidget#subwindowContent，background: viewBg）是不透明子控件，覆盖了父控件背景，导致 paintEvent 绘制的 panelBg 被遮住。
- spacing=6 仅为网格间隙（spec 要求的暗沟），与 QSS 渲染无关，保持不变。

## 验收标准

- 未选中子窗口：1px 边框，颜色 @border@。
- 选中子窗口：2px 边框，颜色仍为 @border@（不加粗之外任何强调色，禁用 @accent@/红色诊断）。
- 子窗口内容区显示 @panelBg@ 背景（不再露出容器 viewBg）。
- 主题热切换后边框/背景颜色随主题刷新。
- 移除临时诊断日志（红色 4px、qInfo 输出）。

## 技术栈

- 既有项目：Qt 6 / C++（QWidget + QSS 主题体系，ThemeManager 渲染 theme_template.qss @token@）
- 边框/背景色来源：`ThemeManager::current()->colors.border` 与 `colors.panelBg`（theme_catalog.h 的 ThemeColors 结构，MainWindow 已有同模式用法）

## 实现方案

**核心策略**：SubwindowView::paintEvent 改为纯 QPainter 自绘，摆脱 QStyleSheetStyle 对 QFrame 边框宽度的 lineWidth 钳制；内容区 content_ 设透明以透出 panelBg。

- paintEvent 自绘：
- `p.fillRect(rect(), panelBg)` 绘制背景。
- 按 `property("selected")` 选择 1px / 2px 的 QPen(@border@)，沿 rect 内侧绘制矩形边框；2px 用 `QRectF::adjusted(half, half, -half, -half)` 内缩，避免被 widget 边界裁剪。
- content_ 设 `Qt::WA_TranslucentBackground`，QSS 的 viewBg 规则不再遮挡 panelBg；标题栏（@dockTitleBg@）与按钮不变。
- 主题热切换刷新：SubwindowView 重写 `event()`，监听 `QEvent::PaletteChange` / `ApplicationPaletteChange` 时调 `update()`（沿用 MainWindow::event 既有模式），否则切换主题后自绘颜色不刷新。
- QSS：选中规则从 `4px solid #FF0000` 改回 `2px solid @border@`，并注释说明实际绘制由 paintEvent 自绘接管（QSS 仅作语义参考，避免双重来源困惑）。
- MainWindow 选中 lambda（setProperty + unpolish/polish + update）保持不变，自绘路径只需 update() 触发重绘。
- 移除 paintEvent 中的诊断日志（qInfo + s_log）。

## 性能与风险

- 自绘仅 fillRect + drawRect，O(1) 每帧，无额外布局/开销。
- 风险点：主题切换后不自绘刷新 → 必须监听 PaletteChange 事件（已纳入方案）；QSS 与代码双来源 → 注释明确自绘接管。
- 不触碰 subwindow_container.cpp（spacing=6 暗沟为 spec 要求）与其他无关模块，控制爆炸半径。

## 目录结构

```
src/ui/subwindow/
├── subwindow_view.h      # [MODIFY] 增加 event() override 声明；更新 paintEvent 注释（自绘）
├── subwindow_view.cpp    # [MODIFY] paintEvent 改 QPainter 自绘（panelBg 填充 + 1/2px @border@ 边框）；
│                         #          content_ 设 WA_TranslucentBackground；新增 event() 监听主题切换调 update()；
│                         #          移除诊断日志；include theme_manager.h / theme_catalog.h
src/ui/theme/
└── theme_template.qss    # [MODIFY] 第 372-374 行选中规则改回 `border: 2px solid @border@`，去红色诊断，加注释
```

## 关键代码结构

paintEvent 核心实现（接口级，2px 内缩防裁剪）：

```cpp
void SubwindowView::paintEvent(QPaintEvent*) {
    const auto* t = ThemeManager::current();
    const QColor bg = t->colors.panelBg;      // @panelBg@
    const QColor bc = t->colors.border;       // @border@
    const bool sel = property("selected").toBool();
    const qreal w = sel ? 2.0 : 1.0;          // 选中 2px / 未选中 1px
    QPainter p(this);
    p.fillRect(rect(), bg);
    p.setPen(QPen(bc, w));
    p.setBrush(Qt::NoBrush);
    p.drawRect(QRectF(rect()).adjusted(w / 2.0, w / 2.0, -w / 2.0, -w / 2.0));
}
```

event() 主题刷新（沿用 MainWindow::event 模式）：

```cpp
bool SubwindowView::event(QEvent* e) {
    if (e->type() == QEvent::PaletteChange || e->type() == QEvent::ApplicationPaletteChange)
        update();
    return QFrame::event(e);
}
```
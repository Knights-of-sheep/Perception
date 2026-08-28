# Research: Multi-Screen Maximize

**Branch**: `005-multi-screen-maximize` | **Date**: 2026-08-28

对应 `plan.md` Technical Context 中所有未知项的调研结论。每条含 Decision / Rationale / Alternatives considered。

---

## R1: 根因——`WM_GETMINMAXINFO` 中 `QWidget::screen()` 解析不可靠

**Context**: `MainWindow::nativeEvent` 的 `WM_GETMINMAXINFO` 分支（`src/ui/MainWindow.cpp` 1912–1924 行）用 `screen()->availableGeometry()` 计算无边框最大化几何（`ptMaxPosition`/`ptMaxSize`）。

**Decision**: 根因确认为 `screen()` 在 native 消息时机解析目标屏幕不可靠——对无边框（`Qt::FramelessWindowHint`）顶层窗口，`QWidget::screen()` 依赖 Qt 内部的 screen 关联缓存（`windowHandle()->screen()`），该缓存随窗口移动异步更新；当用户在 `WM_GETMINMAXINFO` 消息中请求最大化时，缓存可能仍指向主屏或滞后的旧屏幕。此时 `ptMaxPosition` 被设置为错误屏幕的工作区坐标，Windows 按该坐标执行最大化 → 窗口落到用户视线之外的屏幕（表现为"窗口消失"）。若目标屏幕位于虚拟桌面负坐标区（副屏在主屏左侧/上方），错误更明显。

**Rationale**: 该分支是 Windows 无边框窗口最大化的唯一可靠入口（Qt 对 frameless 窗口的 `showMaximized()` 也走 `WM_GETMINMAXINFO`）；错误点定位在目标屏幕解析而非几何计算本身。

**Alternatives considered**:
- 依赖 `QWidget::screen()`（现状）——已知在多屏 frameless 场景不可靠，正是缺陷根因
- 用 `QCursor::pos()` 判定屏幕——最大化入口来自按钮点击/双击，光标位置通常等于窗口位置，但双击 HTCAPTION 时光标在标题栏、窗口中心可能在另一屏（跨屏大窗口），不精确

---

## R2: 目标屏幕解析方案——基于窗口几何中心 + `screenAt`

**Decision**: 新增纯函数（`src/ui/window_geometry.{h,cpp}`）：

```cpp
// 返回窗口当前"主要所在"屏幕：优先窗口几何中心所在屏，其次 screen()，最后主屏
const QScreen* resolveTargetScreen(const QWidget* w);
```

实现要点：
1. 首选 `QGuiApplication::screenAt(frameGeometry().center())`（基于窗口真实几何，不依赖 Qt 内部缓存，native 消息中可用）
2. fallback：`w->screen()`；再 fallback：`QGuiApplication::primaryScreen()`
3. 最大化几何 = 目标屏 `availableGeometry()`（含副屏负坐标场景：直接用该屏幕的虚拟桌面坐标，天然正确）

**Rationale**: `screenAt()` 是 Qt 提供的静态命中查询，基于实际像素坐标解析，跨屏拖拽后立即正确；`frameGeometry().center()` 定义"窗口主体所在屏幕"，与 FR-004（跨屏按主体所在屏幕）语义一致；`screen()`/`primaryScreen()` 仅作兜底，避免空指针。

**Alternatives considered**:
- 监听 `moveEvent` 手动维护"当前屏"成员——维护成本高、易与 native 时机脱节
- 使用 Win32 `MonitorFromWindow`/`MonitorFromRect`——与 Qt 体系重复，且 DPI 处理需自管

---

## R3: 无边框窗口的最大化/恢复几何管理

**Context**: `toggleMaximize()`（MainWindow.cpp 1003–1009 行）直接 `showMaximized()/showNormal()`，无显式 normal geometry 保存。Qt 对 `Qt::FramelessWindowHint` 窗口在最大化→恢复时 normal geometry 的保存/恢复不可靠（已知问题：frameless 窗口不维护 Windows 侧 normal rect，`showNormal()` 可能回到 (0,0) 或错误位置/屏幕）。

**Decision**: 在 `MainWindow` 增加：
- 成员 `QRect normalGeometry_`（会话内，非持久化）
- 在 `changeEvent(QEvent::WindowStateChange)` 中：当窗口进入最大化且前态非最大化时，记录 `normalGeometry_ = frameGeometry()`（即本次进入最大化前的普通态几何）；窗口恢复普通态时，若 `normalGeometry_` 有效则 `setGeometry(normalGeometry_)` 再 `showNormal()`
- `toggleMaximize()` 保持"最大化/恢复"双入口语义不变（FR 与按钮/双击/快捷键共用），仅补齐几何管理

**Rationale**: 显式保存/恢复使 US2（恢复回原位原尺寸、不跳屏）可测、可保证；不依赖 Qt frameless 的内部恢复行为。记录时点在进入最大化瞬间（而非每次 state 变化），避免恢复路径上覆盖已保存值。

**Alternatives considered**:
- 依赖 Qt 内置恢复（现状）——US2 场景下恢复位置不可靠
- 在最大化前记录 `saveGeometry()`/`restoreGeometry()`——引入 QSettings 序列化格式，本功能无持久化需求，过重

---

## R4: 多屏幕与 DPI 处理

**Context**: SC-003 覆盖混合 DPI（不同屏幕缩放不同）与显示器热插拔。

**Decision**:
- 启用 Qt 高 DPI：现有应用已按 `Qt5Cored.dll` + 高 DPI 环境运行；本方案几何全部使用 Qt 设备无关像素（DIP），`availableGeometry()` 已按所在屏幕 DPI 返回正确 DIP 值，无需额外换算
- 显示器断开场景：`screenAt()` 可能返回 `nullptr`（目标屏已不存在）→ fallback 链保证取到剩余可见屏幕；若 Windows 已自动移动窗口，`frameGeometry().center()` 自然指向新位置所在屏
- 不引入 `SetProcessDpiAwarenessContext` 等自管逻辑（Qt 5.15 已处理 per-monitor DPI）

**Rationale**: 全部走 Qt DIP 坐标空间，避免自管 DPI 带来的跨屏换算错误；fallback 链覆盖热插拔。

**Alternatives considered**:
- 自管 Win32 DPI 换算——重复劳动且易与 Qt 坐标体系冲突

---

## R5: 测试策略（宪法 II Test-First）

**Decision**:
- `window_geometry.{h,cpp}` 保持纯函数（仅依赖 `QScreen`/`QWidget` 类型，不依赖 MainWindow 实例），使最大化几何计算可在 `tests/cpp` 直接单测
- 新增 `tests/cpp/window_geometry_test.cpp`：
  - 用虚拟屏幕矩形集（两屏横排/竖排/不对称/负坐标）模拟 `QGuiApplication::screens()` 返回值 → 断言目标屏解析与最大化几何（位置/尺寸/负坐标）正确
  - 断言恢复几何逻辑（normalGeometry_ 保存/还原）的状态转换
- 多屏端到端（真实双屏拖拽/最大化/恢复/热插拔）为手动验证，场景清单在 `quickstart.md`（对应 SC-001..003）
- GUI 测试注册在 `PERCEPTION_BUILD_GUI=ON` 下（与现有 `layout_manager_test` 同模式）

**Rationale**: 纯函数单测保证核心逻辑在多屏组合下可回归；真实多屏硬件场景在 CI 不可复现，故端到端以文档化手动清单交付（与 SC 一一对应）。

**Alternatives considered**:
- 全量自动化 GUI 测试（QTest 驱动真实多屏）——CI 无多屏硬件，不可行
- 仅手动验证——违反宪法 II，且核心几何逻辑可单测，无理由放弃

---

## 汇总

| 决策点 | 结论 | 对应需求 |
|--------|------|----------|
| 根因 | `screen()` 在 native 时机解析错误屏幕 | FR-001/002/004 |
| 目标屏解析 | `screenAt(frameGeometry().center())` + fallback 链 | FR-001/002/004/006 |
| 最大化几何 | 目标屏 `availableGeometry()`（DIP，负坐标天然正确） | FR-001/006 |
| 恢复几何 | `changeEvent` 记录 normal geometry，恢复时显式还原 | FR-003/006 |
| 热插拔/DPI | fallback 链 + Qt DIP，不引入自管逻辑 | FR-005/006 |
| 测试 | 纯函数单测 + 手动多屏场景清单 | SC-001..004 |

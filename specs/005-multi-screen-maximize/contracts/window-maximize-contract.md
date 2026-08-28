# Contract: Window Maximize (Windows 窗口管理器交互)

**Branch**: `005-multi-screen-maximize` | **Date**: 2026-08-28

主窗口为无边框（frameless）Qt 窗口。本契约描述最大化/恢复行为与 Windows 窗口管理器之间的交互约定，以及内部目标屏幕解析契约。对应 spec FR-001..006。

## 1. 触发入口

| 触发源 | 行为 |
|--------|------|
| 标题栏"最大化"按钮点击 | 切换最大化/恢复（`toggleMaximize`） |
| 标题栏双击 | 切换最大化/恢复（`WM_NCLBUTTONDBLCLK` → `toggleMaximize`） |
| 快捷键（若有） | 切换最大化/恢复 |

所有入口语义一致：当前普通态 → 最大化；当前最大化态 → 恢复。

## 2. 最大化几何契约（`WM_GETMINMAXINFO`）

**输入**: Windows 在窗口即将最大化时发送 `WM_GETMINMAXINFO`。

**处理（必须全部满足）**:

1. 目标屏幕解析：`screenAt(frameGeometry().center())`，fallback 依次为 `screen()` → `primaryScreen()`（禁止直接用 `screen()` 作唯一依据）
2. 目标屏幕几何与工作区：目标屏幕完整几何（`QScreen::geometry()`，含任务栏/系统区）与其可用工作区（`QScreen::availableGeometry()`，设备无关像素 DIP）
3. 填充（Windows `MINMAXINFO` 坐标语义，research R6）：`ptMaxPosition` 为**相对目标屏幕左上角**的偏移，非虚拟桌面绝对坐标：
   `ptMaxPosition = (avail.x() - geom.x(), avail.y() - geom.y())`；`ptMaxSize = (avail.width(), avail.height())`（无边框窗口无系统边框，直接取工作区尺寸）
4. 处理结果：返回 `true` 并置 `*result = 0`

**输出**: Windows 按 `MINMAXINFO` 将窗口铺满目标屏幕工作区（不遮挡任务栏）。

**违反后果**: 目标屏解析错误 → 窗口最大化到错误屏幕/屏幕外；`ptMaxPosition` 误用虚拟桌面绝对坐标（如副屏在主屏右侧时填 1920）→ 窗口被推到目标屏之外，副屏最大化"消失"（2026-08-29 缺陷根因）；`ptMaxSize` 缺失 → 无边框窗口可能覆盖任务栏。

## 3. 恢复几何契约

**输入**: 窗口处于最大化态，用户触发恢复。

**处理**:

1. 若记录的 `normalGeometry` 有效：`setGeometry(normalGeometry)` 后 `showNormal()`
2. 记录时机：`changeEvent(WindowStateChange)` 中，窗口由普通态进入最大化瞬间（仅此一次，避免恢复路径覆盖）

**输出**: 窗口回到最大化前的位置、尺寸与所在屏幕。

**违反后果**: 依赖 Qt frameless 内部恢复 → 恢复位置不可靠（可能回到 (0,0) 或错误屏幕），违反 FR-003。

## 4. 边界契约

| 场景 | 约定行为 |
|------|----------|
| 窗口跨越两块屏幕边界 | 按 `frameGeometry().center()` 所在屏幕（主体所在屏）最大化（FR-004） |
| 目标屏幕已断开（热插拔） | `screenAt` 返回空 → fallback 到剩余可见屏幕；窗口不得保持不可见（FR-005） |
| 任务栏在屏幕顶部/左侧 | `ptMaxPosition` 偏移非零（`avail - geom`），窗口仍铺满工作区不遮任务栏（R6 用例） |
| 混合 DPI | 全部使用 DIP 坐标；`availableGeometry` 已含所在屏 DPI 换算（FR-006） |
| 连续最大化→恢复往返 | 每次恢复结果一致，无累计漂移（FR-006） |

## 5. 非契约项（明确不保证）

- 不改变窗口的 normal 态默认位置（启动位置逻辑不变）
- 不介入全屏（`showFullScreen`）路径
- 不持久化窗口布局（无磁盘写入）

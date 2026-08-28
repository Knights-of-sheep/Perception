# 契约: LayoutManager 接口与布局设置界面

> 内部接口契约：供 `tests/cpp` 单测与 `src/ui` 实现使用。对应 spec FR-004~012、FR-016~018。

## 1. LayoutManager（纯 C++，仅依赖 QtCore，可无窗口单测）

```cpp
namespace perception::ui {

enum class LayoutMode { Row, Column, Grid };

struct LayoutConfig {
  LayoutMode mode = LayoutMode::Grid;
  int        maxRows = 0;   // 0 = 未设置（不限/自动）
  int        maxCols = 0;   // 0 = 未设置（按可用宽度自动）
  bool       sameSize = false;
  int        spacing = 4;   // FR-015 / SC-009 常量
};

struct GridLayout {
  int rows = 0;
  int cols = 0;
};

class LayoutManager {
 public:
  // FR-004~008：返回满足约束的行列数（容量不足时保持 cols ≤ maxCols、rows 继续增长，FR-014）
  GridLayout computeGrid(int windowCount, const LayoutConfig&) const;

  // FR-009/FR-015：统一 cell 尺寸（available 减去间隙后按网格均分；sameSize=false 时按比例等分）
  QSize cellSize(const QSize& available, const GridLayout&, const LayoutConfig&) const;

  // 第 index 个子窗口的矩形（含间隙偏移），供容器布局使用
  QRect cellRect(const QSize& available, int index,
                 const GridLayout&, const LayoutConfig&) const;
};

}  // namespace perception::ui
```

**约束**：`maxRows`/`maxCols` 非法值（0 外的 <1 或 >10、非整数）由 UI 层负责校验回退（FR-012），`LayoutManager` 仅接收合法值。`windowCount == 0` 时返回空网格（rows=0, cols=0），不崩溃。

## 2. SubwindowContainer（`src/ui/subwindow/subwindow_container.{h,cpp}`）

- `addSubwindow(SubwindowView*)`：加入并重排（FR-001/002）。
- `setLayoutConfig(const LayoutConfig&)`：立即重排（FR-011 即时生效）。
- `setMaximized(SubwindowView*)` / `exitMaximized()`：FR-016（容器内独占，其余隐藏）。
- `setFullscreen(SubwindowView*)` / `exitFullscreen()`：FR-017（经 MainWindow 协调面板隐藏与 re-parent 覆盖）。
- 单子窗口时铺满可用范围，无多余留白（Edge Cases）。
- 最大/全屏进出不销毁子窗口，保留内容与视图状态（FR-019）。

## 3. SubwindowView（`src/ui/subwindow/subwindow_view.{h,cpp}`）

- 普通 QWidget 子控件，**无系统标题栏**（FR-018），顶部自定义标题区。
- 标题区：标题文本、双击 = 最大化、按钮（最大化 / 全屏 / 关闭）、右键菜单（最大化 / 全屏 / 退出 / 关闭）。
- 单击子窗口任意区域 = 选中（spec Assumptions），选中高亮可见。
- 内容区为占位渲染视图（FR-003），viewState 随子窗口存续（FR-019）。

## 4. 布局设置界面（`layout_settings_dialog.{h,cpp}`）

| 控件 | 类型 | 行为 |
|------|------|------|
| 排列模式 | 三选一（单选/下拉） | 按行 / 按列 / 网格，选择即生效（FR-004~006、FR-011） |
| 最大列数 | SpinBox 0–10（0 = 自动） | 网格换行上限（FR-007）；非法回退（FR-012） |
| 最大行数 | SpinBox 0–10（0 = 不限） | 网格换列上限（FR-008）；非法回退（FR-012） |
| 保持相同宽高 | CheckBox | FR-009/FR-010，修改即生效 |

- 打开时展示当前各值（US5 场景 1）；修改即时生效，无需确认按钮（FR-011、US5 场景 2）。
- 深/浅主题下控件清晰可读（US5 场景 3）；沿用现有 QSS 主题（`theme_template.qss`）。

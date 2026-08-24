# Contract: 图标设计规范（可检查定义）

**Feature**: 002-icon-design | **Version**: 1.0.0 | **Source**: research.md 决策 1–6、data-model.md §2.1

> 本契约是图标**视觉风格的唯一可检查定义**。凡交付图标必须满足以下每一条；任一不满足即计为符合性清单不符合项。

## 1. 风格（Style）

| 条款 | 要求 |
|---|---|
| S-01 | 图标采用**线性描边风格**：无填充（`fill="none"`），仅描边（`stroke`）表达图形 |
| S-02 | 描边端点 `stroke-linecap="round"`；描边连接 `stroke-linejoin="round"` |
| S-03 | 图形语义必须与功能语义一致（识别测试依据），不使用与功能无关的装饰元素 |

## 2. 网格与尺寸（Grid & Size）

| 条款 | 要求 |
|---|---|
| G-01 | 基础绘制网格 16×16；图形必须落在安全区 12×12 内（四边留 ≥2px）；关键图形外接 ≤14×14 |
| G-02 | 尺寸档位为 `16 / 24 / 32`；24=16×1.5、32=16×2 整数倍放大，保证视觉比例一致 |
| G-03 | 描边宽度：16px=2px、24px=2.5px、32px=3px |
| G-04 | 同语义图标在不同档位必须保持一致的视觉比例与辨识度 |

## 3. 色板（Palette）

| 条款 | 要求 |
|---|---|
| P-01 | 图标仅使用 `ui-guidelines.md` §3.1 Token 色值，**禁止任何新增色值** |
| P-02 | 默认单色：`FG_TEXT #D4D4D4`；语义性图标允许使用 `SUCCESS`/`WARNING`/`DANGER` |
| P-03 | 高亮/选中态使用 `ACCENT #0A84FF`；禁用态使用 `FG_TEXT_DISABLED #6E6E6E` |
| P-04 | 应用图标品牌色：`ACCENT`（主）+ `FG_TEXT`（辅），且必须来自现有 Token |

## 4. 交互状态（States）

| 条款 | 状态 | 颜色 | 透明度 | 容器 |
|---|---|---|---|---|
| T-01 | normal | FG_TEXT | 100% | 透明 |
| T-02 | hover | FG_TEXT | 100% | BG_CONTROL 圆角 4px 底 |
| T-03 | pressed | FG_TEXT_WEAK | 100% | BG_CONTROL 加深 + 图形内缩 1px |
| T-04 | disabled | FG_TEXT_DISABLED | 40% | 透明 |
| T-05 | selected | ACCENT | 100% | SELECTION_BG 圆角 4px 底 |

| 条款 | 要求 |
|---|---|
| T-06 | disabled 与 selected 不得仅靠单一通道（仅颜色或仅透明度）区分 |
| T-07 | 每枚功能图标必须提供全部五态（直接资源或规范可派生） |

## 5. 命名（Naming）

| 条款 | 要求 |
|---|---|
| N-01 | 图标命名 kebab-case：`<功能区>-<功能>[-<变体>]`，如 `file-open`、`view-zoom-in` |
| N-02 | 图标名与功能语义一一对应，全库唯一，无同义词重复（语义如 `open`/`load` 二选一） |

## 6. 应用图标（App Icon）

| 条款 | 要求 |
|---|---|
| A-01 | 应用图标与功能按钮图标共享同一线性视觉语言（S-01/S-02 适用） |
| A-02 | 适配深色主题；在 `BG_VIEW #161616` 背景上辨识清晰 |
| A-03 | 交付物：SVG 源 + PNG（16/24/32/48/64/128/256）+ Windows `.ico`（含 16px 分辨率） |
| A-04 | 16px 档仍保留品牌辨识度（SC-006） |

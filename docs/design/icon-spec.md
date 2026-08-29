# 图标设计规范（icon-spec）

> **Feature**: 002-icon-design | **Status**: 正式规范（**v2.0.0**，风格修订：实心填充 → Material Icons Outlined 线性圆角描边，替换 58 枚功能图标）
> 本文件是**图标设计的唯一规范文档**（面向实现者），与契约 `specs/002-icon-design/contracts/icon-style-spec.md` 完全一致、不得矛盾（单一事实源）。
> 可检查定义以契约条款号（S/G/P/T/N/A）为准；交付验收执行 `specs/002-icon-design/contracts/conformance-checklist.md`。
> 修订记录：
> - 2026-08-29 **v2.0.0**：风格由"实心填充"调整为 **Material Icons Outlined 线性圆角描边**（素材来源 Google Material Icons，Apache-2.0 免费许可，Figma 官方社区版 / 官方 GitHub 仓库，本地化为仓库资产，无在线运行时依赖）；基础网格 16→24；描边宽度档位 16=1.5 / 24=2 / 32=2.5。色板 / 状态 / 命名条款不变。对应 `specs/007-replace-icon-set`。
> - 2026-08-25 v1.1.0：风格由"线性描边"调整为"实心填充"（对齐 SVisual 拟物参考）。

---

## 1. 目标

为 Perception 全部功能按钮与应用主图标提供**统一、可逐项核验**的视觉规范：线性圆角描边、深色适配、语义唯一、五态清晰。

## 2. 风格（Style）

| 条款 | 要求 |
|---|---|
| S-01 | **线性圆角描边风格**：主体以描边（`stroke-linecap="round"`、`stroke-linejoin="round"`）表达，默认 `FG_TEXT #D4D4D4`，无填充（`fill="none"`）；`fill` 仅允许 `none` 或 Token 白名单色（个别辅助元素） |
| S-02 | 使用描边（`stroke`）时：端点 `stroke-linecap="round"`、连接 `stroke-linejoin="round"`；各档位描边宽度下限见 G-03 |
| S-03 | 图形语义与功能语义一致，不使用与功能无关的装饰元素 |

**素材说明**：58 枚功能图标采用 Google **Material Icons Outlined** 官方图形（24×24 原生网格，Apache-2.0），归一化后本地化为仓库资产；渲染时以 `FG_TEXT #D4D4D4` 单色表达，适配深/浅/高对比主题。

## 3. 网格与尺寸（Grid & Size）

| 条款 | 要求 |
|---|---|
| G-01 | 基础绘制网格 **24×24**（Material 原生）；图形必须落在安全区 18×18 内（四边留 ≥3px）；关键图形外接 ≤21×21 |
| G-02 | 尺寸档位 `16 / 24 / 32`；按 24 网格等比缩放（16=24×2/3、32=24×4/3），保证视觉比例一致 |
| G-03 | 描边宽度：**16px=1.5px、24px=2px、32px=2.5px**（16px 档下限 ≥1.5px） |
| G-04 | 同语义图标在不同档位保持一致的视觉比例与辨识度 |

**模板**：绘制必须从 `src/ui/theme/icons/templates/template-{16,24,32}.svg` 复制，模板含安全区辅助线。

## 4. 色板（Palette）

色板唯一来源 `docs/design/ui-guidelines.md` §3.1 Token，**禁止新增色值**（conformance C-1）。

| 条款 | 要求 |
|---|---|
| P-01 | 仅使用 ui-guidelines §3.1 Token 色值，禁止任何新增色值 |
| P-02 | 默认单色 `FG_TEXT #D4D4D4`；语义性图标允许 `SUCCESS #4EC9B0` / `WARNING #CCA700` / `DANGER #F14C4C` |
| P-03 | 高亮/选中态 `ACCENT #0A84FF`；禁用态 `FG_TEXT_DISABLED #6E6E6E` |
| P-04 | 应用图标品牌色 `ACCENT`（主）+ `FG_TEXT`（辅），必须来自现有 Token |

**Token 全表**（可被图标引用）：`BG_WINDOW #1E1E1E`、`BG_PANEL #252526`、`BG_CONTROL #3C3C3C`、`BG_VIEW #161616`、`FG_TEXT #D4D4D4`、`FG_TEXT_WEAK #9D9D9D`、`FG_TEXT_DISABLED #6E6E6E`、`BORDER #454545`、`BORDER_WEAK #3F3F3F`、`ACCENT #0A84FF`、`SELECTION_BG #094771`、`SUCCESS #4EC9B0`、`WARNING #CCA700`、`DANGER #F14C4C`。

## 5. 交互状态（States）

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

> 实现：五态以 **SVG 源（normal）+ QSS/Palette 派生**为主；状态容器（hover/selected 底）由 UI 层提供，不在图标内绘制。

## 6. 命名（Naming）

```
<功能区>-<功能>[-<变体>]
```

- `<功能区>`：`file / edit / view / analysis / animation / tools`（对应 spec 对标功能清单六大类别）
- 仅小写字母、数字、连字符；不得含空格、下划线、中文
- 全库唯一；禁止同义词并存（`open`/`load` 二选一）
- 禁用前缀：`menu-*`、`btn-*`、`icon-*`

示例：`file-open`、`view-zoom-in`、`analysis-extract`、`animation-play`、`tools-settings`。

## 7. 应用图标（App Icon）

| 条款 | 要求 |
|---|---|
| A-01 | 与功能按钮图标共享同一视觉语言（S-01/S-02 适用；既有 app-icon 实心填充图形不追溯） |
| A-02 | 适配深色主题；在 `BG_VIEW #161616` 背景上辨识清晰 |
| A-03 | 交付物：SVG 源 + PNG（16/24/32/48/64/128/256）+ Windows `.ico`（含 16px 分辨率） |
| A-04 | 16px 档仍保留品牌辨识度（SC-006） |

## 8. 工作流

1. **选型**：按映射契约 `specs/007-replace-icon-set/contracts/icon-replacement-map.md` 选取 Material Icons（默认 Outlined）。
2. **下载**：从 Google Material Icons 官方仓库（GitHub `google/material-design-icons`）下载 24×24 SVG，本地化到 `build/icon-staging/`（不入库）。
3. **归一化**：将素材非 Token 色值（`#000000`/`black`）替换为 `FG_TEXT #D4D4D4`，`fill="none"` 保留（一次性操作，不新增仓库脚本）。
4. **入库**：写入 `src/ui/theme/icons/actions/<icon_id>.svg`（文件名与 `icon-map.yaml` 的 `icon_id` 完全一致）。
5. **登记**：`icon-map.yaml` 维护条目（`icon_id`/`semantic`/`category`/`benchmark_ref`/`states`/`sizes`）。
6. **校验**：`python scripts/check_icons.py`——色板白名单 / 命名 / 覆盖检查，违规即非零退出。
7. **渲染**：`python scripts/render_icons.py`——SVG → PNG（16/24/32；应用图标 16/24/32/48/64/128/256 + `.ico`）。
8. **集成**：`python scripts/gen_qrc.py` 注册 `/perception/icons/` 资源；窗口图标挂接 `main.cpp`。
9. **视觉**：`python scripts/make_mockups.py` 生成 mockup 预览；语义盲测（正确率 ≥90%）与符合性清单验收。

## 9. 与既有文档的关系

- 契约（可检查定义）：`specs/002-icon-design/contracts/icon-style-spec.md` —— 本规范与其一致。
- 替换功能：`specs/007-replace-icon-set/`（映射契约 / 素材来源契约 / 任务 / 计划 / 规格）。
- 设计系统：`docs/design/ui-guidelines.md` §3.1（Token 唯一来源）。
- 映射表：`src/ui/theme/icons/icon-map.yaml`（图标↔功能↔对标条目关联，schema 见 `contracts/icon-function-map.md`）。

# 图标设计规范（icon-spec）

> **Feature**: 002-icon-design | **Status**: 正式规范（v1.1.0，风格修订：线性描边 → 实心填充）
> 本文件是**图标设计的唯一规范文档**（面向实现者），与契约 `specs/002-icon-design/contracts/icon-style-spec.md` 完全一致、不得矛盾（单一事实源）。
> 可检查定义以契约条款号（S/G/P/T/N/A）为准；交付验收执行 `specs/002-icon-design/contracts/conformance-checklist.md`。
> 修订记录：2026-08-25 风格由"线性描边"调整为"实心填充"（对齐 SVisual 拟物参考）。

---

## 1. 目标

为 Perception 全部功能按钮与应用主图标提供**统一、可逐项核验**的视觉规范：实心填充、深色适配、语义唯一、五态清晰。

## 2. 风格（Style）

| 条款 | 要求 |
|---|---|
| S-01 | 实心填充风格：主体以 Token 色实心填充（默认 `FG_TEXT #D4D4D4`），内部细节以 `BG_VIEW #161616` / `BG_CONTROL #3C3C3C` 负形挖空表达层次；`fill="none"` 仅允许用于辅助描边元素 |
| S-02 | 使用描边（`stroke`）时：端点 `stroke-linecap="round"`、连接 `stroke-linejoin="round"`；16px 档负形/细线宽度 ≥1.5px 保证可读 |
| S-03 | 图形语义与功能语义一致，不使用与功能无关的装饰元素 |

**负形挖空示例**：软盘 = 亮色外壳 + 深色标签/顶槽/滑块；放大镜 = 亮色镜圈 + 深色镜心 + 亮色加号；相机 = 亮色机身 + 深色镜头 + 亮色镜芯。

## 3. 网格与尺寸（Grid & Size）

| 条款 | 要求 |
|---|---|
| G-01 | 基础绘制网格 16×16；图形必须落在安全区 12×12 内（四边留 ≥2px）；关键图形外接 ≤14×14 |
| G-02 | 尺寸档位 `16 / 24 / 32`；24=16×1.5、32=16×2 整数倍放大，保证视觉比例一致 |
| G-03 | 描边宽度：16px=**2px**、24px=**2.5px**、32px=**3px** |
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
| A-01 | 与功能按钮图标共享同一实心填充视觉语言（S-01/S-02 适用） |
| A-02 | 适配深色主题；在 `BG_VIEW #161616` 背景上辨识清晰 |
| A-03 | 交付物：SVG 源 + PNG（16/24/32/48/64/128/256）+ Windows `.ico`（含 16px 分辨率） |
| A-04 | 16px 档仍保留品牌辨识度（SC-006） |

## 8. 工作流

1. **绘制**：从模板复制 → 按 §2–§6 绘制 → 交付前删除安全区辅助线。
2. **登记**：在 `src/ui/theme/icons/icon-map.yaml` 登记条目（`icon_id`/`semantic`/`category`/`benchmark_ref`/`states`/`sizes`）。
3. **校验**：`python scripts/check_icons.py`——色板白名单 / 命名 / 覆盖检查，违规即非零退出。
4. **渲染**：SVG → PNG（16/24/32；应用图标 16/24/32/48/64/128/256 + `.ico`）。
5. **集成**：`.qrc` 注册 `/perception/icons/` 资源；窗口图标挂接 `main.cpp`。

## 9. 与既有文档的关系

- 契约（可检查定义）：`specs/002-icon-design/contracts/icon-style-spec.md` —— 本规范与其一致。
- 设计系统：`docs/design/ui-guidelines.md` §3.1（Token 唯一来源）。
- 映射表：`src/ui/theme/icons/icon-map.yaml`（图标↔功能↔对标条目关联，schema 见 `contracts/icon-function-map.md`）。

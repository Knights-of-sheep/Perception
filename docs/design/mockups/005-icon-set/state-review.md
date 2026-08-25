# 五态与应用图标评审记录（conformance Section D / F）

> **Feature**: 002-icon-design | **契约**: `specs/002-icon-design/contracts/conformance-checklist.md`
> 本节记录 Section D（五态与可辨识）与 Section F（应用图标）的实现满足证据；`[x]` 由正式评审确认。

## Section D — 五态与可辨识

| 项 | 判据 | 实现满足证据 | 状态 |
|---|---|---|---|
| D-1 | 每枚功能图标五态齐全（normal/hover/pressed/disabled/selected） | SVG 源（normal）+ icon-map.yaml `states` 全含五态（T-07 派生规则）；QSS/Palette 提供 hover/pressed/selected 容器与 disabled 透明度 | □ |
| D-2 | 五态视觉与 icon-style-spec T-01~T-05 一致 | 状态色/透明度定义见 `icon-style-spec.md` §5 与 `icon-spec.md` §5（FG_TEXT / FG_TEXT_WEAK / FG_TEXT_DISABLED@40% / ACCENT + SELECTION_BG 容器） | □ |
| D-3 | 非单通道区分（disabled 与 selected 不得仅靠颜色或仅靠透明度） | disabled = FG_TEXT_DISABLED + 40% 透明度（双通道）；selected = ACCENT + SELECTION_BG 底（双通道），见 T-06 | □ |
| D-4 | 深色主题下所有状态清晰可辨 | 全部色值来自 ui-guidelines §3.1 Token，深色 BG_VIEW/BG_PANEL 上对比度由 Token 保证 | □ |

## Section F — 应用图标

| 项 | 判据 | 实现满足证据 | 状态 |
|---|---|---|---|
| F-1 | 与功能图标同一线性视觉语言 | `app-icon.svg` 使用 S-01/S-02（fill=none、round cap/join、2px 描边） | □ |
| F-2 | 深色主题适配，BG_VIEW 上辨识清晰 | 主色 ACCENT #0A84FF + 辅色 FG_TEXT #D4D4D4，于 BG_VIEW #161616 上对比充足 | □ |
| F-3 | 交付物齐全（SVG + PNG + ICO） | `icons/app/app-icon.svg`、`png/app/app-icon-{16..256}.png`（7 档）、`app-icon.ico`（7 分辨率） | □ |
| F-4 | 16px 档仍保留品牌辨识度（SC-006） | 16px 渲染件 `app-icon-16.png`；图形为圆角框 + 单条上升曲线，16px 可辨（盲测项含 16px 档） | □ |

## 评审结论

- 五态实现路径：SVG 源（normal）为唯一图标资产，其余四态由 UI 层（QSS 容器 + 透明度）派生——符合 T-07「直接资源或规范可派生」。
- 视觉验证材料：`icon-bar-mockup.png`（含 normal/hover/pressed/disabled/selected 五态示例）；`preview.png`（全量总览）；`main-window-mockup.png`（集成效果）。
- 盲测（SC-003）与 16px 辨识度（SC-004）见同目录 `blind-test.md`。

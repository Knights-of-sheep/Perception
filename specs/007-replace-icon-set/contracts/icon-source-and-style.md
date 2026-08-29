# Contract: 图标素材来源规范与风格契约修订（v2.0.0 要点）

**Feature**: 007-replace-icon-set | **Version**: 1.0.0 | **Date**: 2026-08-29 | **Source**: research.md 决策 1/2/3

> 本契约定义替换素材的**获取与本地化规范**，以及 `icon-style-spec.md` 由 v1.1.0 修订至 **v2.0.0** 的**条款变更要点**。最终契约文本以修订后的 `specs/002-icon-design/contracts/icon-style-spec.md` 为准（实现阶段按「治理」写入修订记录）。

## 1. 素材来源与许可

- **来源**：Google Material Icons 官方集（Figma 官方社区版 / 官方 GitHub 仓库 `google/material-design-icons`），许可 **Apache-2.0**（免费，含商用）。
- **获取方式**：人工下载 SVG 并本地化到仓库；**禁止**接入 Figma 扩展、Figma REST/MCP、在线运行时依赖（宪法「本地设计源」）。
- **风格**：默认 **Outlined**（线性圆角描边）；个别图标按 `icon-replacement-map.md` 标注使用 **Filled**。

## 2. 素材落地流程（三步）

1. **下载**：按 `icon-replacement-map.md` 逐枚下载 Material SVG（24×24 viewBox）。
2. **归一化**：批量将素材中的非 Token 色值替换为 Token 白名单色（`#000000` / `black` → `#D4D4D4` FG_TEXT），`fill="none"` 保留；此为一次性工具操作，不新增仓库脚本。
3. **入库**：写入 `src/ui/theme/icons/actions/<icon_id>.svg`（文件名与 icon_id 完全一致，N-01）。

## 3. 门禁与保留项

- 入库后运行 `scripts/check_icons.py` 必须 0 违规（P-01 / N-01 / 覆盖 / schema）。
- **保留项零改动**：`app/app-icon.svg`、`app/app-icon.ico`、`actions/view-panel-console.svg` 及其 PNG 禁止任何修改（SC-002，git diff 为空验证）。

## 4. icon-style-spec.md v1.1.0 → v2.0.0 修订要点

| 组 | v1.1.0（现状） | v2.0.0（目标） | 变更 |
|---|---|---|---|
| S-01 | 实心填充：主体 Token 色实心填充，内部负形挖空 | **线性圆角描边**：主体以描边（round cap/join）表达，无填充（`fill="none"`） | **MAJOR 变更** |
| S-02 | 描边时 round cap/join；16px 负形/细线 ≥1.5px | 描边 round cap/join；各档描边下限见 G-03 | 措辞调整 |
| G-01 | 基础网格 16×16，安全区 12×12 | 基础网格 **24×24**（Material 原生），安全区 18×18，关键图形 ≤21×21 | **MAJOR 变更** |
| G-02 | 尺寸档位 16/24/32（24=16×1.5、32=16×2） | 尺寸档位 16/24/32（按 24 网格等比缩放：16=24×2/3、32=24×4/3） | 换算基准调整 |
| G-03 | 描边宽度 16=2 / 24=2.5 / 32=3 | 描边宽度 **16=1.5 / 24=2 / 32=2.5**（≥1.5px 下限） | 数值调整 |
| G-04 | 同语义跨档位视觉一致 | 保持不变 | 无 |
| P-01~P-04 | Token 白名单 / 单色 / 状态色 / 品牌色 | **保持不变**（色板单一事实源不变） | 无 |
| T-01~T-07 | 五态规则 | **保持不变** | 无 |
| N-01/N-02 | 命名规则 | **保持不变** | 无 |
| A-01~A-04 | 应用图标条款 | **保持不变**（app-icon 不替换，条款继续约束既有图标） | 无 |

## 5. 16px 档可读性保障

- Material Outlined 原描边 2px@24，缩至 16px ≈ 1.33px < 1.5px 下限。
- 处置（按序）：① 对 16px 档 SVG 描边加粗至 ≥1.5px；② 个别图标改用 Filled 变体；③ 盲测 SC-003 验收兜底（正确率 <90% 的图标重新处理）。
- 所有处置必须保持 `check_icons.py` 通过。

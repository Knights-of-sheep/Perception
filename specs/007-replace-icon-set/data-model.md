# Data Model: 图标替换（Icon / IconMap / 替换映射）

**Feature**: 007-replace-icon-set | **Branch**: `007-replace-icon-set` | **Date**: 2026-08-29

> 基于 `spec.md` Key Entities 与既有 `src/ui/theme/icons/icon-map.yaml` schema（`specs/002-icon-design/contracts/icon-function-map.md` §3）。本功能**不修改数据模型本身**（icon-map.yaml 59 条目保持不变），仅新增一层「替换映射」关系。

## 1. 实体 Icon（功能图标）

功能图标的图形单元，由 `actions/<icon_id>.svg` 唯一对应。

| 字段 | 类型 | 约束 | 说明 |
|---|---|---|---|
| `icon_id` | string | 唯一；kebab-case `[file\|edit\|view\|analysis\|animation\|tools]-<功能>[-<变体>]`（N-01） | 文件系统标识，与 `icon-map.yaml` 条目对应 |
| `semantic` | string | 非空；全库唯一 | 中文功能语义（如「打开文件」） |
| `category` | enum | file / edit / view / analysis / animation / tools | 功能区分类 |
| `source` | enum | paraview / svisual / both / custom | benchmark 来源（替换后仍保留原值，语义基准不变） |
| `states` | set | 必须含 normal/hover/pressed/disabled/selected 五态 | 状态派生依据（T-07） |
| `sizes` | set | 必须含 16；渲染 16/24/32 | 尺寸档位 |
| `svg` | file | `actions/<icon_id>.svg`；色值 ∈ Token 白名单（P-01） | 图形源 |
| `png` | file set | `png/actions/<icon_id>-{16,24,32}.png` | 渲染产物 |
| `retention` | enum（本功能） | **keep** / **replace** | `view-panel-console` = keep；其余 58 枚 = replace |

## 2. 实体 IconMap（icon-map.yaml）

图标-功能映射表，**唯一登记源**。本功能**不改动** 59 个条目（icon_id/semantic/category 等全部保持），仅替换图形。

- entries ↔ `actions/*.svg` 一一对应（覆盖规则 1）
- 版本字段 `version: 1.0.0` 保持不变

## 3. 关系：Replacement Mapping（替换映射，新增）

本功能在 Icon 与 Material 素材之间建立映射，登记于 `contracts/icon-replacement-map.md`：

```
Icon.icon_id --1:1--> Material.icon_name + Material.style (outlined|filled)
Icon.retention = keep  ⟹ 无映射，图形不变
Icon.retention = replace ⟹ 恰好 1 条映射，且入库 SVG 与映射一致
```

| 关系 | 基数 | 规则 |
|---|---|---|
| Icon → Material 图标 | 1:1 | 每枚 replace 图标映射恰好 1 个 Material 图标名 |
| Material 图标 → Icon | 1:1 | 不得复用同一 Material 图标于两个 icon_id（保持语义唯一，N-02） |
| Icon → 渲染产物 | 1:N | 每枚 icon 渲染 16/24/32 三档 PNG |

## 4. 校验规则（源自 spec 与门禁）

- **P-01 色板**：`actions/*.svg` 与 `app/*.svg` 的 fill/stroke 色值必须 ∈ 14 色 Token 白名单（`check_icons.py` 强制）。
- **覆盖规则 1**：icon-map entries ↔ SVG 文件一一对应，无孤儿文件、无缺失文件。
- **保留项不变**：`app-icon.svg`、`app-icon.ico`、`view-panel-console.svg` 及其 PNG 交付前后字节级不变（git diff 为空，SC-002）。
- **唯一性**：icon_id 唯一、semantic 唯一、Material 映射名唯一。
- **状态派生**：单色图标五态由 UI 层按 T 组规则着色/透明度派生，SVG 仅提供图形（normal 主色 FG_TEXT）。

## 5. 状态与约束小结

- 本功能无状态机、无生命周期（纯静态资源替换）。
- 新增「替换映射」是一次性登记，不进入运行时数据。

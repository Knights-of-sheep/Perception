# Contract: 图标-功能映射表（Schema 与命名规则）

**Feature**: 002-icon-design | **Version**: 1.0.0 | **Source**: spec.md Key Entities「图标-功能映射」、data-model.md §2.5/§2.6

> 映射表是**图标 ↔ 功能 ↔ 对标条目**的唯一关联来源，覆盖检查（SC-001/SC-004）以本表为准。交付时应以本 schema 产出 `icon-map.yaml`（实现目录：`src/ui/theme/icons/icon-map.yaml`）。

## 1. 命名规则

```
<功能区>-<功能>[-<变体>]

<功能区>  必填，小写 kebab，取值：file / edit / view / analysis / animation / tools（对应 spec 对标功能清单六大类别）
<功能>    必填，小写 kebab，如 open / extract / zoom-in / play
<变体>    可选，用于同功能多态（如 zoom-in / zoom-out）
```

**语法规则**
- 仅小写字母、数字、连字符 `-`；不得含空格、下划线、中文。
- 全库唯一；禁止同义词并存（如 `open` 与 `load` 描述同一功能时只保留一个）。

**禁用名（Deprecated）**
- `menu-*`、`btn-*`、`icon-*` 前缀（无语义，禁止）。
- 中文、拼音、大小写混排。

## 2. 映射表 Schema（YAML）

```yaml
version: 1.0.0            # 与 icon-style-spec 版本联动
generated: 2026-08-25     # 生成/更新时间
entries:
  -     icon_id: file-open            # kebab-case，唯一
    semantic: 打开文件              # 中文语义短语（识别测试用）
    category: file                 # file / edit / view / analysis / animation / tools
    benchmark_ref:
      source: both                 # paraview / svisual / both / custom
      function: open               # 对标功能名
    states: [normal, hover, pressed, disabled, selected]
    sizes: [16, 24]                # 菜单/侧边栏档；主按钮 [16,24,32]
  - icon_id: analysis-extract
    semantic: 数据提取
    category: analysis
    benchmark_ref: { source: svisual, function: extract }
    states: [normal, hover, pressed, disabled, selected]
    sizes: [16, 24, 32]
```

## 3. 字段约束

| 字段 | 必填 | 约束 |
|---|---|---|
| `icon_id` | ✓ | 满足命名规则 N-01/N-02；全表唯一 |
| `semantic` | ✓ | 非空中文短语；与 `icon_id` 一一对应 |
| `category` | ✓ | 六大类别枚举之一 |
| `benchmark_ref.source` | ✓ | 四大来源枚举之一 |
| `benchmark_ref.function` | ✓ | 引用「对标功能清单」中已登记功能名 |
| `states` | ✓ | 必须含全部五态 |
| `sizes` | ✓ | 至少含 `16` |

## 4. 覆盖检查规则（机器可校验）

1. **图标↔条目一一对应**：`entries` 数 == 实际图标文件数（`src/ui/theme/icons/actions/*.svg`）。
2. **对标覆盖**：「对标功能清单」中 `required_icon=true` 的条目必须出现在 `benchmark_ref.function` 中（缺失即不符合 SC-001）。
3. **命名唯一性**：`icon_id` 无重复；同 `semantic` 无重复。
4. **规范一致**：全部图标满足 `icon-style-spec.md` 各条款。

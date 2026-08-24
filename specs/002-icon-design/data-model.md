# Data Model: 图标设计体系

**Feature**: 002-icon-design | **Source**: [spec.md](spec.md) Key Entities / FR / SC

> 本设计需求无运行时数据结构；"数据模型"特指**设计交付物的一致模型**，用于保证规范、图标、映射、验收四者可机器/人工交叉校验。

## 1. 实体总览

```text
IconStyleSpec (1) ──约束──> FunctionIcon (*)
       │                        │
       └──约束──> AppIcon (1)   └──映射──> IconFunctionMap (1) ──引用──> BenchmarkFeature (*)
                    │
                    └──用于──> Mockups (1)
```

## 2. 实体定义

### 2.1 IconStyleSpec（图标设计规范）

| 字段 | 类型 | 必填 | 规则/说明 |
|---|---|---|---|
| `style` | enum | ✓ | 固定值 `line-outline`（决策 1） |
| `stroke_width_px` | int | ✓ | 16px=2 / 24px=2.5 / 32px=3（决策 3） |
| `stroke_cap` | enum | ✓ | 固定值 `round` |
| `stroke_join` | enum | ✓ | 固定值 `round` |
| `fill` | enum | ✓ | 固定值 `none`（透明底） |
| `grid` | int×int | ✓ | 16×16 基础网格；安全区 12×12；图形占位 ≤14×14（决策 3） |
| `size_slots` | int[] | ✓ | `[16, 24, 32]`（整数倍放大 1×/1.5×/2×） |
| `palette_ref` | ref | ✓ | 引用 `ui-guidelines.md` §3.1 Token；**禁止新增色值** |
| `state_rules` | StateRule[5] | ✓ | 五态表现规则（见 §2.4） |
| `version` | semver | ✓ | 规范版本号，变更需 MINOR/PATCH 语义 |

**校验规则**
- R1: `style`/`stroke_cap`/`stroke_join`/`fill` 为固定枚举，不允许偏离。
- R2: 任一图标不得引入规范色板以外的色值（自动扫描 SVG `fill/stroke` 可校验）。
- R3: 尺寸档位必须为 `size_slots` 子集。

### 2.2 FunctionIcon（功能图标）

| 字段 | 类型 | 必填 | 规则/说明 |
|---|---|---|---|
| `icon_id` | string | ✓ | kebab-case，格式 `<功能区>-<功能>[-<变体>]`，全库唯一 |
| `semantic` | string | ✓ | 图标表达的语义（中文短语，如"打开文件"） |
| `category` | enum | ✓ | 对标功能类别之一（见 §2.5） |
| `benchmark_ref` | ref | ✓ | 对应「对标功能清单」中的功能条目（ParaView/SVisual 对标来源） |
| `states` | State[5] | ✓ | 必须提供全部五态资源/规则派生 |
| `sizes` | int[] | ✓ | 必须覆盖 `[16]`；菜单/侧边栏图标覆盖 `[16,24]`；主按钮覆盖 `[16,24,32]` |
| `source_file` | path | ✓ | SVG 源路径 `src/ui/theme/icons/actions/<icon_id>.svg` |

**校验规则**
- R4: `icon_id` 与 `semantic` 一一对应（映射表唯一键）；无同义词重复。
- R5: 五态必须齐全且符合 `state_rules`（符合性清单逐项检查）。
- R6: 图标数量与映射表条目数一致（覆盖检查：每枚图标有映射，每条映射有图标）。

### 2.3 AppIcon（应用图标）

| 字段 | 类型 | 必填 | 规则/说明 |
|---|---|---|---|
| `icon_id` | string | ✓ | 固定 `app` |
| `brand_colors` | color[] | ✓ | `ACCENT #0A84FF` 主 + `FG_TEXT #D4D4D4` 辅（现有 Token，不新增） |
| `theme` | enum | ✓ | 固定值 `dark-optimized`（深色主题适配） |
| `deliverables` | path[] | ✓ | SVG 源 + PNG（16/24/32/48/64/128/256）+ Windows `.ico`（含全部尺寸） |
| `window_integration` | ref | ✓ | `main.cpp` `setWindowIcon` 挂接点（FR-011 交付要求） |

**校验规则**
- R7: 交付物必须含 `.ico`（含 16px 分辨率）——SC-006 极小尺寸可辨需在 16px 档验证。
- R8: 品牌色必须来自现有 Token（自动扫描可校验）。

### 2.4 InteractionState（交互状态）

| 状态 | 颜色 | 透明度 | 容器 | 校验要点 |
|---|---|---|---|---|
| `normal` | FG_TEXT | 100% | 透明 | 基线状态 |
| `hover` | FG_TEXT | 100% | BG_CONTROL 圆角 4px | 悬停有底，颜色不变 |
| `pressed` | FG_TEXT_WEAK | 100% | BG_CONTROL 加深 + 内缩 1px | 按压反馈 |
| `disabled` | FG_TEXT_DISABLED | 40% | 透明 | **颜色 + 透明度双重信号** |
| `selected` | ACCENT | 100% | SELECTION_BG 圆角 4px | **颜色 + 背景双重信号** |

- R9: `disabled` 与 `selected` 不得只靠单一通道区分（FR-005 色盲友好）。

### 2.5 BenchmarkFeature（对标功能条目）

| 字段 | 类型 | 必填 | 规则/说明 |
|---|---|---|---|
| `category` | enum | ✓ | `file` / `edit` / `view` / `analysis` / `animation` / `tools`（六大类别，对应 spec 对标功能清单：文件与数据/编辑/视图与相机/数据操作与分析/动画与播放/工具与设置） |
| `function` | string | ✓ | 功能名（如 `open`、`extract`、`zoom-in`、`play`） |
| `source` | enum | ✓ | `paraview` / `svisual` / `both` / `custom` |
| `required_icon` | bool | ✓ | 是否必须有图标（覆盖检查基准） |

- R10: `required_icon=true` 的条目必须存在于映射表（覆盖检查）。

### 2.6 IconFunctionMap（图标-功能映射表）

| 字段 | 类型 | 必填 | 规则/说明 |
|---|---|---|---|
| `entries` | MapEntry[] | ✓ | 每条含 `icon_id` / `semantic` / `category` / `benchmark_ref` / `states` / `sizes` |
| `version` | semver | ✓ | 与 IconStyleSpec 版本对齐联动 |

- R11: 映射表是**唯一关联来源**（spec Key Entities 明确）；覆盖检查以本表为准。

### 2.7 Mockups（视觉稿）

| 字段 | 类型 | 必填 | 规则/说明 |
|---|---|---|---|
| `location` | path | ✓ | `docs/design/mockups/005-icon-set/`（宪法 II 视觉稿事实源） |
| `content` | enum[] | ✓ | 图标集总览（全量图标 × 五态）+ 应用图标展示 + 深色主题上下文示例 |
| `preview` | path | ✓ | `preview.png` |

## 3. 生命周期/状态转换

本模型无运行时状态机。**设计交付物生命周期**（版本化演进）：

```text
Draft（设计稿）──评审──> Approved（定稿）──实现──> Integrated（接入 .qrc）
   │                       │
   └── Revision（变更）───> 版本升级（IconStyleSpec/Map version MINOR）
```

- 变更任一 `semantic`/`state_rules`/`size_slots` → 规范 MINOR 升级，映射表同步。
- 纯措辞/排版修订 → PATCH。

## 4. 与验收的映射

| 成功标准 | 对应模型校验 |
|---|---|
| SC-001（≥60 枚图标） | FunctionIcon 计数 |
| SC-002（0 不符合项） | 符合性清单（contracts/conformance-checklist.md） |
| SC-003（语义识别 ≥90%） | 盲测程序（§2.1 评审方法） |
| SC-004（命名无重复歧义） | R4 / 映射表唯一键校验 |
| SC-005（五态可辨） | R5 / R9 清单检查 |
| SC-006（应用图标 16px 可辨 + 风格一致） | R7 / R8 清单检查 |

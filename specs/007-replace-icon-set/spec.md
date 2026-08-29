# Feature Specification: Replace Icon Set

**Feature Branch**: `007-replace-icon-set`

**Created**: 2026-08-29

**Status**: Draft

**Input**: User description: "当前除了程序的icon和控制pyshell显隐的icon我满意，其他的我都不满意，要求其他所有icon全部更新替换，优先从figma官方免费版里获取"

## User Scenarios & Testing

### User Story 1 - 替换全部功能图标为 Figma 官方免费图标（Priority: P1）

作为桌面可视化工具的使用者，我希望除"程序图标"与"Python 控制台显隐图标"以外的**所有功能图标**（当前 58 枚）都被统一替换为来自 Figma 官方免费图标集的图形，使图标视觉风格与现代设计语言一致、辨识度更高，从而改善整体界面观感。

**Why this priority**: 这是用户本次需求的核心诉求——当前全部功能图标均不被用户认可；图标是界面观感的第一要素，替换收益最直接、覆盖面最大。

**Independent Test**: 可独立验证——对比替换前后 `src/ui/theme/icons/actions/` 下 58 个 SVG 的图形内容（排除保留项后全部变化），且 `icon-map.yaml` 条目与 SVG 文件一一对应、`scripts/check_icons.py` 校验全绿；应用启动后各工具栏/菜单按钮均显示新图形。

**Acceptance Scenarios**:

1. **Given** 当前 59 枚功能图标中 58 枚将被替换，**When** 完成替换并渲染，**Then** `src/ui/theme/icons/actions/` 下除 `view-panel-console.svg` 外其余 SVG 的图形均来自 Figma 官方免费图标集（本地化资产），应用运行可见新图形。
2. **Given** 替换后的图标库，**When** 运行 `python scripts/check_icons.py`，**Then** 退出码为 0，色板白名单 / 命名规则 / icon-map 覆盖 / 字段 schema 全部通过。
3. **Given** 任一替换图标，**When** 以 16px 档位显示，**Then** 图形清晰可辨，语义与功能一致（视觉抽查 100% 通过）。

---

### User Story 2 - 更新视觉契约与渲染链路（Priority: P2）

作为项目维护者，我希望图标替换后其视觉契约（icon-style-spec）同步更新、渲染链路（SVG→PNG→qrc→mockups→截图）重新跑通，使新图标风格有据可依、工程资产完整一致。

**Why this priority**: 现有契约 icon-style-spec v1.1.0 为「实心填充」风格；Figma 官方免费图标集多为线性/圆角风格，若不一致需修订契约。契约与渲染产物不更新会导致门禁误报或运行时资源缺失。

**Independent Test**: 可独立验证——运行 `scripts/render_icons.py` + `scripts/gen_qrc.py` 生成全部 PNG 与 qrc；`scripts/check_theme_contrast.py` 通过；应用正常构建启动、无图标缺失。

**Acceptance Scenarios**:

1. **Given** 新图标风格与 icon-style-spec v1.1.0 存在冲突，**When** 完成契约修订，**Then** 契约版本升级（≥1.2.0），S/G/P/T/N/A 条款与新风格一致，色板 Token 白名单 P-01 保持不变。
2. **Given** 全部替换后的 SVG，**When** 执行渲染与 qrc 生成，**Then** 每个动作图标具备 16/24/32 三档 PNG，`theme.qrc` 资源条目与文件系统一一对应。
3. **Given** 应用以任意主题（深色/浅色/高对比）启动，**When** 显示全部图标，**Then** 无颜色违规（`check_theme_contrast.py` 通过），无缺失资源。

---

### User Story 3 - 同步设计产物与文档（Priority: P3）

作为项目维护者，我希望图标替换后设计产物（mockups、截图）与文档（icon-spec、README、架构说明）同步更新，保持「本地设计源」与文档导航的一致性，避免设计源与实现脱节。

**Why this priority**: 宪法质量门禁要求「合并前必须核对 docs/ 与 README.md 已同步最新功能/重构内容」，且 UI 设计源（mockups）必须与实际界面一致。

**Independent Test**: 可独立验证——`docs/design/mockups/005-icon-set` 重新生成后与 `src/ui/theme/icons` 实际图标一致；README 图标章节与 `scripts/` 清单无过时描述；截图重生成后与 mockups 对比一致。

**Acceptance Scenarios**:

1. **Given** 新图标集，**When** 运行 `make_mockups.py`，**Then** `docs/design/mockups/005-icon-set` 预览图与最终图标一致。
2. **Given** 文档更新完成，**When** 审阅 `README.md` 与 `docs/design/icon-spec.md`，**Then** 无描述新风格/图标来源的过时内容，流程说明与实际脚本一致。

---

### Edge Cases

- **语义缺失**：Figma 官方免费图标集中找不到与专业语义匹配的图标（如 `analysis-cutline` 截线、`animation-param-scan` 参数扫描）。处理：优先选语义最接近的免费图标；若无，按新风格本地重绘，且必须满足 check_icons.py 全部校验。
- **色值违规**：下载的 Figma SVG 含非 Token 色值。处理：转色为 Token 白名单色值后入库，P-01 门禁拦截非法色值。
- **16px 辨识度退化**：线性图标在 16px 档线条过细。处理：视觉抽查识别测试，不达标者加粗或采用实心表达（契约允许负形/填充）。
- **保留项被误改**：`app-icon` 与 `view-panel-console` 不得改动。处理：交付时 git diff 校验保留项零改动。
- **资源引用中断**：PNG/qrc 未同步导致图标缺失。处理：渲染 + qrc + 全量构建链完整跑通后验收。

## Requirements

### Functional Requirements

- **FR-001**: 程序图标 `src/ui/theme/icons/app/app-icon.svg` 与 `app-icon.ico` 必须保持不变，禁止任何图形修改。
- **FR-002**: Python 控制台显隐图标 `view-panel-console`（`actions/view-panel-console.svg` 及其 PNG）必须保持不变。
- **FR-003**: `icon-map.yaml` 中除 `view-panel-console` 外的全部 58 个功能图标必须替换为来自 Figma 官方免费图标集的图形（人工下载并本地化为项目资产）。
- **FR-004**: 所有新 SVG 必须通过 `scripts/check_icons.py` 全部校验：色板 Token 白名单（P-01）/ 命名规则（N-01）/ icon-map 覆盖一一对应 / 字段 schema，退出码为 0。
- **FR-005**: 每个替换图标必须渲染 16/24/32 三档 PNG，并满足五态（normal/hover/pressed/disabled/selected）派生要求。
- **FR-006**: 必须重新生成 `theme.qrc`（`scripts/gen_qrc.py`），保证资源条目与文件系统一致。
- **FR-007**: 必须重新生成图标集 mockup（`scripts/make_mockups.py` → `docs/design/mockups/005-icon-set`）。
- **FR-008**: 必须重新生成界面截图（`scripts/update_screenshots.ps1` → `docs/screenshots/`）。
- **FR-009**: 若新图标风格与 `icon-style-spec.md` v1.1.0 冲突，必须将契约修订至 ≥1.2.0 并保持色板 Token 白名单（P-01）不变。
- **FR-010**: 必须同步更新文档：`docs/design/icon-spec.md`（图标来源与流程）、`README.md`（图标体系描述与脚本清单）、必要时 `docs/design/ui-guidelines.md` 与 mockups 说明。

### Key Entities

- **Icon**: 功能图标的图形单元。关键属性：`icon_id`（唯一标识，kebab-case）、`semantic`（功能语义）、`category`（file/edit/view/analysis/animation/tools）、`states`（五态）、`sizes`（16/24/32）、图形源（SVG）、渲染产物（PNG/ICO）。`app-icon` 为应用图标，独立于 actions。
- **IconMap（icon-map.yaml）**: 图标-功能映射表，条目与 `actions/*.svg` 一一对应，是图标功能语义的唯一登记源。

## Success Criteria

### Measurable Outcomes

- **SC-001**: 58 枚功能图标全部替换完成，`check_icons.py` 校验 0 违规，icon-map 覆盖 100%。
- **SC-002**: 保留项（`app-icon` 与 `view-panel-console`）交付前后图形零改动（git diff 为空）。
- **SC-003**: 替换图标 16px 档视觉抽查 100% 可辨识，语义与功能一致。
- **SC-004**: 应用在深色 / 浅色 / 高对比三类主题下图标显示无颜色或对比度违规（`check_theme_contrast.py` 通过）。
- **SC-005**: 图标色板无新增非 Token 色值（P-01 保持，`check_icons.py` 验证）。
- **SC-006**: 全量构建（`build.ps1 -Gui`）+ `ctest` + `pytest` 全部通过，运行时无图标缺失。
- **SC-007**: mockups 与截图重生成并与最终界面一致；`README.md` 与 `docs/design/icon-spec.md` 已同步，无过时描述。

## Assumptions

- **Figma 素材获取方式**：Figma 官方免费图标集（官方社区/免费图标库）的 SVG 由人工下载并**本地化**为项目资产；工程内不引入 Figma 扩展、Figma REST/MCP 或任何在线依赖（宪法「本地设计源」约束优先）。
- **"控制 pyshell 显隐的 icon" 指代**：即 `view-panel-console`（Python 控制台面板的显示/隐藏切换图标，icon-map.yaml 中 semantic="Python 控制台"）。
- **替换范围**：`icon-map.yaml` 中除保留项（`app-icon`、`view-panel-console`）外的全部 58 个 actions 图标；仅替换图形视觉，`icon_id` 与命名规则 N-01 保持不变。
- **契约演进**：若 Figma 官方图标风格与现有「实心填充」契约冲突，修订 `icon-style-spec.md`（≥1.2.0）；色板 Token 白名单与命名/覆盖/schema 校验保持不变。
- **依赖**：图标流程依赖现有 `scripts/render_icons.py` / `gen_qrc.py` / `make_mockups.py` / `update_screenshots.ps1` 与 `check_icons.py` / `check_theme_contrast.py` 门禁，均需可运行（PyQt5 / PyYAML / Pillow）。

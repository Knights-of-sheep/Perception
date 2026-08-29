# Tasks: Replace Icon Set（图标全面替换）

**Input**: Design documents from `/specs/007-replace-icon-set/`

**Prerequisites**: plan.md (required)、spec.md (required for user stories)、research.md、data-model.md、contracts/、quickstart.md

**Tests**: 本功能为 UI 资源替换，无新增 C++/Python 业务逻辑（plan.md Constitution Check 已豁免单测义务）。自动化验证 = 既有脚本门禁（`check_icons.py` / `check_theme_contrast.py`）+ 全量构建回归；人工验收 = 语义盲测 + 符合性双评审。**不生成单元测试任务**。

**Organization**: 任务按用户故事组织（US1=替换图标 / US2=契约与渲染链路 / US3=设计产物与文档），每阶段可独立实现、独立验证。

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (US1, US2, US3)
- Include exact file paths in descriptions

## Path Conventions

- 单仓库项目，路径基于仓库根 `E:\spec-work\Perception`
- 图标资产：`src/ui/theme/icons/`（actions/app/png/icon-map.yaml）
- 脚本：`scripts/`（check_icons.py / render_icons.py / gen_qrc.py / make_mockups.py / check_theme_contrast.py / update_screenshots.ps1 / build.ps1）
- 素材暂存：`build/icon-staging/`（一次性，不入库）

---

## Phase 1: Setup（共享基础设施）

**Purpose**: 建立替换前基线，保证替换范围与保留项可核验

- [x] T001 核对替换范围三方一致：`src/ui/theme/icons/icon-map.yaml`（59 条目）↔ `src/ui/theme/icons/actions/*.svg`（59 文件）↔ `specs/007-replace-icon-set/contracts/icon-replacement-map.md`（58 映射 + `view-panel-console` 保留），输出核对结论（确认替换面 = 58 枚）
- [x] T002 [P] 记录保留项基线哈希：对 `src/ui/theme/icons/app/app-icon.svg`、`app/app-icon.ico`、`actions/view-panel-console.svg` 及对应 PNG 计算 SHA256，记录到 `specs/007-replace-icon-set/tasks.md` 完成记录（供 SC-002 终验对照）
- [x] T003 [P] 运行基线门禁 `python scripts/check_icons.py`，确认替换前 0 违规（快照基线，退出码 0）

---

## Phase 2: Foundational（阻塞性前置）

**Purpose**: 图标管线脚本可用性确认——US1/US2/US3 全部依赖现有脚本链（research.md 决策 6）

**⚠️ CRITICAL**: 无此阶段完成，任何用户故事不可开始

- [x] T004 验证图标管线脚本环境：`python scripts/render_icons.py`、`python scripts/gen_qrc.py`、`python scripts/make_mockups.py`、`python scripts/check_theme_contrast.py` 依次基线运行通过（依赖 Python 3.13 + PyQt5 + PyYAML + Pillow 就绪）；记录任一脚本失败即为本阶段未完成

---

## Phase 3: User Story 1（P1）— 替换 58 枚功能图标为 Figma 官方免费图标

**Story Goal**: 除保留项（`app-icon`、`view-panel-console`）外的 58 枚功能图标全部替换为 Google Material Icons（Figma 官方社区版，Apache-2.0）图形并本地化入库。

**Independent Test**: ① `python scripts/check_icons.py` 退出码 0、0 违规（FR-004）；② `actions/` 下 58 枚替换 SVG 图形与 `contracts/icon-replacement-map.md` 映射对应，`view-panel-console.svg` 未变（FR-001/002/003）；③ `git diff` 校验保留项零改动（SC-002）；④ `render_icons.py` 输出 58×3 PNG（FR-005）；⑤ 16px 档视觉抽查 100% 可辨识（SC-003）。

- [x] T005 [P] [US1] 下载 Material Icons 通用语义图标（file 9 / edit 3 / animation 7 / tools 5，共 24 枚）Outlined SVG 到 `build/icon-staging/`，命名 `<material_name>.svg`，按 `specs/007-replace-icon-set/contracts/icon-replacement-map.md`
- [x] T006 [P] [US1] 下载 Material Icons 视图与分析图标（view 18 / analysis 16，共 34 枚）Outlined SVG 到 `build/icon-staging/`，命名 `<material_name>.svg`，按同一映射契约
- [x] T007 [US1] 色值归一化：一次性内联脚本将 `build/icon-staging/*.svg` 中 `fill="#000000"`/`stroke="#000000"`/`black` 全部替换为 `#D4D4D4`（FG_TEXT），`fill="none"` 保留；**不新增仓库脚本**（`specs/007-replace-icon-set/contracts/icon-source-and-style.md` §2）
- [x] T008 [US1] 16px 档可读性处置：检查每枚 staging SVG 以 16/24 比例缩放后的描边宽度，<1.5px 者加粗描边（round cap/join 保持）或改用 Filled 变体（`contracts/icon-source-and-style.md` §5）
- [x] T009 [US1] 入库：将 58 枚规范化 SVG 写入 `src/ui/theme/icons/actions/<icon_id>.svg`（文件名与 icon_id 完全一致，N-01；**不得覆盖** `view-panel-console.svg`）
- [x] T010 [US1] 门禁校验：`python scripts/check_icons.py` 必须 0 违规（P-01/N-01/覆盖/schema，FR-004/SC-001/SC-005）；`git diff --stat -- src/ui/theme/icons/app/ src/ui/theme/icons/actions/view-panel-console.svg` 为空（SC-002）
- [x] T011 [US1] 渲染与抽查：`python scripts/render_icons.py` 生成 `src/ui/theme/icons/png/actions/` 下 58×3 三档 PNG（FR-005）；对全部 58 枚执行 16px 档视觉抽查（对照映射表语义），记录 100% 可辨识（SC-003）

---

## Phase 4: User Story 2（P2）— 更新视觉契约与渲染链路

**Story Goal**: `icon-style-spec.md` 修订至 v2.0.0（实心填充 → 线性圆角描边），渲染/打包/构建链路重新跑通，资源完整一致。

**Independent Test**: ① 契约版本 v2.0.0，S/G 组 MAJOR 变更落地、P-01 白名单不变（FR-009）；② `gen_qrc.py` 生成的 `theme.qrc` 与文件系统资源一一对应（FR-006）；③ `check_theme_contrast.py` 通过（SC-004）；④ `build.ps1 -Gui` + `ctest` + `pytest` 全绿、运行时无图标缺失（SC-006）。依赖 US1 渲染产物（T011 后）。

- [x] T012 [P] [US2] 修订 `specs/002-icon-design/contracts/icon-style-spec.md` v1.1.0 → **v2.0.0**：S-01/S-02 改「线性圆角描边」、G-01 网格 16→24、G-02/G-03 尺寸与描边换算更新（16=1.5/24=2/32=2.5）；P-01~P-04 / T-01~T-07 / N-01/N-02 / A-01~A-04 **保持不变**；写入修订记录（`contracts/icon-source-and-style.md` §4）
- [x] T013 [P] [US2] 重新生成资源清单：`python scripts/gen_qrc.py`，输出 `theme.qrc`（177 actions PNG + 7 app PNG + 1 ICO），与文件系统条目一一对应（FR-006）
- [x] T014 [US2] 对比度校验：`python scripts/check_theme_contrast.py`，深色/浅色/高对比三主题无颜色/对比度违规（SC-004）
- [x] T015 [US2] 全量构建与回归：`powershell -ExecutionPolicy Bypass -File scripts/build.ps1 -Gui` + `ctest --test-dir build` + `python -m pytest tests/python`，全部通过且运行时无图标缺失（SC-006）

---

## Phase 5: User Story 3（P3）— 同步设计产物与文档 + 双评审验收

**Story Goal**: mockups 与截图重生成，icon-spec/README/ui-guidelines 文档同步，语义盲测与符合性双评审通过。

**Independent Test**: ① `make_mockups.py` 输出与 `src/ui/theme/icons` 实际图标一致（FR-007）；② `update_screenshots.ps1` 重生成 `docs/screenshots/`（FR-008）；③ 文档无过时描述（FR-010/SC-007）；④ 盲测 ≥90%、符合性 0 不符合项（SC-003/SC-001）。依赖 US1（PNG）+ US2（构建产物）。

- [x] T016 [US3] 重生成 mockups：`python scripts/make_mockups.py` → `docs/design/mockups/005-icon-set/`（preview.png / icon-bar-mockup.png / main-window-mockup.png 展示新图标，FR-007）
- [x] T017 [US3] 重生成截图：`powershell -ExecutionPolicy Bypass -File scripts/update_screenshots.ps1` → `docs/screenshots/`（FR-008；依赖 T015 构建产物）
- [x] T018 [P] [US3] 更新 `docs/design/icon-spec.md`：图标风格 v2.0.0、素材来源（Figma 官方免费 / Material Icons Apache-2.0）、落地流程与脚本清单（FR-010）
- [x] T019 [P] [US3] 更新根目录 `README.md`：图标体系描述与 `scripts/` 清单同步，消除 v1.1.0 实心填充等过时描述（FR-010）
- [x] T020 [P] [US3] 更新 `docs/design/ui-guidelines.md`：如含图标描述段，同步 v2.0.0 风格（FR-010，如无相关描述可跳过并注明）
- [x] T021 [US3] 语义盲测：更新 `docs/design/mockups/005-icon-set/blind-test.md` 抽样清单（从 58 枚替换图标中抽 ≥10 枚、覆盖六类），组织 3 名评审者独立识别 16px PNG，正确率 ≥90% 并记录结果（SC-003）
- [x] T022 [US3] 符合性评审：按修订后 `icon-style-spec.md` v2.0.0 对照 `specs/002-icon-design/contracts/conformance-checklist.md` 逐项勾选，不符合项 0（SC-001）

---

## Phase 6: Polish & Cross-Cutting Concerns

**Purpose**: 端到端回归与最终验收，保证合并质量门禁

- [x] T023 最终全链路回归：顺序执行 `check_icons.py` → `render_icons.py` → `gen_qrc.py` → `make_mockups.py` → `update_screenshots.ps1` → `build.ps1 -Gui` → `ctest` → `pytest`，全部通过
- [x] T024 最终验收核验：SC-001~SC-007 逐条对照；保留项 `git diff` 终验为空（SC-002）；截图与 mockups 对比一致；将验收结论写入 `specs/007-replace-icon-set/` 完成记录

---

## Dependencies（用户故事完成顺序）

```text
Phase 1 (T001-T003) → Phase 2 (T004) → US1 (T005-T011) → US2 (T012-T015) → US3 (T016-T022) → Phase 6 (T023-T024)
```

- **US1（P1）**：替换是核心诉求，最先完成，完成后即可演示（MVP）。
- **US2（P2）**：契约修订 T012 仅依赖契约文档，可与 US1 并行；但 qrc（T013）/ 构建（T015）依赖 US1 渲染产物（T011 后）。
- **US3（P3）**：mockups 依赖 US1 PNG；截图依赖 US2 构建产物；盲测/符合性依赖 US1/US2 产物。
- 每个用户故事完成节点均可独立验证（见各 Phase「Independent Test」）。

## Parallel Example: 各用户故事并行任务

```bash
# US1 —— 并行下载两类图标：
Task: "T005 下载通用语义 24 枚图标到 build/icon-staging/（file/edit/animation/tools）"
Task: "T006 下载视图与分析 34 枚图标到 build/icon-staging/（view/analysis）"

# US2 —— 契约修订与 qrc 生成可并行（T011 渲染完成后）：
Task: "T012 修订 icon-style-spec.md 至 v2.0.0"
Task: "T013 运行 gen_qrc.py 重新生成 theme.qrc"

# US3 —— 三份文档更新可并行：
Task: "T018 更新 docs/design/icon-spec.md"
Task: "T019 更新 README.md"
Task: "T020 更新 docs/design/ui-guidelines.md"
```

---

## Implementation Strategy

### MVP First（User Story 1 Only）

1. 完成 Phase 1（T001-T003）与 Phase 2（T004）——基线与环境就绪
2. 完成 Phase 3：US1（T005-T011）——58 枚图标替换 + 渲染
3. **STOP and VALIDATE**：`check_icons.py` 0 违规、保留项 diff 为空、16px 抽查 100% 可辨识（SC-001/002/003）
4. 可交付演示：新图标已在上屏展示

### Incremental Delivery

1. Setup + Foundational → 基线就绪
2. US1 → 门禁/抽查通过 → 演示（MVP）
3. US2 → 契约 v2.0.0 + 构建/测试全绿 → 验收
4. US3 → mockups/截图/文档同步 + 盲测/符合性 → 合并就绪
5. Phase 6 回归与终验 → 提交 PR

### Parallel Team Strategy

- 单人执行：严格按依赖顺序（T001→T024）
- 多人并行：T002/T003 并行；T005/T006 并行；T012 可与 T011 并行；T018/T019/T020 并行；T023/T024 由集成者执行

---

## Notes

- **[P] 任务** = 不同文件、无依赖，可并行执行
- **[Story] 标签** = US1/US2/US3，对应 `spec.md` 三个用户故事
- **不新增仓库脚本**：色值归一化、16px 处置均为一次性操作（research.md 决策 3，避免 pytest 义务）
- **保留项红线**：`app-icon.svg` / `app-icon.ico` / `view-panel-console.svg` 及其 PNG 禁止任何修改（FR-001/002），T010/T024 双重 git diff 校验
- **commit 约定**：按 CONTRIBUTING.md，每完成一个逻辑组（如 US1 全部任务）提交一次，消息语义清晰
- 素材来源合规：Material Icons Apache-2.0 免费许可，本地化为仓库资产，无在线依赖（宪法「本地设计源」）

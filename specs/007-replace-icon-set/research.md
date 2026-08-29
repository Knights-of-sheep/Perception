# Research: 图标全面替换（Figma 官方免费图标集）

**Feature**: 007-replace-icon-set | **Branch**: `007-replace-icon-set` | **Date**: 2026-08-29

> 本文档解决 Technical Context 中的全部未知项，产出可执行的替换决策。信息来源：现有图标体系调研（`specs/002-icon-design/` 契约与 research、`scripts/` 图标管线实现、`src/ui/theme/icons/` 资产盘点）+ Figma 官方免费图标集（Google Material Icons 官方社区版）公开资料 + 深色桌面工具图标惯例（VS Code Dark+ 体系）。全部决策以「可检查」为第一标准。

---

## 现状盘点（调研结论）

- **资产规模**：`src/ui/theme/icons/actions/` 共 **59 枚 SVG**；`icon-map.yaml` 59 条映射（file 9 / edit 3 / view 19 / analysis 16 / animation 7 / tools 5）。
- **保留项**：`app/app-icon.svg`（程序图标）与 `view-panel-console.svg`（Python 控制台显隐）。**替换范围 = 58 枚**。
- **现有风格契约**：`icon-style-spec.md` v1.1.0 —— 实心填充（S-01：主体 Token 色实心填充，内部以 BG_VIEW/BG_CONTROL 负形挖空），16×16 网格、安全区 12×12，三档 16/24/32，五态。
- **门禁脚本**（`scripts/check_icons.py`）：P-01 色板 Token 白名单（14 色）、N-01 命名 `[file|edit|view|analysis|animation|tools]-xxx.svg`、覆盖规则（map↔SVG 一一对应）、schema（icon_id 唯一 / semantic 非空 / category 枚举 / 五态 / sizes 含 16 / benchmark_ref 枚举）。`fill="none"`/`currentColor` 已豁免。
- **渲染链路**：`render_icons.py`（SVG→PNG 16/24/32 + app 7 档 + .ico）→ `gen_qrc.py`（`/perception/icons` qrc）→ `make_mockups.py`（`docs/design/mockups/005-icon-set/`：preview / icon-bar / main-window）→ `update_screenshots.ps1`（`docs/screenshots/`）。**全链路可复用，无需新增脚本**。
- **验收先例**（002）：双评审制——conformance-checklist 逐项勾选 + 语义盲测（抽样 ≥10 枚 × 3 人，正确率 ≥90%），盲测材料为 16px PNG。

---

## 决策 1：图标素材来源（Figma 官方免费图标集）

- **Decision**: 采用 **Google Material Icons 官方集**（Figma 官方社区版，Apache-2.0 免费许可）作为唯一素材源；由人工从其官方仓库/社区下载 SVG 并**本地化**入库。优先 **Outlined（线性圆角）** 风格；个别在 16px 档识别度不足的图标改用同语义 **Filled** 变体。
- **Rationale**: ① 用户明确要求「优先从 Figma 官方免费版里获取」——Material Icons 是 Figma 官方社区免费图标集的代表，素材可得、许可自由、图形质量高；② 深度覆盖通用语义（打开/保存/撤销/缩放/播放/设置等），58 枚中绝大多数可直接对应；③ 线性圆角风格契合深色桌面工具行业惯例（VS Code Dark+ 体系），也回归 002 research 决策 1 最初倾向的线性路线；④ 人工下载本地化，工程内**不引入 Figma 扩展、Figma REST/MCP、在线运行时依赖**，满足宪法「本地设计源」。
- **Alternatives considered**: Material Filled 全套（实心风格与现有契约兼容、16px 更清晰，但图形均为实心，风格变化幅度小，作为个别图标的补充而非主路线）；Tabler Icons / Lucide（免费但非 Figma 官方来源，不符合用户「Figma 官方」要求）；自绘重绘 58 枚（工作量大、无外部基准，仅作为语义缺失兜底）。

## 决策 2：视觉风格契约修订（icon-style-spec v1.1.0 → v2.0.0）

- **Decision**: 将 `icon-style-spec.md` 修订至 **v2.0.0（MAJOR）**：风格从「实心填充」改为「线性圆角描边」——主体以 2px 描边（round cap/join）表达，不填充；色板 Token 白名单 **P-01 不变**；命名规则 **N-01 不变**；五态规则 **T 组不变**；网格基准由 16×16 调整为 **24×24（Material 原生网格）**，16/24/32 三档按比例缩放（16 = 24×2/3 线性缩放）。
- **Rationale**: 新素材为线性风格，与 v1.1.0 实心条款直接冲突，必须按宪法治理升级契约（MAJOR = 语义变更）；色板/命名/状态规则与素材无关，保持稳定减少回归风险；24 网格是 Material 官方制图标准，避免重绘网格转换。
- **16px 档风险与处置**: Material Outlined 原描边 2px@24 网格，缩至 16px 约 1.33px，偏细。处置：**G-03 更新为分档描边（16px=1.5px、24px=2px、32px=2.5px，≥1.5px 下限）**；对 16px 识别度不达标的图标，改用 Filled 变体或对 SVG 做描边加粗微调，以盲测 SC-003 验收兜底。
- **Alternatives considered**: 保留实心填充、仅换图形来源（风格变化不足，用户「不满意」无法被充分回应）；完全采用 Filled 全套（见决策 1）。

## 决策 3：色值归一化（P-01 门禁兼容）

- **Decision**: Material 官方 SVG 使用 `fill="#000000"`/`stroke="#000000"`，不在 P-01 白名单。素材落地流程 = **下载 → 批量归一化（`#000000`/`black` → `#D4D4D4` FG_TEXT）→ 入库 `actions/` → 现有 `check_icons.py` 门禁验证**。归一化为一次性工具操作（tasks 阶段内联脚本执行），**不新增仓库脚本**，避免引入 pytest 义务。
- **Rationale**: 保持门禁纯净（P-01 白名单不变，无豁免）；图标运行色统一为 FG_TEXT 默认主色（T-01），五态由 UI 层着色派生。
- **Alternatives considered**: 修改 check_icons.py 增加 Material 黑豁免（放宽门禁，否决）；在 render_icons.py 中加色值替换参数（改变共享脚本行为，收益不抵风险，否决）。

## 决策 4：58 枚语义映射（contracts/icon-replacement-map.md）

- **Decision**: 建立 **`contracts/icon-replacement-map.md`**，逐条给出 58 个 `icon_id` → Material 图标名 + 来源风格（outlined/filled）+ 备注。映射规则：① 通用语义直接对应官方图标名（如 `file-open`→`folder_open`、`edit-undo`→`undo`、`view-zoom-in`→`zoom_in`、`animation-play`→`play_arrow`）；② 无直接对应者选语义最近（如 `analysis-probe`→`ads_click`、`analysis-slice`→`content_cut`、`analysis-contour`→`terrain`、`tools-colormap`→`palette`）；③ 实在无对应且语义差异大的专业图标（如 `analysis-cutline`、`animation-param-scan`），按决策 1 兜底本地重绘或 Filled 变体，仍须过门禁。
- **Rationale**: 映射表是「可检查」的替换验收契约（覆盖规则 1 继续成立：map ↔ SVG 一一对应）；避免实现阶段逐枚临场发挥。
- **Alternatives considered**: 无映射表直接替换（无法评审与验收，否决）。

## 决策 5：验收方法（复用 002 双评审制）

- **Decision**: 替换后验收 = ① 自动化门禁：`check_icons.py` 0 违规 + `render_icons.py`/`gen_qrc.py` 全链路 + 全量构建 + `ctest`/`pytest`；② 人工评审：更新 `docs/design/mockups/005-icon-set/blind-test.md` 抽样清单（从 58 枚中抽 ≥10 枚覆盖六类，3 人盲测正确率 ≥90%），`conformance-review.md` 按 v2.0.0 契约逐项勾选；③ 保留项零改动：`git diff` 校验 `app-icon.svg` 与 `view-panel-console.svg` 无变化。
- **Rationale**: 与 002 SC-003/SC-004 验收程序一致，可重复、可复现；资源型功能无新增逻辑，自动化验证以门禁脚本 + 构建为准（宪法 Test-First 适用性见 plan.md Constitution Check）。

## 决策 6：渲染与文档链路（全复用）

- **Decision**: `render_icons.py` / `gen_qrc.py` / `make_mockups.py` / `update_screenshots.ps1` **全部复用、零改动**。保留项 SVG 未变 → 重渲染后 PNG 无内容 diff（SC-002 自动满足）。文档同步：`docs/design/icon-spec.md`（风格与来源更新）、`docs/design/ui-guidelines.md`（如有涉及）、`README.md`（图标体系描述）、mockups 说明。
- **Rationale**: 管线已验证（002 交付），避免重复实现；文档同步为宪法 v4.2.0 质量门禁要求。

---

## 未决项（Deferred）

| 项 | 说明 | 移交 |
|---|---|---|
| 58 枚逐一 Material 映射的最终确认 | 映射表中个别专业语义图标（cutline/param-scan 等）的取舍 | tasks 阶段按映射表执行，必要时人工拍板 |
| icon-style-spec v2.0.0 具体条款措辞 | 修订发生在实现阶段（002 契约文件内），plan 只锁定方向 | 实现阶段按「治理」记录修订 |

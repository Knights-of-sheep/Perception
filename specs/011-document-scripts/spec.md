# Feature Specification: scripts 脚本说明文档

**Feature Branch**: `011-document-scripts`

**Created**: 2026-09-19

**Status**: Draft

**Input**: User description: "在 E:\spec-work\Perception\scripts 下增加说明文档，讲明白各脚本的作用和使用方法；同时删除掉无用的脚本，另外要有对 E:\spec-work\Perception\tests 下各类型用例的脚本"

## Clarifications

### Session 2026-09-19

- Q: 你提到要「删除掉无用的脚本」——希望如何确定并删除？ → A: 由我审查 13 个脚本，列出疑似无用/冗余候选，用户确认后再删除（不擅自删除）。
- Q: 文档怎样覆盖 `tests/` 下各类型用例？ → C: 既新增「测试运行」说明章节，也在 `tests/` 下新增针对各类型的辅助脚本，并一并文档化。

## User Scenarios & Testing *(mandatory)*

### User Story 1 - 新人/协作者快速理解并运行脚本 (Priority: P1)

一名刚接触仓库的开发者或 CI 维护者，面对 `scripts/` 下的现有脚本（`.ps1` 与 `.py` 混合，清理前共 13 个）以及 `tests/` 下的测试辅助脚本，无法从文件名判断其用途、参数与前置依赖。他希望有一个集中的说明文档，能快速查到每个脚本「做什么、怎么跑、需要什么环境、有哪些注意事项」，并能据此运行各类型测试。

**Why this priority**: 这是本功能的核心价值——把散落在各脚本头部注释里的信息，统一沉淀为一份可读、可检索的文档，直接降低上手成本与误操作风险。

**Independent Test**: 任取一个脚本（如 `build.ps1`），仅凭本文档即可在 5 分钟内理解其用途并成功执行一条示例命令；文档中该脚本条目包含用途、参数、依赖、示例四要素。

**Acceptance Scenarios**:

1. **Given** 文档已生成，**When** 用户查阅 `scripts/` 说明文档，**Then** 能看到 `scripts/` 下经清理后保留的全部脚本条目，无遗漏。
2. **Given** 用户想运行 `build.ps1`，**When** 他按文档中给出的示例执行，**Then** 能复现脚本的预期行为（如编译或跑测试）。
3. **Given** 用户机器未安装某脚本依赖（如 PyQt5），**When** 他阅读文档的依赖说明，**Then** 能提前知晓并安装，而非运行后才报错。
4. **Given** 用户想运行测试，**When** 他查阅文档「测试」章节，**Then** 能据此用 CTest/pytest 或 `build.ps1` 开关成功运行对应类型用例。

---

### User Story 2 - 维护者保证文档与脚本同步 (Priority: P2)

当脚本被新增、重命名、删除或参数变更时，维护者需要一份明确的约定：改动脚本时必须同步更新说明文档，使文档始终是脚本的真实反映，避免出现「文档说一套、脚本做一套」。

**Why this priority**: 文档若落后于代码，会迅速失去信任、变成负资产。把「同步」作为明确规则，保证文档长期可用。

**Independent Test**: 在 PR 评审清单中核对——若本次变更涉及 `scripts/` 或 `tests/` 下脚本的增删改，则同一 PR 必须包含对应文档更新。

**Acceptance Scenarios**:

1. **Given** 某次提交新增了一个脚本，**When** 评审该 PR，**Then** 文档中已出现该新脚本的条目（或文档更新作为 PR 一部分）。
2. **Given** 某脚本的参数发生变更，**When** 评审该 PR，**Then** 文档中该脚本的参数说明已同步更新。

---

### Edge Cases

- 新增/删除脚本后文档未同步：需在 PR 评审清单中作为门禁项检查（见 FR-006）。
- 删除候选脚本需经用户确认后再删：误删会破坏构建/校验工作流，故采用「先审查列候选、确认后删」流程（FR-008），不得未经确认擅自删除。
- 部分脚本（如 `format_all.ps1`）依赖外部工具（clang-format）且未安装：文档必须明示依赖与安装方式，并说明未安装时的退出码/报错行为。
- 脚本头部注释与实际行为不一致：文档以脚本实际行为为准，必要时反馈修正注释（文档不复制明显过时的注释）。
- 用户环境为 Linux/macOS：`build.ps1`/`clean.ps1` 为 PowerShell，文档应说明主要在 Windows + VS2022 环境使用；`.py` 脚本可跨平台但依赖项需自行安装。
- 新增 `tests/` 辅助脚本后，文档需同步覆盖其用途/参数/依赖（FR-006/FR-010）。

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: 系统/项目 MUST 在 `scripts/` 目录下提供一份说明文档（建议 `scripts/README.md`），集中描述 `scripts/` 下每个脚本的作用与使用方法。
- **FR-002**: 文档 MUST 为每个脚本包含以下要素：脚本文件名、一句话用途说明、支持的参数/开关及其含义、运行前置依赖（环境/工具/Python 包）、至少一条可复制的运行示例、以及必要的注意事项（如退出码含义、关联 spec、相关产物路径）。
- **FR-003**: 文档 MUST 覆盖 `scripts/` 下经清理后保留的全部脚本（初始 13 个，删除候选见 FR-008 流程），不得遗漏。
- **FR-004**: 文档 MUST 使用中文编写，并与 `README.md`、constitution 的文档风格保持一致（面向人类读者，避免堆砌内部实现细节）。
- **FR-005**: 文档 MUST 对每个脚本标注其前置依赖与安装方式（例如：`PyQt5`、`PyYAML`、`clang-format`、VS2022/CMake、Pillow 等），使读者可独立准备运行环境。
- **FR-006**: 项目 MUST 约定：新增/重命名/删除/修改 `scripts/` 或 `tests/` 下脚本（尤其是参数）时，必须在同一 PR 中同步更新说明文档；该约定应写入 PR 评审清单作为同步门禁。
- **FR-007**: 文档 MAY 对脚本进行归类（如：构建与清理、代码门禁/lint、资源生成、测试数据/截图、图标/主题校验、文档同步、测试辅助），并可提供一张「快速索引表」便于速查。
- **FR-008**: 项目 MUST 在执行文档化前审查 `scripts/` 现有脚本，列出疑似无用/冗余的候选清单（附理由），经用户确认后方可删除；不得未经确认擅自删除脚本（避免破坏构建/校验工作流）。删除后文档须同步更新（见 FR-006）。
- **FR-009**: 文档 MUST 新增「测试」章节，说明 `tests/` 下各类型用例的运行方式：C++ 测试经 CTest、Python 测试经 pytest；并注明 `build.ps1 -UnitTests` / `-Pytest` 开关，以及可直接调用的 `ctest` / `pytest` 命令与典型参数。
- **FR-010**: 项目 MUST 在 `tests/` 下新增针对各类型用例的辅助脚本（例如批量运行、结果报告生成等），且这些脚本 MUST 同样满足 FR-002 的文档要素，并在「测试」章节一并说明。

### Key Entities *(include if feature involves data)*

- **Script（脚本）**: 属性包括文件名、语言（`.ps1`/`.py`）、用途、参数列表、依赖项、运行示例、退出码、关联规格/spec。文档中每条目即一个 Script 的投影。
- **Documentation File（说明文档）**: 单一来源，描述 `scripts/` 下保留 Script 与 `tests/` 下辅助 Script；其一致性由 FR-006 的同步约定保证。
- **Test Helper Script（测试辅助脚本）**: 位于 `tests/` 下，用于运行/汇总各类型测试用例（C++ CTest、Python pytest），自身亦作为 Script 被文档化（FR-010）。

当前 `scripts/` 下 13 个脚本清单（清理前快照；删除候选经 FR-008 确认后将从文档中移除）：

| 文件 | 语言 | 用途 |
|------|------|------|
| `build.ps1` | ps1 | CMake 配置 + 编译 +（可选）CTest/pytest，支持 GUI 构建 |
| `clean.ps1` | ps1 | 清理构建/运行产物，恢复到「克隆即构建」状态 |
| `format_all.ps1` | ps1 | 全库 clang-format 对齐（检查或就地改写） |
| `check_line_counts.ps1` | ps1 | 行数红线门禁（.cpp≤800/.h≤500/.hpp≤800） |
| `check_pragma_once.ps1` | ps1 | 头文件 `#pragma once` 缺失检测 |
| `update_screenshots.ps1` | ps1 | 用最新构建重生成 `docs/screenshots` 截图 |
| `check_icons.py` | py | 图标 SVG 色板/命名/覆盖/字段符合性校验 |
| `check_theme_contrast.py` | py | 主题色 WCAG 对比度校验 |
| `gen_qrc.py` | py | 由已渲染 PNG/ICO 生成 `theme.qrc` 图标资源 |
| `make_mockups.py` | py | 生成图标总览/图标栏/主窗口 mockup 视觉稿 |
| `make_test_data.py` | py | 生成 `tests/data` 各格式族测试夹具 |
| `render_icons.py` | py | 将 SVG 源渲染为多尺寸 PNG/ICO |
| `sync_file_types.py` | py | 由 `file_type_catalog.h` 同步/校验文件类型文档 |

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: 一名不熟悉仓库的开发者，仅凭本文档即可定位、理解并成功运行任意一个脚本或任一类型测试，单条目上手时间 ≤ 5 分钟。
- **SC-002**: 文档覆盖率 = 100%——`scripts/` 下保留脚本与 `tests/` 下新增辅助脚本均在文档中有对应条目，无缺失。
- **SC-003**: 文档准确性——当脚本参数/依赖变更时，文档在同一次 PR 内更新（可由评审清单核对，目标 100% 同步率）。
- **SC-004**: 每个脚本条目至少包含「用途 + 运行示例 + 依赖」三要素（可逐项自检）。
- **SC-005**: 无用脚本清理流程已执行——`scripts/` 下疑似无用/冗余候选已列出并经用户确认，确认删除的已从仓库与文档中移除。

## Assumptions

- **放置位置**：按用户明确要求，文档置于 `scripts/` 目录下（建议 `scripts/README.md`）。注：constitution 的「工作流规则」要求功能/重构同步 `docs/` 与根 `README.md`；本功能为纯脚本文档，遵循用户显式指定的 `scripts/` 位置，但建议在根 `README.md` 中以一行链接指向本文件，以满足「文档同步」精神（不强制，列为可选）。
- **语言**：中文（与 `README.md`、constitution 一致）。
- **删除脚本流程（Q1=A）**：采用「先审查列候选、用户确认后删」；除经确认删除外，本功能不修改既有脚本逻辑。本功能会新增 `tests/` 辅助脚本与文档（见下）。
- **tests 覆盖方式（Q2=C）**：既在文档新增「测试运行」说明章节，也在 `tests/` 下新增针对各类型的辅助脚本并一并文档化。新增测试辅助脚本默认置于 `tests/` 下（建议 `tests/runners/` 或 `tests/` 根目录），统一在 `scripts/README.md` 的「测试」章节说明，并交叉引用。
- **运行环境**：`.ps1` 主要面向 Windows + PowerShell + VS2022；`.py` 脚本跨平台但依赖需自装（文档须注明）。
- **覆盖范围**：`scripts/` 下现有 13 个脚本为清理前快照，最终以 FR-008 确认结果为准；`tests/` 辅助脚本为本次新增。

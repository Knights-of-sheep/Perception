# Research: scripts 脚本说明文档（含无用脚本清理 + tests 辅助脚本）

**Branch**: `011-document-scripts` | **Date**: 2026-09-19
**Feature**: [spec.md](./spec.md)

本文件解决 Phase 0 的未知项，为 plan/data-model/quickstart 提供依据。

---

## R1. 无用脚本审计（FR-008，Q1=A）

**方法**：遍历 `scripts/` 全部 13 个脚本，检查其在仓库中的引用情况（`README.md` / `CONTRIBUTING.md` / `CMakeLists.txt` / CI / 各 specs），判断是否有「死代码 / 重复 / 低价值」迹象。

**发现**：通过全仓引用检索（含 `*.md/*.ps1/*.py/*.yml/*.cmake` 等），**13 个脚本均被引用**，无未使用迹象：

| 脚本 | 引用位置（节选） | 判定 |
|------|------------------|------|
| `build.ps1` | README、CONTRIBUTING、CI、多 specs | 核心，保留 |
| `clean.ps1` | README、CONTRIBUTING | 常用，保留 |
| `format_all.ps1` | 006-refactor、CONTRIBUTING（门禁） | CI 门禁，保留 |
| `check_line_counts.ps1` | 006-refactor（FR-001/002/003） | 红线门禁，保留 |
| `check_pragma_once.ps1` | 006-refactor（FR-004） | 头文件门禁，保留 |
| `update_screenshots.ps1` | docs/screenshots 生成流程 | 文档依赖，保留 |
| `check_icons.py` | 002/007 icon 规格、conformance-review | 图标门禁，保留 |
| `check_theme_contrast.py` | 主题体系 | 主题校验，保留 |
| `gen_qrc.py` | 图标渲染流水线（T022） | 资源生成，保留 |
| `make_mockups.py` | 002/007（T020/T021） | 设计稿生成，保留 |
| `make_test_data.py` | 009 测试夹具 | 测试数据，保留 |
| `render_icons.py` | 002/007（T017） | 图标渲染，保留 |
| `sync_file_types.py` | 009（FR-011/SC-005） | 文档同步门禁，保留 |

**结论**：
- **客观层面无可判定为「无用」的脚本**——全部参与构建/门禁/资源/测试/文档链路。
- 因此「无用脚本」清单需由用户主观确认（FR-008 流程）：实现阶段将向用户呈交上述审计表，由用户勾选要删除的项；未被勾选的一律保留。
- **提供性候选（供用户参考，非结论）**：若坚持精简，可优先考虑 `update_screenshots.ps1`（重、依赖最新构建、偶发使用）与 `make_mockups.py`（一次性设计稿、依赖 PyQt5）；但二者均有明确用途，默认建议保留。

**Decision**：删除清单以「用户确认」为唯一依据；审计表即 FR-008 的候选清单载体。

---

## R2. tests/ 测试辅助脚本设计（FR-009 / FR-010，Q2=C）

**目标**：既在文档新增「测试运行」章节，也在 `tests/` 下新增针对各类型用例的辅助脚本。

**各类型用例现状**：
- **C++ 单元测试**：`tests/cpp/`，经 CMake+CTest 运行；`build.ps1 -UnitTests` 即触发 `ctest`。
- **Python 命令层测试**：`tests/python/`，经 `pytest` 运行；`build.ps1 -Pytest` 触发。

**方案比选**：
- 方案 A：复用 `build.ps1 -UnitTests -Pytest`（不新增脚本）。→ 不满足 FR-010（要求新增辅助脚本）。
- 方案 B：在 `tests/` 下新增跨平台 Python 运行器，按类型拆分。
- 方案 C：新增 PowerShell 运行器（与 build.ps1 同生态，但限 Windows）。

**Decision**：采用 **方案 B**——新增两个跨平台 Python 辅助脚本（与仓库既有 `.py` 脚本生态一致，且 Python 测试本身即 pytest，复用 `subprocess` 调用 `ctest`/`pytest` 最自然）：

- `tests/run_cpp_tests.py`：调用 `ctest`（定位构建目录，默认 `build/` 或 `build-gui/`），支持 `--config Release|Debug`、解析结果、可选输出 JUnit XML 报告（`--junit <path>`）。
- `tests/run_python_tests.py`：调用 `pytest tests/python`，支持 `--report <path>` 输出摘要/HTML。

**Rationale**：
- 跨平台（开发者可在非 Windows 跑 Python 测试）；
- 与现有 `.py` 脚本（make_test_data、render_icons 等）风格统一；
- 职责单一、可独立运行，且被 `scripts/README.md` 的「测试」章节统一文档化（FR-002/FR-009/FR-010）。

**Alternatives considered**：纯 PowerShell 方案（C）被拒——限制 Windows，且 pytest 在 Linux CI 更常用；复用 build.ps1（A）不满足 FR-010 的「新增辅助脚本」硬性要求。

---

## R3. 文档放置与结构（FR-001/FR-004/FR-007）

**Decision**：
- 文档文件：`scripts/README.md`（用户显式指定 `scripts/` 目录）。
- 结构：按 FR-007 归类（构建与清理 / 代码门禁 / 资源生成 / 测试数据·截图 / 图标·主题校验 / 文档同步 / 测试辅助），并提供「快速索引表」。
- 新增「测试」章节（FR-009），覆盖 CTest / pytest 运行方式及 `build.ps1` 开关，并文档化 R2 的两个新辅助脚本。
- 语言：中文（与 README、constitution 一致）。
- 可选：根 `README.md` 增加一行指向 `scripts/README.md`（满足 constitution「文档同步」精神，非强制）。

---

## R4. 宪法契合性

- 本特性**不修改** `src/core`、`src/ui`、`src/render`、VTK、格式读取器，因此「Layered Core / Command-Driven / Local Design Source / Scope / Technology Stack」各门禁均不适用（无违反）。
- 「Spec-First / Test-First」：spec 已通过 clarify 阶段；新增的 `tests/` 辅助脚本本身属测试基础设施，经运行即验证（见 quickstart）。
- 「文档与代码共生 / 工作流规则」：本特性正是补 `scripts/` 文档，并约定脚本增删同步文档（FR-006），契合 constitution IV 与工作流规则。

---

## 残留假设（非阻塞）

- 新测试辅助脚本默认置于 `tests/` 根（非 `tests/runners/` 子目录），命名 `run_cpp_tests.py` / `run_python_tests.py`；如后续规划需要可调整。
- 删除清单最终以用户确认为准（R1）。

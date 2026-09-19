# Implementation Plan: scripts 脚本说明文档（含无用脚本清理 + tests 辅助脚本）

**Branch**: `011-document-scripts` | **Date**: 2026-09-19 | **Spec**: [spec.md](./spec.md)

**Input**: Feature specification from `/specs/011-document-scripts/spec.md`

## Summary

在 `scripts/` 下新增集中说明文档 `scripts/README.md`，讲清现有脚本的用途、参数、依赖与示例；按 FR-008 先审计并列出疑似无用脚本候选供用户确认后删除；按 FR-009/FR-010 在文档新增「测试」章节，并在 `tests/` 下新增跨平台测试辅助脚本（`run_cpp_tests.py` / `run_python_tests.py`）。纯文档 + 脚本工具特性，不改动 `src/` 业务代码。

## Technical Context

**Language/Version**: PowerShell 5.1（`*.ps1` 运行环境）；Python 3.13（仓库既定 CPython，`.py` 脚本生态一致）

**Primary Dependencies**: 现有脚本依赖 `PyQt5` / `PyYAML` / `Pillow` / `clang-format` / VS2022(CMake,Ninja)；新增测试辅助脚本仅依赖标准库 `subprocess` + `pytest`（运行期）

**Storage**: N/A（仅 Markdown 文档与脚本文件）

**Testing**: 既有 `ctest`（C++ 单元测试）+ `pytest`（Python 命令层测试）；本特性新增的 `tests/` 辅助脚本本身即测试运行器，经运行即验证

**Target Platform**: Windows（`.ps1` 主环境）+ 跨平台（`.py` 脚本，含新增测试运行器）

**Project Type**: 内部工具 / 文档（internal tooling & docs）

**Performance Goals**: N/A（文档与轻量脚本，无性能约束）

**Constraints**: 不修改 `src/core`、`src/ui`、`src/render`、VTK、格式读取器（见宪法门禁）

**Scale/Scope**: 覆盖 `scripts/` 13 个脚本（清理前）+ `tests/` 2 个新增辅助脚本

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

- [x] **Spec-First**：`specs/011-document-scripts/spec.md` 已通过 clarify 阶段定稿（宪法 I）
- [x] **Test-First**：本特性不新增 `src/core` 逻辑；新增 `tests/` 辅助脚本属测试基础设施，经运行即验证（宪法 II）；既有 `ctest`/`pytest` 门禁不受影响
- [x] **Layered Core**：未改动 `src/core` 的 model/io/process/event 层（N/A，无违反）（宪法 III）
- [x] **Command-Driven**：未新增数据处理逻辑，未绕过命令层（N/A，无违反）（宪法 IV）
- [x] **Local Design Source**：未改动 UI，未引入 figma（N/A，无违反）（宪法 V）
- [x] **Scope**：未涉及文件格式范围（N/A，无违反）（宪法 VI）
- [x] **Technology Stack**：仅新增 `.py` 测试运行器（标准库+pytest），符合技术栈约束；未引入新依赖/框架（宪法「技术栈约束」）

> 全部门禁通过或 N/A 无违反；无需 Complexity Tracking 条目。

## Project Structure

### Documentation (this feature)

```text
specs/011-document-scripts/
├── plan.md              # This file (/speckit.plan command output)
├── research.md          # Phase 0 output (audit + tests 辅助脚本设计)
├── data-model.md        # Phase 1 output (Script / Documentation / Test Helper Script)
├── quickstart.md        # Phase 1 output (end-to-end validation)
├── contracts/           # N/A — 纯内部工具/文档，无外部接口契约（见下）
└── tasks.md             # Phase 2 output (/speckit.tasks command - NOT created here)
```

### Source Code (repository root)

```text
scripts/
└── README.md            # 新增：集中说明文档（本特性核心交付）

tests/
├── run_cpp_tests.py     # 新增：CTest 运行器（--config / --junit）
└── run_python_tests.py  # 新增：pytest 运行器（--report）

# scripts/ 下经用户确认后可能删除的脚本（默认无）
```

**Structure Decision**: 文档置于用户显式指定的 `scripts/README.md`；测试辅助脚本置于 `tests/` 根（与 `tests/cpp`、`tests/python`、`tests/data` 同级，语义清晰）。`contracts/` 不适用——本特性为内部工具与文档，无对外接口契约。

## Complexity Tracking

> 无需填写：Constitution Check 无违反，无复杂性豁免项。

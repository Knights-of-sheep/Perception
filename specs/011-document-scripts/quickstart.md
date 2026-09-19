# Quickstart: scripts 脚本说明文档（含清理 + tests 辅助脚本）

**Branch**: `011-document-scripts` | **Date**: 2026-09-19
**Feature**: [spec.md](./spec.md) | **Plan**: [plan.md](./plan.md) | **Data Model**: [data-model.md](./data-model.md)

本指南描述如何端到端验证本特性（文档生成、无用脚本清理确认、tests 辅助脚本运行）。不含实现细节（见 `tasks.md` 与实现阶段）。

---

## 前置条件

- 仓库已克隆，已切到 `011-document-scripts` 分支。
- 运行 `.ps1`：Windows + PowerShell + VS2022（CMake/Ninja）。
- 运行 `.py`：`python` 可用，且已安装脚本所需依赖（见各脚本文档，如 `PyQt5`、`PyYAML`、`Pillow`、`pytest`）。
- 构建产物存在（跑测试前需先 `build.ps1 -Gui` 或对应配置）。

---

## 验证场景

### 场景 1：文档可读且覆盖全部脚本（SC-002 / SC-004）

1. 打开 `scripts/README.md`。
2. 核对存在「快速索引表」与各分类章节。
3. 逐个核对 `scripts/` 下保留脚本均有条目，且每条目含「用途 + 运行示例 + 依赖」三要素。
4. **预期**：13 个脚本（清理前）均有条目；确认删除的项已从文档移除。

### 场景 2：无用脚本清理流程（FR-008 / SC-005）

1. 查看实现阶段呈交的「无用脚本审计表」（research.md R1）。
2. 用户勾选要删除的脚本（默认无；候选参考 `update_screenshots.ps1` / `make_mockups.py`）。
3. 实现按勾选执行 `git rm scripts/<name>` 并同步更新 `scripts/README.md`。
4. **预期**：未被勾选的脚本全部保留；删除项在文档中消失；仓库仍可正常构建。

### 场景 3：测试「运行说明」章节可用（FR-009）

1. 在 `scripts/README.md` 的「测试」章节，按示例运行：
   - C++：`ctest --test-dir build --output-on-failure`（或 `powershell -File scripts/build.ps1 -UnitTests`）
   - Python：`pytest tests/python -q`（或 `powershell -File scripts/build.ps1 -Pytest`）
2. **预期**：两类测试均可按文档独立运行并产出结果。

### 场景 4：新增 tests 辅助脚本运行（FR-010）

1. 运行 `python tests/run_cpp_tests.py --config Release` → 调 `ctest`，打印摘要（可选 `--junit report.xml`）。
2. 运行 `python tests/run_python_tests.py --report report.txt` → 调 `pytest tests/python`，输出报告。
3. **预期**：两个脚本独立运行成功，输出可读摘要；文档「测试」章节已含其用途/参数/依赖说明。

---

## 成功判据对照

- SC-001：新人凭文档 ≤5 分钟上手任一脚本/测试类型。
- SC-002：文档覆盖 `scripts/` 保留脚本 + `tests/` 辅助脚本，无遗漏。
- SC-003：脚本变更同 PR 含文档更新（评审清单核查）。
- SC-004：每条目含「用途+示例+依赖」。
- SC-005：无用脚本清理流程已执行并确认。

## 关联产物

- 文档：`scripts/README.md`
- 新增脚本：`tests/run_cpp_tests.py`、`tests/run_python_tests.py`
- 可能删除：`scripts/` 下经用户确认的项

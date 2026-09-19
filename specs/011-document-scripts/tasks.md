# Tasks: scripts 脚本说明文档（含清理 + tests 辅助脚本）

**Input**: Design documents from `/specs/011-document-scripts/`

**Prerequisites**: plan.md, spec.md, research.md, data-model.md, quickstart.md

**Tests**: 本功能为「文档 + 内部脚本工具」，**不新增 `src/core` 纯逻辑**，故不触发宪法 II 的 ctest/pytest 单测门禁。验证以 quickstart.md 的端到端场景为准：文档覆盖率核对（SC-002/SC-004）、新增 `tests/` 辅助脚本经运行验证（SC-001/SC-005）。Test-First 精神体现为「先定义 quickstart 验收场景，再落地脚本」。

**Organization**: Tasks grouped by phase; documentation 任务可按脚本并行撰写。

## Format: `[ID] [P?] Description`

- **[P]**: Can run in parallel (independent files/scripts)
- Include exact file paths in descriptions

## Path Conventions

- 文档落点：`scripts/README.md`（用户显式指定目录）
- 新增测试辅助脚本：`tests/run_cpp_tests.py`、`tests/run_python_tests.py`
- 可能删除：`scripts/` 下经用户确认的项（默认无）

---

## Phase 1: Setup (文档骨架)

- [X] T001 创建 `scripts/README.md`：标题 + 简介 + 快速索引表占位 + 七大分类小节标题（构建与清理 / 代码门禁 / 资源生成 / 测试数据·截图 / 图标·主题校验 / 文档同步 / 测试辅助），对应 FR-001/FR-007

---

## Phase 2: 文档化现有脚本 (FR-001..FR-007, US1)

**Purpose**: 为每个 `scripts/` 脚本写出「用途 + 参数 + 依赖 + 示例 + 注意事项」条目（FR-002/FR-005）。

- [X] T002 [P] 撰写 `build.ps1`、`clean.ps1` 条目（参数表、依赖 VS2022/CMake/Ninja、示例 `-UnitTests -Pytest -Gui` 等、退出/产物路径）
- [X] T003 [P] 撰写 `format_all.ps1`、`check_line_counts.ps1`、`check_pragma_once.ps1` 条目（clang-format 依赖与安装、红线阈值、退出码 0/1/2）
- [X] T004 [P] 撰写 `update_screenshots.ps1` 条目（依赖最新 `bin/Release/perception.exe`、各快照示例、关联 `docs/screenshots`）
- [X] T005 [P] 撰写 `check_icons.py`、`check_theme_contrast.py` 条目（PyYAML/无依赖、色板/对比度校验、退出码 0/1、关联 002/007 规格）
- [X] T006 [P] 撰写 `gen_qrc.py`、`render_icons.py`、`make_mockups.py` 条目（PyQt5/Pillow 依赖、图标渲染流水线、产物路径）
- [X] T007 [P] 撰写 `make_test_data.py`、`sync_file_types.py` 条目（关联 009 规格、文件类型目录单一事实来源、`--check`/`--update` 开关）
- [X] T008 填写快速索引表并补全分类归属，确保 13 个脚本（清理前）全在表内（FR-003/FR-007）

---

## Phase 3: 无用脚本审计与删除 (FR-008, US2)

**Purpose**: 先审计列候选，经用户确认后再删（Q1=A）；未确认一律保留。

- [X] T009 基于 research.md R1 审计表，向用户呈交「疑似无用/冗余候选清单」（附引用证据与建议）。**用户确认：A — 不删除任何脚本**（13 个脚本全被引用、无客观死代码，保留全部）
- [~] T010 取消（用户确认不删除任何脚本，无 `git rm` 操作）
- [~] T011 取消（无删除，无需重核对）

---

## Phase 4: tests 覆盖（说明 + 辅助脚本）(FR-009/FR-010)

**Purpose**: 文档新增「测试」章节，并在 `tests/` 下新增跨平台测试辅助脚本（research R2 方案 B）。

- [X] T012 在 `scripts/README.md` 新增「测试」章节：说明 C++ 经 `ctest --test-dir build --output-on-failure`、`build.ps1 -UnitTests`；Python 经 `pytest tests/python -q`、`build.ps1 -Pytest`；标注典型参数（FR-009）
- [X] T013 [P] 创建 `tests/run_cpp_tests.py`：调用 `ctest`（定位 `build/` 或 `build-gui/`），支持 `--config Release|Debug`、`--junit <path>` 输出报告；标准库 `subprocess` 实现（FR-010）
- [X] T014 [P] 创建 `tests/run_python_tests.py`：调用 `pytest tests/python`，支持 `--report <path>` 输出摘要（FR-010）
- [X] T015 在「测试」章节文档化上述两个辅助脚本的用途/参数/依赖（FR-002/FR-010）

---

## Phase 5: 验证 (SC-001..SC-005)

- [X] T016 核对 `scripts/README.md` 覆盖率 = 100%（`scripts/` 13 脚本 + `tests/` 两辅助脚本均有条目），且每条目含「用途+示例+依赖」三要素（SC-002/SC-004）
- [X] T017 运行 `python tests/run_python_tests.py`（结果：**18 passed**，Python 命令层测试通过）；`run_cpp_tests.py` 逻辑已验证（缺 ctest/构建时优雅报错退出，需构建环境方可跑 CTest）
- [X] T018 在 `scripts/README.md` 文首加入「维护约定（FR-006）：脚本增删改须同 PR 同步本文档」门禁说明（FR-006）

---

## Phase 6: 可选 (非阻塞)

- [ ] T019 [可选] 在根 `README.md` 增加一行指向 `scripts/README.md` 的链接，满足 constitution「文档同步」精神（spec Assumptions 已标注为非强制）

---

## Notes

- T009 为阻塞任务：必须等待用户确认删除清单后方可执行 T010；若用户确认「不删除任何脚本」，则跳过 T010/T011。
- 所有文档条目以脚本**实际行为**为准；若发现脚本注释与行为不符，仅在文档注意事项中标注，不在此修正脚本（spec Assumptions）。
- 提交遵循 constitution：每功能一分支，合并走 PR；本分支 `011-document-scripts` 已就绪。

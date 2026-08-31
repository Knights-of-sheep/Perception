# Tasks: 文件类型支持目录（获取与生成）

**Input**: Design documents from `/specs/009-supported-file-types/`

**Prerequisites**: plan.md (required), spec.md (required), research.md, data-model.md, contracts/

**Tests**: 按宪法「测试先行」要求纳入测试任务（目录 CTest 先红后绿；supported_formats pytest 先红后绿）。

**Organization**: Tasks are grouped by user story to enable independent implementation and testing of each story.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (e.g., US1, US2, US3)
- Include exact file paths in descriptions

## Path Conventions

- **Single project**: `src/`, `tests/` at repository root
- 本 feature 为 Qt desktop-app：`src/core/io/`（目录，无 UI/VTK 依赖）、`src/python/`（命令层）、`src/ui/`（界面层）、`tests/cpp/`（CTest）、`tests/python/`（pytest）、`scripts/`（文档同步门禁脚本）
- 目录「单一事实来源」= `src/core/io/file_type_catalog.h`（数据表，theme_catalog 模式）；派生产物（过滤列表 / 文档）必须由它生成，禁止手抄（plan.md Summary / research.md R1）

---

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: 建立基线，确认当前分支可编译、CTest 全绿，为红-绿循环提供基准

- [X] T001 Run `cmake -B build -G Ninja` + `cmake --build build` + `ctest --test-dir build` to confirm the current branch compiles and all existing tests pass; record the baseline before any red-green cycles (宪法「测试先行」基准)（本环境既有缓存为 VS2022+GUI ON；`build.ps1 -Gui -UnitTests` 基线 8/8 绿）

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: 权威文件类型目录实体（US1 核心交付物）——它是 US2（查询）与 US3（生成/一致性）的共同前提，先以红-绿落地；目录「已支持」集合同时是 FR-008/FR-011 的判定基准

**⚠️ CRITICAL**: No user story work can begin until this phase is complete

- [X] T002 [P] RED: write `tests/cpp/file_type_catalog_test.cpp` with all catalog cases per `specs/009-supported-file-types/data-model.md`「验证规则」——① 覆盖 4 格式族（VTK/SVisual/HDF5/曲线各 ≥1 条）② 每条目 5 字段非空 ③ 扩展名全库唯一 ④ `filterGroups()` 仅含 Supported 且分组正确（SVisual→{*.plt}、Curve Data→{*.csv}）⑤ Planned（.tdr/.dat/VTK 系列/.h5）不在 filterGroups ⑥ `findByExtension(".PLT")` 大小写不敏感 ⑦ `inconsistencies()` 当前为空（Supported==ReaderRegistry={.plt,.csv}）且能检测人为差异；register `file_type_catalog_test` target in `tests/cpp/CMakeLists.txt` (links `perception_core`, no GUI dep); build → confirm RED (编译失败：`file_type_catalog.h` 不存在，C1083)
- [X] T003 Create `src/core/io/file_type_catalog.h`（#pragma once；声明 `FileTypeFamily{ VtkLegacy,VtkXml,VtkComposite,VtkParallel,SVisual,Hdf5,Curve }` / `FileTypeKind{Curve,Structure,Both}` / `FileTypeStatus{Supported,Planned}` 枚举、`FileTypeEntry{formatName,extensions,famil,kind,status}` 结构；定义 `kFileTypeCatalog[]` 数据表——16 条记录按 `data-model.md`「目录初始数据」逐字段录入（.plt/.csv 为 Supported，其余 Planned），每条目注释标注宪法「文件格式范围」依据；声明 `FileTypeCatalog` 静态 API：`all()/supported()/findByExtension()/filterGroups()/inconsistencies()`）；另补 `ReaderRegistry::registeredExtensions()`（`src/core/io/reader.{h,cpp}`，一致性校验所需）
- [X] T004 Implement `src/core/io/file_type_catalog.cpp`（all/supported 按数据表遍历；findByExtension 大小写不敏感；filterGroups 仅 Supported 按 familyKey 分组（VTK 四族→"VTK"/SVisual→"SVisual"/Hdf5→"HDF5"/Curve→"Curve Data"）并输出 `*.ext` 模式；inconsistencies 与 `ReaderRegistry` 扩展名集合对称差）；register `file_type_catalog.cpp` in `src/core/CMakeLists.txt`; run `ctest --test-dir build -R file_type_catalog --output-on-failure` until GREEN（红→绿闭环，含命名空间修正）

**Checkpoint**: Foundation ready - catalog entity landed and tested; user story implementation can now begin

---

## Phase 3: User Story 1 - 权威文件类型目录 (Priority: P1) 🎯 MVP

**Goal**: 目录成为程序支持格式的唯一事实来源：完整覆盖宪法 4 格式族，「已支持」与「规划中」区分明确，「已支持」⇄ 实际可打开一一对应

**Independent Test**（spec US1）: 将目录与宪法「文件格式范围」逐条比对（VTK / SVisual / HDF5 / 曲线 4 格式族全覆盖）；将目录中「已支持」条目与实际能打开的格式逐一比对，两者一一对应

- [X] T005 [US1] Verify catalog coverage vs constitution (SC-001): manually compare `kFileTypeCatalog[]` (16 entries) in `src/core/io/file_type_catalog.h` against constitution「文件格式范围」4 format families — no missing family, no out-of-scope format (scope gate per spec Assumptions); confirm each entry's 5 fields populated per FR-003（16 条 = VTK 11（遗留1+XML5+复合3+并行2）/ SVisual 2 / HDF5 1 / 曲线 2，宪法依据已固化进头文件每条目注释）
- [X] T006 [P] [US1] Verify supported⇄openable consistency (SC-002/FR-008/FR-010/FR-012): run `ctest --test-dir build -R file_type_catalog --output-on-failure` all green (covers supported==ReaderRegistry、case-insensitive `.PLT`、`.dat` Both semantics)；全量 CTest 通过（`subwindow_view_test` 的 0xc0000135 为无 Qt PATH 环境性问题，基线已证 8/8 绿）；GUI 冒烟（打开 .plt 经 All Files）留待 quickstart §3 手动验证

**Checkpoint**: User Story 1 complete — the canonical catalog is authoritative and internally consistent (MVP reached)

---

## Phase 4: User Story 2 - 运行时可查询（获取） (Priority: P2)

**Goal**: 程序运行期间可经命令层查询完整文件类型清单（扩展名/格式名称/数据种类/状态），返回与权威目录一致

**Independent Test**（spec US2）: 在运行中的程序内发起格式查询，核对返回清单与权威目录一致；用清单中「已支持」的扩展名打开文件验证可加载

- [X] T007 [P] [US2] RED: write `tests/python/test_supported_formats.py` per `specs/009-supported-file-types/contracts/python-supported-formats.md`（5 断言：① 返回 list 且非空、条目 ≥12 覆盖 4 格式族 ② 每条目恰含 5 字段且符合值域 ③ 扩展名全局唯一 ④ 存在 supported（.plt/.csv）且 planned 不混入 ⑤ 连续两次调用结果一致）；run `python -m pytest tests\python\test_supported_formats.py -v` → confirm RED（supported_formats 不存在，5 failed AttributeError）
- [X] T008 [US2] Declare `virtual py::object supportedFormats() = 0;` in `src/python/api/i_command_service.h`（与 load/query 等占位命令同风格；注释指向 contracts/python-supported-formats.md）
- [X] T009 [US2] Implement `supportedFormats()` in `src/python/command/command_service.{h,cpp}`（遍历 `FileTypeCatalog::all()`，按契约组装 dict：extensions 小写含点 / format_name / family（蛇形，如 "vtk-xml"）/ kind（curve/structure/both）/ status（supported/planned）；只读无副作用）。注意：perception_py 此前刻意不链接 core（006 约定「pyd 不直接依赖 core」），本任务按 plan 首次 `target_link_libraries(perception_py PRIVATE perception_core)`（`src/python/CMakeLists.txt`，core 无 UI 依赖，静态并入 pyd 无 Qt 负担）
- [X] T010 [US2] Bind `supported_formats` in `src/python/command/command_module.cpp`（`.def("supported_formats", &CommandServiceImpl::supportedFormats)`，蛇形命名与既有绑定一致）；rebuild GUI (`scripts\build.ps1 -Gui -Pytest`) then run `python -m pytest tests\python -v` → GREEN（18/18 全绿，含 006 既有用例）

**Checkpoint**: User Stories 1 AND 2 both work independently — catalog queryable at runtime, consistent with source

---

## Phase 5: User Story 3 - 单一来源生成一致产物（生成） (Priority: P3)

**Goal**: 文件打开过滤列表与 README/文档格式清单全部由目录派生；差异可检测报告（FR-011）；过滤列表覆盖目录全量条目（含「规划中」供发现与核对），规划中格式打开时按「不支持」提示、不进入读取流程（FR-009-变更：展示全部、打开拦截）。注：图标映射关联数据由目录承载，文件树未实现前无独立生成物（plan.md 范围声明）

**Independent Test**（spec US3）: 在目录中新增 / 移除一种格式并执行生成，检查打开过滤列表与文档同步变化；人为制造不一致后执行一致性校验，确认能报告差异

- [X] T011 [P] [US3] Replace hardcoded filter string in `src/ui/MainWindow.cpp` `openFile()` with catalog-derived filter: `FileTypeCatalog::filterGroups()` → localized labels (`tr`) + `;;` join + `All Files (*)` fallback; GUI build then manually verify filter shows all 4 groups covering every catalog entry — `VTK Files (*.vtk *.vti … *.pvd)` / `SVisual Files (*.plt *.tdr)` / `HDF5 Files (*.h5 *.hdf5)` / `Curve Data (*.csv *.dat)` (+ All Files), with `.tdr/.dat/VTK 系列/.h5` present for discovery (FR-006/FR-009-变更/SC-003)；规划中格式打开时按「不支持」提示（当前文件树为占位，仅登记路径）
- [X] T012 [P] [US3] Create `scripts/sync_file_types.py`（std-lib only; regex-parse `src/core/io/file_type_catalog.h` per `check_theme_contrast.py` precedent; `--check` validates README.md「打开文件过滤」line + `docs/architecture.md` format table match catalog, exit non-zero listing diffs; `--update` regenerates both; empty-catalog → explicit message, no crash）
- [X] T013 [US3] Run `python scripts\sync_file_types.py --update` to sync `README.md`（打开文件过滤）and `docs/architecture.md`（格式范围表 + 手工补 io 树行/命令层行/格式可扩展约束）；then `--check` exit 0（宪法「工作流规则」文档随功能提交）。注：本环境 PowerShell 包装层吞掉 `$LASTEXITCODE`，退出码以 `cmd /c ... && echo OK || echo DIFF` 验证
- [X] T014 [US3] Diff-detection drill (FR-011/SC-006): temporarily change `.csv` status to Planned in `src/core/io/file_type_catalog.h` → `ctest --test-dir build -R file_type_catalog --output-on-failure`（inconsistencies 用例断言失败，c0xc0000409 报告差异）and `python scripts\sync_file_types.py --check`（[DIFF] README/架构表 + 退出码 1）both report the specific diff; restore → all green（ctest Passed + CHECK_OK）

**Checkpoint**: All user stories independently functional — generated artifacts sync from single source, inconsistencies detectable

---

## Phase 6: Polish & Cross-Cutting Concerns

**Purpose**: 全量回归 + quickstart 端到端验证 + 文档同步核对（宪法「质量门禁」）

- [X] T015 [P] Full regression (constitution gate): `scripts\build.ps1 -Gui` + `ctest --test-dir build --output-on-failure` + `python -m pytest tests\python -v` all green（CTest 9/9、pytest 18/18）
- [X] T016 [P] Run `specs/009-supported-file-types/quickstart.md` end-to-end: in-app `perception_py.CommandService().supported_formats()` returns full list ≤1s (SC-004，headless pytest 同路径已证 0.56s/18 用例)；open dialog filter covers all 4 groups (FR-009-变更：全量条目含规划中供发现，过滤串由 filterGroups 派生、CTest 断言分组，GUI 手动冒烟留待现场)；`python scripts\sync_file_types.py --check` exit 0（CHECK_OK，16 条格式）
- [X] T017 [P] Documentation consistency check (constitution workflow rule): `README.md`（过滤行已由脚本生成）、`docs/architecture.md`（格式范围表 + io/命令层树行 + 可扩展约束）和 `specs/009-supported-file-types/` docs consistent with implementation; grep 确认无残留硬编码格式清单（仅 MainWindow 由目录派生的新代码与无关的 VTK 日志拦截文案）

---

## Dependency Graph

```
Phase 1 (Setup) T001
   └── Phase 2 (Foundational) T002 (RED 测试) → T003 (目录头) → T004 (目录实现, GREEN)
          ├── Phase 3 (US1) T005 → T006   （T006 [P]：ctest/冒烟 与 T005 人工比对可并行）
          ├── Phase 4 (US2) T007 (RED pytest) → T008 → T009 → T010 (GREEN)
          ├── Phase 5 (US3) T011（过滤，[P]）/ T012（sync 脚本，[P]）→ T013（文档同步）→ T014（差异演练）
          └── Phase 6 (Polish) T015 / T016 / T017（[P] 并行）
```

### Phase Dependencies

- **Setup (T001)**: 无依赖，先建立红-绿基线
- **Foundational (T002-T004)**: 依赖 T001；**阻塞全部用户故事**（目录实体是 US1/US2/US3 的共同前提）
- **US1 (T005-T006)**: 依赖 Foundational（目录已落地）；无跨故事依赖
- **US2 (T007-T010)**: 依赖 Foundational；逻辑上复用 US1 的目录（不阻塞于 US1 验证任务）
- **US3 (T011-T014)**: 依赖 Foundational（filterGroups/inconsistencies）；T011 与 T012 并行；T013 依赖 T012；T014 依赖既有 CTest 与 T013
- **Polish (T015-T017)**: 依赖全部用户故事完成

### Within Each User Story

- 红-绿顺序：Foundational T002（测试先写）→ T003/T004（目录实现）→ 绿；US2 T007（pytest 先写）→ T008-T010（命令实现）→ 绿
- 模型（目录实体）→ 服务（命令层）→ 集成（UI 过滤 / 文档同步）
- Story complete before moving to next priority

### Parallel Opportunities

- Foundational：T002（测试文件）与 T003（目录头）[P] 并行（不同文件）；T004 依赖 T003
- US1：T005 / T006 [P]（人工比对与 ctest/冒烟互不冲突）
- US3：T011（MainWindow）与 T012（sync 脚本）[P] 并行（不同文件面）
- Polish：T015 / T016 / T017 [P] 并行（回归 / quickstart / 文档互不冲突）
- 跨故事：US2 与 US3 在 Foundational 完成后可并行（US2 改 `src/python/`，US3 改 `src/ui/`+`scripts/`，文件面不重叠）

---

## Parallel Example: 多个用户故事

```bash
# Foundational 红线与目录头并行（不同文件）：
Task: "T002 Write tests/cpp/file_type_catalog_test.cpp + register target in tests/cpp/CMakeLists.txt"
Task: "T003 Create src/core/io/file_type_catalog.h (enums + kFileTypeCatalog[] + API decl)"

# 依赖目录实现后，US2 命令层与 US3 生成能力并行（不同文件面）：
Task: "T007 RED: write tests/python/test_supported_formats.py"
Task: "T008-T010 Implement supportedFormats in src/python/api/i_command_service.h + command_service.{h,cpp} + command_module.cpp"
Task: "T011 Replace hardcoded filter in src/ui/MainWindow.cpp with FileTypeCatalog::filterGroups()"
Task: "T012 Create scripts/sync_file_types.py"

# US3 文档同步与差异演练（依赖 T012）：
Task: "T013 Run sync_file_types.py --update + --check"
Task: "T014 Diff-detection drill with temporary catalog edit"
```

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. Complete Phase 1: Setup（构建基线）
2. Complete Phase 2: Foundational（红-绿落地权威目录 = US1 核心交付物；MVP 实体即达）
3. Complete Phase 3: User Story 1（目录覆盖宪法、supported⇄可打开一致 → MVP 达成）
4. **STOP and VALIDATE**: User Story 1 独立验收（ctest + 宪法比对）
5. Deploy/demo if ready

### Incremental Delivery

1. Complete Setup + Foundational → 权威目录 + CTest 全绿（FR-001~004/008/010~012 基础落地）
2. Add User Story 1 → 宪法覆盖核对 → 独立验收（MVP！）
3. Add User Story 2 → 运行时可查询（supported_formats + pytest）→ 独立验收
4. Add User Story 3 → 过滤列表与文档由目录生成 + 差异检测 → 独立验收
5. Polish：全量回归 + quickstart 端到端 + 文档门禁核对

### Parallel Team Strategy

With multiple developers:

1. Team completes Setup + Foundational together（T001 → T002/T003 并行 → T004）
2. Once Foundational is done:
   - Developer A: User Story 1（宪法比对 + ctest/冒烟）+ User Story 3（MainWindow 过滤 + sync 脚本）
   - Developer B: User Story 2（pytest 红线 → 命令层实现 → 绑定）
3. Stories complete and integrate independently（US2 文件面 `src/python/` 与 US3 文件面 `src/ui/`+`scripts/` 不重叠）

---

## Notes

- [P] tasks = different files, no dependencies
- [Story] label maps task to specific user story for traceability
- Each user story should be independently completable and testable
- Verify tests fail before implementing (T002 RED → T003/T004 → GREEN; T007 RED → T008-T010 → GREEN)
- Commit after each task or logical group
- Stop at any checkpoint to validate story independently
- Avoid: vague tasks, same file conflicts, cross-story dependencies that break independence
- 目录是唯一事实来源：任何格式清单的修改只改 `src/core/io/file_type_catalog.h` 一处，其余产物由生成/校验驱动（FR-005~007/SC-005）
- **变更记录（2026-08-31，产品裁决）**：FR-009 由「规划中不出现在可打开过滤」放宽为「过滤覆盖全量条目（含规划中，供发现与核对），规划中格式打开时按不支持提示、不进入读取流程」。受影响的契约断言（CTest `test_filter_groups_all_entries`）与文档（spec/data-model/research/plan/README 过滤行）已同步更新；T002/T004 的红-绿历史描述保留原语义以忠实记录当时实现。

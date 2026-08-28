# Tasks: Multi-Screen Maximize

**Input**: Design documents from `/specs/005-multi-screen-maximize/`

**Prerequisites**: plan.md (required), spec.md (required for user stories), research.md, data-model.md, contracts/window-maximize-contract.md, quickstart.md

**Tests**: 宪法 II Test-First + research R5 明确要求：核心几何/屏幕解析逻辑为纯函数并配 CTest 单测（`tests/cpp/window_geometry_test.cpp`）；多屏端到端为手动验证（quickstart.md 场景清单，对应 SC-001..003）。

**Organization**: Tasks are grouped by user story to enable independent implementation and testing of each story.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (e.g., US1, US2, US3)
- Include exact file paths in descriptions

## Path Conventions

- 单项目结构：`src/`, `tests/` 位于仓库根
- GUI 源码：`src/ui/`；C++ 单测：`tests/cpp/`（`PERCEPTION_BUILD_GUI=ON` 下注册，与 `layout_manager_test` 同模式）

---

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: 确认修复前基线，保证后续改动可对照验证（spec SC-004）

- [X] T001 在 `005-multi-screen-maximize` 分支确认工作区干净，执行 `.\scripts\build.ps1 -Gui -UnitTests -Config Debug` 确认修复前构建与 CTest 全绿，记录基线（对应 spec SC-004 回归对照）。注：基线构建发现并修复 004 遗留的 Debug Python 链接问题（subwindow_view_test LNK1104/LNK2019，tests/cpp/CMakeLists.txt 补充 /NODEFAULTLIB 与 /alternatename，与 src/app 一致）

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: 新增可测的窗口几何纯函数层——所有 User Story 的修复都复用该层，必须先于任何 US 完成

**⚠️ CRITICAL**: No user story work can begin until this phase is complete

- [X] T002 新增 `src/ui/window_geometry.h`：声明纯函数接口——目标屏解析（输入：屏幕几何矩形列表 + 窗口 frameGeometry；输出：目标屏矩形）与最大化几何计算（输入：屏幕几何矩形列表 + 窗口 frameGeometry；输出：最大化位置/尺寸）。接口输入为几何数据而非 `QWidget*`，保证可在 `tests/cpp` 无 GUI 实例直接单测（research R2/R5）
- [X] T003 新增 `tests/cpp/window_geometry_test.cpp`：按 Test-First 编写用例——两屏横排/竖排/不对称/负坐标（副屏在主屏左侧）的目标屏解析、最大化几何（位置/尺寸/负坐标）、窗口中心位于屏幕外时 fallback、恢复几何状态转换（normal geometry 保存/还原）。此时编译失败或断言失败（宪法 II：先失败）
- [X] T004 修改 `tests/cpp/CMakeLists.txt`：注册 `window_geometry_test`（在 `PERCEPTION_BUILD_GUI=ON` 条件下，与现有 `layout_manager_test` 同模式）
- [X] T005 新增 `src/ui/window_geometry.cpp`：实现纯函数——按窗口 frameGeometry 中心命中屏幕几何列表（含空指针/未命中 fallback 到首个屏幕），返回目标屏 `availableGeometry`（DIP，负坐标天然正确；research R2/R4）。运行 `ctest --test-dir build -C Debug --output-on-failure` 使 T003 用例全部转绿
- [X] T006 [P] 修改 GUI 目标构建配置（根 `CMakeLists.txt` 中 GUI 目标或对应子目录 CMakeLists）：将 `src/ui/window_geometry.cpp` 加入目标源文件，确认构建通过

---

## Phase 3: User Story 1 — 任意屏幕最大化成功 (P1)

**Story Goal**: 将主界面拖到任意屏幕后点击最大化，窗口在该屏幕工作区内完整最大化，不再"消失"（FR-001/002/004）

**Independent Test**: `ctest` 中 `window_geometry_test` 通过 + quickstart.md 3.1 全部步骤（副屏/主屏/跨屏最大化完整可见、不越界、不遮任务栏）

- [X] T007 [US1] 修改 `src/ui/MainWindow.cpp` 的 `nativeEvent` 中 `WM_GETMINMAXINFO` 分支（约 1933 行）：将 `screen()` 解析替换为调用 `window_geometry` 目标屏解析，以目标屏 `availableGeometry()` 填充 `ptMaxPosition/ptMaxSize`（根因修复，research R1/R2）
- [X] T008 [US1] 验证 US1：`window_geometry_test` 相关用例全部通过（跨屏边界/副屏在左/竖排/不对称等）；手动多屏场景（quickstart.md 3.1）需用户在真实多屏环境复核

---

## Phase 4: User Story 2 — 最大化状态可正确恢复 (P1)

**Story Goal**: 最大化后点击恢复，窗口回到最大化前的位置/尺寸与所在屏幕，不跳屏不越界；反复往返无漂移（FR-003/006）

**Independent Test**: quickstart.md 3.2 全部步骤（副屏最大化→恢复回原位原尺寸；连续 3 次往返无漂移）

- [X] T009 [US2] 修改 `src/ui/MainWindow.cpp`：扩展 `changeEvent(QEvent::WindowStateChange)`——进入最大化时确保 `normalGeometry_` 已记录（兜底取当前几何），退出最大化时 `setGeometry(normalGeometry_)` 还原（按钮/双击/任务栏右键统一路径，research R3）；`WM_GETMINMAXINFO` 在窗口尺寸改变前记录 `normalGeometry_`（最可靠时机）
- [X] T010 [US2] 修改 `src/ui/MainWindow.cpp` 的 `toggleMaximize()`（约 1004 行）：保持原逻辑（`showNormal()`/`showMaximized()`），几何恢复统一由 `changeEvent` 完成，按钮/双击语义一致（spec Assumptions：不改按钮语义）
- [X] T011 [US2] 验证 US2：恢复几何归属断言（最大化前原几何中心仍命中原屏）单测通过；手动多屏场景（quickstart.md 3.2）需用户在真实多屏环境复核

---

## Phase 5: User Story 3 — 显示配置变化后仍可最大化 (P2)

**Story Goal**: 分辨率调整、显示器连接/断开、混合 DPI 后触发最大化仍完整可见，绝不落入不可见区域（FR-005/006）

**Independent Test**: quickstart.md 3.3 全部步骤（分辨率降低后最大化、最大化时断开副屏回落主屏、混合 DPI 各屏最大化可见）

- [X] T012 [US3] 加固 `src/ui/MainWindow.cpp` 的 `WM_GETMINMAXINFO` 路径：每次消息从 `QGuiApplication::screens()` 动态解析（不缓存屏幕指针）、纯函数 fallback 链覆盖（未命中 → 首屏；对应项为空/断开 → 首个非空项），`changeEvent` 只使用 `normalGeometry_` 数值、不引用具体屏幕对象（research R4）
- [X] T013 [US3] 扩展 `tests/cpp/window_geometry_test.cpp`：补充"目标屏已断开（命中为空）→ fallback 到剩余屏幕"与"混合 DPI 屏幕几何组合（不同尺寸/坐标的屏幕矩形）"用例，断言最大化几何完整落在可见屏幕内
- [X] T014 [US3] 验证 US3：断开 fallback 与混合 DPI 扩展用例全部通过；手动多屏场景（quickstart.md 3.3）需用户在真实多屏环境复核

---

## Phase 6: Polish & Cross-Cutting Concerns

**Purpose**: 全量回归与最终一致性审查（spec SC-004）

- [X] T015 全量回归：`.\scripts\build.ps1 -Gui -UnitTests -Config Debug`（CTest 7/7 全绿）与 `.\scripts\build.ps1 -Pytest`（4/4 通过，M5 未实现按基线 skip）
- [X] T016 最终审查：无边框窗口设计未变（`FramelessWindowHint` 未触碰）、最大化/恢复按钮与双击语义未变（`toggleMaximize` 逻辑保持）、无功能增删；变更仅限 `WM_GETMINMAXINFO` 目标屏解析 + `changeEvent` 几何恢复

---

## Revision 2026-08-29（多屏最大化缺陷复核）

- [X] T017 复核发现副屏最大化仍失败（"窗口消失"）：`window_geometry::maximizeGeometry` 返回的虚拟桌面绝对坐标直接填入 `ptMaxPosition`，违反 Windows `MINMAXINFO` 相对目标显示器坐标语义——主屏（原点 0,0）恰好正常、副屏（正/负坐标）被推到屏外。修改 `src/ui/window_geometry.{h,cpp}`：`MaximizeInfo{maxPosition(相对目标屏左上角偏移), maxSize}` 替换 `maximizeGeometry`，回退链覆盖"对应项为空"双列表校验；`src/ui/MainWindow.cpp` WM_GETMINMAXINFO 按新契约填充；`tests/cpp/window_geometry_test.cpp` 补"副屏在右/左/上、任务栏在顶部/左侧"相对偏移回归用例（红-绿，research R6 / contracts §2）

## Dependencies

### Story Completion Order

```text
Phase 1 Setup → Phase 2 Foundational → Phase 3 US1 → Phase 4 US2 → Phase 5 US3 → Phase 6 Polish
```

- **Foundational (Phase 2)**: 阻塞全部 US（纯函数层是所有修复的复用基础）
- **User Story 1 (P1)**: 依赖 Foundational；不依赖其他 US
- **User Story 2 (P1)**: 依赖 US1（均修改 `src/ui/MainWindow.cpp` 的 nativeEvent/changeEvent，串行避免同文件冲突；恢复逻辑需在最大化修复之上验证）
- **User Story 3 (P2)**: 依赖 US2（热插拔 fallback 审查涉及恢复路径同文件逻辑）
- 每个 US 完成后可独立验证（对应 quickstart 3.x 场景 + 对应单测）

### Within Each User Story

- Tests (if included) MUST be written and FAIL before implementation（宪法 II Test-First）
- 纯函数实现 → 窗口接线 → 手动场景验证
- Story complete before moving to next priority

### Parallel Opportunities

- T002/T006 可并行（不同文件：新头文件 vs CMake 配置）
- T003 与 T006 可并行（不同文件：新测试 vs CMake 配置）
- T005 完成后，Foundational 阶段即完成，US1/US2/US3 可按序推进
- 不同 US 需串行（同文件 `MainWindow.cpp` 冲突规避），但每步验证点可独立卡关

---

## Parallel Example: Foundational (Phase 2)

```bash
# 接口头文件与 CMake 注册并行启动：
Task: "新增 src/ui/window_geometry.h 接口声明（T002）"
Task: "修改 tests/cpp/CMakeLists.txt 注册 window_geometry_test（T004）"

# 测试文件编写与 GUI 目标 CMake 注册并行启动：
Task: "新增 tests/cpp/window_geometry_test.cpp（T003）"
Task: "修改根 CMakeLists.txt 将 window_geometry.cpp 纳入 GUI 目标（T006）"
```

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. Complete Phase 1: Setup（T001 基线）
2. Complete Phase 2: Foundational（T002–T006，纯函数层 + 先失败单测）
3. Complete Phase 3: User Story 1（T007–T008）
4. **STOP and VALIDATE**: 执行 quickstart.md 3.1 全部场景 + `ctest`
5. 缺陷根因已修复（副屏最大化不再"消失"），可交付 MVP

### Incremental Delivery

1. Setup + Foundational → 几何层就绪（单测绿）
2. Add User Story 1 → 副屏最大化修复 → 验证 → Demo（MVP）
3. Add User Story 2 → 恢复行为修复 → 验证 → Demo
4. Add User Story 3 → 热插拔/DPI 加固 → 验证 → Demo
5. Polish（全量回归 + 审查）→ 完成

### Parallel Team Strategy

- 单人/双人建议串行（`MainWindow.cpp` 同文件修改）
- 双人时：A 做 Foundational 几何层；B 准备 quickstart 手动验证环境与基线构建（T001），随后接力 US1→US2→US3

---

## Notes

- [P] tasks = different files, no dependencies
- [Story] label maps task to specific user story for traceability
- 每个 US 独立可完成、可验证（几何单测 + quickstart 手动场景）
- 验证测试先失败再转绿（宪法 II）
- Commit after each task or logical group
- Stop at any checkpoint to validate story independently
- Avoid: vague tasks, same file conflicts, cross-story dependencies that break independence

# Implementation Plan: 文件类型支持目录（获取与生成）

**Branch**: `009-supported-file-types` | **Date**: 2026-08-31 | **Spec**: [spec.md](spec.md)

**Input**: Feature specification from `/specs/009-supported-file-types/spec.md`

**Note**: This template is filled in by the `/speckit.plan` command; its definition describes the execution workflow.

## Summary

本 feature 为 Perception 建立**权威文件类型目录**（单一事实来源），并提供「获取」与「生成」两种能力：

- **单一事实来源**：`src/core/io/file_type_catalog.h` 以 C++ 头文件数据表定义全部文件类型条目（格式名称、扩展名、格式族、数据种类、支持状态），沿用 `theme_catalog_*.h` 既有模式（C++ 运行时直读 + Python 脚本正则解析）。目录覆盖宪法「文件格式范围」全部 4 格式族，区分「已支持」（当前 .plt/.csv）与「规划中」（.tdr/.dat / VTK 各变体 / HDF5）。
- **获取（运行时可查询）**：命令层 `CommandService.supported_formats()` 经 pybind11 绑定返回完整清单，Python REPL 可直接调用（宪法「命令驱动」：查询走命令层）。
- **生成（派生产物）**：① 文件打开对话框过滤列表改由目录生成（全量条目含「规划中」，供发现与核对；规划中格式打开时按「不支持」提示，UI 不再硬编码）；② `scripts/sync_file_types.py` 由目录头解析同步 README / 文档表格（`--check` 门禁 / `--update` 重生成，对标 `check_theme_contrast.py` 与 `format_all.ps1` 模式）。
- **一致性校验**：`FileTypeCatalog` 对比目录「已支持」与 `ReaderRegistry` 实际注册扩展名（对称差），差异可检测报告（CTest 覆盖，FR-011）。

**行为变化**：打开文件过滤列表自本 feature 起仅列出可真正打开的格式（.plt / .csv），「规划中」格式不再出现在过滤列表（FR-009），但保留在权威目录、文档与查询结果中；`All Files (*)` 兜底保留。

## Technical Context

**Language/Version**: C++17（MSVC VS2022）+ CPython 3.13 / pybind11——宪法锁定的技术栈，不引入新语言/新语法。

**Primary Dependencies**:
- 现有 `ReaderRegistry` / `IReader`（`src/core/io/reader.{h,cpp}`）：目录「已支持」一致性校验的对照方。
- 现有 pybind11 命令层（`src/python/command/command_service.{h,cpp}` + `command_module.cpp` + `src/python/api/i_command_service.h`）：新增 `supported_formats` 命令的宿主。
- `src/ui/MainWindow.cpp` `openFile()`：替换硬编码过滤字符串为目录生成结果。
- `scripts/` 既有门禁脚本模式（`check_theme_contrast.py` 正则解析头数据表 / `format_all.ps1` `-Check`）。
- **无新增第三方依赖**（宪法「依赖约束」）；sync 脚本仅用 Python 标准库。

**Storage**: 不涉及数据库 / 持久化。目录为编译期头文件数据表（与 theme_catalog 一致）；不引入运行时文件解析。

**Testing**:
- `tests/cpp/file_type_catalog_test.cpp`（链接 `perception_core`，无 GUI 依赖，红-绿 TDD）：目录覆盖完整性、已支持⇄注册表一致性、过滤分组正确性、规划中排除、大小写不敏感、差异检测。
- `tests/python/test_supported_formats.py`（pytest，`perception_py`）：`supported_formats()` 返回结构、字段完整性、状态语义。
- 手动验证：GUI 打开文件对话框过滤、Python REPL 查询、`sync_file_types.py --check`。

**Target Platform**: Windows 10/11 桌面（VS2022 + Ninja，`scripts/build.ps1`）；core 测试无头可跑（`cmake -B build -G Ninja` 即可）。

**Project Type**: desktop-app（Qt Widgets + 内嵌 Python REPL + pybind11 命令层）。

**Performance Goals**: 目录为静态数据 + 线性遍历，查询与过滤生成均毫秒级，对启动与交互无影响。

**Constraints**:
- 宪法「文件格式范围」：目录边界 = VTK / SVisual / HDF5 / 曲线，不新增格式族；「规划中」条目**不实现**读取器（后续独立 feature）。
- 宪法「命令驱动」：用户可感知的格式查询走命令层；UI 直读静态目录元数据（对标 theme 目录直读）用于过滤列表呈现，属纯 UI 展示（宪法明示「纯 UI 变更除外」）。
- 宪法「Layered Core」：目录落 `src/core/io`（无 UI/VTK 依赖）；UI 只消费生成结果，不触碰 core 数据。
- 文本 UTF-8（宪法）；扩展名匹配大小写不敏感（与既有 `ReaderRegistry::findByPath` 行为一致）。
- 合并前 `ctest` + `pytest` 全绿；`docs/` 与 `README.md` 随功能同步（宪法「工作流规则」）。

**Scale/Scope**:
- 目录条目 ≈ 15 条（8 格式族分组 × 状态），当前「已支持」2 条（.plt / .csv）。
- 改动面：`src/core/io` +2 文件、`src/python/` 4 文件、`src/ui/MainWindow.cpp` 1 处、`tests/cpp` +1 测试、`tests/python` +1 测试、`scripts/` +1 脚本、`src/core/CMakeLists.txt`、README / docs 同步。

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

- [x] **Spec-First**：`specs/009-supported-file-types/spec.md` 已批准且通过质量清单校验（16/16，无 [NEEDS CLARIFICATION]）（宪法 I / 宪法 VI）
- [x] **Test-First**：`file_type_catalog_test.cpp`（CTest）与 `test_supported_formats.py`（pytest）先红后绿；目录「已支持」⇄注册表一致性、过滤分组、查询契约均有对应用例；合并前 `ctest` + `pytest` 全绿（宪法 II / 架构契约 Test-First）
- [x] **Layered Core**：目录与一致性校验落在 `src/core/io`（新增 `file_type_catalog`，与 `reader`/`plt_reader`/`csv_reader` 同层），无跨层直接数据操作；UI 仅消费过滤分组结果；`core` 不依赖 UI/VTK（宪法 III / 架构契约 Layered Core）
- [x] **Command-Driven**：用户可感知的格式查询经命令层 `supported_formats()`（ICommandService → pybind11），UI 未绕过命令层访问用户数据；打开过滤列表属纯 UI 展示（目录元数据直读，对标 theme 目录），宪法明示「纯 UI 变更（布局、主题、面板可见性…）除外」（宪法 IV / 架构契约）
- [x] **Local Design Source**：无弹窗/文件对话框专项 mockup；对话框过滤行为属功能行为（非视觉稿），`docs/design/ui-guidelines.md` 无冲突；不引入 figma 依赖（宪法 V）
- [x] **Scope**：目录内容严格限定宪法「文件格式范围」4 格式族（VTK 全部类型 / SVisual(.plt/.tdr) / HDF5(.h5/.hdf5) / 曲线(.csv/.dat)），「规划中」不越界新增格式族（宪法 VI）
- [x] **Technology Stack**：C++17 头文件数据表 + 既有 pybind11 命令层 + 标准库 Python 脚本；无新依赖、无新构建系统、VTK 不触碰（宪法「技术栈约束」）

> 任一检查不通过：先修订方案并注明处置，或如实填入下方 Complexity Tracking。

## Project Structure

### Documentation (this feature)

```text
specs/009-supported-file-types/
├── plan.md              # This file (/speckit.plan command output)
├── research.md          # Phase 0 output (/speckit.plan command)
├── data-model.md        # Phase 1 output (/speckit.plan command)
├── quickstart.md        # Phase 1 output (/speckit.plan command)
├── contracts/           # Phase 1 output (/speckit.plan command)
│   └── python-supported-formats.md   # supported_formats() 命令契约（获取能力）
└── tasks.md             # Phase 2 output (/speckit.tasks command - NOT created by /speckit.plan)
```

### Source Code (repository root)

```text
src/core/io/
├── file_type_catalog.h              # [新增] FileTypeFamily/Kind/Status 枚举 + FileTypeEntry 结构
│                                    #        + kFileTypeCatalog[] 数据表（单一事实源，theme_catalog 模式）
│                                    #        + FileTypeCatalog 静态 API：all/supported/findByExtension/
│                                    #          filterGroups/inconsistencies
└── file_type_catalog.cpp            # [新增] API 实现（一致性校验对照 ReaderRegistry）

src/python/
├── api/i_command_service.h          # [改] + virtual supportedFormats() = 0
├── command/command_service.h/.cpp   # [改] + supportedFormats() 实现（读 FileTypeCatalog 组装 dict 列表）
└── command/command_module.cpp       # [改] pybind 绑定 supported_formats（蛇形命名，与既有绑定一致）

src/ui/MainWindow.cpp                # [改] openFile() 过滤串改由 FileTypeCatalog::filterGroups() 生成
                                     #      （标签本地化 + ";;" 拼接 + All Files 兜底），删除硬编码字符串

tests/cpp/
├── file_type_catalog_test.cpp       # [新增] 目录覆盖/一致性/过滤分组/规划中排除/大小写/差异检测（无头 CTest）
└── CMakeLists.txt                   # [改] 注册 file_type_catalog_test（链接 perception_core）

tests/python/
└── test_supported_formats.py        # [新增] supported_formats 契约测试（pytest）

scripts/
└── sync_file_types.py               # [新增] 正则解析 file_type_catalog.h → --check 校验 README/docs 表格
                                     #      与目录一致 / --update 重生成（对标 check_theme_contrast.py）

src/core/CMakeLists.txt              # [改] 注册 file_type_catalog.cpp

README.md                            # [改] 「打开文件过滤」由目录生成并同步为已支持清单
docs/architecture.md                 # [改] io 层描述 + 格式可扩展节：目录为格式范围唯一事实来源
```

**Structure Decision**: 目录落在 `src/core/io`（与既有 reader 注册表同层，可无头单测、可被命令层与 UI 共同消费）；过滤分组与一致性校验为纯函数（无 Qt 依赖），UI 只做标签本地化与拼接；脚本沿用 `scripts/` 既有门禁模式（正则解析头数据表 + `--check`）。不引入新目录、不改架构分层。

## Complexity Tracking

> **Fill ONLY if Constitution Check has violations that must be justified**

无违规。打开文件过滤的硬编码字符串被替换为本 feature 自身规格要求的目录生成行为（FR-006/FR-009），属功能改进而非绕过；不触碰其他 feature 已交付的接口或测试语义。

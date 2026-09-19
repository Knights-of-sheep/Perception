# scripts/ 脚本说明

本目录（及 `tests/` 下的测试辅助脚本）收录了 Perception 项目构建、门禁、资源生成、测试数据与截图、图标/主题校验、文档同步所需的全部脚本。本文档说明每个脚本的**用途、参数、依赖、运行示例与注意事项**，便于新人与协作者快速上手。

> **运行环境**
> - `*.ps1`（PowerShell）主要面向 **Windows + PowerShell + VS2022**（CMake/Ninja）。
> - `*.py` 脚本跨平台，但各自依赖需自行安装（见各条目「依赖」）。

> **维护约定（FR-006）**：新增 / 重命名 / 删除 / 修改任一脚本（尤其是参数）时，必须在同一次 PR 中同步更新本文档。PR 评审清单应包含该同步门禁。

## 快速索引表

| 脚本 | 语言 | 分类 | 一句话用途 |
|------|------|------|-----------|
| `build.ps1` | ps1 | 构建与清理 | CMake 配置 + 编译 +（可选）CTest/pytest，支持 GUI 构建 |
| `clean.ps1` | ps1 | 构建与清理 | 清理构建/运行产物，恢复到「克隆即构建」状态 |
| `format_all.ps1` | ps1 | 代码门禁 | 全库 clang-format 对齐（检查或就地改写） |
| `check_line_counts.ps1` | ps1 | 代码门禁 | 行数红线门禁（.cpp≤800 / .h≤500 / .hpp≤800） |
| `check_pragma_once.ps1` | ps1 | 代码门禁 | 头文件 `#pragma once` 缺失检测 |
| `update_screenshots.ps1` | ps1 | 测试数据/截图 | 用最新构建重生成 `docs/screenshots` 截图 |
| `check_icons.py` | py | 图标/主题校验 | 图标 SVG 色板/命名/覆盖/字段符合性校验 |
| `check_theme_contrast.py` | py | 图标/主题校验 | 主题色 WCAG 对比度校验 |
| `gen_qrc.py` | py | 资源生成 | 由已渲染 PNG/ICO 生成 `theme.qrc` 图标资源 |
| `make_mockups.py` | py | 资源生成 | 生成图标总览/图标栏/主窗口 mockup 视觉稿 |
| `make_test_data.py` | py | 测试数据/截图 | 生成 `tests/data` 各格式族测试夹具并回读校验 |
| `render_icons.py` | py | 资源生成 | 将 SVG 源渲染为多尺寸 PNG/ICO |
| `sync_file_types.py` | py | 文档同步 | 由 `file_type_catalog.h` 同步/校验文件类型文档 |
| `tests/run_cpp_tests.py` | py | 测试辅助 | 封装 `ctest` 运行 C++ 单元测试（本特性新增） |
| `tests/run_python_tests.py` | py | 测试辅助 | 封装 `pytest` 运行 Python 命令层测试（本特性新增） |

---

## 构建与清理

### `build.ps1`

- **用途**：CMake 配置 + 编译；可选运行 C++ 单测（CTest）与 Python 命令层测试（pytest）；`-Gui` 时构建 Qt5/VTK 界面层并部署 Qt 运行库。
- **参数**：
  - `-CmakeExe <path>`：cmake.exe 完整路径（缺省按 PATH → VS 自带 CMake 探测）。
  - `-Version <ver>`：透传 `-DPERCEPTION_VERSION`（如 `0.2.0`）。
  - `-UnitTests`：构建成功后运行 CTest。
  - `-Pytest`：构建成功后运行 pytest（`tests/python`）。
  - `-Gui`：构建 GUI 层（`PERCEPTION_BUILD_GUI=ON`）。
  - `-Qt5Dir <path>` / `-VtkDir <path>`：GUI 构建时的 Qt5/VTK cmake 目录（缺省为 CMakeLists 缓存默认值）。
  - `-Config <Debug|Release>`：构建配置（默认 `Release`，多配置生成器生效）。
  - `-Clean`：先删除 `build` 目录再重新配置。
  - `-BinDir <dir>` / `-LibDir <dir>`：exe/dll 与库文件输出目录（默认 `<根>/bin`、`/<根>/lib`）。
- **依赖**：VS2022（含 CMake/Ninja）；`-UnitTests` 需 ctest；`-Pytest` 需 `python` + `pytest`；`-Gui` 需 windeployqt（随 Qt 安装）。
- **示例**：
  ```powershell
  # 仅编译（沿用默认版本号）
  .\scripts\build.ps1
  # 设置版本号并依次跑 CTest 与 pytest
  .\scripts\build.ps1 -Version 0.2.0 -UnitTests -Pytest
  # 构建 GUI 层
  .\scripts\build.ps1 -Version 0.3.0 -Gui
  # 清理后重新构建并跑单测
  .\scripts\build.ps1 -Clean -UnitTests -Config Debug
  ```
- **注意事项**：
  - 多配置生成器（VS）产物落在 `bin\<Config>` 与 `lib\<Config>`；单配置（Ninja）直接输出到 `bin`/`lib`。
  - `-Gui` 默认 `Qt5_DIR=D:\Qt\5.15.2\msvc2019_64\lib\cmake\Qt5`、`VTK_DIR=D:\vtk-941-qt\bak\lib\cmake\vtk-9.4`，路径不存在将报错退出。
  - 编译失败会以非零退出码终止脚本。

### `clean.ps1`

- **用途**：删除 CMake 中间生成物与构建/运行产物，恢复到「克隆即构建」状态（源码/规格/文档/脚本一律保留）。
- **参数**：
  - `-Force`：跳过 `y/N` 确认，直接删除。
  - `-KeepPythonCache`：保留 Python 缓存（`__pycache__` / `.pytest_cache`）。
  - `-WhatIf`：预览模式，仅列出将删除的路径，不执行删除。
- **依赖**：PowerShell（无外部依赖）。
- **示例**：
  ```powershell
  .\scripts\clean.ps1 -WhatIf      # 预览
  .\scripts\clean.ps1              # 交互确认后清理
  .\scripts\clean.ps1 -Force       # 免确认直接清理
  ```
- **注意事项**：
  - 删除范围：`build/`、`build-gui/`、`bin/`、`lib/`、`.pytest_cache/`、递归 `__pycache__` / `*.py[cod]`，以及源码树中可能残留的 `CMakeFiles/` / `CMakeCache.txt`。
  - 受保护根目录 `third-party/`、` .git` 永不清理。
  - 无内容可删时打印提示并 `exit 0`。

---

## 代码门禁 / lint

### `format_all.ps1`

- **用途**：对 `src/` 与 `tests/cpp/` 全部 C/C++ 源文件执行 `.clang-format` 格式化。
- **参数**：
  - `-Check`：门禁模式（`clang-format --dry-run --Werror`），仅报告不合规文件，退出码非 0。
  - `-Root <repo>`：仓库根（默认脚本上级目录）。
  - 无 `-Check`：`-i` 就地改写。
- **依赖**：`clang-format`（LLVM）。安装：`winget install LLVM.LLVM` 或 `pip install clang-format`。
- **示例**：
  ```powershell
  powershell -File scripts/format_all.ps1 -Check      # 门禁检查
  powershell -File scripts/format_all.ps1             # 就地改写
  ```
- **注意事项**：
  - 退出码：`0` 通过 / `1` 不合规 / `2` 未找到 clang-format。
  - **就地改写应与逻辑变更分次提交**，便于审查与回滚。

### `check_line_counts.ps1`

- **用途**：行数红线门禁（宪法头文件约束）。
- **参数**：`-Root <repo>`（默认脚本上级目录）。
- **依赖**：PowerShell（无外部依赖）。
- **示例**：`powershell -File scripts/check_line_counts.ps1`
- **注意事项**：
  - 红线：`.cpp` ≤ 800、`.h` ≤ 500、`.hpp` ≤ 800（模板放宽）。`.h` 另建议 ≤ 300，并报告达成率（供 SC-001 验收）。
  - 退出码：`0` 通过 / `1` 有红线违规 / `2` 路径错误。

### `check_pragma_once.ps1`

- **用途**：扫描 `src/` 与 `tests/cpp/` 的 `.h`/`.hpp`，检测缺失 `#pragma once` 的文件。
- **参数**：`-Root <repo>`（默认脚本上级目录）。
- **依赖**：PowerShell（无外部依赖）。
- **示例**：`powershell -File scripts/check_pragma_once.ps1`
- **注意事项**：退出码 `0` 合规 / `1` 存在缺失 / `2` 路径错误。

---

## 测试数据 / 截图

### `make_test_data.py`

- **用途**：生成 `tests/data` 下各格式族（VTK / SVisual / HDF5 / Curve）测试夹具，并回读校验可读性。单一事实来源为 `src/core/io/file_type_catalog.h`。
- **参数**：无。
- **依赖**：`vtk`、`numpy`、`h5py`。
- **示例**：`python scripts/make_test_data.py`
- **注意事项**：
  - 生成目录：`tests/data/{vtk,svisual,hdf5,curve}`。真实样例（legacy.vtk 等）从公开源下载，本脚本补无法下载格式（如 `.vth`/`.vto`/并行 `.pv*`/`.pvd`）。
  - 退出码 `0` 全部成功 / `1` 存在警告（如某格式生成失败）。

### `update_screenshots.ps1`

- **用途**：用最新构建重生成 `docs/screenshots` 截图（主窗口、Python 控制台、UI 打磨系列、25 套主题画廊等）。
- **参数**：无。
- **依赖**：`bin\Release\perception.exe`（**须先 `scripts/build.ps1 -Gui`**）。
- **示例**：`powershell -ExecutionPolicy Bypass -File scripts/update_screenshots.ps1`
- **注意事项**：
  - 未找到 exe 会报错提示先构建 GUI。
  - 所有快照显式 `--theme`（避免 QSettings 记忆的上次主题污染无主题渲染）。
  - 产物：`docs/screenshots/main.png`、`subwindows.png`、`python-console.png`、`console-echo*.png`、`dock-floating.png`、`dock-restored.png`、`ui-opt-*.png`、`themes/<theme>.png`（25 套）。

---

## 资源生成（图标 / 主题）

### `render_icons.py`

- **用途**：将 SVG 源渲染为多尺寸 PNG/ICO（图标流水线 T017）。
  - 功能图标：`16/24/32 px` → `src/ui/theme/icons/png/actions/<icon_id>-<size>.png`
  - 应用图标：`16/24/32/48/64/128/256 px` + `.ico` → `src/ui/theme/icons/png/app/`
- **参数**：无。
- **依赖**：`PyQt5`（需 QtSvg 插件）、`Pillow`（仅 `.ico` 合成）。
- **示例**：`python scripts/render_icons.py`
- **注意事项**：无 SVG 源目录时退出 `1`；需确保 PyQt5 的 `QtSvg` 插件可用。

### `gen_qrc.py`

- **用途**：扫描 `src/ui/theme/icons/png/` 下已渲染 PNG/ICO，生成 `theme.qrc` 图标资源（保留既有 theme qresource/QSS 不变）。
- **参数**：无。
- **依赖**：无（标准库）；**需先运行 `render_icons.py`**。
- **示例**：`python scripts/gen_qrc.py`
- **注意事项**：缺少已渲染 PNG 时打印错误并退出 `1`；内容无变化时打印「无变化」不写文件。

### `make_mockups.py`

- **用途**：生成图标集视觉稿（T020/T021）：`preview.png`（全部图标总览）、`icon-bar-mockup.png`（侧边栏图标栏含状态）、`main-window-mockup.png`（主窗口）。
- **参数**：无。
- **依赖**：`PyQt5`、`PyYAML`。
- **示例**：`python scripts/make_mockups.py`
- **注意事项**：输出至 `docs/design/mockups/005-icon-set/`；需先 `render_icons.py` 生成 PNG（脚本会 `QPainter` 绘制，需 `QGuiApplication`）。

---

## 图标 / 主题校验

### `check_icons.py`

- **用途**：图标符合性校验——色板白名单、命名规则、映射覆盖、schema 字段约束（依据 002-icon-design 规格与 `docs/design/ui-guidelines.md` §3.1）。
- **参数**：
  - `--icons-dir <dir>`：功能图标目录（默认 `src/ui/theme/icons/actions`）。
  - `--app-dir <dir>`：应用图标目录（默认 `src/ui/theme/icons/app`）。
  - `--map <path>`：映射表（默认 `src/ui/theme/icons/icon-map.yaml`）。
- **依赖**：`PyYAML`（可选；缺失时回退轻量行级解析）。
- **示例**：`python scripts/check_icons.py`
- **注意事项**：退出码 `0` 通过 / `1` 存在违反项。色值必须来自 Token 白名单（禁止新增色值）。

### `check_theme_contrast.py`

- **用途**：解析主题目录头文件，按 WCAG 校验文字/图标色组合（正文/弱文字/禁用文字/选中/强调/危险等）。
- **参数**：可选 `sys.argv[1]` 指定 `theme_catalog.h`（默认 `src/ui/theme/theme_catalog.h`）。
- **依赖**：无（标准库）。
- **示例**：`python scripts/check_theme_contrast.py`
- **注意事项**：扫描 `src/ui/theme/theme_catalog_*.h`；退出码 `0` 通过 / `1` 存在失败组合。

---

## 文档同步

### `sync_file_types.py`

- **用途**：以 `src/core/io/file_type_catalog.h` 为唯一事实来源，同步/校验文件类型文档（`README.md` 打开过滤行、`docs/architecture.md` 格式范围表），避免手抄格式清单。
- **参数**：
  - `--check`：校验文档与目录一致；不一致打印差异并退出 `1`。
  - `--update`：按目录重生成文档标记块（`<!-- sync_file_types:start/end -->`）。
- **依赖**：无（标准库）。
- **示例**：
  ```powershell
  python scripts/sync_file_types.py --check     # 门禁
  python scripts/sync_file_types.py --update    # 同步
  ```
- **注意事项**：退出码 `0` 一致 / `1` 不一致 / `2` 目录为空（解析失败）。目录变更后须 `--update` 同步，`--check` 作为门禁（对应 009 规格 FR-011 / SC-005）。

---

## 测试

项目测试分两类，均可用 `build.ps1` 开关触发，或用下方 `tests/` 辅助脚本独立运行。

### C++ 单元测试（CTest）

- 前置：先构建（`scripts/build.ps1`，可选 `-Gui`）。
- 运行方式：
  ```powershell
  # 方式一：经构建脚本（构建并跑测试）
  .\scripts\build.ps1 -UnitTests
  # 方式二：直接调用 ctest
  ctest --test-dir build --output-on-failure
  ```
- 或经辅助脚本 `tests/run_cpp_tests.py`（见下）。

### Python 命令层测试（pytest）

- 前置：安装 `pytest`（`python -m pip install pytest`），并在可导入 `perception`/`extract` 包的环境。
- 运行方式：
  ```powershell
  # 方式一：经构建脚本
  .\scripts\build.ps1 -Pytest
  # 方式二：直接 pytest
  pytest tests/python -q
  ```
- 或经辅助脚本 `tests/run_python_tests.py`（见下）。

### `tests/run_cpp_tests.py`（本特性新增）

- **用途**：封装 `ctest` 运行 C++ 单元测试，输出摘要与可选 JUnit XML 报告。
- **参数**：
  - `--config <Debug|Release>`：构建配置（默认 `Release`）。
  - `--build-dir <dir>`：构建目录（默认自动探测 `build/` 或 `build-gui/`）。
  - `--junit <path>`：输出 JUnit XML 报告。
  - `--timeout <sec>`：单测试超时秒数。
  - `--list`：仅列出测试，不运行。
- **依赖**：`ctest`（CMake，与 cmake 同目录或 PATH）。
- **示例**：`python tests/run_cpp_tests.py --config Release`

### `tests/run_python_tests.py`（本特性新增）

- **用途**：封装 `pytest` 运行 `tests/python` 命令层测试，输出摘要与可选报告。
- **参数**：
  - `--tests-dir <dir>`：测试目录（默认 `tests/python`）。
  - `--report <path>`：输出摘要报告（纯文本）。
  - 其余位置参数透传 pytest（如具体文件、`-k`、`-v`、`-x`）。
- **依赖**：`pytest`。
- **示例**：
  ```powershell
  python tests/run_python_tests.py
  python tests/run_python_tests.py -k extract
  python tests/run_python_tests.py --report report.txt
  ```

---

## 相关规格与约定

- 图标流水线：`specs/002-icon-design`、`specs/007-replace-icon-set`
- 代码门禁与行数红线：`specs/006-constitution-refactor`
- 文件类型目录：`specs/009-supported-file-types`（含 `sync_file_types.py` 单一事实来源约定）
- 宪法「工作流规则」：脚本增删改须同 PR 同步本文档（见文首维护约定）

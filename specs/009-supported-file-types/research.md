# Research: 文件类型支持目录（获取与生成）

> Phase 0 输出：消解 Technical Context 中的设计未知项。所有决策基于代码库既有模式与宪法约束，无 [NEEDS CLARIFICATION] 残留。

## R1: 目录的单一事实来源形态（头文件数据表 vs 运行时数据文件）

- **Decision**: C++ 头文件数据表 `src/core/io/file_type_catalog.h`（`kFileTypeCatalog[]` + `FileTypeCatalog` 静态 API）
- **Rationale**:
  - 宪法「不随意引入第三方库」+ 无运行时文件 I/O → 排除"运行时解析 YAML/JSON"（需解析器或手写解析，违反无新依赖约束）。
  - 项目既有 `theme_catalog_*.h` 先例：C++ 运行时直读 + `check_theme_contrast.py` 用正则解析同一头文件——一个数据源被 C++ 与 Python 脚本双消费，正是本 feature 需要的形态。
  - 与 `IReader::extensions()`（`src/core/io/reader.h`）同构：目录「已支持」扩展名与注册表扩展名可直接做集合比较。
- **Alternatives considered**: YAML + 运行时解析（需引入解析器，宪法「依赖约束」）；YAML + 构建期生成头（引入 codegen 步骤，宪法限定 CMake+Ninja 且无此先例）；纯代码枚举常量散落各处（破坏单一来源）。

## R2: 派生产物（README / 文档表格）的同步机制

- **Decision**: `scripts/sync_file_types.py`，`--check` 校验模式（退出码 0 = 同步）+ `--update` 重生成模式；用正则解析目录头（与 `check_theme_contrast.py` 同一手法）。
- **Rationale**: 宪法「工作流规则」要求文档随功能同步；单一来源要求文档必须由目录派生而非手抄。`format_all.ps1` 已确立 `-Check` 门禁形态（dry-run 非零退出），`check_icons.py` / `check_theme_contrast.py` 确立脚本门禁先例。
- **Alternatives considered**: 手动同步（必然漂移，违反 FR-007 与宪法）；构建期脚本钩子（过度设计，门禁脚本模式已足够）。

## R3: 文件打开过滤列表的生成位置

- **Decision**: `FileTypeCatalog::filterGroups()`（`src/core/io`）返回纯数据 `[{familyKey, patterns[]}]`（全量条目含「规划中」，按格式族分组；规划中格式由加载路径按「不支持」拦截）；Qt 过滤串（`tr` 标签 + `;;` 拼接 + `All Files (*)`）由 `src/ui/MainWindow.cpp` 组装。
- **Rationale**: 宪法「Layered Core」禁止 core 依赖 Qt；分组与过滤逻辑放 core 可被无头 CTest 直接测试；UI 只做标签本地化与拼接，职责最小。
- **Alternatives considered**: UI 直接遍历目录（过滤逻辑无法无头单测）；core 直接产 Qt 串（违反分层，`tr`/`QFileDialog` 语义泄漏进 core）。

## R4: 「获取」命令的形态与契约

- **Decision**: `CommandService.supported_formats()` 返回 `list[dict]`（`extensions` / `format_name` / `family` / `kind` / `status`），经既有 `ICommandService` 虚接口 + `command_module.cpp` pybind 绑定（蛇形命名 `supported_formats`）暴露于 `perception_py`。
- **Rationale**: 宪法「命令驱动与 Python 包」明示查询属命令层职责；复用 006 US4 已打通的「Python import → 虚接口 → C++ 实现」链路，零新机制；pytest 可直接断言契约（对标 `test_command_layer.py`）。
- **Alternatives considered**: UI 内直接读目录（违反命令驱动）；仅 C++ 内部 API（无用户可达入口，不满足 FR-005）。

## R5: VTK「全部类型」的具体化枚举

- **Decision**: 目录含 11 条 VTK 条目，全部「规划中」：
  - 遗留 `.vtk`
  - XML 数据 `.vti` / `.vtp` / `.vtu` / `.vts` / `.vtr`
  - 复合 `.vtm` / `.vtmb` / `.vth` / `.vto`
  - 并行 `.pvti` / `.pvtp` / `.pvtu` / `.pvts` / `.pvtr` / `.pvtm` 与集合 `.pvd`
- **Rationale**: 宪法「VTK 全部类型（.vtk/.vti/.vtp/.vtu/.vts/.vtr 等）」的"等"字 + 对标 ParaView 生态（`docs/prd.md` 目标用户产出物可能含 .vtm/.pvd）；全部标「规划中」= 范围批准但读取器留待后续 feature，不越权实现。
- **Alternatives considered**: 仅宪法明文 6 扩展名（忽略"等"与 ParaView 集合，目录不完整）；无限扩展至 Ensight/Tecplot 等（超出宪法批准范围，spec Assumptions 已声明不新增格式族）。

## R6: 一致性校验（FR-011）的对照基准

- **Decision**: 目录「已支持」扩展名集合 vs `ReaderRegistry` 已注册扩展名集合的对称差；两侧差异分别报告（catalog-only / registry-only）。
- **Rationale**: `ReaderRegistry` 是实际读取能力的唯一事实（`reader.cpp` 按扩展名分派）；对称差同时捕获「目录有 reader 无」与「reader 有目录无」两类漂移，满足 FR-011「可读差异报告」。
- **Alternatives considered**: 仅单向检查（漏检反向漂移）；仅靠人工（无自动化，违背 spec 要求）。
- **当前基线**：目录「已支持」= {.plt, .csv}，注册表 = {.plt, .csv} → 一致；现有 `MainWindow.cpp` 硬编码过滤含 .tdr/.dat/.vtk 等但无 reader，属本 feature 修正范围（FR-009：规划中不出现在可打开过滤）。

## 决策汇总

| 未知项 | 决策 | 依据 |
|---|---|---|
| 目录形态 | C++ 头文件数据表 | theme_catalog 先例 + 无新依赖 |
| 文档同步 | sync_file_types.py（--check/--update） | check_theme_contrast.py + format_all.ps1 先例 |
| 过滤生成 | core 纯函数分组 + UI 组装 | Layered Core |
| 获取命令 | CommandService.supported_formats() | 命令驱动 + 006 链路 |
| VTK 枚举 | 11 条规划中条目 | 宪法"等" + ParaView 生态 |
| 一致性基准 | 注册表对称差 | ReaderRegistry 唯一事实 |

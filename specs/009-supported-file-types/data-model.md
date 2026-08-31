# Data Model: 文件类型支持目录

> Phase 1 输出。定义权威目录的数据结构与验证规则；落地形态见 [plan.md](plan.md)（`src/core/io/file_type_catalog.h` 头文件数据表）。

## 实体与字段

### FileTypeEntry（文件类型条目）

| 字段 | 类型 | 说明 | 约束 |
|---|---|---|---|
| `formatName` | string | 格式名称（如 "SVisual 曲线数据"） | 非空 |
| `extensions` | string[] | 扩展名集合（小写、含点，如 `{".plt"}`） | 非空；全库唯一；每个以 `.` 开头 |
| `family` | FileTypeFamily | 格式族 | 枚举 |
| `kind` | FileTypeKind | 数据种类 | 枚举 |
| `status` | FileTypeStatus | 支持状态 | 枚举 |

### 枚举

- **FileTypeFamily**：`VtkLegacy` / `VtkXml` / `VtkComposite` / `VtkParallel` / `SVisual` / `Hdf5` / `Curve`
  - 打开过滤分组映射（filterGroups 的 familyKey）：`VtkLegacy|VtkXml|VtkComposite|VtkParallel → "VTK"`；`SVisual → "SVisual"`；`Hdf5 → "HDF5"`；`Curve → "Curve Data"`
- **FileTypeKind**：`Curve`（曲线）/ `Structure`（结构）/ `Both`（双用途）
- **FileTypeStatus**：`Supported`（有读取能力）/ `Planned`（范围已批准、未实现）

### FileTypeCatalog（目录访问器，静态 API）

| API | 返回 | 说明 |
|---|---|---|
| `all()` | FileTypeEntry[] | 全部条目（含 Planned） |
| `supported()` | FileTypeEntry[] | 仅 Supported 条目 |
| `findByExtension(ext)` | FileTypeEntry? | 大小写不敏感查找 |
| `filterGroups()` | FilterGroup[] | `{familyKey, patterns[]}`，全量条目（含 Planned），按族分组 |
| `inconsistencies()` | Diff 报告 | 目录 Supported 扩展名 ⇄ ReaderRegistry 扩展名的对称差 |

## 目录初始数据（单一事实来源内容）

| formatName | extensions | family | kind | status |
|---|---|---|---|---|
| VTK Legacy | `.vtk` | VtkLegacy | Structure | Planned |
| VTK Image Data | `.vti` | VtkXml | Structure | Planned |
| VTK Polygonal Data | `.vtp` | VtkXml | Structure | Planned |
| VTK Unstructured Grid | `.vtu` | VtkXml | Structure | Planned |
| VTK Structured Grid | `.vts` | VtkXml | Structure | Planned |
| VTK Rectilinear Grid | `.vtr` | VtkXml | Structure | Planned |
| VTK MultiBlock | `.vtm` `.vtmb` | VtkComposite | Structure | Planned |
| VTK HyperTreeGrid | `.vth` | VtkComposite | Structure | Planned |
| VTK Overlapping AMR | `.vto` | VtkComposite | Structure | Planned |
| VTK Parallel XML | `.pvti` `.pvtp` `.pvtu` `.pvts` `.pvtr` `.pvtm` | VtkParallel | Structure | Planned |
| ParaView Collection | `.pvd` | VtkParallel | Structure | Planned |
| SVisual 曲线数据 | `.plt` | SVisual | Curve | **Supported** |
| SVisual 结构数据 | `.tdr` | SVisual | Structure | Planned |
| HDF5 数据 | `.h5` `.hdf5` | Hdf5 | Both | Planned |
| CSV 曲线数据 | `.csv` | Curve | Curve | **Supported** |
| 通用曲线数据 | `.dat` | Curve | Both | Planned |

> 初始过滤结果（全量分组，与打开对话框一致）：`VTK → {*.vtk *.vti *.vtp *.vtu *.vts *.vtr *.vtm *.vtmb *.vth *.vto *.pvti *.pvtp *.pvtu *.pvts *.pvtr *.pvtm *.pvd}`、`SVisual → {*.plt *.tdr}`、`HDF5 → {*.h5 *.hdf5}`、`Curve Data → {*.csv *.dat *.plt}`；规划中格式出现在过滤列表供发现与核对，打开时按「不支持」提示、不进入读取流程（FR-009-变更：展示全部、打开拦截）。
>
> 交叉聚合（产品要求 2026-08-31）：`Curve Data` 组除 Curve 族条目（.csv/.dat）外，还包含其他格式族中数据种类为「曲线」的条目（当前为 SVisual 的 .plt），便于按数据种类检索曲线文件；规则由 `filterGroups()` 与 `scripts/sync_file_types.py` 共同实现，仍由目录派生、禁止手抄。

## 验证规则（映射自 spec FR）

| 规则 | 来源 | 实现校验点 |
|---|---|---|
| 每条目必有 5 字段且非空 | FR-003 | 数据表静态断言 |
| 覆盖 4 格式族（VTK/SVisual/HDF5/曲线） | FR-002 | 每族 ≥1 条 |
| 扩展名全库唯一 | FR-003 | 静态断言 |
| Supported 扩展名集 == ReaderRegistry 扩展名集 | FR-008 | `inconsistencies()` 断言为空 |
| 过滤分组覆盖全量条目（含 Planned） | FR-009 | filterGroups 断言 |
| 扩展名匹配大小写不敏感 | FR-010 | `findByExtension(".PLT")` |
| 多用途（.dat → Both） | FR-012 | 数据表 |
| 查询返回与目录一致 | FR-005 | pytest 断言 |

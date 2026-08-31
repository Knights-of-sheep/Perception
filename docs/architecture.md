# 架构设计：Perception

> 顶层架构文档，M0 建立骨架，随里程碑细化。约束源：`.specify/memory/constitution.md`（版本见文件头部）。
> 流程图：见 [architecture/flows.md](architecture/flows.md)（模块依赖 / 启动 / 数据流 / 日志 / 主题 / UI 交互）。

## 1. 技术栈

| 层 | 技术 | 说明 |
|---|---|---|
| 语言 | C++17 | 主体 |
| 界面 | Qt 5.15.2 (Widgets + QSS) | 深色主题，前后端分离 |
| 渲染 | VTK 9.4.1 (C++) | Charts 2D 曲线 + 结构可视化 |
| 脚本 | pybind11 | 命令驱动模块 `perception_py` |
| 测试 | CTest + pytest | C++ 单测 + Python 命令层测试 |

## 2. 分层

```
src/
├─ core/                数据核心（无 UI/渲染依赖，可独立单测）
│  ├─ model/            格式无关数据模型：曲线（IDataSet/Curve）+ 结构（IStructure/FieldData）
│  ├─ io/               格式读取器 + 注册表 + 文件类型权威目录（file_type_catalog.h，
│  │                     宪法「文件格式范围」唯一事实来源：打开过滤/文档/查询均由此派生）
│  ├─ process/          数据处理：变换管道（resample / unit_scale / 投影）
│  └─ event/            事件总线（发布-订阅）：数据变更 → 渲染 / UI 更新
├─ render/              渲染层：VTK Charts 2D 曲线 + 色板（订阅事件，不轮询）
├─ ui/                  界面层：Qt5 Widgets + Dock + QSS 深色主题（命令桥接 + 展示）
│  │                     006-constitution-refactor 拆分：MainWindow.cpp（窗口行为/事件）+
│  │                     MainWindow_assembly.cpp（动作/菜单/标题栏/功能栏装配）+
│  │                     职责控制器（maximize/ 多屏最大化、subwindow/dock_drag_overlay 拖拽高亮、
│  │                     log/log_settings_controller 日志设置）+ Dock 辅助（subwindow/dock_title_bar）+
│  │                     通用弹窗（frameless_dialog / themed_file_dialog）+
│  │                     console/python_bridge（PythonConsole 的 CPython 胶水层）+
│  │                     theme/theme_catalog_{types,dark,light,hc}（主题色板分层）
├─ app/                 入口：main.cpp（内嵌 Python 解释器，接线命令驱动）
└─ python/              pybind 命令驱动层：load_plt / transform / query / export / supported_formats
tests/
├─ cpp/                 C++ 单元测试（CTest，core 层，无头可跑）
└─ python/              pytest（命令层，M5 起依赖 perception_py）
```

## 3. 依赖方向

```
        ┌──────────── core ────────────┐
        │  model ← io ← process        │
        │         ↑        ↑           │
        │         └── event ───────────┘
        │              │ 发布
        ├──────────────┼───────────────┤
        │              ↓ 订阅           │
        │  render（VTK）    ui（Qt）     │
        │        \        /            │
        │         \      /             │
        │          app（入口）          │
        └──────── python（命令层）──────┘
```

- core 不依赖任何 UI/渲染库；render 只依赖 core + VTK；ui 依赖 core + render + Qt
- **命令驱动**：数据 CRUD 全部经 `python/` 命令层；ui 的数据操作翻译为 Python 命令
- **事件驱动**：render/ui 订阅 `event/`，数据变更自动刷新，不直接轮询 core

## 4. 数据流（事件驱动）

```
加载：命令层 load_plt(path)
  → io::PltReader（注册表按扩展名分派）
  → model::IDataSet
  → publish(DataSetChanged)
  → render 重绘曲线 / ui 刷新列表

变换：命令层 transform(ds, "unit_scale", factor=1e3)
  → process 变换管道（不变更原始数据，产生副本）
  → publish(DataSetChanged)
  → render/ui 跟随更新
```

## 5. 关键设计约束

- **前后端分离**：ui 只持有展示抽象（`ICurveChart`），不直接操作 VTK / core 数据内部
- **格式可扩展**：新曲线格式 = 新增 `ICurveReader` 并注册；新结构格式 = 新增 `IStructureReader`；核心与消费者零改动。格式范围以 `core/io/file_type_catalog.h` 为唯一事实来源（单一来源原则）：打开过滤、文档清单、`supported_formats` 查询全部由目录派生，一致性由 ctest `file_type_catalog_test`（supported⇄注册表对称差）与 `scripts/sync_file_types.py --check` 门禁
- **文件格式范围**：见下表（由 `scripts/sync_file_types.py` 按目录生成，禁止手抄）
- **命令驱动**：所有增删改查数据必须经 `perception_py`；纯 UI 变化（布局/主题/面板）例外
- **安全**：io 层统一路径校验 + 大小上限（`ReadOptions::maxBytes`）；错误以异常抛出，不崩 UI/Python
- **测试**：core 逻辑在 `tests/cpp`（CTest）；命令链路在 `tests/python`（pytest），两条线在合并前都必须绿

### 文件格式范围（唯一事实来源）

<!-- sync_file_types:start -->
| 格式名称 | 扩展名 | 数据种类 | 状态 |
|---|---|---|---|
| VTK Legacy | `.vtk` | 结构 | 规划中 |
| VTK Image Data | `.vti` | 结构 | 规划中 |
| VTK Polygonal Data | `.vtp` | 结构 | 规划中 |
| VTK Unstructured Grid | `.vtu` | 结构 | 规划中 |
| VTK Structured Grid | `.vts` | 结构 | 规划中 |
| VTK Rectilinear Grid | `.vtr` | 结构 | 规划中 |
| VTK MultiBlock | `.vtm` `.vtmb` | 结构 | 规划中 |
| VTK HyperTreeGrid | `.vth` | 结构 | 规划中 |
| VTK Overlapping AMR | `.vto` | 结构 | 规划中 |
| VTK Parallel XML | `.pvti` `.pvtp` `.pvtu` `.pvts` `.pvtr` `.pvtm` | 结构 | 规划中 |
| ParaView Collection | `.pvd` | 结构 | 规划中 |
| SVisual 曲线数据 | `.plt` | 曲线 | 已支持 |
| SVisual 结构数据 | `.tdr` | 结构 | 规划中 |
| HDF5 数据 | `.h5` `.hdf5` | 双用途 | 规划中 |
| CSV 曲线数据 | `.csv` | 曲线 | 已支持 |
| 通用曲线数据 | `.dat` | 双用途 | 规划中 |
<!-- sync_file_types:end -->

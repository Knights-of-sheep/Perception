# 架构设计：Perception

> 顶层架构文档，M0 建立骨架，随里程碑细化。约束源：`.specify/memory/constitution.md` v1.1。

## 1. 技术栈

| 层 | 技术 | 说明 |
|---|---|---|
| 语言 | C++17 | 主体 |
| 界面 | Qt 5.12.12 (Widgets + QSS) | 深色主题，前后端分离 |
| 渲染 | VTK 9.4.1 (C++) | Charts 2D 曲线 + 结构可视化 |
| 脚本 | pybind11 | 命令驱动模块 `perception_py` |
| 测试 | CTest + pytest | C++ 单测 + Python 命令层测试 |

## 2. 分层

```
src/
├─ core/                数据核心（无 UI/渲染依赖，可独立单测）
│  ├─ model/            格式无关数据模型：曲线（IDataSet/Curve）+ 结构（IStructure/FieldData）
│  ├─ io/               格式读取器 + 注册表：.plt/.csv/.dat 曲线；.tdr 结构
│  ├─ process/          数据处理：变换管道（resample / unit_scale / 投影）
│  └─ event/            事件总线（发布-订阅）：数据变更 → 渲染 / UI 更新
├─ render/              渲染层：VTK Charts 2D 曲线 + 色板（订阅事件，不轮询）
├─ ui/                  界面层：Qt5 Widgets + Dock + QSS 深色主题（命令桥接 + 展示）
├─ app/                 入口：main.cpp（内嵌 Python 解释器，接线命令驱动）
└─ python/              pybind 命令驱动层：load_plt / transform / query / export
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
- **格式可扩展**：新曲线格式 = 新增 `ICurveReader` 并注册；新结构格式 = 新增 `IStructureReader`；核心与消费者零改动
- **命令驱动**：所有增删改查数据必须经 `perception_py`；纯 UI 变化（布局/主题/面板）例外
- **安全**：io 层统一路径校验 + 大小上限（`ReadOptions::maxBytes`）；错误以异常抛出，不崩 UI/Python
- **测试**：core 逻辑在 `tests/cpp`（CTest）；命令链路在 `tests/python`（pytest），两条线在合并前都必须绿

# 架构设计：Perception

> 顶层架构文档，M0 建立骨架，随里程碑细化。

## 1. 技术栈

| 层 | 技术 | 说明 |
|---|---|---|
| 语言 | C++17 | 主体 |
| 界面 | Qt 5.12.12 (Widgets + QSS) | 深色主题 |
| 渲染 | VTK 9.4.1 (C++) | Charts 2D 曲线 |
| 脚本 | pybind11 | 模块 `perception_py` |
| 测试 | CTest + 单元测试 | 无 UI 依赖 |

## 2. 分层

```
src/
├─ core/    数据层：IDataSet / Curve 模型 + .plt 解析器（无 UI 依赖，可单测）
├─ render/  渲染层：VTK Charts 2D 曲线 + 色板
├─ ui/      界面层：Qt5 Widgets + Dock + QSS 深色主题
├─ app/     入口：main.cpp
└─ python/  pybind 模块：load_plt() / plot() / export()
```

## 3. 依赖方向

```
core  <-  render  <-  ui  <-  app
   \                    /
    \---- python ------/
```

- core 不依赖任何 UI/渲染库
- render 只依赖 core + VTK
- ui 依赖 core + render + Qt

## 4. 数据流（MVP）

```
.plt 文件 -> core::PltParser -> IDataSet/Curve -> render::CurveChart -> ui 视图
```

## 5. 关键设计约束

- core 层必须可脱离 UI 单独测试（.plt 解析正确性）
- UI 与渲染解耦：ui 只持有 ICurveChart 抽象，不直接操作 VTK

# Perception

对标 Synopsys svisual（Inspect .plt 曲线 / TecplotSV .tdr 结构）的 TCAD 数据可视化桌面工具（重构版）。

## 技术栈

- C++17 / Qt 5.12.12 / VTK 9.4.1 / pybind11

## 目录结构

```
src/
├─ core/    数据层：IDataSet / Curve 模型 + .plt 解析器（无 UI 依赖，可单测）
├─ render/  渲染层：VTK Charts 2D 曲线 + 色板（M3 接入）
├─ ui/      界面层：Qt5 Widgets + Dock + QSS 深色主题（M4 接入）
├─ app/     入口：main.cpp（M0 占位，M4 替换为真实主窗口）
└─ python/  pybind 模块：load_plt() / plot() / export()（M5 接入）
tests/      CTest 单测（core 层，无 UI 依赖）
docs/       顶层文档 + 设计稿（docs/design/mockups/）
```

## 里程碑

| 里程碑 | 内容 | 状态 |
|---|---|---|
| M0 | 仓库清理 + 骨架 | 完成 |
| M1 | src/core：.plt 解析器 + 单测 | 待办 |
| M2 | 设计稿（本地 mockup） | 待办 |
| M3 | src/render：VTK 曲线渲染 | 待办 |
| M4 | src/ui：端到端 MVP | 待办 |
| M5 | src/python：pybind 绑定 | 待办 |
| M6 | 打包 + 文档 | 待办 |

## 构建

> 依赖：CMake ≥ 3.16、支持 C++17 的编译器（Windows 用 VS2022 + Ninja）。
> 在 VS2022 开发者 PowerShell 中执行，或调用 VS 自带 CMake 全路径。

### 骨架模式（默认，M1 core 层 + 单测）

无需 Qt/VTK，克隆即构建：

```powershell
cmake -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
```

### GUI 模式（M3+，打开 Qt5/VTK 界面层）

```powershell
cmake -B build -G Ninja -DPERCEPTION_BUILD_GUI=ON `
  -DQt5_DIR="<Qt5 安装>/lib/cmake/Qt5" `
  -DVTK_DIR="<VTK 9.4 安装>/lib/cmake/vtk-9.4"
cmake --build build
```

- Qt 5.12.12：`C:\Qt\Qt5.12.12\5.12.12\msvc2019_64\lib\cmake\Qt5`
- VTK 9.4.1（预建 Qt5 版）：`D:\vtk-941-qt\bak\lib\cmake\vtk-9.4`

## 流程

遵循 `spec-kit` 规约闸门流水线（`.specify/`），设计稿经本地 mockup 注入（`docs/design/mockups/`），不使用远端设计服务。

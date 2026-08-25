# Perception

对标 ParaView、Synopsys svisual 的数据可视化桌面工具（重构版）。

## 当前方向（2026-08-23）

**界面优先**：先搭建可运行的 UI 主界面框架（主窗口 + Dock + 深色主题），
运行 `perception.exe` 启动后首先展示主界面；数据层（.plt 解析、命令层）随后并行接入。
UI 界面事实源为 `docs/design/mockups/`（当前为本地设计稿）。

已实现：深色主题（25 套可切换）、Dock 面板（数据/属性/Python 控制台）拖拽重组与
窗口控制（无边框浮动、最小化/最大化/恢复嵌入/关闭、右下角缩放）、命令面板预留。

## 技术栈

- C++17 / Qt 5.12.12 / VTK 9.4.1 / pybind11

## 目录结构

```
src/
├─ core/               数据核心（无 UI 依赖，可单测）
│  ├─ model/           格式无关数据模型：曲线（IDataSet/Curve）+ 结构（IStructure/FieldData）
│  ├─ io/              格式读取器 + 注册表（.plt/.csv 曲线；.tdr 结构；新格式=新 reader）
│  ├─ process/         数据处理：变换管道（unit_scale 等）
│  └─ event/           事件总线（发布-订阅，数据变更驱动渲染/UI 更新）
├─ render/             渲染层：VTK Charts 2D 曲线（M3 接入，主界面之后）
├─ ui/                 界面层：Qt5 Widgets + Dock + QSS 深色主题（M3a 优先搭建）
├─ app/                入口：main.cpp（M0 占位，M3a 替换为真实主窗口）
└─ python/             pybind 命令驱动层：load_plt/transform/query/export（M5 接入）
tests/
├─ cpp/                C++ 单元测试（CTest，core 层，无 UI 依赖）
└─ python/             pytest（命令层测试，M5 起依赖 perception_py）
docs/                  顶层文档 + 设计稿（docs/design/mockups/）
```

## 里程碑

| 里程碑 | 内容 | 状态 |
|---|---|---|
| M0 | 仓库清理 + 骨架 | 完成 |
| M3a | **UI 主界面框架**：主窗口 + Dock + QSS 深色主题，运行即见主界面 | **下一步（优先）** |
| M1 | src/core/io：.plt 解析器 + 单测 | 待办（与 M3a 并行） |
| M2 | 设计稿（本地 mockup）+ 结构数据模型 | 待办 |
| M3 | src/render：VTK 曲线渲染 | 待办（M3a 之后） |
| M4 | src/ui：端到端 MVP | 待办 |
| M5 | src/python：pybind 命令层 | 待办 |
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

### GUI 模式（界面优先，M3a 起）

编译打开 Qt5/VTK 界面层，构建后运行程序即可看到主界面：

```powershell
.\scripts\build.ps1 -Gui -Qt5Dir "<Qt5 安装>/lib/cmake/Qt5" -VtkDir "<VTK 9.4 安装>/lib/cmake/vtk-9.4"
.\scripts\build.ps1 -Gui -Qt5Dir "<Qt5 安装>/lib/cmake/Qt5" -VtkDir "<VTK 9.4 安装>/lib/cmake/vtk-9.4" -Pytest
```

或直接使用 CMake：

```powershell
cmake -B build -DPERCEPTION_BUILD_GUI=ON `
  -DQt5_DIR="<Qt5 安装>/lib/cmake/Qt5" `
  -DVTK_DIR="<VTK 9.4 安装>/lib/cmake/vtk-9.4"
cmake --build build --config Release
```

依赖（当前开发机状态）：

- **Qt 5.12.12（msvc2019_64）**：⚠️ 尚未安装，需先用 Qt 官方在线安装器安装
  `Qt 5.12.12 → MSVC 2019 64-bit` 组件，目标路径如 `C:\Qt\Qt5.12.12\...`
- **VTK 9.4.1（预建 Qt5 版）**：✅ 已就绪 `D:\vtk-941-qt\bak\lib\cmake\vtk-9.4`

### 运行

```powershell
# 多配置生成器产物在 bin 下按配置分子目录
.\bin\Release\perception.exe
```

### 清理

删除 CMake 中间生成物与构建产物（`build\`、`build-gui\`、`bin\`、`lib\`、Python 缓存），恢复「克隆即构建」状态；源码 / 规格 / 文档 / `third-party\` 一律保留：

```powershell
.\scripts\clean.ps1        # 交互确认后清理
.\scripts\clean.ps1 -Force # 免确认直接清理
.\scripts\clean.ps1 -WhatIf # 仅预览将删除的内容
```

## 测试

- **C++ 单测**（CTest）：`tests/cpp/`，core 层逻辑，无头可跑（见上）
- **pytest**：`tests/python/`，命令驱动链路 load → transform → query → export（M5 后启用，见 `tests/python/README.md`）

## 流程

遵循 `spec-kit` 规约闸门流水线（`.specify/`），设计稿经本地 mockup 注入（`docs/design/mockups/`），不使用远端设计服务。架构约束见 `.specify/memory/constitution.md` v1.1。

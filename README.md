# Perception

对标 Synopsys svisual（Inspect .plt 曲线 / TecplotSV .tdr 结构）的 TCAD 数据可视化桌面工具（重构版）。

## 技术栈

- C++17 / Qt 5.12.12 / VTK 9.4.1 / pybind11

## 里程碑

| 里程碑 | 内容 | 状态 |
|---|---|---|
| M0 | 仓库清理 + 骨架 | 完成 |
| M1 | src/core：.plt 解析器 + 单测 | 待办 |
| M2 | 设计稿（Figma 或本地 mockup） | 待办 |
| M3 | src/render：VTK 曲线渲染 | 待办 |
| M4 | src/ui：端到端 MVP | 待办 |
| M5 | src/python：pybind 绑定 | 待办 |
| M6 | 打包 + 文档 | 待办 |

## 构建

- Windows: `scripts/build.ps1`
- Linux: `scripts/build.sh`

（脚本在 M3 提供）

## 流程

遵循 `spec-kit` 规约闸门流水线（`.specify/`），设计稿经本地 mockup 注入（`docs/design/mockups/`）。

# PRD：Perception

对标 Synopsys svisual（Inspect .plt 曲线 + TecplotSV .tdr 结构）的 TCAD 数据可视化桌面工具。

> 顶层规划文档，由 M0 建立骨架，随各里程碑完善。

## 1. 背景与目标

- 手工版 Perception 实验项目：界面不够美观、架构耦合重
- 重构目标：更美观、更好用、架构清晰的 C++ 桌面工具

## 2. 目标用户

- TCAD 仿真工程师（日常查看 .plt 曲线 / .tdr 结构）

## 3. 功能范围

### MVP（先做）
- .plt 曲线查看器（对标 svisual Inspect）：加载 .plt、多曲线渲染、缩放/平移、图例、色板

### 后续
- .tdr 结构查看器（对标 TecplotSV）

## 4. 非目标

- 不做仿真计算，只做可视化
- 不做 .plt 编辑/写回（只读）

## 5. 成功指标

- 界面美观度：对标现代 IDE 深色主题
- 交互流畅：大文件曲线平移缩放无卡顿

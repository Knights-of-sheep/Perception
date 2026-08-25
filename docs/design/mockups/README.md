# 本地 Mockup（设计稿事实源）

> 本目录是 Perception 界面的**唯一设计事实源**（A 方案主选）。
> CodeBuddy 在 `specify` / `plan` / `tasks` 阶段**必须读取本目录**，作为 UI 实现的视觉依据。

## 目录规则

每个界面单元一个子目录，命名 `NNN-<界面名>/`，内含：

- `preview.png` —— 设计稿截图（必需）
- `notes.md` —— 设计说明（可选，但推荐）
- 额外素材（色板、参考图等）随意

```
docs/design/mockups/
├── README.md
└── 001-main-window/
    ├── preview.png
    └── notes.md
```

## 设计稿制作方式（任选）

| 工具 | 说明 |
|---|---|
| Figma 免费版 | 画完「导出 PNG」放进来 |
| Penpot | 开源 Figma 替代，全免费 |
| Qt Designer | 直接拖 Qt 控件，导出预览 PNG，最贴近最终实现 |

## 当前覆盖范围（Perception）

| 界面单元 | 状态 | 说明 |
|---|---|---|
| 001-main-window（主窗口布局） | 待制作 | Dock 布局 + 深色主题 |
| 002-curve-view（曲线视图） | 待制作 | 2D 曲线渲染区 |
| 003-dock（侧边栏） | 待制作 | 文件/图层/属性面板 |
| 004-color-palette（色板） | 待制作 | 深色主题色板 |
| 005-icon-set（图标集） | 已制作 | 全量预览 + 侧边栏/主窗口 mockup + 盲测/五态/conformance 记录 |

## 读取约定（CodeBuddy 遵守）

1. **specify 前**：读取 `docs/design/mockups/**/preview.png` + `notes.md`，把视觉要求写进 `specs/<feature>/spec.md`
2. **plan 阶段**：对照 mockup 排 UI 实现顺序
3. **tasks 阶段**：每个 UI 任务引用对应 `NNN-<界面名>/` 作为完成标准
4. 实现完成后自查：截图对比 mockup，不一致则修正

## 设计稿方案：纯本地（不使用 Figma 远端）

- **figma 扩展已卸载**（`.specify/extensions.yml` 仅剩 git），**无 `FIGMA_PAT` / Figma REST / MCP 依赖**。
- 本目录是界面**唯一**设计事实源；CodeBuddy 在 specify/plan/tasks 阶段读取本目录注入规约。
- 将来若改用 Web 前端技术栈并需要 `verify` 门禁，可重新引入 figma 扩展（`specify extension add figma --dev <目录>`），届时本目录仍可作为快速预览。

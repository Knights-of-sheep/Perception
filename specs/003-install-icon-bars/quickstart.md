# Quickstart: 图标安装验收指南

**Feature**: 003-install-icon-bars | **Branch**: `003-install-icon-bars` | **Date**: 2026-08-25

> 本文档定义「如何证明图标安装功能端到端达成」的可运行验证场景。映射契约见 [contracts/icon-action-map.md](contracts/icon-action-map.md)，实体与校验规则见 [data-model.md](data-model.md)。实现细节属于 `tasks.md`，不在此展开。

## 前置条件

| 项 | 说明 |
|---|---|
| 构建环境 | Windows + MSVC + Qt 5.15.2（含 Qt5Svg）+ VTK 9.4；`scripts/build.ps1 -Gui` 可跑通 |
| Python 环境 | Python 3 + PyQt5/Pillow（图标管线）+ pytest |
| 图标资产 | 002 交付的 52 枚 SVG/PNG + `icon-map.yaml`；本次新增 4 枚 SVG 已放入 `src/ui/theme/icons/actions/` |

## 场景 1：图标管线（新增图标符合性 + 资源齐备，SC-006 / FR-008）

```powershell
cd E:\spec-work\Perception
python scripts/render_icons.py   # 渲染新增 4 枚 SVG → png/actions/*-{16,24,32}.png
python scripts/check_icons.py    # 符合性校验：风格/网格/色板/命名/映射 0 不符合
python scripts/gen_qrc.py        # 重新生成 theme.qrc（含新增 PNG 条目）
```

**通过条件**：check_icons 无告警；`png/actions/` 出现 `file-load-script-*`、`file-record-screen-*`、`view-refresh-*`、`view-panel-toggle-*` 各三档；`icon-map.yaml` 含 4 条新登记。

## 场景 2：构建 + 契约单测（SC-003 / SC-004）

```powershell
scripts/build.ps1 -Gui -UnitTests
```

**通过条件**：
- 编译通过（qrc 资源编译、链接成功）
- CTest 含 `icon_action_map_test` 且通过，断言：
  - 左侧功能栏集合 = 10 个固定 id（V-1）
  - 右侧功能栏集合 = 9 个固定 id（V-2）
  - 菜单操作项 iconId 非空（V-3）；tooltip 非空（V-4）
  - 每动作图标 PNG 三档资源存在（V-5）
  - 未实现动作 enabled=false 且无槽连接（V-6）；三面板开关 checkable（V-7）
  - 左栏可用按钮与菜单同动作（V-8）

## 场景 3：快照验证（FR-002/003/005 视觉呈现）

```powershell
.\bin\Release\perception.exe --snapshot E:\spec-work\Perception\build\verify-icons.png
```

**通过条件**：快照图中可见：
- 全部菜单项显示语义图标（主题/日志级别状态列表除外，见契约 §1）
- 左侧纵向功能栏 10 个按钮带图标、右侧纵向功能栏 9 个按钮带图标
- 窗口标题栏/任务栏显示设计版应用图标（FR-001）

## 场景 4：交互验证清单（手动，FR-003/004/006/009/011）

启动 `.\bin\Release\perception.exe`，逐项核对：

| # | 操作 | 预期 |
|---|---|---|
| 1 | 悬停任意功能栏按钮 | 显示中文 tooltip（FR-006） |
| 2 | 点击「加载文件」按钮 | 打开文件对话框，与菜单「打开文件…」行为一致（FR-004） |
| 3 | 点击「数据面板显隐」 | 左侧文件树面板显隐切换，按钮选中态反映面板可见性（FR-003 场景 3） |
| 4 | 点击「属性面板显隐」「命令窗口显隐」 | 同上，作用于对应面板 |
| 5 | 点击「主界面截图」 | 弹出保存对话框并导出主界面截图，与菜单一致 |
| 6 | 点击任一禁用按钮（撤销/重做/加载脚本/视频录制/刷新/右侧全部 9 枚） | 无任何反应、无崩溃、无日志 ERROR（FR-011） |
| 7 | 视图菜单 → 重置布局 | 两条功能栏回到左右两侧原位（FR-009） |
| 8 | 150%/200% 缩放 | 图标清晰、布局不错位（FR-010，SC-005） |
| 9 | 主题菜单切换主题 | 菜单图标与功能栏图标在全部主题下正常显示 |

## 场景 5：回归（宪法 V）

```powershell
scripts/build.ps1 -Gui -Pytest
```

**通过条件**：pytest（命令层）全部通过；本功能未触碰 `src/core`，既有 CTest 全部通过。

## 完成信号

| 验证 | 通过标准 | 状态 |
|---|---|---|
| 场景 1 | check_icons 0 不符合；4 枚新图标三档齐备 | ☐ |
| 场景 2 | 构建通过；icon_action_map_test 全绿 | ☐ |
| 场景 3 | 快照含菜单图标 + 左右功能栏 + 应用图标 | ☐ |
| 场景 4 | 9 项交互全部符合预期 | ☐ |
| 场景 5 | ctest + pytest 回归通过 | ☐ |

全部通过后，本功能视为达成（对应 spec SC-001~SC-006）。

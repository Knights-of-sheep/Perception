# Quickstart: 图标替换端到端验证指南

**Feature**: 007-replace-icon-set | **Branch**: `007-replace-icon-set` | **Date**: 2026-08-29

> 本指南按顺序执行，证明「图标替换」功能端到端可用。契约与数据模型详见：
> - 替换映射：[contracts/icon-replacement-map.md](contracts/icon-replacement-map.md)
> - 素材来源与风格修订：[contracts/icon-source-and-style.md](contracts/icon-source-and-style.md)
> - 数据模型：[data-model.md](data-model.md)

## 前置条件

- 仓库根目录：`E:\spec-work\Perception`，分支 `007-replace-icon-set`
- Python 3.13 + PyQt5 + PyYAML + Pillow（图标管线依赖）
- 已按 `contracts/icon-source-and-style.md` §2 完成素材下载、色值归一化并入库 `src/ui/theme/icons/actions/`（58 枚替换 + `view-panel-console` 保留）

## 验证步骤

### 步骤 1：保留项零改动检查

```powershell
git diff --stat -- src/ui/theme/icons/app/ src/ui/theme/icons/actions/view-panel-console.svg
```

**预期**：无输出（或为空 diff），`app-icon.svg` / `app-icon.ico` / `view-panel-console.svg` 未修改（SC-002）。

### 步骤 2：门禁校验

```powershell
python scripts/check_icons.py
```

**预期**：退出码 0，输出 `check_icons: PASS - 59 个 SVG、59 条命名、59 条映射全部符合`。
验证点：P-01 色板白名单、N-01 命名、覆盖规则 1（map ↔ SVG 一一对应）、schema（FR-004）。

### 步骤 3：渲染与资源打包

```powershell
python scripts/render_icons.py
python scripts/gen_qrc.py
```

**预期**：
- `render_icons.py`：输出 `[actions] 渲染 177 个 PNG (59×3)` + `[app] 渲染 7 个 PNG + .ico`（FR-005）
- `gen_qrc.py`：输出 `theme.qrc: 更新（177 actions PNG + 7 app PNG + 1 ICO）`（FR-006）

### 步骤 4：mockup 与截图重生成

```powershell
python scripts/make_mockups.py
powershell -ExecutionPolicy Bypass -File scripts/update_screenshots.ps1
```

**预期**：
- `make_mockups.py`：生成 `docs/design/mockups/005-icon-set/preview.png`、`icon-bar-mockup.png`、`main-window-mockup.png`，展示新图标（FR-007）
- `update_screenshots.ps1`：重生成 `docs/screenshots/` 下界面截图（FR-008）

### 步骤 5：全量构建与回归测试

```powershell
powershell -ExecutionPolicy Bypass -File scripts/build.ps1 -Gui
ctest --test-dir build
python -m pytest tests/python
```

**预期**：构建成功、`ctest` 与 `pytest` 全部通过（SC-006）；运行时界面图标正常显示、无缺失资源。

### 步骤 6：主题对比度校验

```powershell
python scripts/check_theme_contrast.py
```

**预期**：通过，无颜色/对比度违规（SC-004）。

### 步骤 7：人工验收（双评审）

1. **语义盲测**：更新并执行 `docs/design/mockups/005-icon-set/blind-test.md`——从 58 枚替换图标中抽 ≥10 枚（覆盖六类），3 名评审者独立识别，正确率 ≥90%（SC-003）。
2. **符合性评审**：按修订后 `icon-style-spec.md` v2.0.0 对照 `specs/002-icon-design/contracts/conformance-checklist.md` 逐项勾选，不符合项 0（SC-001）。
3. **截图对比**：`docs/screenshots/` 与 mockups 对比一致（宪法质量门禁）。

### 步骤 8：文档同步核验

审阅 `docs/design/icon-spec.md`、`docs/design/ui-guidelines.md`（如涉及）、根目录 `README.md` 与 mockups 说明：图标风格 v2.0.0、素材来源（Figma 官方免费 / Material Icons）、流程与脚本清单无过时描述（FR-010 / SC-007）。

## 完成定义（Definition of Done）

- [ ] 步骤 1~6 全部自动化通过（无违规、构建/测试全绿）
- [ ] 步骤 7 盲测 ≥90%、符合性 0 不符合项、截图与 mockups 一致
- [ ] 步骤 8 文档已同步
- [ ] `icon-style-spec.md` 已修订至 v2.0.0（含修订记录）

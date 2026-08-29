# Implementation Record: Replace Icon Set

**Feature**: 007-replace-icon-set | **Branch**: 007-replace-icon-set | **Date**: 2026-08-29

> 本文件记录实现阶段的核对结论、基线数据、技术债登记与最终验收结果，供评审追溯。

## T001 三方核对结论（替换范围基线）

| 数据源 | 数量 | 结论 |
|---|---|---|
| `src/ui/theme/icons/icon-map.yaml` entries | 59（file 9 / edit 3 / view 19 / analysis 16 / animation 7 / tools 5） | ✓ |
| `src/ui/theme/icons/actions/*.svg` | 59 个文件 | ✓ |
| `contracts/icon-replacement-map.md` | 58 映射 + `view-panel-console` 保留 | ✓ |

**结论**：三方一致。替换面 = 58 枚；保留项 = `view-panel-console`（及其 PNG）+ `app/app-icon.svg` / `app/app-icon.ico`。

## T002 保留项基线哈希（SHA-256）

| 文件 | SHA-256 |
|---|---|
| `src/ui/theme/icons/app/app-icon.svg` | `0508EF7EE9F34AC412A9BDEB912219C135E4F70F88BD69427AB81866B40AA003` |
| `src/ui/theme/icons/app/app-icon.ico` | `E51167842FCB27F96E93580A487BC258924D9201DBC96093E33FE06AFCEC12D3` |
| `src/ui/theme/icons/actions/view-panel-console.svg` | `8770603C1D821EC37A1BA44D4BBC5324DCABB4791FAE5A16B5794E29C12572B1` |
| `src/ui/theme/icons/png/actions/view-panel-console-16.png` | `27E2498DDA81832FA16A7F26CCB1689B546D1493A9D54F80CD6BB25A7494EC59` |
| `src/ui/theme/icons/png/actions/view-panel-console-24.png` | `635AC7F089539CDD07799B33E52E2621F483BB99023ABCB9E78C54AC9D2C7EC4` |
| `src/ui/theme/icons/png/actions/view-panel-console-32.png` | `49DE578B032EC7826999CE7FBA3E899F4178AC12B60CC60EE19ECE10B5AE1D74` |

**SC-002 终验**：以上 6 个文件哈希须与终验时完全一致。

## T003 基线门禁结论 + 技术债登记（用户已确认）

`python scripts/check_icons.py` 基线运行结果：**FAIL（4 项违反，扫描 60 个 SVG）**。

```
[P-01] app/app-icon.svg: 非 Token 色值 #8A4B0F ×2（罗盘刻度线）
[P-01] app/app-icon.svg: 非 Token 色值 #FFFFFF（中心光点）
[P-01] app/app-icon.svg: 非 Token 色值 #0B6EB8（辉光描边）
```

**技术债根源**：`ff1e353`（2026-08-25）将 `app-icon.svg` 更新为多色 v2.4 时引入非白名单色值；`check_icons.py` 自 `f8f80d4` 起即扫描 app 目录，该违规当时即存在（现存问题，非本功能引入）。

**处理决定（用户确认，2026-08-29）**：
- 保持 `app-icon.svg` 零改动（遵守 FR-001，禁止修改保留项）。
- 冻结违规清单：`#8A4B0F`、`#8A4B0F`、`#FFFFFF`、`#0B6EB8`（共 4 项，均来自 `app-icon.svg`）。
- **验收口径调整**：最终验收以「58 枚新 SVG 零违规 + 保留项违规条目冻结不新增」为准。`check_icons.py` 全局退出码 0 的硬性要求因保留项现存违规暂不可达，验收时需向用户说明门禁非全绿的原因（本功能引入的违规数 = 0）。

## T004 管线基线验证结论

- Python 3.13.5；`PyQt5.QtSvg` / `Pillow` / `PyYAML` 导入 OK。
- `render_icons.py` / `gen_qrc.py` / `make_mockups.py` / `check_theme_contrast.py` / `check_icons.py` 语法编译通过。
- 说明：`render_icons.py` 与 `make_mockups.py` 的全量运行分别留给 T011/T016（避免在基线阶段重写保留项 PNG 与 mockups 产物）；本阶段以依赖导入 + 语法编译验证脚本可用性。

## US1 执行摘要（T005–T011）

- **下载（T005/T006）**：58 枚 Material Icons Outlined SVG 全部从 `google/material-design-icons`（`src/<category>/<name>/materialiconsoutlined/24px.svg`）下载至 `build/icon-staging/`。
- **映射修正**：`file-export-screenshot` 的 Material 图标名由 `screenshot_monitor` 调整为 **`screenshot`**（Material Icons 官方命名；`screenshot_monitor` 仅存在于 Material Symbols）。已同步更新 `contracts/icon-replacement-map.md` 修订记录。
- **色值归一化（T007）**：58 枚 SVG 根元素统一添加 `fill="#D4D4D4"`（FG_TEXT），显式黑色替换为 FG_TEXT，`fill="none"` 保留；`check_icons.py` P-01 全过。
- **16px 处置（T008）**：58 枚 Material Outlined 均为纯 fill 路径（无 stroke 属性），无需描边加粗；16px 原生 ≈1.33px 按契约 §5 处置路径 ③ 交由盲测兜底。
- **入库（T009）**：58 枚写入 `src/ui/theme/icons/actions/<icon_id>.svg`；`view-panel-console.svg` 未覆盖。
- **门禁（T010）**：新 58 枚 SVG 零违规；保留项 git diff 为空。
- **渲染（T011）**：177 个 actions PNG 三档生成；保留项 6 文件哈希与基线一致。

## US2 执行摘要（T012–T015）

- **契约修订（T012）**：`icon-style-spec.md` v1.1.0 → **v2.0.0**（线性圆角描边 / 24 网格 / 描边 1.5-2-2.5；P/T/N/A 条款保持；A-01 措辞同步、app-icon 不追溯）。
- **qrc（T013）**：`theme.qrc` 186 条目（177 actions + 7 app + 1 ico + 1 QSS）与文件系统一致，无变化。
- **对比度（T014）**：`check_theme_contrast.py` 修复后通过（25 主题 × 17 组，0 失败）。
  - **脚本修复（现存 bug）**：006 重构后主题色块拆分至 `theme_catalog_{dark,light,hc}.h` 且字段增至 28（新增 `dockDropHighlight`），脚本仍按旧结构解析导致 `KeyError: 'DarkClassic'`。已最小修复：FIELDS 补齐 `dockDropHighlight`、块扫描改为遍历 `src/ui/theme/theme_catalog_*.h`（校验逻辑不变）。README 脚本表描述已同步。
- **构建回归（T015）**：`build.ps1 -Gui` + CTest + pytest（13 passed）全绿。

## US3 执行摘要（T016–T022）

- mockups（preview / icon-bar / main-window）与 25 主题截图全部重生成。
- `docs/design/icon-spec.md` 更新至 v2.0.0（含工作流与素材来源）；`README.md` 图标体系与脚本表同步；`ui-guidelines.md` 无图标风格描述段，无需修改（T020 跳过）。
- 盲测材料就绪：`blind-test.md` 抽样扩至 12 枚（覆盖六类）+ `blind-test-16px.png`；3 人评审结果待正式评审阶段填写。
- 符合性自查：`conformance-checklist.md` v1.1.0（数值对齐 v2.0.0）逐项勾选，不符合项 0/31。

## T023 全链路回归

顺序执行 check_icons → render_icons → gen_qrc → make_mockups → update_screenshots → build.ps1 -Gui -UnitTests -Pytest：
- check_icons：仅 4 项冻结违规（app-icon 技术债，非本功能引入）
- render_icons / gen_qrc / make_mockups：退出码 0
- 截图：25 主题全部写入 `docs/screenshots/`
- 构建 + CTest + pytest（13 passed）：退出码 0

## T024 最终验收核验

| 验收项 | 结果 |
|---|---|
| SC-001 58 枚新图标命名/映射合规 | ✓ `check_icons.py` 新 58 枚零违规 |
| SC-002 保留项零改动 | ✓ git diff 为空；6 文件 SHA-256 与基线（T002）完全一致 |
| SC-003 语义盲测 ≥90% | 材料已备（12 枚 × 3 人），正式评审阶段执行 |
| SC-004 色板无新增违规 | ✓ 58 枚新 SVG 零 P-01；app-icon 4 项冻结技术债 |
| SC-005 对比度 | ✓ `check_theme_contrast.py` 25 主题 0 失败 |
| SC-006 构建 + 运行时无图标缺失 | ✓ build.ps1 -Gui + CTest + pytest 全绿 |
| SC-007 mockups/截图与最终图标一致 | ✓ 全量重生成 |
| 门禁口径说明 | `check_icons.py` 全局退出码因保留项 `app-icon.svg` 4 项现存违规（2026-08-25 引入，已冻结登记）非全绿；本功能引入违规数 = 0 |

**最终结论**：实现完成。58 枚功能图标替换为 Material Icons Outlined 并本地化入库；契约 v2.0.0 与文档/产物同步；构建与测试全绿；保留项零改动。盲测（SC-003）与正式符合性评审由 Reviewer 阶段执行（材料已备）。

## T025 导出截图图标调换（评审后修正）

- **背景**：`action.exportImage`（导出主界面图片）使用 `file-export-screenshot`，其 Material `screenshot`（手机形状带刻度）与「导出桌面主窗口」语义不匹配。
- **两轮调换**：Material 图标名 `screenshot`（手机形状）→ `crop_landscape`（矩形框，候选一）→ **`photo_camera`（相机，最终定稿）**；每次替换 `actions/file-export-screenshot.svg`（fill 保持主题前景 `#D4D4D4` 约定）并重渲染 16/24/32 PNG，其余 174 枚 PNG 字节不变。
- **映射同步**：`contracts/icon-replacement-map.md` 表项由 `screenshot_monitor`（早期误记，实际入库为 `screenshot`）→ `photo_camera`。
- **回归**：render_icons 退出码 0；check_icons 零新增违规；mockups（preview/icon-bar/main-window）与 `docs/screenshots/main.png` 重生成。

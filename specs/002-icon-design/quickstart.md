# Quickstart: 图标交付验收指南

**Feature**: 002-icon-design | **Branch**: `002-icon-design` | **Date**: 2026-08-25

> 本文档定义「如何证明图标功能端到端达成」的可运行验证场景。设计契约见 [contracts/](contracts/)，数据模型见 [data-model.md](data-model.md)。**本功能为设计交付，验证以评审 + 资源编译为主，不涉及 core 测试套件。**

## 前置条件

| 项 | 说明 |
|---|---|
| 交付物齐备 | 全量功能图标（SVG + 相应 PNG）、应用图标（SVG/PNG/ICO）、`icon-map.yaml`、mockups 视觉稿 |
| 评审组就绪 | ≥3 名评审者（含至少 1 名非设计角色） |
| 构建环境 | Windows + MSVC + Qt 5.15.2（含 Qt5Svg，若缺见风险项） |

## 场景 1：规范符合性评审（SC-002 / SC-005）

1. 评审者逐项勾选 `contracts/conformance-checklist.md`（31 项：A 风格 / B 尺寸 / C 色板 / D 五态 / E 命名映射 / F 应用图标 / G mockups 集成）。
2. 任一 `[ ]` 未勾 = 1 个不符合项。
3. **通过条件**：不符合项 = **0**（SC-002），且 D 组（五态深色可辨）全勾（SC-005）。

## 场景 2：语义识别盲测（SC-003）

1. 从映射表随机抽取 **≥10% 且 ≥10 枚**图标（覆盖全部六大类别）。
2. 3 名评审者**独立**观看图标（不含文字标签），写出猜测的功能语义。
3. 与映射表 `semantic` 比对。
4. **通过条件**：正确率 ≥ **90%**。

## 场景 3：应用图标多尺寸检查（SC-006）

1. 检查交付物：SVG 源 + PNG（16/24/32/48/64/128/256）+ `.ico`（含 16px 分辨率）齐备。
2. 在深色背景（`#161616`）上分别查看 256px 与 16px 档。
3. **通过条件**：256px 品牌辨识清晰；16px 档仍可辨图形轮廓（线条不糊、主色可辨）。

## 场景 4：资源编译与窗口图标集成（FR-011 接口位）

```powershell
# 1) 资源编译验证（.qrc 条目正确、SVG/PNG/ICO 路径有效）
cmake --build build --target Perception   # 预期：qrc 编译通过，链接成功

# 2) 运行验证窗口图标已挂接
./build/Perception.exe                    # 预期：任务栏与窗口标题栏显示应用图标（ACCENT 主色线性风格）
```

> 注：按钮图标实际挂接至 `QAction` 依赖后续「按钮布局」需求（ActionRegistry 落地）；本功能仅交付资源与 `main.cpp` 窗口图标挂接。SVG 渲染依赖 Qt5Svg，若构建环境缺失则改用预渲染 PNG 方案（见 research.md 决策 6 风险登记）。

## 场景 5：Mockups 视觉评审（FR-009 / 宪法 II）

1. 打开 `docs/design/mockups/005-icon-set/preview.png`（或对应界面目录）。
2. 核对：
   - 全量功能图标以五态展示；
   - 应用图标在深色主题上下文（窗口/任务栏场景）中展示；
   - 图标集与既有界面 mockups（如已有主窗口稿）风格一致。
3. **通过条件**：无异议项；评审结论记录到 `conformance-checklist.md` G 组。

## 回归确认

- 本功能不触碰 `src/core`，**无需**新增 CTest/pytest 用例；执行既有 `ctest` 全量回归确保构建未被破坏。
- 若构建环境不含 Qt5Svg 导致方案切换（SVG → 全 PNG），需更新本指南场景 4 与 `contracts/icon-style-spec.md` A-03 交付物清单。

## 完成信号

| 验证 | 通过标准 | 状态 |
|---|---|---|
| 场景 1 | 不符合项 0 | ☑ 实施核对 0 不符合（`mockups/005-icon-set/conformance-review.md`，待正式评审勾选） |
| 场景 2 | 识别率 ≥90% | ☐ 待正式评审（`mockups/005-icon-set/blind-test.md`） |
| 场景 3 | 多尺寸交付 + 16px 可辨 | ☑ 已交付（SVG + 7 档 PNG + 7 分辨率 ICO） |
| 场景 4 | 编译通过 + 窗口图标显示 | ☑ 构建通过（RCC `qrc_theme.cpp` 编译 + `perception.exe` 生成）；窗口图标已挂接 main.cpp（运行目检由正式评审确认） |
| 场景 5 | mockups 评审无异议 | ☐ 待正式评审（preview / icon-bar / main-window mockup 已生成） |

全部通过后，本功能视为达成（对应 spec 全部成功标准 SC-001~SC-006）。

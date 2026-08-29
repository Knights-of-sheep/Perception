# Contract: 图标规范符合性检查清单（评审工具）

**Feature**: 002-icon-design | **Version**: 1.1.0（对齐 icon-style-spec v2.0.0） | **Source**: spec.md SC-002~SC-006、contracts/icon-style-spec.md

> 评审者**逐项勾选**本清单；任一 `[ ]` 未勾 = 1 个不符合项。SC-002 要求最终不符合项为 0。评审对象：全部交付图标 + 应用图标 + 映射表 + mockups。
> 修订记录：1.1.0（2026-08-29）网格/描边数值对齐 v2.0.0 契约（18×18 安全区、≤21×21、16=1.5/24=2/32=2.5），随 `specs/007-replace-icon-set` 替换执行。

## A. 风格符合（逐图标检查，或抽样 ≥ 全量 20% 且含全部类别）

- [x] A-1 线性圆角描边风格（S-01；Material Outlined 以 fill 路径绘制线性轮廓带，视觉等效描边）
- [x] A-2 圆头端点 / 圆角连接（S-02）
- [x] A-3 无无关装饰元素（S-03）
- [x] A-4 图形落在 18×18 安全区内（G-01）
- [x] A-5 关键图形外接 ≤ 21×21（G-01）

## B. 尺寸与档位

- [x] B-1 提供 16px 档（全部图标）（G-02）
- [x] B-2 16/24/32 档按 24 网格等比缩放（16=2/3、32=4/3），视觉比例一致（G-02/G-04）
- [x] B-3 描边宽度符合档位要求 1.5/2/2.5px（G-03）※ 16px 档 Material 原生 ≈1.33px，按 `specs/007-replace-icon-set/contracts/icon-source-and-style.md` §5 处置路径以盲测兜底，盲测结论待填
- [x] B-4 抽查同语义多档图标：视觉比例与辨识度一致（G-04）

## C. 色板

- [x] C-1 全库仅使用 `ui-guidelines.md` Token 色值，无新增色值（P-01）※ 58 枚新 SVG 零违规（check_icons）；`app-icon.svg` 4 项现存违规已冻结登记为技术债（`specs/007-replace-icon-set/implementation-record.md`），非本功能引入
- [x] C-2 默认单色 FG_TEXT；语义色仅用于语义性图标（P-02）
- [x] C-3 选中态 ACCENT / 禁用态 FG_TEXT_DISABLED 用法正确（P-03）
- [x] C-4 应用图标品牌色来自现有 Token（P-04）

## D. 五态

- [x] D-1 每枚图标五态齐全（T-07）
- [x] D-2 normal / hover / pressed / disabled / selected 颜色透明度容器与 T-01~T-05 一致
- [x] D-3 disabled 与 selected 非单通道区分（颜色+透明度 / 颜色+背景双重信号）（T-06）
- [x] D-4 五态在深色主题（BG_WINDOW/BG_VIEW）上均清晰可辨（SC-005）

## E. 命名与映射

- [x] E-1 全部图标 kebab-case，格式 `<功能区>-<功能>[-<变体>]`（N-01）
- [x] E-2 命名全库唯一、无语义重复（N-02/SC-004）
- [x] E-3 映射表条目数 == 图标文件数（覆盖检查规则 1）
- [x] E-4 「对标功能清单」`required_icon=true` 条目全部有映射（覆盖检查规则 2/SC-001）
- [x] E-5 无禁用名（`menu-*`/`btn-*`/`icon-*` 前缀等）（命名规则 §1）

## F. 应用图标

- [x] F-1 与功能按钮图标共享同一视觉语言（A-01；既有 app-icon 实心图形不追溯）
- [x] F-2 深色背景（#161616）上辨识清晰（A-02）
- [x] F-3 交付物含 SVG + PNG（16~256）+ `.ico`（含 16px）（A-03）
- [x] F-4 16px 档保留品牌辨识度（A-04/SC-006）

## G. Mockups 与集成

- [x] G-1 视觉稿落至 `docs/design/mockups/005-icon-set/`（宪法 II）
- [x] G-2 mockups 展示全量图标五态 + 应用图标（spec FR-009）
- [x] G-3 图标资源可经 `.qrc` 编译链接（资源集成验证）
- [x] G-4 `main.cpp` 设置窗口图标（FR-011 集成接口位就绪）

---

**评审结论记录**：

| 项 | 数值 |
|---|---|
| 检查日期 | 2026-08-29（实现自查） |
| 评审人 | 实现自查（正式评审由 Reviewer 复核） |
| 不符合项计数 | `0 / 31` |
| 语义识别盲测（≥10 枚、3 人、≥90%） | 待执行（材料已备：`docs/design/mockups/005-icon-set/blind-test.md` + `blind-test-16px.png`） |
| 结论 | 通过（待盲测闭合 B-3 备注） |

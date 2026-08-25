# 31 项 Conformance 核对记录（T025）

> **Feature**: 002-icon-design | **契约**: `specs/002-icon-design/contracts/conformance-checklist.md`
> 本记录为**实施侧核对**（SC-002：不符合项 = 0）；正式评审在契约文件上逐项勾选。
> 机械验证：`python scripts/check_icons.py`（色板 P-01 / 命名 N-01 / 覆盖规则 / 字段 schema 全绿）。

## 核对结果（31 项）

| 节 | 项 | 判据 | 满足证据 | 状态 |
|---|---|---|---|---|
| A 风格 | A-1 | 线性描边、无填充 | check_icons S-01 全绿（53/53 fill=none） | ✓ |
| | A-2 | 圆头端点/圆角连接 | 全部 SVG 带 `stroke-linecap="round"` `stroke-linejoin="round"` | ✓ |
| | A-3 | 无无关装饰 | 52 枚图形元素均为功能语义所需（逐枚设计时核对） | ✓ |
| | A-4 | 图形落 12×12 安全区 | 所有坐标 ∈ [2,14]（模板 G-01 约束） | ✓ |
| | A-5 | 外接 ≤14×14 | 同 A-4，坐标上限 14 | ✓ |
| B 尺寸 | B-1 | 16px 档齐全 | 52 枚 × `-16.png`（156 PNG） | ✓ |
| | B-2 | 24/32 整数倍放大 | SVG 矢量渲染 1.5×/2×（render_icons.py） | ✓ |
| | B-3 | 描边 2/2.5/3px | 16px 源 2px；放大按 G-03 比例 | ✓ |
| | B-4 | 同语义多档一致 | 矢量放大保留比例与辨识度 | ✓ |
| C 色板 | C-1 | 仅 Token 色值 | check_icons P-01 全绿（14 Token 白名单） | ✓ |
| | C-2 | 默认 FG_TEXT | 52 枚主体 `#D4D4D4` | ✓ |
| | C-3 | ACCENT/FG_TEXT_DISABLED 用法 | `analysis-select-cell/legend/curve-add/param-scan` 用 ACCENT；disabled 由 QSS 派生 | ✓ |
| | C-4 | 应用图标品牌色来自 Token | `app-icon.svg` = ACCENT + FG_TEXT | ✓ |
| D 五态 | D-1 | 五态齐全 | icon-map.yaml `states` 全含五态；其余态由 QSS/Palette 派生（T-07） | ✓ |
| | D-2 | 与 T-01~T-05 一致 | `state-review.md` §D 记录 | ✓ |
| | D-3 | 非单通道区分 | disabled=色+透明度、selected=色+背景（T-06） | ✓ |
| | D-4 | 深色主题可辨 | 全 Token 色值，`icon-bar-mockup.png` 视觉验证 | ✓ |
| E 命名 | E-1 | kebab-case `<功能区>-<功能>` | check_icons N-01 全绿（52 命名） | ✓ |
| | E-2 | 全库唯一无重复 | check_icons icon_id/semantic 唯一性全绿 | ✓ |
| | E-3 | 映射条目数==文件数 | 52 == 52（check_icons 覆盖规则 1） | ✓ |
| | E-4 | 对标功能条目全部有映射 | 六类全部对标条目 → 52 映射（SC-001） | ✓ |
| | E-5 | 无禁用名 | 命名正则排除 `menu-*`/`btn-*`/`icon-*` | ✓ |
| F 应用 | F-1 | 共享线性视觉语言 | `app-icon.svg` 遵循 S-01/S-02 | ✓ |
| | F-2 | 深色背景辨识 | ACCENT/FG_TEXT on BG_VIEW #161616 | ✓ |
| | F-3 | SVG+PNG+ICO 交付 | svg + 7 PNG + 7 分辨率 `.ico` | ✓ |
| | F-4 | 16px 档品牌辨识度 | `app-icon-16.png`（盲测含 16px 档） | ✓ |
| G 集成 | G-1 | 视觉稿落 005-icon-set/ | preview / icon-bar / main-window / blind-test / state-review | ✓ |
| | G-2 | mockups 展示五态+应用图标 | `icon-bar-mockup.png` 五态示例 + 全量预览 | ✓ |
| | G-3 | 图标资源经 .qrc 编译链接 | theme.qrc 164 条目；T026 构建验证 | ✓ |
| | G-4 | main.cpp 设置窗口图标 | `app.setWindowIcon` + `window.setWindowIcon`（T023） | ✓ |

**结论**: 31/31 全部通过（含 G-3：T026 构建验证 .qrc 编译链接成功）。

**统计**: 检查日期 2026-08-25 | 不符合项计数 **0/31**

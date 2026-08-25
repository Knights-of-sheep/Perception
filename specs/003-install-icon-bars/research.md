# Research: 安装程序图标与功能图标

**Feature**: 003-install-icon-bars | **Date**: 2026-08-25 | **Phase**: 0（Outline & Research）

> 本文件解决 Technical Context 中全部 NEEDS CLARIFICATION。每项决策记录 Decision / Rationale / Alternatives。最终可检查映射见 [contracts/icon-action-map.md](contracts/icon-action-map.md)。

## 决策 1：菜单项图标映射策略（TC-1）

**Decision**: 菜单操作项按「语义唯一」原则配置图标。文件/编辑/视图/帮助菜单全操作项配图标；主题菜单（15 个动态勾选项）与日志级别菜单（5 个级别项 + 全部启用/禁用）为状态列表，不配图标；日志管理类操作（打开日志目录/设置日志路径/VTK 拦截）复用 `tools-settings`，清除日志复用 `edit-delete-selection`。完整映射见契约。

**Rationale**: icon-style-spec N-02 要求图标语义一一对应、无同义词重复；状态列表配图标无辨识增益反而产生语义噪音。最小新增原则（spec SC-006「新增图标如需要」）控制新增数量为 4 枚。

**Alternatives considered**:
- 所有菜单项（含主题/级别列表）全部配图 → 否决：需新增 10+ 枚近义图标，违反 N-02，辨识收益趋零
- 为日志管理新建专用图标 → 否决：`tools-settings` 语义足够接近，克制新增

## 决策 2：图标缺口的设计与命名（TC-2）

**Decision**: 新增 4 枚功能图标（SVG 源 + render_icons.py 渲染 16/24/32 PNG + icon-map.yaml 登记），命名遵循 N-01 kebab-case `<功能区>-<功能>`：

| 新图标 id | 功能 | 类别 |
|---|---|---|
| `file-load-script` | 加载脚本 | file |
| `file-record-screen` | 主界面视频录制 | file |
| `view-refresh` | 刷新 | view |
| `view-panel-toggle` | 面板显隐（通用，备用） | view |
| `view-panel-data` | 数据面板（数据集列表条目，被控面板语义） | view |
| `view-panel-property` | 属性面板（属性行-标签+值色块，被控面板语义） | view |
| `view-panel-console` | Python 控制台（终端窗口+命令提示符，被控面板语义） | view |

三个面板开关（数据面板/属性面板/命令窗口）均为「显隐切换」意图：图标直接表达被控面板本身的样子（数据集列表 / 属性行 / 终端窗口），让用户一眼看出"这个按钮控制显示/隐藏哪个面板"；`view-panel-toggle` 保留为通用面板显隐图标（备用，无动作引用）。

**Rationale**: 从按钮意图（被控面板）出发，图标直接画出被控面板的样子；图标内部用紧凑形状（避免大块亮色面板框），让 checked 态下与 selectionBg 形成亮度对比时形状仍然清晰可辨（icon_factory.cpp T-06：选中态保留原图，不染色）；图标风格遵循 icon-style-spec（S-01 实心填充 FG_TEXT、G-01 16 网格安全区 12×12）。

**Alternatives considered**:
- 三开关继续复用 `view-panel-toggle`（初始方案）→ 否决：用户反馈三按钮图标相同且语义不明，难以区分
- 复用既有 `view-layer-visibility`（图层可见性）→ 否决：语义为「图层可见」，与「面板显示/隐藏」不一致，违反 S-03 图形语义一致
- 复用 `file-open` 表示加载脚本 → 否决：与加载文件语义冲突

## 决策 3：功能栏实现与五态呈现（TC-3）

**Decision**: 使用 Qt `QToolBar`×2：左侧 `Qt::LeftToolBarArea`、右侧 `Qt::RightToolBarArea`，均 `setOrientation(Qt::Vertical)`，objectName 设为 `leftToolBar` / `rightToolBar`；QSS 在 theme_template.qss 既有 QToolButton 样式基础上为两条功能栏定义纵向样式（固定宽度、`ToolButtonIconOnly`、iconSize 24px）。五态呈现：

| 状态 | 实现方式 | 依据 |
|---|---|---|
| normal | 原始图标（FG_TEXT 实心 PNG） | T-01 |
| hover | QSS 容器 `BG_CONTROL` 圆角底 + 图标不变 | T-02 |
| pressed | QSS 容器加深 | T-03 |
| disabled | 运行时派生：QPixmap 染 `FG_TEXT_DISABLED #6E6E6E` + 40% 透明度，注册为 QIcon 的 Disabled 状态 | T-04 |
| selected | checkable 按钮 `setChecked` + QSS `:checked` 容器底色（SELECTION_BG）+ 可选 ACCENT 着色图标（运行时派生） | T-05/T-06 |

五态派生由 `src/ui/theme/` 层一个轻量工具（IconFactory）完成，对同一枚 PNG 用 QPainter 染色生成 disabled/selected 变体。满足 T-07「直接资源或规范可派生」。

**Rationale**: 零资源膨胀（不将 52×5 全量渲染入 qrc，体积与加载时间最优，支撑 SC-002 <1s）；派生逻辑集中可单测；与 check_icons.py 符合性兼容（T-07 允许派生）。

**Alternatives considered**:
- render_icons.py 全量渲染五态 PNG（52×5×3 尺寸）→ 否决：资源体积膨胀约 5 倍、qrc 编译与启动加载变慢、维护成本高
- 仅靠 QSS 颜色区分（不做图标染色）→ 否决：disabled 与 normal 需双通道区分（T-06），禁用图标必须变灰
- 引入第三方图标主题库 → 否决：违反宪法设计约束（不引入外部 UI 框架），且 002 图标集是唯一事实源

## 决策 4：未实现功能按钮的行为（TC-4）

**Decision**: 撤销/重做/加载脚本/主界面视频录制/刷新 5 个动作创建为 `QAction`，`setEnabled(false)`，tooltip 注明「功能即将推出」；不连接占位槽（点击无响应，符合禁用态语义）；图标立即展示（用新增缺口图标或既有图标）。撤销/重做同时加入「编辑」菜单（禁用态展示）。

**Rationale**: 用户确认 Q2=A（仅安装图标按钮，功能本体由后续独立 spec 实现）；禁用态保留可发现性（spec FR-011）；不连接槽确保无副作用。

**Alternatives considered**:
- 不创建按钮 → 否决：用户明确要求按钮存在于功能栏
- 连接占位槽弹「未实现」提示 → 否决：与 Qt 禁用态惯例不一致，spec 允许禁用或提示二选一，禁用更简洁

## 决策 5：右侧功能栏按钮的动作绑定（TC-5）

**Decision**: 右侧 9 枚按钮（view-zoom-in/zoom-out/fit-screen/reset-camera + analysis-curve-add/remove/extract/axis-settings/legend）全部创建为禁用态 QAction（`setEnabled(false)`，tooltip 注明「功能待实现」），图标立即展示。后续对应功能 spec 逐枚启用并绑定命令层或 VTK 相机操作。

**Rationale**: 用户确认 Q1=C（按钮集合=视图 4 + 数据 5）且 Q2=A（仅装按钮）；当前中央区域为占位视图、数据操作命令层尚未实现对应命令，启用按钮将违反宪法 IV（UI 不得绕过命令层）。

**Alternatives considered**:
- 接线到现有命令层占位 → 否决：命令层无对应命令，违反宪法 IV
- 隐藏右侧功能栏直至功能就绪 → 否决：spec FR-005 要求本迭代交付右侧功能栏布局与图标

## 决策 6：图标挂接方式与可测试性（TC-6）

**Decision**: 动作与图标挂接集中在 `MainWindow::createActions()`，由一个常量表（ActionIconMap，与 [contracts/icon-action-map.md](contracts/icon-action-map.md) 同源）驱动：每个动作的 text/tooltip/iconId/checkable/enabled 均从常量表读取。图标经 `QIcon(":/perception/icons/icons/png/actions/<id>-24.png")` 挂接（24px 为功能栏基准，菜单自动用 16px）。新增 `tests/cpp/icon_action_map_test.cpp` 验证：映射表集合完整（左侧 10 / 右侧 9 / 菜单全操作项）、每个动作 iconId 对应图标文件存在、tooltip 非空、未实现动作 enabled=false、checkable 动作正确。

**Rationale**: 契约驱动 + 单测锁定，防止图标挂接漂移；宪法 V 测试先行（先红后绿）。契约表是单测输入，单测是可执行化契约。

**Alternatives considered**:
- 每个动作散落硬编码 setIcon → 否决：不可测、易遗漏、与契约脱节
- 仅手动 UI 验证 → 否决：违反宪法 V 测试先行，无法在无头 CI 上回归

# Perception UI 设计规范（ui-guidelines）

> 界面**设计系统**唯一事实源。视觉稿事实源为 `docs/design/mockups/`（本地 mockup）。
> 本文件回答：界面怎么做才好看、专业、易用、好扩展、好修改。
> 实现者在 `specify` / `plan` / `tasks` 阶段必须同时读取本文件与对应 mockup。

---

## 1. 设计原则

1. **对标现代 IDE**：深色主题、扁平化、高对比、克制。
2. **数据为先**：产品是 TCAD 曲线/结构查看器，视图区永远是视觉主角。
3. **无例外原则（主题统一）**：任何组件——含弹层、系统对话框、第三方渲染——不得脱离主题单独存在。没有"来不及改就先默认样式"的例外。
4. **留白即专业**：控件宁少勿多，MVP 只放"必须有"的控件，其余收进菜单/右键/命令面板。
5. **单一事实源**：所有颜色、字体、间距只定义一次（见 §3 设计系统），任何组件引用 Token，不复制色值。

---

## 2. 参考系：对标产品范式

| 对标产品 | 界面范式 | 借鉴点 |
|---|---|---|
| Synopsys svisual（直接对标） | 左侧数据集树 + 中央图区 + 右侧属性 | 极简克制、数据为先 |
| TecplotSV | 强侧边栏 + 多窗格 + 灵活布局 | 多视图、布局可存 |
| ParaView（VTK 生态标杆） | Pipeline Browser + 属性面板 + 视图 + 信息面板 | 信息层级范式：左"做什么"→ 中"看什么"→ 右"改什么" |
| VS Code / Qt Creator | 深色主题 + Dock 布局 + 命令面板 | 视觉语言、命令面板、状态栏密度 |

本项目界面本质是**左-中-右三段式**（文件/数据集 → 视图 → 属性），与现有
`MainWindow.h` 骨架（左文件树 Dock + 中央视图 + 右属性 Dock）一致。

---

## 3. 设计系统（Token 定义，唯一来源）

### 3.1 分层色板（深色主题，参考 VS Code Dark+）

背景分层（3 层，形成空间感）：

| Token | 色值 | 用途 |
|---|---|---|
| `BG_WINDOW` | `#1E1E1E` | 主窗口背景 |
| `BG_PANEL` | `#252526` | Dock / 侧边栏 / 图例底 |
| `BG_CONTROL` | `#3C3C3C` | 按钮、输入框、树选中底 |
| `BG_VIEW` | `#161616` | 中央视图底色（比 L0 略深，突出绘图区） |

前景与边框：

| Token | 色值 | 用途 |
|---|---|---|
| `FG_TEXT` | `#D4D4D4` | 主文字 |
| `FG_TEXT_WEAK` | `#9D9D9D` | 次要文字、状态栏弱信息 |
| `FG_TEXT_DISABLED` | `#6E6E6E` | 禁用文字 |
| `BORDER` | `#454545` | 边框、分隔线 |
| `BORDER_WEAK` | `#3F3F3F` | 弱分隔线（网格线等） |

强调与状态色：

| Token | 色值 | 用途 |
|---|---|---|
| `ACCENT` | `#0A84FF` | 全 UI 唯一主色：选中、焦点、活动 Tab、主按钮 |
| `SELECTION_BG` | `#094771` | 列表/树选中底 |
| `SUCCESS` | `#4EC9B0` | 成功状态 |
| `WARNING` | `#CCA700` | 警告状态 |
| `DANGER` | `#F14C4C` | 错误状态 |

语义色只用于状态表达，不用于装饰。

### 3.2 曲线序列色板：Okabe-Ito（色盲友好）

曲线是产品核心内容，序列色板采用科研界标准 Okabe-Ito 8 色：

```
#0072B2 蓝   #E69F00 橙   #009E73 绿   #D55E00 红
#56B4E9 天蓝  #CC79A7 紫   #F0E442 黄   #000000 黑
```

颜色自动分配，不硬编码到数据集；超过 8 条曲线时循环。

### 3.3 字体层级

| 用途 | 字体 | 说明 |
|---|---|---|
| 界面文字 | `Segoe UI` | Win10/11 系统字体，免打包 |
| 数值/数据/图例 | `Cascadia Mono` / `Consolas` | 等宽，数字对位 |
| 字号 | 基础 9pt / 标题 11pt / 坐标轴 10pt | 只 3 档，不滥用 |

由 `ThemeManager` 统一 `qApp->setFont()` 下发，控件继承。

### 3.4 间距与密度

- 统一间距基准 4px，控件间距用 4/8/12/16 四档。
- Dock 内边距 8px；工具栏图标 16px（+文字可选）。
- 留白优先：宁可信息少，不堆满。

### 3.5 几何一致性

- 圆角半径统一 **4px**（按钮、输入框、ToolTip、菜单）。
- 内边距统一 8px/12px 两档。
- Focus 焦点环统一：2px `ACCENT` 描边，所有可获得焦点的控件一致。
- 悬停/按压反馈延迟统一（hover 即刻、菜单展开 200ms 内）。

---

## 4. 主题统一原则（无例外）

> 核心：深色界面 + 亮色控件 = 花斑 = 不专业。以下措施根除。

### 4.1 QSS 控件覆盖矩阵（全家族 × 全状态）

| 家族 | 必须覆盖的控件 | 容易漏的坑 |
|---|---|---|
| 弹层 | `QToolTip`、`QMenu`（弹出态）、`QComboBox` 下拉列表 | **ToolTip 默认系统黄底，必改** |
| 窗口 | `QMainWindow`、`QDialog`、`QMessageBox` | 对话框内容区、按钮条 |
| 列表树表 | `QTreeWidget`、`QListWidget`、`QTableView`、`QHeaderView` | 表头、展开箭头、选中/交替行 |
| 输入 | `QLineEdit`、`QComboBox`、`QSpinBox`、`QDoubleSpinBox`、`QTextEdit` | 光标色、placeholder 色 |
| 按钮 | `QPushButton`、`QToolButton`、`QCheckBox`、`QRadioButton` | 图标按钮 hover/checked 底 |
| 容器 | `QTabBar`、`QGroupBox`、`QSplitter`（handle）、`QDockWidget` 标题栏 | 分割线、Tab 活动态 |
| 反馈 | `QStatusBar`、`QProgressBar`、`QScrollBar`（横/纵） | 滚动条滑块 hover/pressed 三态 |
| 状态 | 全部控件的 normal/hover/pressed/checked/disabled/focus | **disabled 态最易漏，漏了灰成一片** |

QSS 定位规则：

```css
/* 用 对象名 + 自定义属性 定位，不用全局选择器堆叠 */
QDockWidget[perceptionRole="fileDock"] { background: @BG_PANEL@; }
QPushButton[emphasis="primary"] { background: @ACCENT@; color: #FFFFFF; }
/* 全部状态覆盖：normal / hover / pressed / checked / disabled / focus */
```

### 4.2 QSS + QPalette 双轨兜底

QSS 覆盖不到的原生绘制路径，用统一 `QPalette` 兜底，在 `main()` 一次性设置：

```cpp
QApplication app(argc, argv);
ThemeManager::applyPalette(app);     // Window/Base/Text/Highlight/Button/Disabled 全组
ThemeManager::applyStyleSheet(app);  // dark.qss 精修
```

即使未来新控件漏写 QSS，也不至于白底黑字穿帮。

### 4.3 系统对话框/弹层禁用原生

- `QFileDialog`：Windows 默认弹系统原生对话框，不受 QSS 控制。
  方案：`QFileDialog::DontUseNativeDialog` 强制 Qt 自绘；MVP 更优解是用左侧文件树 + 中央拖放代替。
- `QToolTip`：QSS 强制深色。
- 规范：**项目内禁用一切原生弹层**。

### 4.4 VTK 渲染侧染色同步（QSS 管不到 OpenGL）

`vtkChartXY` 是 OpenGL 自绘，不吃 QSS，C++ 侧必须同步主题色：

```cpp
// theme/ 提供：把主题色板同步到 VTK 图表对象
void applyThemeToVtkChart(vtkChartXY* chart);
// 坐标轴文字 = FG_TEXT；图例底 = BG_PANEL 半透明
// 图表底 = BG_VIEW；网格线 = BORDER_WEAK
```

render 层验收标准：**图区颜色只能来自 `theme/palette.h`**，禁止硬编码。

### 4.5 主题令牌：颜色单点定义，三方共用

```
src/ui/theme/
├─ palette.h              ← 唯一语义色定义（C++ 侧常量，含注释）
├─ dark.qss.template      ← @BG_WINDOW@ @ACCENT@ 等令牌占位
├─ dark.qss               ← 生成物（令牌替换后）
└─ theme_manager.h/.cpp   ← applyPalette / applyStyleSheet / 热切换 / 记住选择
```

改主题 = 改 `palette.h` 一处 → 重新生成 qss + VTK 染色同步。杜绝各组件各说各话。

### 4.6 Showcase 验证

调试用 `ShowcaseWindow`：把全部控件类型铺在一页（菜单、ToolTip、下拉、滚动条、
Tab、对话框、禁用态…），每次主题改动后视觉巡检 + 截图基线对比。

**强制规则**：新增任何控件类型，必须先确认 QSS 矩阵已覆盖，并在 Showcase 页可检查。

---

## 5. 布局与功能范式（M3a 目标态）

```
+----------------------------------------------------+
| 菜单栏   文件 / 视图 / 工具 / 帮助                  |
| 工具栏   [打开] [导出] [重置视图] [主题切换]        |
+-------------+------------------------+-------------+
| Dock: 数据集树| 中央视图区             | Dock: 属性  |
| 加载文件     | QVTKOpenGLNativeWidget | 选中项属性/ |
| + 内部曲线/  | + vtkChartXY           | 变换参数    |
|  结构        | + 图例 + 坐标轴        |             |
| (ParaView    |                        |             |
|  Pipeline式) |                        |             |
+-------------+------------------------+-------------+
| 状态栏  鼠标数据坐标 | 数据集名 | 进度 | 版本       |
+----------------------------------------------------+
```

### 5.1 专业细节

1. **Dock 布局记忆**：`saveState()/restoreState()` + `QSettings`，下次打开还原布局。
2. **状态栏信息密度**：左 = 鼠标处曲线数据坐标（`x=1.2e-3 y=4.5e2`）；右 = 数据集名、加载耗时、版本。
3. **命令面板（Ctrl+Shift+P）**：项目是命令驱动架构，命令输入框统一收敛菜单操作；
   同时为 M5 Python 命令层预留入口。本项目招牌功能。
4. **右键上下文菜单**：图区 = 导出图像 / 重置视图 / 复制坐标；树 = 加载 / 移除 / 重命名。
5. **空状态设计**：未打开文件时中央显示引导页（图标 + "拖放 .plt 文件到此处，或 Ctrl+O 打开"），不显示黑屏。
6. **视图操作**：左键平移、滚轮以光标为锚缩放、双击自适应范围（肌肉记忆）。

---

## 6. 交互规范（易用）

| 交互 | 实现要点 |
|---|---|
| 拖放打开 | `setAcceptDrops`，支持 .plt/.csv 拖入 |
| 双击打开 | 树节点双击 → 加载并聚焦中央视图 |
| 操作反馈 | 大文件加载用状态栏内嵌 `QProgressBar` |
| 错误反馈 | 非模态（状态栏红字 + 可选通知），不用 `QMessageBox` 打断 |
| Undo/Redo | M4 引入，UI 层先留 `QUndoStack` 接口位 |
| 快捷键 | 打开 Ctrl+O / 导出 Ctrl+E / 重置视图 Home / 命令面板 Ctrl+Shift+P |

> 易用原则：控件越少越好用；一切操作都能在 3 步内完成或找到入口。

---

## 7. UI 架构（好扩展、好修改）

### 7.1 src/ui 内部结构

```
src/ui/
├─ mainwindow/       MainWindow + 布局装配（只做组合）
├─ actions/          动作注册表（所有 QAction 唯一来源）
├─ commands/         命令桥（对齐 python 命令层）
├─ docks/            各 Dock 面板（file_tree_dock / property_dock ...）
├─ views/            中央视图（curve_view / 未来 structure_view）
├─ widgets/          通用小控件（坐标显示、图例控件 ...）
├─ theme/            palette.h + dark.qss(.template) + ThemeManager
├─ dialogs/          对话框（关于、设置）
└─ showcase/         控件博览页（ShowcaseWindow，调试用）
```

依赖方向：`MainWindow` 只依赖 `actions / docks / views / theme` 的接口，
具体实现经注册表注入——单文件可独立测试。

### 7.2 动作集中管理（ActionRegistry）

菜单项、工具栏按钮、快捷键、右键菜单**共享同一 `QAction` 实例**——改一处全局生效：

```cpp
class ActionRegistry {
public:
    enum Id { Open, Export, ResetView, ToggleTheme, CommandPalette, /* ... */ };
    QAction* action(Id id) const;      // 唯一实例
    QMenu*   buildMenu() const;        // 菜单从注册表生成
    QToolBar* buildToolBar() const;    // 工具栏从注册表生成
};
```

### 7.3 命令桥（对齐命令驱动架构）

UI 操作翻译成命令对象执行，与 pybind 命令层（`load_plt / transform / query / export`）一一对应：

```cpp
struct ICommand {
    virtual ~ICommand() = default;
    virtual void execute() = 0;
    virtual void undo() {}                    // 默认不支持撤销
    virtual QString description() const = 0;  // 状态栏显示 / 日志
};
```

好处：菜单、命令面板、Python 脚本、快捷键走同一条路，M5 命令层接入时 UI 零改动。

### 7.4 视图/面板注册表

新增视图（如 .tdr 结构查看器）= 注册一个工厂，`MainWindow` 不写死任何视图类：

```cpp
struct ViewFactory {
    QString id; QString title;
    std::function<QWidget*()> create;
};
void registerView(ViewFactory f);   // 各视图模块启动时自注册
```

### 7.5 主题热切换

`ThemeManager` 提供 `qApp->setStyleSheet()` 热切换（dark/light），
选择持久化到 `QSettings`。色板常量收进 `theme/palette.h`，QSS 与 C++ 引用同一语义色。

---

## 8. 落地路线（M3a 三步）

| 步骤 | 交付物 | 验证标准 |
|---|---|---|
| ① 视觉系统 | `theme/palette.h` + `dark.qss` + 字体/间距规范 | 纯 QSS 可预览；无 Qt 也能评审色板 |
| ② 布局框架 | MainWindow + 3 Dock + 中央占位 + 状态栏 + Dock 布局记忆 | 运行即见完整三段式深色界面 |
| ③ 动作系统 | ActionRegistry + 命令面板骨架 + 菜单/工具栏/快捷键 | Ctrl+O / Ctrl+Shift+P 可触达占位操作 |

每步独立构建、独立评审；Qt 环境就绪后在此骨架上接入 VTK 曲线视图（M3）。

---

## 9. 与既有文档的关系

- `docs/design.md`：设计规范入口，引用本文件。
- `docs/design/mockups/`：视觉稿事实源（PNG + notes.md），本规范是其"规则层"。
- `src/ui/`：实现层，必须同时满足本规范与对应 mockup。

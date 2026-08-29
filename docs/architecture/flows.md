# Perception 架构流程图

> 用 Mermaid 描述项目整体逻辑，与 [architecture.md](../architecture.md) 互为补充。
> Mermaid 可直接在 GitHub 渲染；VS Code 需安装 Mermaid 预览插件。
> 图例：── 实线 = 直接依赖 / 调用；┄┄ 虚线 = 事件订阅 / 发布。

## 1. 模块分层与依赖

```mermaid
flowchart TD
    subgraph L5["app · 程序入口"]
        MAIN["main.cpp<br/>高DPI → 元信息 → 字体 → 主题 → 主窗口 → 日志装配 → 事件循环"]
    end

    subgraph L4["ui · 界面层（Qt5 Widgets + QSS）"]
        MW["MainWindow<br/>菜单 / 工具栏 / 3 Dock / 拖拽高亮 / 全屏协调 / 多屏最大化"]
        SUB["subwindow<br/>SubwindowContainer · SubwindowView · LayoutManager"]
        CON["console/PythonConsole<br/>内嵌 CPython REPL"]
        THEME["theme/ThemeManager<br/>25 套主题 · QSS 模板渲染"]
        BRIDGE["log/qt_message_bridge<br/>qDebug/qWarning 纳入统一日志"]
        MISC["action_icon_map · dialog_title_bar<br/>window_geometry"]
    end

    subgraph L3["render · 渲染层（M3b 未落地）"]
        REND["VTK Charts 2D 曲线<br/>vtkChartXY + 色板"]
    end

    subgraph L2["python · 命令驱动层（M5 未落地）"]
        PYCMD["perception_py（pybind）<br/>load_plt / transform / query / export"]
    end

    subgraph L1["core · 数据核心（无 UI/渲染依赖，可独立单测）"]
        MODEL["model<br/>Curve · IDataSet · FieldData · IStructure"]
        IO["io<br/>IReader · ReaderRegistry · PltReader · CsvReader"]
        PROC["process<br/>ITransform · UnitScale"]
        EVT["event/EventBus<br/>发布-订阅"]
        LOG["log<br/>Logger · FileSink · TerminalSink · LogLevelMatrix"]
    end

    MAIN --> MW & CON & THEME & BRIDGE
    MAIN --> LOG
    MW --> SUB & CON & THEME
    CON --> PYCMD
    SUB --> REND
    PYCMD --> IO & PROC
    IO --> MODEL
    PROC --> MODEL
    REND --> MODEL
    EVT -. "发布 DataSetChanged" .-> REND
    EVT -. "发布 DataSetChanged" .-> MW
    LOG -. "配置 / 读取 sink" .-> MAIN
```

要点：

- **core 不依赖任何 UI/渲染库**，独立可单测（CTest）；render 只依赖 core + VTK；ui 依赖 core + Qt。
- **事件驱动**：render / ui 只订阅 `event/EventBus` 刷新画面，不直接轮询 core。
- **命令驱动**（规划）：数据 CRUD 全部经 `python/` 命令层；ui 的数据操作翻译为 Python 命令，纯 UI 变化（布局 / 主题 / 面板）例外。
- **格式可扩展**：新曲线格式 = 新增 `ICurveReader` 并注册，核心与消费者零改动。

## 2. 程序启动流程（src/app/main.cpp）

```mermaid
flowchart TD
    A["启动 perception.exe"] --> B["高 DPI 属性<br/>AA_EnableHighDpiScaling / AA_UseHighDpiPixmaps"]
    B --> C["QApplication 构造<br/>组织名/应用名/版本 0.1.0"]
    C --> D["Fusion 风格<br/>QSS 深色主题 100% 生效"]
    D --> E["Segoe UI 9pt 字体"]
    E --> F["ThemeManager::apply<br/>读 QSettings 上次主题 → QPalette + QSS"]
    F --> G["MainWindow 创建并 show<br/>恢复 Dock 布局 + 子窗口容器"]
    G --> H["日志装配<br/>%APPDATA%/Perception/logs/app.log"]
    H --> H1["Logger::configure<br/>文件路径 · 默认级别矩阵 · 5MB ×3 归档"]
    H1 --> H2["addSink TerminalSink<br/>彩色终端输出"]
    H2 --> H3["restoreLogSettings<br/>恢复 QSettings 级别矩阵与 VTK 开关"]
    H3 --> H4["installQtMessageBridge<br/>qDebug/qWarning 纳入统一日志流"]
    H4 --> I["app.exec 事件循环"]
    I --> J["退出<br/>shutdownPython 释放内嵌 Python 运行时"]
```

调试参数（`--snapshot` / `--theme` / `--console-script` / `--subwindows` 等）在进入事件循环后经 `QTimer::singleShot` 驱动，用于截图回归。

## 3. 数据加载与事件驱动

```mermaid
flowchart LR
    CMD["命令层 load_plt(path)"] --> REG["io::ReaderRegistry<br/>按文件扩展名分派"]
    REG --> RD["PltReader / CsvReader<br/>路径校验 + ReadOptions.maxBytes 上限"]
    RD --> DS["model::IDataSet（格式无关）"]
    DS --> PUB["event::EventBus.publish<br/>DataSetChanged"]
    PUB --> R["render 订阅 → 重绘曲线"]
    PUB --> U["ui 订阅 → 刷新文件树 / 属性"]
    DS --> PROC["process 变换管道<br/>unit_scale 等（产生副本）"]
    PROC --> PUB
```

规则：数据变更必须先 `publish` 再返回；回调中避免再次发布同类型事件（`ScopedSubscription` RAII 防悬挂回调）。

## 4. 日志系统（M4 统一日志）

```mermaid
flowchart TD
    SRC["日志来源<br/>PERCEPTION_LOG_* 宏 · qDebug/qWarning（经 qt_message_bridge）"]
    SRC --> LOG["core::log::Logger 单例<br/>统一入口"]
    LOG --> MATRIX["LogLevelMatrix 过滤<br/>DEBUG/INFO/WARN/ERROR/FATAL（全局单一矩阵）"]
    MATRIX --> FS["FileSink<br/>app.log · 5MB 滚动 ×3 归档"]
    MATRIX --> TS["TerminalSink<br/>彩色输出 stdout/stderr"]
    SET["设置菜单<br/>全局级别勾选 · VTK 开关 · 日志路径 · 清除历史"] -. "配置 / 迁移 / 清除" .-> LOG
```

## 5. 主题系统（25 套）

```mermaid
flowchart TD
    TRIG["触发<br/>启动 ThemeManager::apply / 菜单热切换 applyTheme"] --> CAT["theme_catalog.h<br/>25 套色板定义"]
    CAT --> RQ["renderQss<br/>theme_template.qss @token@ 占位符 → 最终 QSS"]
    RQ --> APP["QApplication::setStyleSheet"]
    TRIG --> PAL["applyPalette<br/>QPalette 兜底"]
    PAL --> APP
    APP --> REF["refreshActionIcons<br/>重建动作图标 · 控制台配色刷新"]
    REF --> PERSIST["saveThemeId → QSettings<br/>下次启动恢复"]
```

## 6. UI 交互

### 6.1 子窗口创建（004-dock-layout-manager）

```mermaid
flowchart LR
    P["REPL 输入 create_window()<br/>或菜单 视图 → 新建子窗口"] --> EXEC["PythonConsole.executeCommand<br/>与手工输入同一执行路径"]
    EXEC --> CB["CreateWindowCallback"]
    CB --> MW["MainWindow::createSubwindow<br/>返回 id（Plot_ 全局递增）"]
    MW --> SC["SubwindowContainer.addSubwindow"]
    SC --> LM["LayoutManager<br/>按 LayoutConfig 重排 QGridLayout"]
```

子窗口支持：容器内最大化 / 隐藏（View 菜单恢复）/ 最大化窗口前后循环切换；全屏由 MainWindow 隐藏三个 Dock 使容器扩展占满主界面。

### 6.2 多屏最大化（005-multi-screen-maximize）

```mermaid
flowchart TD
    WMM["nativeEvent 收到 WM_GETMINMAXINFO<br/>窗口尺寸改变前到达"] --> NG["记录 normalGeometry_<br/>最大化前几何（原屏原尺寸）"]
    NG --> MAX["最大化到目标屏<br/>ptMaxPosition 改相对目标屏坐标"]
    MAX --> CH["changeEvent<br/>prevMaximized_ 判断进入/退出过渡"]
    CH --> ICON["同步最大化/还原按钮图标"]
    CH --> RESTORE["退出最大化<br/>按 normalGeometry_ 还原原屏原尺寸"]
```

修复要点：`ptMaxPosition` 必须以**目标屏**的虚拟桌面坐标为准，而非主屏，否则副屏最大化后窗口会"消失"在屏幕外。

### 6.3 Dock 拖拽高亮

```mermaid
flowchart TD
    DOWN["按住 Dock 标题栏"] --> BEG["beginDockDrag<br/>显示全窗口覆盖层（鼠标穿透）"]
    BEG --> UPD["updateDockDrag<br/>按鼠标位置计算目标区域"]
    UPD --> OVER["绘制 VSCode 风格分割线高亮"]
    UP["松开鼠标"] --> END["endDockDrag<br/>执行放置 · 隐藏高亮"]
```

分界线（dock 边缘分隔条）resize 拖拽复用同类覆盖层，高亮条只覆盖分隔条缝隙，局部重绘不卡顿。

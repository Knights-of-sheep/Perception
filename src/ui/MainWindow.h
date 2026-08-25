// ===== Perception 主窗口（M3a：界面框架）=====
// 布局：菜单栏 / 工具栏 / 左侧文件树 Dock / 中央曲线视图（M3 接入 VTK）/ 右侧属性 Dock / 状态栏。
// UI 事实源：docs/design/mockups/。
#pragma once

#include <QMainWindow>

class QActionGroup;
class QCloseEvent;
class QDragEnterEvent;
class QDropEvent;
class QEvent;
class QLabel;
class QToolBar;
class QToolButton;
class QTreeWidget;

#include <QElapsedTimer>
#include <QList>
#include <QPair>
#include <QPointer>

namespace perception {
namespace core {
namespace log {
enum class LogLevel : int;
}
}  // namespace core
}  // namespace perception

namespace perception {
namespace ui {

class PythonConsole;  // 前向声明须与定义同名空间（类在 perception::ui 内）

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

public:
    // 程序退出前释放 Python 运行时（main 中 app.exec() 返回后调用）
    void shutdownPython();
    // 访问底部 Python 控制台（--console-script 调试注入等）
    PythonConsole* pythonConsole() const { return pythonConsole_; }

public slots:
    void resetLayout();            // 恢复默认布局（Ctrl+Shift+L / --snapshot 模式调用）
    void applyTheme(const QString& themeId);  // 主题热切换（菜单触发 / --snapshot 模式调用）

public:
    // Dock 拖拽高亮（VSCode 风格分割线指示）：由 DockTitleBar 拖拽回调驱动
    void beginDockDrag(QDockWidget* dock);            // 进入拖拽：显示高亮覆盖层
    void updateDockDrag(const QPoint& globalPos);     // 拖拽中：按鼠标位置更新高亮目标
    void endDockDrag(const QPoint& globalPos);        // 结束：执行放置并隐藏高亮
    // 分界线（dock 边缘分隔条）resize 拖拽高亮：由 MainWindow::event 检测分隔条命中驱动，
    // 高亮条 = 细条 overlay（只覆盖分隔条缝隙，局部重绘，不卡顿）
    void beginSashDrag(int hit);  // 按住分隔条：进入高亮
    void updateSashDrag();        // 拖拽中/布局变化后：同步高亮条到分隔条
    void endSashDrag();           // 松开：隐藏高亮条
    // 命中检测：主窗口局部坐标是否落在某条真实分隔条上（返回 SashHit，未命中返回 0）
    int  sashHitTest(const QPoint& pos) const;

private slots:
    void openFile();
    void about();
    void exportMainWindowImage();  // 导出主界面图片（grab + PNG）
    void exportPythonCommands();   // 导出控制台已执行的命令为 .py 脚本
    void onLevelToggled(bool checked);         // 全局级别矩阵勾选变化（FR-012）
    void onAllLevels(bool enabled);            // 全局级别：全部启用(true)/全部禁用(false)
    void openLogDir();                         // 打开日志文件所在目录（FR-014）
    void setLogPath();                         // 设置日志路径：选目录 + 迁移旧日志（FR-016）
    void clearLogHistory();                    // 清除历史日志（FR-017）

public:
    // 从 QSettings 恢复日志级别并应用（main.cpp 在 Logger::configure + addSink 后调用；FR-013）
    void restoreLogSettings();
    // 注入日志文件实际路径（main.cpp 在 Logger::configure 后调用；FR-014）
    void setLogFilePath(const QString& path);

protected:
    // 拖放打开 / 布局记忆（ui-guidelines §6 / §5.1）
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
    // Dock 拖拽高亮覆盖层跟随主窗口尺寸（拖拽中窗口 resize 时）
    void resizeEvent(QResizeEvent* event) override;
    // 无边框自定义标题栏：原生消息处理（Windows 拖拽 / 最大化 / 边缘 resize）
    bool nativeEvent(const QByteArray& eventType, void* message, long* result) override;
    // 最大化/还原时同步窗口控制按钮图标
    void changeEvent(QEvent* event) override;
    // 主题切换（palette 变化）时重新生成标题栏按钮图标颜色；
    // 同时检测分隔条命中（press/release）驱动分界线高亮
    bool event(QEvent* event) override;

private:
    void createActions();
    QRect sashHighlightRect() const;  // 当前高亮线矩形（由命中类型 + 最新 dock geometry 计算）
    void createMenus();
    void createTitleBar();  // 自定义标题栏：标题+窗口按钮，与菜单栏同一行（无边框）
    void createToolbars();  // 003：左右功能栏（纵向，ToolButtonIconOnly）
    void createDocks();
    void createCentralArea();
    void createStatusBar();
    void addFileToTree(const QString& path);  // 占位：加载后加入文件树
    void updateEmptyHints();                  // 空状态提示节点
    void toggleMaximize();                    // 最大化/还原切换
    void updateWindowButtonIcons();           // 同步最大化/还原按钮图标（changeEvent）
    // 003-install-icon-bars：动作图标注册（QIcon 创建时固化主题色，切换主题需重建）
    void setActionIcon(QAction* action, const QString& iconId);
    void refreshActionIcons();                // 主题切换后按当前色板重建全部动作图标

    // 动作
    QAction* openAction_ = nullptr;
    QAction* exportAction_ = nullptr;
    QAction* exitAction_ = nullptr;
    QAction* toggleFileDockAction_ = nullptr;
    QAction* togglePropertyDockAction_ = nullptr;
    QAction* resetLayoutAction_ = nullptr;
    QAction* aboutAction_ = nullptr;
    QAction* exportImageAction_ = nullptr;  // 导出主界面图片（grab + save）
    QAction* togglePythonConsoleAction_ = nullptr;  // 底部 Python 控制台开关
    // 003-install-icon-bars：未实现功能占位动作（禁用态 + 明确提示，FR-011）
    QAction* undoAction_ = nullptr;
    QAction* redoAction_ = nullptr;
    QAction* loadScriptAction_ = nullptr;
    QAction* recordScreenAction_ = nullptr;
    QAction* refreshAction_ = nullptr;
    QAction* helpAction_ = nullptr;  // 帮助菜单项（契约 §1）

    // 003：功能栏（纵向，ToolButtonIconOnly，iconSize 24）
    QToolBar* leftToolBar_ = nullptr;   // 左：通用功能（FR-003）
    QToolBar* rightToolBar_ = nullptr;  // 右：领域功能（FR-005）

    // 主题
    QActionGroup* themeGroup_ = nullptr;          // 单选互斥（25 项）
    QList<QAction*> themeActions_;                // 主题菜单动作（勾选当前项）
    QList<QPair<QAction*, QString>> iconItems_;   // (动作, 契约图标 id)，主题切换时重建

    // 日志级别（FR-002/012/013）：全局单一矩阵 5 项，顺序对齐 LogLevel 0..4；
    // 同时应用到全部 sink（终端/面板/文件），不再区分"控制台/文件"（FR-012 修订）
    QList<QAction*> levelActions_;
    QAction* allLevelsAction_ = nullptr;      // 全局级别：全部启用（菜单快捷）
    QAction* noneLevelsAction_ = nullptr;     // 全局级别：全部禁用（菜单快捷）
    QAction* vtkLogAction_ = nullptr;  // VTK 日志拦截开关（FR-011）

    // 日志路径可达性（FR-014）：设置菜单只读路径 + 打开日志目录
    QAction* logPathAction_ = nullptr;
    QAction* openLogDirAction_ = nullptr;
    QAction* setLogPathAction_ = nullptr;  // 设置日志路径（FR-016）
    QAction* clearLogAction_ = nullptr;    // 清除历史日志（FR-017）
    QString  logFilePath_;  // 由 main.cpp 注入（Logger 实际使用的文件路径）

    // Dock / 中央
    QDockWidget* fileDock_ = nullptr;
    QDockWidget* propertyDock_ = nullptr;
    QDockWidget* pythonDock_ = nullptr;    // 底部：Python 控制台
    // Dock 拖拽高亮（VSCode 风格放置指示）
    QWidget*      dockDragOverlay_ = nullptr;  // 全窗口覆盖层（鼠标穿透，绘制分割线高亮）
    QWidget*      sashHighlight_ = nullptr;    // 分界线高亮细条（仅覆盖分隔条缝隙，亮色）
    QDockWidget*  dragDock_ = nullptr;         // 正在拖拽的 dock（endDockDrag 执行放置）
    // 分界线（dock 边缘分隔条）拖拽高亮：按住分隔条 resize 时高亮分隔线（VSCode sash 风格）。
    // 分隔条 = 相邻 dock/中央区域之间 1px 边界线（由 Qt 绘制在 QMainWindow 上，非独立 widget），
    // 高亮条 = 复用 DockDragOverlay 的细条 overlay，只覆盖分隔条缝隙，位置随布局同步更新。
    enum SashHit {
        SashMiss = 0,
        SashFileRight,    // 垂直分隔条：左侧 fileDock 右缘
        SashPropertyLeft, // 垂直分隔条：右侧 propertyDock 左缘
        SashPythonTop,    // 水平分隔条：底部 pythonDock 上缘
    };
    bool sashDragging_ = false;
    int  sashHit_ = SashMiss;   // 当前拖拽命中的分隔条类型
    PythonConsole* pythonConsole_ = nullptr;
    QTreeWidget* fileTree_ = nullptr;      // 左侧：文件树
    QTreeWidget* propertyTree_ = nullptr;  // 右侧：属性/曲线列表
    QLabel*      centralPlaceholder_ = nullptr; // 中央：曲线视图占位
    QLabel*      versionLabel_ = nullptr;       // 状态栏版本号（主题切换时刷新颜色）

    // 无边框自定义标题栏（与菜单栏同一行）
    QWidget*     titleBarWidget_ = nullptr;  // 左：应用图标 + 标题
    QWidget*     titleBarDragArea_ = nullptr;  // 中：拖拽区（菜单与按钮之间）
    QLabel*      titleLabel_ = nullptr;      // 窗口标题文本
    QToolButton* winMinBtn_ = nullptr;       // 最小化
    QToolButton* winMaxBtn_ = nullptr;       // 最大化/还原
    QToolButton* winCloseBtn_ = nullptr;     // 关闭
};

}  // namespace ui
}  // namespace perception

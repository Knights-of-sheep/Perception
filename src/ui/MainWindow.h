// ===== Perception 主窗口（M3a：界面框架）=====
// 布局：菜单栏 / 工具栏 / 左侧文件树 Dock / 中央曲线视图（M3 接入 VTK）/ 右侧属性 Dock / 状态栏。
// UI 事实源：docs/design/mockups/。
// 006-constitution-refactor：多屏最大化、Dock 拖拽高亮、日志设置已提取为独立控制器
//（WindowMaximizeController / DockDragOverlay / LogSettingsController），本类聚焦装配与转发。
#pragma once

#include <QMainWindow>

class QActionGroup;
class QCloseEvent;
class QDragEnterEvent;
class QDropEvent;
class QEvent;
class QKeyEvent;
class QLabel;
class QToolBar;
class QToolButton;
class QTreeWidget;

#include <QElapsedTimer>
#include <QList>
#include <QPair>
#include <QPointer>
#include <QRect>
#include <QVector>

namespace perception {
namespace ui {

class PythonConsole;  // 前向声明须与定义同名空间（类在 perception::ui 内）
class SubwindowContainer;  // 004-dock-layout-manager：中央区域子窗口容器
class SubwindowView;
class LayoutSettingsDialog;
class WindowMaximizeController;  // 005-multi-screen-maximize：多屏最大化/还原
class DockDragOverlay;           // Dock 拖拽 / 分隔条拖拽高亮
class LogSettingsController;     // 日志级别矩阵 / 路径 / 清除历史

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
    // 004-dock-layout-manager：创建渲染子窗口（Python create_window 桥 + 菜单双入口，FR-001/002）；
    // 返回生成的窗口 id（"Plot_" + 全局递增序号）
    QString createSubwindow(const QString& title);
    void openLayoutSettings();     // 打开布局设置界面（US5 统一入口）
    void showHiddenSubwindows();   // 恢复全部"隐藏"的子窗口（View 菜单）
    // 004-dock-layout-manager：暴露子窗口容器（快照模式用——触发首个 subwindow 选中以展示高亮）
    perception::ui::SubwindowContainer* centralContainer() const { return subwindowContainer_; }

public:
    // Dock 拖拽高亮（VSCode 风格分割线指示）：由 DockTitleBar 拖拽回调驱动，转发给控制器
    void beginDockDrag(QDockWidget* dock);            // 进入拖拽：显示高亮覆盖层
    void updateDockDrag(const QPoint& globalPos);     // 拖拽中：按鼠标位置更新高亮目标
    void endDockDrag(const QPoint& globalPos);        // 结束：执行放置并隐藏高亮

private slots:
    void openFile();
    void about();
    void exportMainWindowImage();  // 导出主界面图片（grab + PNG）
    void exportPythonCommands();   // 导出控制台已执行的命令为 .py 脚本

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
    // 最大化/还原时同步窗口控制按钮图标（逻辑转发给 WindowMaximizeController）
    void changeEvent(QEvent* event) override;
    // 主题切换（palette 变化）时重新生成标题栏按钮图标颜色；
    // 同时检测分隔条命中（press/release）驱动分界线高亮（转发给 DockDragOverlay）
    bool event(QEvent* event) override;
    // 全屏/最大化状态：Esc 退出（FR-017）
    void keyPressEvent(QKeyEvent* event) override;

private:
    void createActions();
    void createMenus();
    void createTitleBar();  // 自定义标题栏：标题+窗口按钮，与菜单栏同一行（无边框）
    void createToolbars();  // 003：左右功能栏（纵向，ToolButtonIconOnly）
    void createDocks();
    void createCentralArea();
    void createStatusBar();
    void addFileToTree(const QString& path);  // 占位：加载后加入文件树
    void updateEmptyHints();                  // 空状态提示节点
    void updateWindowButtonIcons();           // 同步最大化/还原按钮图标（maximizedChanged 信号）
    // 003-install-icon-bars：动作图标注册（QIcon 创建时固化主题色，切换主题需重建）
    void setActionIcon(QAction* action, const QString& iconId);
    void refreshActionIcons();                // 主题切换后按当前色板重建全部动作图标

    // 004-dock-layout-manager：全屏协调（FR-017）。
    // 全屏 = 中间区域（子窗口容器）扩展至整个主界面：隐藏三个 Dock，容器（centralWidget）
    // 自动扩展；退出时按记录恢复各 Dock 显隐。侧边栏功能按钮触发。
    void setContainerFullscreen(bool on);
    bool isContainerFullscreen() const { return containerFullscreen_; }

    // 动作
    QAction* openAction_ = nullptr;
    QAction* exportAction_ = nullptr;
    QAction* exitAction_ = nullptr;
    QAction* toggleFileDockAction_ = nullptr;
    QAction* togglePropertyDockAction_ = nullptr;
    QAction* resetLayoutAction_ = nullptr;
    // 004-dock-layout-manager：视图菜单
    QAction* newSubwindowAction_ = nullptr;      // 新建子窗口（FR-001/002）
    QAction* layoutSettingsAction_ = nullptr;    // 布局设置入口（US5）
    QAction* toggleFullscreenAction_ = nullptr;  // 全屏：中间区域扩展至整个主界面（侧边栏按钮）
    QAction* showHiddenSubwindowsAction_ = nullptr;  // 恢复"隐藏"的子窗口
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

    // 日志设置控制器（级别矩阵/路径/清除历史，006 提取；actions 由 attachLogMenu 创建）
    LogSettingsController* logSettingsController_ = nullptr;

    // Dock / 中央
    QDockWidget* fileDock_ = nullptr;
    QDockWidget* propertyDock_ = nullptr;
    QDockWidget* pythonDock_ = nullptr;    // 底部：Python 控制台
    // Dock 拖拽高亮 / 分隔条拖拽高亮控制器（006 提取）
    DockDragOverlay* dockDragOverlay_ = nullptr;
    PythonConsole* pythonConsole_ = nullptr;
    QTreeWidget* fileTree_ = nullptr;      // 左侧：文件树
    QTreeWidget* propertyTree_ = nullptr;  // 右侧：属性/曲线列表
    QLabel*      centralPlaceholder_ = nullptr; // 中央：曲线视图占位
    QLabel*      versionLabel_ = nullptr;       // 状态栏版本号（主题切换时刷新颜色）

    // 004-dock-layout-manager：子窗口容器与全屏状态
    SubwindowContainer* subwindowContainer_ = nullptr;
    LayoutSettingsDialog* layoutSettingsDialog_ = nullptr;  // 单例对话框（无模式）
    bool containerFullscreen_ = false;  // 全屏中（三个 Dock 隐藏，容器扩展占满主界面）
    QVector<bool> docksVisibleBeforeFullscreen_;  // 全屏前各 Dock 显隐，退出时恢复
    int subwindowSeq_ = 0;  // 子窗口标题序号（plot_1、plot_2、…，全局递增不回退）

    // 005-multi-screen-maximize：多屏最大化/还原控制器（006 提取；按钮图标由
    // maximizedChanged 信号驱动刷新）
    WindowMaximizeController* windowMaximizeController_ = nullptr;

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

// ===== Perception 主窗口（M3a：界面框架）=====
// 布局：菜单栏 / 工具栏 / 左侧文件树 Dock / 中央曲线视图（M3 接入 VTK）/ 右侧属性 Dock / 状态栏。
// UI 事实源：docs/design/mockups/。
#pragma once

#include <QMainWindow>

class QActionGroup;
class QCloseEvent;
class QDragEnterEvent;
class QDropEvent;
class QLabel;
class QToolButton;
class QTreeWidget;

#include <QList>

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

private:
    void createActions();
    void createMenus();
    void createDocks();
    void createCentralArea();
    void createStatusBar();
    void addFileToTree(const QString& path);  // 占位：加载后加入文件树
    void updateEmptyHints();                  // 空状态提示节点

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

    // 主题
    QActionGroup* themeGroup_ = nullptr;          // 单选互斥（15 项）
    QList<QAction*> themeActions_;                // 主题菜单动作（勾选当前项）

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
    PythonConsole* pythonConsole_ = nullptr;
    QTreeWidget* fileTree_ = nullptr;      // 左侧：文件树
    QTreeWidget* propertyTree_ = nullptr;  // 右侧：属性/曲线列表
    QLabel*      centralPlaceholder_ = nullptr; // 中央：曲线视图占位
    QLabel*      versionLabel_ = nullptr;       // 状态栏版本号（主题切换时刷新颜色）
};

}  // namespace ui
}  // namespace perception

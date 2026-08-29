// ===== 日志设置控制器（006-constitution-refactor：自 MainWindow 提取）=====
// 职责（FR-002/012/013/014/016/017）：
//   - 设置菜单"日志级别"子菜单装配（全局单一矩阵，批量开关 + 5 级别勾选）；
//   - 级别矩阵应用到全部 sink（终端/文件）并持久化 QSettings；
//   - 日志路径展示/注入、选目录迁移旧日志（FR-016）、打开日志目录（FR-014）、
//     清除历史（FR-017）。
// 不持有 MainWindow 的 UI 结构，仅依赖 QMainWindow 指针（父窗口/状态栏）与
// core::log::Logger 单例。
#pragma once

#include <QList>
#include <QObject>
#include <QString>

class QAction;
class QMenu;
class QMainWindow;

namespace perception {
namespace core {
namespace log {
enum class LogLevel : int;
}
}  // namespace core
}  // namespace perception

namespace perception {
namespace ui {

// 设置菜单中的日志菜单项集合（attachLogMenu 返回，供 MainWindow 做图标注册/主题刷新）。
struct LogMenuActions {
    QAction* allLevels = nullptr;
    QAction* noneLevels = nullptr;
    QList<QAction*> levelActions;  // 5 项，顺序对齐 LogLevel 0..4
    QAction* vtkLog = nullptr;
    QAction* logPath = nullptr;  // 只读展示（路径注入后更新文本）
    QAction* openLogDir = nullptr;
    QAction* setLogPath = nullptr;
    QAction* clearLog = nullptr;
};

class LogSettingsController : public QObject {
    Q_OBJECT

public:
    explicit LogSettingsController(QMainWindow* window, QObject* parent = nullptr);

    // 在"设置"菜单下装配日志子菜单并 connect（MainWindow::createMenus 调用）。
    LogMenuActions attachLogMenu(QMenu* settingsMenu);

    // 从 QSettings 恢复日志级别并应用（main.cpp 在 Logger::configure + addSink 后调用；
    // FR-013）。
    void restoreFromSettings();

    // 注入日志文件实际路径（main.cpp 在 Logger::configure 后调用；FR-014）。
    void setLogFilePath(const QString& path);

    // 应用单个级别到全部 sink（FR-002/012；菜单勾选变化时内部调用）。
    void applyLevel(perception::core::log::LogLevel level, bool enabled);

private slots:
    void onLevelToggled(bool checked);  // 全局级别矩阵勾选变化（FR-012）
    void onAllLevels(bool enabled);     // 全局级别：全部启用(true)/全部禁用(false)
    void onSetLogPath();                // 设置日志路径：选目录 + 迁移旧日志（FR-016）
    void openLogDir();                  // 打开日志文件所在目录（FR-014）
    void clearHistory();                // 清除历史日志（FR-017）

private:
    // 将指定级别应用到全部已注册 sink（终端/面板/文件），保持全局一致。
    bool applyLevelToAllSinks(perception::core::log::LogLevel level, bool enabled);

    QMainWindow* window_ = nullptr;
    LogMenuActions actions_;
    QString logFilePath_;  // 由 main.cpp 注入（Logger 实际使用的文件路径）
};

}  // namespace ui
}  // namespace perception

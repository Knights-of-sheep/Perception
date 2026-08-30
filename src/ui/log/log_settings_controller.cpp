#include "ui/log/log_settings_controller.h"

#include "core/log/logger.h"
#include "ui/themed_file_dialog.h"
#include "ui/themed_message_box.h"

#include <QAction>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QMainWindow>  // window_->statusBar() / QMessageBox 父窗口转换需要完整类型
#include <QMenu>
#include <QMessageBox>
#include <QSettings>
#include <QStatusBar>
#include <QUrl>

namespace perception {
namespace ui {

namespace {
// 日志级别持久化 key（FR-013）：全局单一矩阵（FR-012 修订，不再区分控制台/文件）。
// 兼容：旧版本曾分别持久化 log/console/<LEVEL> 与 log/file/<LEVEL>；升级后优先读新
// 全局 key，未设置时回退到旧 log/console/<LEVEL>，保证既有用户设置不丢。
constexpr const char* kLogLevelPrefix = "log/level/";
constexpr const char* kLogLegacyConsolePrefix = "log/console/";
constexpr const char* kLogVtkEnabled = "log/vtkEnabled";
constexpr const char* kLogPathKey = "log/path";  // 用户配置的日志文件路径（FR-016）
constexpr const char* kLevelNames[] = {"DEBUG", "INFO", "WARN", "ERROR", "FATAL"};
}  // namespace

LogSettingsController::LogSettingsController(QMainWindow* window, QObject* parent)
    : QObject(parent), window_(window) {}

bool LogSettingsController::applyLevelToAllSinks(perception::core::log::LogLevel level,
                                                 bool enabled) {
    const auto sinks = perception::core::log::Logger::instance().sinks();
    bool applied = false;
    for (const auto& sink : sinks) {
        if (sink) {
            sink->setLevelEnabled(level, enabled);
            applied = true;
        }
    }
    return applied;
}

LogMenuActions LogSettingsController::attachLogMenu(QMenu* settingsMenu) {
    // ---- 设置 → 日志级别（FR-002/012/013 修订）----
    // 全局单一矩阵：级别同时作用于全部 sink（终端/文件），不再区分控制台/文件（FR-012 修订）。
    QMenu* logLevelMenu = settingsMenu->addMenu(tr("Log Level(&L)"));
    logLevelMenu->setToolTipsVisible(true);

    // 批量开关（置于级别列表顶部，解决单级别勾选状态不一致/找不到入口）
    logLevelMenu->addSeparator();
    actions_.allLevels = logLevelMenu->addAction(tr("Enable All"));
    actions_.noneLevels = logLevelMenu->addAction(tr("Disable All"));
    connect(actions_.allLevels, &QAction::triggered, this, [this] { onAllLevels(true); });
    connect(actions_.noneLevels, &QAction::triggered, this, [this] { onAllLevels(false); });

    for (int i = 0; i < 5; ++i) {
        QAction* c = logLevelMenu->addAction(QString::fromLatin1(kLevelNames[i]));
        c->setCheckable(true);
        c->setChecked(i != 0);  // 默认矩阵：DEBUG 关、其余开
        c->setData(i);          // LogLevel 索引
        connect(c, &QAction::toggled, this, &LogSettingsController::onLevelToggled);
        actions_.levelActions.append(c);
    }

    // VTK 日志拦截开关（FR-011；VTK 未引入，仅配置项）
    actions_.vtkLog = settingsMenu->addAction(tr("VTK Log Interception(&V)"));
    actions_.vtkLog->setCheckable(true);
    actions_.vtkLog->setChecked(true);  // 默认开启
    connect(actions_.vtkLog, &QAction::toggled, this, [](bool checked) {
        QSettings settings;
        settings.setValue(QLatin1String(kLogVtkEnabled), checked);
    });

    // ---- 日志路径可达性（FR-014）：只读路径 + 一键打开日志目录 ----
    // 日志写入 %APPDATA%\Perception\logs（隐藏目录），用户无从查找，
    // 故在设置菜单直接展示完整路径并给出"打开日志目录"直达入口。
    settingsMenu->addSeparator();
    actions_.logPath = settingsMenu->addAction(tr("Log file: not configured"));
    actions_.logPath->setEnabled(false);  // 只读展示（可选中复制），路径由 main.cpp 注入后更新
    actions_.openLogDir = settingsMenu->addAction(tr("Open Log Directory(&O)"));
    actions_.openLogDir->setEnabled(false);  // 路径注入前不可用
    connect(actions_.openLogDir, &QAction::triggered, this,
            &LogSettingsController::openLogDir);

    // 日志路径可配置（FR-016）：选择目录后迁移旧日志并持久化
    actions_.setLogPath = settingsMenu->addAction(tr("Set Log Path...(&P)"));
    actions_.setLogPath->setEnabled(false);  // 路径注入前不可用
    connect(actions_.setLogPath, &QAction::triggered, this,
            &LogSettingsController::onSetLogPath);

    // 清除历史日志（FR-017）：删除当前日志目录全部日志文件与归档
    actions_.clearLog = settingsMenu->addAction(tr("Clear Log History(&C)"));
    actions_.clearLog->setEnabled(false);
    connect(actions_.clearLog, &QAction::triggered, this,
            &LogSettingsController::clearHistory);

    return actions_;
}

void LogSettingsController::onLevelToggled(bool checked) {
    auto* act = qobject_cast<QAction*>(sender());
    if (!act) return;
    const int idx = act->data().toInt();
    const auto level = static_cast<perception::core::log::LogLevel>(idx);

    // 立即生效：同一级别同步到全部 sink（终端/文件）
    if (!applyLevelToAllSinks(level, checked)) {
        // 之前静默返回——导致用户看到菜单勾上但无输出，以为"设置没用"。
        // 现写入 Logger，让"设置失败"可在日志/终端中看到。
        perception::core::log::Logger::instance().warnAt(
            __FILE__, __LINE__,
            std::string("no sink registered; level toggle has no effect (level=")
            + perception::core::log::toString(level) + ")");
    }

    // 持久化（全局 key，FR-013）
    QSettings settings;
    settings.setValue(QString::fromLatin1(kLogLevelPrefix)
                          + QString::fromLatin1(perception::core::log::toString(level)),
                      checked);
}

// 全局级别批量开关：解决"用户找不到单级别怎么开"和"QSettings 残留导致状态混乱"。
void LogSettingsController::onAllLevels(bool enabled) {
    const auto sinks = perception::core::log::Logger::instance().sinks();
    if (sinks.empty()) {
        perception::core::log::Logger::instance().warnAt(
            __FILE__, __LINE__,
            "no sink registered; batch toggle has no effect");
        return;
    }
    QSettings settings;
    for (int i = 0; i < actions_.levelActions.size() && i < 5; ++i) {
        const auto level = static_cast<perception::core::log::LogLevel>(i);
        // setChecked 不会触发 toggled（避免重入），需显式应用 + 持久化
        actions_.levelActions[i]->setChecked(enabled);
        for (const auto& sink : sinks) {
            if (sink) sink->setLevelEnabled(level, enabled);
        }
        settings.setValue(QString::fromLatin1(kLogLevelPrefix)
                              + QString::fromLatin1(perception::core::log::toString(level)),
                          enabled);
    }
}

// 启动恢复（main.cpp 在 configure + addSink 后调用，FR-013）：
// 读全局矩阵 log/level/<LEVEL>；未设置时回退旧版 log/console/<LEVEL>（迁移保设置），
// 再回退默认（DEBUG 关、其余开）。
void LogSettingsController::restoreFromSettings() {
    QSettings settings;
    for (int i = 0; i < actions_.levelActions.size() && i < 5; ++i) {
        const auto level = static_cast<perception::core::log::LogLevel>(i);
        const QString levelName = QString::fromLatin1(perception::core::log::toString(level));
        const QString key = QString::fromLatin1(kLogLevelPrefix) + levelName;
        const QString legacyKey = QString::fromLatin1(kLogLegacyConsolePrefix) + levelName;
        const bool on = settings.contains(key)
                            ? settings.value(key).toBool()
                            : settings.value(legacyKey, i != 0).toBool();
        actions_.levelActions[i]->setChecked(on);
        for (const auto& sink : perception::core::log::Logger::instance().sinks()) {
            if (sink) sink->setLevelEnabled(level, on);
        }
    }
    if (actions_.vtkLog) {
        const bool on = settings.value(QLatin1String(kLogVtkEnabled), true).toBool();
        actions_.vtkLog->setChecked(on);
        // VTK 未引入：仅持久化开关；拦截桥随后续落地（FR-011）
    }
}

// ---- 日志路径可达性（FR-014）----
void LogSettingsController::setLogFilePath(const QString& path) {
    logFilePath_ = path;
    if (actions_.logPath)
        actions_.logPath->setText(tr("Log file: %1").arg(path));
    if (actions_.openLogDir)
        actions_.openLogDir->setEnabled(!path.isEmpty());
    if (actions_.setLogPath)
        actions_.setLogPath->setEnabled(!path.isEmpty());
    if (actions_.clearLog)
        actions_.clearLog->setEnabled(!path.isEmpty());
}

void LogSettingsController::openLogDir() {
    if (logFilePath_.isEmpty()) return;
    const QString dir = QFileInfo(logFilePath_).absolutePath();
    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(dir))) {
        showThemedMessageBox(window_, QMessageBox::Warning, tr("Cannot Open Log Directory"),
                             tr("Cannot open directory: %1").arg(dir));
    }
}

// ---- 设置日志路径（FR-016）：选目录 -> 迁移旧日志 -> 重建 FileSink -> 持久化 ----
void LogSettingsController::onSetLogPath() {
    if (logFilePath_.isEmpty()) return;
    const QString curDir = QFileInfo(logFilePath_).absolutePath();
    const QString dir = runThemedFileDialog(window_, tr("Select Log Directory"),
                                            curDir, QString(), FileDialogMode::Directory);
    if (dir.isEmpty()) return;

    const QString newPath = QDir(dir).filePath(QStringLiteral("app.log"));
    if (newPath == logFilePath_) return;

    const bool ok = perception::core::log::Logger::instance()
                        .setFilePath(newPath.toStdString());
    setLogFilePath(newPath);  // 更新菜单展示与"打开日志目录"/动作可用性
    QSettings settings;
    settings.setValue(QLatin1String(kLogPathKey), newPath);

    PERCEPTION_LOG_I(std::string("log path changed to ") + newPath.toStdString());
    window_->statusBar()->showMessage(
        ok ? tr("Log path changed and history migrated: %1").arg(newPath)
           : tr("Log path set (failed to migrate old logs): %1").arg(newPath), 6000);
}

// ---- 清除历史日志（FR-017）：删除当前日志目录全部日志文件与归档 ----
void LogSettingsController::clearHistory() {
    if (logFilePath_.isEmpty()) return;
    const QString dir = QFileInfo(logFilePath_).absolutePath();
    const auto ret = showThemedMessageBox(
        window_, QMessageBox::Question, tr("Clear Log History"),
        tr("This will delete all log files (including archives) in the log directory.\n\nDirectory: %1\n\nContinue?").arg(dir),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (ret != QMessageBox::Yes) return;

    const bool ok = perception::core::log::Logger::instance().clearLogFiles();
    if (ok) {
        PERCEPTION_LOG_I("log history cleared");
        window_->statusBar()->showMessage(tr("Cleared log history: %1").arg(dir), 5000);
    } else {
        showThemedMessageBox(window_, QMessageBox::Warning, tr("Clear Failed"),
                             tr("Unable to clear log files. Check directory permissions or disk status."));
    }
}

}  // namespace ui
}  // namespace perception

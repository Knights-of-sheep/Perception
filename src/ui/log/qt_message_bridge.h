#pragma once

// Qt 日志重定向桥（FR-010）：qInstallMessageHandler 将 Qt 自身输出
// （qDebug/qInfo/qWarning/qCritical/qFatal）纳入统一日志流，
// 遵守统一格式与各 sink 级别开关。
// 本桥位于 ui 层（core 层不接触 Qt，宪法 III）。
#include <QtGlobal>

namespace perception {
namespace ui {

// 安装 Qt 消息重定向。返回先前 handler（供 RAII restore 使用）。
QtMessageHandler installQtMessageBridge();

// 恢复先前 handler。
void restoreQtMessageBridge(QtMessageHandler previous);

}  // namespace ui
}  // namespace perception
